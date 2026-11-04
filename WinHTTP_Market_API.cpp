module LostArkMultiTool.Services.WinHTTPMarketAPI;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Services.JSONValue;
import LostArkMultiTool.Services.WinHTTPClient;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 서버 주소와 경로 =====
        constexpr std::wstring_view API_HOST{ L"developer-lostark.game.onstove.com" };
        constexpr std::wstring_view MARKET_PATH{ L"/markets/items" };
        constexpr std::string_view SIBLINGS_PATH_PREFIX{ "/characters/" };
        constexpr std::string_view SIBLINGS_PATH_SUFFIX{ "/siblings" };

        // ===== HTTP 동사 · 헤더 =====
        constexpr std::wstring_view VERB_GET{ L"GET" };
        constexpr std::wstring_view VERB_POST{ L"POST" };
        constexpr std::wstring_view ACCEPT_HEADER{ L"accept: application/json\r\n" };
        constexpr std::wstring_view CONTENT_TYPE_HEADER{ L"Content-Type: application/json\r\n" };

        // ===== 응답 상태 코드 (성공 범위 판정은 WinHTTPClient::IsSuccessStatus) =====
        constexpr int HTTP_STATUS_UNAUTHORIZED{ 401 };
        constexpr int HTTP_STATUS_TOO_MANY_REQUESTS{ 429 };

        // ===== 시간 제한 · 시세 계산 =====
        constexpr int API_TIMEOUT_MS{ Core::APPConstants::API_TIMEOUT_SECONDS * Core::APPConstants::MS_PER_SECOND };
        constexpr std::size_t MARKET_CATEGORY_CODE{ 50000 };   // 거래소 재료 카테고리
        constexpr int FIRST_PAGE{ 1 };
        constexpr double SHARD_POUCH_COUNT{ 3000.0 };          // 파편 주머니 1개당 파편 수
        constexpr double PRICE_SCALE{ 100.0 };                 // 소수 둘째 자리까지 (10^2)

        // ===== 응답 JSON 키 =====
        constexpr std::string_view KEY_ITEMS{ "Items" };
        constexpr std::string_view KEY_NAME{ "Name" };
        constexpr std::string_view KEY_RECENT_PRICE{ "RecentPrice" };
        constexpr std::string_view KEY_CURRENT_MIN_PRICE{ "CurrentMinPrice" };
        constexpr std::string_view KEY_BUNDLE_COUNT{ "BundleCount" };
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };
        constexpr std::string_view KEY_CHARACTER_CLASS_NAME{ "CharacterClassName" };
        constexpr std::string_view KEY_ITEM_MAX_LEVEL{ "ItemMaxLevel" };
        constexpr std::string_view KEY_ITEM_AVG_LEVEL{ "ItemAvgLevel" };

        // ===== URL 인코딩 =====
        constexpr std::string_view HEX_DIGITS{ "0123456789ABCDEF" };
        constexpr char PERCENT_SIGN{ '%' };
        constexpr int NIBBLE_BITS{ 4 };
        constexpr unsigned NIBBLE_MASK{ 0x0F };

        // 영문·숫자·-_.~ 만 그대로 두고 나머지는 %XX로 바꿈 (C# Uri.EscapeDataString과 같은 규칙)
        bool IsUnreservedByte(unsigned char byte) noexcept
        {
            return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9')
                || byte == '-' || byte == '_' || byte == '.' || byte == '~';
        }

        std::string EscapeDataString(std::string_view text)
        {
            std::string escaped;
            escaped.reserve(text.size());

            for (const char character : text)
            {
                const auto byte{ static_cast<unsigned char>(character) };
                if (IsUnreservedByte(byte))
                {
                    escaped.push_back(character);
                    continue;
                }

                escaped.push_back(PERCENT_SIGN);
                escaped.push_back(HEX_DIGITS[static_cast<std::size_t>(byte >> NIBBLE_BITS)]);
                escaped.push_back(HEX_DIGITS[static_cast<std::size_t>(byte & NIBBLE_MASK)]);
            }
            return escaped;
        }

        // JSON 문자열 리터럴로 감쌈 (재료 이름에 들어갈 수 있는 따옴표와 역슬래시만 이스케이프)
        std::string QuoteJSONString(std::string_view text)
        {
            std::string quoted{ '"' };
            for (const char c : text)
            {
                if (c == '"' || c == '\\') quoted.push_back('\\');
                quoted.push_back(c);
            }
            quoted.push_back('"');
            return quoted;
        }

        // 거래소 검색 요청 본문 (C# JObject와 같은 키 순서)
        std::string BuildMarketPayload(std::string_view itemName)
        {
            return std::format(R"({{"Sort":"CURRENT_MIN_PRICE","CategoryCode":{},"ItemName":{},"PageNo":{},"SortCondition":"ASC"}})",
                MARKET_CATEGORY_CODE, QuoteJSONString(itemName), FIRST_PAGE);
        }

        // C# Math.Round(value, 2)와 같이 짝수 반올림 (std::nearbyint의 기본 반올림 모드)
        double RoundPrice(double value) noexcept
        {
            return std::nearbyint(value * PRICE_SCALE) / PRICE_SCALE;
        }

        // 실패 응답을 상황별 예외로 바꿈 (C# EnsureSuccess와 같음)
        void EnsureSuccess(const HTTPResponse& response, std::string_view apiName)
        {
            if (WinHTTPClient::IsSuccessStatus(response.m_statusCode)) return;
            if (response.m_statusCode == HTTP_STATUS_UNAUTHORIZED) throw Core::APIKeyInvalidException{};
            if (response.m_statusCode == HTTP_STATUS_TOO_MANY_REQUESTS) throw Core::APIRateLimitException{};
            throw std::runtime_error{ std::format("{} API 요청 실패 (HTTP {})", apiName, response.m_statusCode) };
        }

        // 거래소 응답에서 이름이 정확히 같은 아이템 찾기 (C# FirstOrDefault와 같음)
        const JSONValue* FindExactItem(const JSONValue& root, std::string_view itemName)
        {
            for (const JSONValue& item : root.FindMember(KEY_ITEMS).GetItems())
            {
                if (std::string_view{ item.FindMember(KEY_NAME).GetString() } == itemName) return &item;
            }
            return nullptr;
        }

        // 문자열 또는 숫자로 온 값을 텍스트로 바꿈 (C# object → ToString과 같음, 없으면 nullopt)
        std::optional<std::string> ReadOptionalText(const JSONValue& parent, std::string_view key)
        {
            const JSONValue& value{ parent.FindMember(key) };
            switch (value.GetType())
            {
                case JSONType::STRING_VALUE:
                    return value.GetString();
                case JSONType::NUMBER_VALUE:
                {
                    // 편집기의 consteval 형식 검사를 거치지 않도록 런타임 포맷 사용 (결과는 std::format과 같음)
                    const double number{ value.GetNumber() };
                    return std::vformat("{}", std::make_format_args(number));
                }
                default:
                    return std::nullopt;
            }
        }
    }

    WinHTTPMarketAPI::WinHTTPMarketAPI(std::string apiKey)
        : m_client{ API_HOST, API_TIMEOUT_MS }
    {
        SetAPIKey(std::move(apiKey));
    }

    void WinHTTPMarketAPI::SetAPIKey(std::string apiKey)
    {
        std::wstring header{ std::format(L"authorization: Bearer {}\r\n", Core::TextEncoding::UTF8ToWide(apiKey)) };

        std::scoped_lock lock{ m_headerMutex };
        m_authorizationHeader = std::move(header);
    }

    std::wstring WinHTTPMarketAPI::BuildHeaders()
    {
        std::scoped_lock lock{ m_headerMutex };

        std::wstring headers{ ACCEPT_HEADER };
        headers += CONTENT_TYPE_HEADER;
        headers += m_authorizationHeader;
        return headers;
    }

    double WinHTTPMarketAPI::GetPrice(std::string_view itemName, bool isShard, bool isBundle)
    {
        const HTTPResponse response{ m_client.Send(VERB_POST, MARKET_PATH, BuildHeaders(), BuildMarketPayload(itemName)) };
        EnsureSuccess(response, "거래소");

        const JSONValue root{ ParseJSONText(response.m_body) };
        const JSONValue* item{ FindExactItem(root, itemName) };
        if (item == nullptr)
        {
            throw std::runtime_error{ std::format("거래소에서 '{}' 시세를 찾을 수 없습니다.", itemName) };
        }

        // 최근 거래가가 있으면 우선, 없으면 현재 최저가 (C# 원본 순서와 같음)
        const double recentPrice{ item->FindMember(KEY_RECENT_PRICE).GetNumber() };
        const double price{ recentPrice > 0.0 ? recentPrice : item->FindMember(KEY_CURRENT_MIN_PRICE).GetNumber() };
        if (price <= 0.0) throw std::runtime_error{ std::format("'{}'의 유효한 시세가 없습니다.", itemName) };

        if (isShard) return RoundPrice(price / SHARD_POUCH_COUNT);
        if (!isBundle) return price;

        // 하드코딩 묶음 수량 대신 응답의 실제 BundleCount 사용
        const int bundleCount{ static_cast<int>(item->FindMember(KEY_BUNDLE_COUNT).GetNumber()) };
        if (bundleCount <= 0) throw std::runtime_error{ std::format("'{}'의 묶음 수량 정보가 없습니다.", itemName) };
        return RoundPrice(price / static_cast<double>(bundleCount));
    }

    std::vector<Models::CharacterInfo> WinHTTPMarketAPI::GetRoster(std::string_view charName)
    {
        const std::string path{ std::string{ SIBLINGS_PATH_PREFIX } + EscapeDataString(charName)
                                + std::string{ SIBLINGS_PATH_SUFFIX } };

        const HTTPResponse response{ m_client.Send(VERB_GET, Core::TextEncoding::UTF8ToWide(path), BuildHeaders(), std::string{}) };
        EnsureSuccess(response, "원정대");

        // 없는 닉네임은 200 + null로 옴 → 빈 목록 (C# roster ?? [] 와 같음)
        const JSONValue root{ response.m_body.empty() ? JSONValue{} : ParseJSONText(response.m_body) };

        std::vector<Models::CharacterInfo> roster;
        roster.reserve(root.GetItems().size());
        for (const JSONValue& entry : root.GetItems())
        {
            roster.emplace_back(
                std::string{ entry.FindMember(KEY_CHARACTER_NAME).GetString() },
                ReadOptionalText(entry, KEY_CHARACTER_CLASS_NAME),
                ReadOptionalText(entry, KEY_ITEM_MAX_LEVEL),
                ReadOptionalText(entry, KEY_ITEM_AVG_LEVEL));
        }
        return roster;
    }
}