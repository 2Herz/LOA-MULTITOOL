module LostArkMultiTool.Services.WinHTTPMarketAPI;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.APIFormatMonitor;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Services.ArmoryParser;
import LostArkMultiTool.Services.JSONValue;
import LostArkMultiTool.Services.WinHTTPClient;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 서버 주소와 경로 =====
        constexpr std::wstring_view API_HOST{ L"developer-lostark.game.onstove.com" };
        constexpr std::wstring_view MARKET_PATH{ L"/markets/items" };
        constexpr std::wstring_view AUCTION_PATH{ L"/auctions/items" };
        constexpr std::wstring_view AUCTION_OPTIONS_PATH{ L"/auctions/options" };
        constexpr std::string_view SIBLINGS_PATH_PREFIX{ "/characters/" };
        constexpr std::string_view SIBLINGS_PATH_SUFFIX{ "/siblings" };
        constexpr std::string_view ARMORY_PATH_PREFIX{ "/armories/characters/" };  // 무기고 API 경로 앞부분
#ifdef _DEBUG  // Debug 빌드에서만 적용
        // Debug 빌드: 필터 없이 전체 묶음을 받아 원본 응답을 파일로 남김 (악세 · 각인 · 아크 패시브 · 아크 그리드 등 필드 이름 확인용)
        constexpr std::string_view ARMORY_PATH_SUFFIX{ "" };  // Debug 빌드의 무기고 요청 경로 뒷부분 (필터 없음)
        constexpr std::string_view ARMORY_DEBUG_DUMP_FILE_NAME{ "ArmoryDebugDump.json" };   // 사용자 데이터 폴더에 저장
#else  // Release 빌드에서는
        // Release 빌드도 필터 없이 전체 묶음을 받음 (악세 · 팔찌 · 스톤 · 아바타 · 각인 · 아크 패시브 · 아크 그리드까지 필요하고, 필터 이름을 확인하지 못해서 전체 응답이 가장 안전함)
        constexpr std::string_view ARMORY_PATH_SUFFIX{ "" };  // Release 빌드의 무기고 요청 경로 뒷부분 (필터 없음)
#endif  // 빌드 구성 분기 끝

        // ===== HTTP 동사 · 헤더 =====
        constexpr std::wstring_view VERB_GET{ L"GET" };
        constexpr std::wstring_view VERB_POST{ L"POST" };
        constexpr std::wstring_view ACCEPT_HEADER{ L"accept: application/json\r\n" };
        constexpr std::wstring_view CONTENT_TYPE_HEADER{ L"Content-Type: application/json\r\n" };

        // ===== 응답 상태 코드 (성공 범위 판정은 WinHTTPClient::IsSuccessStatus) =====
        constexpr int HTTP_STATUS_OK{ 200 };
        constexpr int HTTP_STATUS_UNAUTHORIZED{ 401 };
        constexpr int HTTP_STATUS_TOO_MANY_REQUESTS{ 429 };
        constexpr int HTTP_STATUS_SERVICE_UNAVAILABLE{ 503 };

        // ===== 호출 한도 응답 헤더 =====
        constexpr std::wstring_view HEADER_RATE_LIMIT{ L"X-RateLimit-Limit" };
        constexpr std::wstring_view HEADER_RATE_REMAINING{ L"X-RateLimit-Remaining" };
        constexpr std::wstring_view HEADER_RATE_RESET{ L"X-RateLimit-Reset" };
        constexpr int DECIMAL_BASE{ 10 };

        // ===== 오류 설명 =====
        constexpr std::string_view KEY_ERROR_MESSAGE{ "Message" };
        constexpr std::string_view KEY_ERROR_MESSAGE_LOWER{ "message" };
        constexpr std::string_view ELLIPSIS{ "…" };                       // 긴 오류 설명을 자를 때 붙이는 기호
        constexpr std::string_view LINE_BREAKS{ "\r\n" };
        constexpr std::string_view TRIM_SPACES{ " \t" };
        constexpr unsigned char UTF8_CONTINUATION_MASK{ 0xC0 };           // 10xxxxxx = 글자 중간 바이트
        constexpr unsigned char UTF8_CONTINUATION_BITS{ 0x80 };
        constexpr std::string_view INVALID_AUCTION_RESPONSE_MESSAGE{ "경매장 API 응답 형식이 올바르지 않습니다." };
        constexpr std::string_view INVALID_MARKET_RESPONSE_MESSAGE{ "거래소 API 응답 형식이 올바르지 않습니다." };
        constexpr std::string_view INVALID_OPTIONS_RESPONSE_MESSAGE{ "경매장 옵션 API 응답 형식이 올바르지 않습니다." };
        constexpr std::string_view INVALID_ARMORY_RESPONSE_MESSAGE{ "무기고 API 응답 형식이 올바르지 않습니다." };

        // ===== 시간 제한 · 시세 계산 =====
        constexpr int API_TIMEOUT_MS{ Core::APPConstants::API_TIMEOUT_SECONDS * Core::APPConstants::MS_PER_SECOND };
        constexpr std::size_t MARKET_CATEGORY_CODE{ 50000 };   // 거래소 재료 카테고리
        constexpr int FIRST_PAGE{ 1 };                         // 검색 첫 페이지 (거래소 · 경매장 공통)
        constexpr double PRICE_SCALE{ 100.0 };                 // 소수 둘째 자리까지 (10^2)
        constexpr wchar_t PATH_SEPARATOR{ L'/' };              // 상세 조회 경로 구분자 ("/markets/items/{Id}")

        // 운명의 파편 주머니 3종 (이름 · 주머니 1개당 파편 수) → 파편 1개 단가가 가장 싼 주머니 기준
        struct ShardPouch
        {
            std::string_view m_name;       // 거래소 아이템 이름
            double m_shardCount;           // 주머니 1개에 든 파편 수
        };
        constexpr std::array SHARD_POUCHES{
            ShardPouch{ .m_name = "운명의 파편 주머니(소)", .m_shardCount = 1000.0 },   // 소 = 1000개
            ShardPouch{ .m_name = "운명의 파편 주머니(중)", .m_shardCount = 2000.0 },   // 중 = 2000개
            ShardPouch{ .m_name = "운명의 파편 주머니(대)", .m_shardCount = 3000.0 }    // 대 = 3000개
        };

        // ===== 응답 JSON 키 =====
        constexpr std::string_view KEY_ITEMS{ "Items" };
        constexpr std::string_view KEY_NAME{ "Name" };
        constexpr std::string_view KEY_RECENT_PRICE{ "RecentPrice" };
        constexpr std::string_view KEY_CURRENT_MIN_PRICE{ "CurrentMinPrice" };
        constexpr std::string_view KEY_BUNDLE_COUNT{ "BundleCount" };
        constexpr std::string_view KEY_ID{ "Id" };                     // 거래소 아이템 번호 (상세 조회용)
        constexpr std::string_view KEY_STATS{ "Stats" };               // 상세 응답의 날짜별 통계 배열
        constexpr std::string_view KEY_AVG_PRICE{ "AvgPrice" };        // 그날 평균 거래가
        constexpr std::string_view KEY_SERVER_NAME{ "ServerName" };
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };
        constexpr std::string_view KEY_CHARACTER_CLASS_NAME{ "CharacterClassName" };
        constexpr std::string_view KEY_ITEM_MAX_LEVEL{ "ItemMaxLevel" };
        constexpr std::string_view KEY_ITEM_AVG_LEVEL{ "ItemAvgLevel" };
        constexpr std::string_view KEY_AUCTION_INFO{ "AuctionInfo" };
        constexpr std::string_view KEY_BUY_PRICE{ "BuyPrice" };
        constexpr std::string_view KEY_PAGE_SIZE{ "PageSize" };
        constexpr std::string_view KEY_TOTAL_COUNT{ "TotalCount" };
        constexpr std::string_view KEY_YDAY_AVG_PRICE{ "YDayAvgPrice" };
        constexpr std::string_view KEY_PAGE_NO{ "PageNo" };
        constexpr std::string_view KEY_ETC_OPTIONS{ "EtcOptions" };
        constexpr std::string_view KEY_ETC_SUBS{ "EtcSubs" };
        constexpr std::string_view KEY_ETC_VALUES{ "EtcValues" };
        constexpr std::string_view KEY_VALUE{ "Value" };
        constexpr std::string_view KEY_TEXT{ "Text" };
        constexpr std::string_view KEY_DISPLAY_VALUE{ "DisplayValue" };
        constexpr std::string_view KEY_GRADE{ "Grade" };
        constexpr std::string_view KEY_GRADE_QUALITY{ "GradeQuality" };
        constexpr std::string_view KEY_OPTIONS{ "Options" };
        constexpr std::string_view KEY_TYPE{ "Type" };
        constexpr std::string_view KEY_OPTION_NAME{ "OptionName" };
        constexpr std::string_view KEY_IS_VALUE_PERCENTAGE{ "IsValuePercentage" };
        constexpr std::string_view KEY_BID_START_PRICE{ "BidStartPrice" };
        constexpr std::string_view KEY_END_DATE{ "EndDate" };
        constexpr std::string_view KEY_TRADE_ALLOW_COUNT{ "TradeAllowCount" };
        constexpr std::string_view JSON_NULL{ "null" };

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

        // 비어 있으면 JSON null, 아니면 JSON 문자열 (요청 본문용)
        std::string ToJSONStringOrNull(std::string_view text)
        {
            return text.empty() ? std::string{ JSON_NULL } : QuoteJSONString(text);
        }

        // 거래소 목록 요청 본문 (카테고리 + 등급 · 이름 조건(비어 있으면 없음), 현재 최저가 높은 순)
        std::string BuildMarketListPayload(int categoryCode, std::string_view itemGrade, std::string_view itemName, int page)
        {
            return std::format(R"({{"Sort":"CURRENT_MIN_PRICE","CategoryCode":{},"ItemGrade":{},"ItemName":{},"PageNo":{},"SortCondition":"DESC"}})",
                categoryCode, ToJSONStringOrNull(itemGrade), ToJSONStringOrNull(itemName), page);
        }

        // 값이 있으면 숫자, 없으면 JSON null (요청 본문용)
        std::string ToJSONNumberOrNull(const std::optional<int>& value)
        {
            return value.has_value() ? std::to_string(*value) : std::string{ JSON_NULL };
        }

        // 경매장 악세 검색 요청 본문 (연마 효과 조건은 EtcOptions 배열, 즉시 구매가 낮은 순)
        std::string BuildAccessoryPayload(const Models::AccessorySearchQuery& query)
        {
            std::string etcOptions;
            for (const Models::AccessoryOptionFilter& option : query.m_options)
            {
                if (!etcOptions.empty()) etcOptions.push_back(',');
                etcOptions += std::format(R"({{"FirstOption":{},"SecondOption":{},"MinValue":{},"MaxValue":null}})",
                    option.m_firstOption, option.m_secondOption, ToJSONNumberOrNull(option.m_minValue));
            }

            return std::format(R"({{"ItemGradeQuality":{},"EtcOptions":[{}],"Sort":"BUY_PRICE","CategoryCode":{},"ItemTier":{},"ItemGrade":{},"PageNo":{},"SortCondition":"ASC"}})",
                ToJSONNumberOrNull(query.m_minQuality), etcOptions, query.m_categoryCode, Core::APPConstants::ACCESSORY_TIER,
                QuoteJSONString(query.m_gradeName), query.m_page);
        }

        // 응답 본문을 JSON으로 (HTML 오류 페이지 등이 오면 알기 쉬운 예외로)
        JSONValue ParseResponseBody(const HTTPResponse& response, std::string_view invalidMessage)
        {
            try
            {
                return ParseJSONText(response.m_body);
            }
            catch (const std::exception&)
            {
                throw Core::APIRequestException{ HTTP_STATUS_OK, std::string{ invalidMessage } };
            }
        }

        // 숫자 칸 읽기 (없거나 null이면 0 → 거래 · 매물이 없는 아이템)
        double ReadNumberOrZero(const JSONValue& parent, std::string_view key) noexcept
        {
            const JSONValue& value{ parent.FindMember(key) };
            return value.GetType() == JSONType::NUMBER_VALUE ? value.GetNumber() : 0.0;
        }

        // C# Math.Round(value, 2)와 같이 짝수 반올림 (std::nearbyint의 기본 반올림 모드)
        double RoundPrice(double value) noexcept
        {
            return std::nearbyint(value * PRICE_SCALE) / PRICE_SCALE;
        }

        // UTF-8 글자 중간에서 자르지 않도록 maxLength 이하의 글자 경계 위치를 찾음
        std::size_t FindUTF8Boundary(std::string_view text, std::size_t maxLength) noexcept
        {
            std::size_t length{ std::min(maxLength, text.size()) };
            while (length > 0 && length < text.size()
                && (static_cast<unsigned char>(text[length]) & UTF8_CONTINUATION_MASK) == UTF8_CONTINUATION_BITS)
            {
                --length;
            }
            return length;
        }

        // 오류 응답 본문에서 설명만 뽑아 짧게 정리 (JSON의 Message 필드 우선, 없으면 원문, 원본 ExtractErrorDetail)
        std::optional<std::string> ExtractErrorDetail(std::string_view body)
        {
            std::string detail{ body };
            try
            {
                const JSONValue root{ ParseJSONText(body) };
                const JSONValue& message{ root.FindMember(KEY_ERROR_MESSAGE).GetType() == JSONType::STRING_VALUE
                    ? root.FindMember(KEY_ERROR_MESSAGE) : root.FindMember(KEY_ERROR_MESSAGE_LOWER) };
                if (message.GetType() == JSONType::STRING_VALUE) detail = message.GetString();
            }
            catch (const std::exception&)
            {
                // JSON이 아니면 원문 사용
            }

            std::replace_if(detail.begin(), detail.end(), [](char c) { return LINE_BREAKS.contains(c); }, ' ');
            const std::size_t first{ detail.find_first_not_of(TRIM_SPACES) };
            if (first == std::string::npos) return std::nullopt;
            detail = detail.substr(first, detail.find_last_not_of(TRIM_SPACES) - first + 1);

            if (detail.size() <= Core::APPConstants::ERROR_DETAIL_MAX_LENGTH) return detail;
            detail.resize(FindUTF8Boundary(detail, Core::APPConstants::ERROR_DETAIL_MAX_LENGTH));
            return detail + std::string{ ELLIPSIS };
        }

        // 실패 응답을 상황별 예외로 바꿈 (C# EnsureSuccess와 같음, 그 밖의 오류는 서버 설명을 덧붙임)
        void EnsureSuccess(const HTTPResponse& response, std::string_view apiName)
        {
            if (WinHTTPClient::IsSuccessStatus(response.m_statusCode)) return;
            if (response.m_statusCode == HTTP_STATUS_UNAUTHORIZED) throw Core::APIKeyInvalidException{};
            if (response.m_statusCode == HTTP_STATUS_TOO_MANY_REQUESTS) throw Core::APIRateLimitException{};
            if (response.m_statusCode == HTTP_STATUS_SERVICE_UNAVAILABLE) throw Core::APIMaintenanceException{};

            std::string message{ std::format("{} API 요청 실패 (HTTP {})", apiName, response.m_statusCode) };
            if (const std::optional<std::string> detail{ ExtractErrorDetail(response.m_body) }; detail.has_value()) message += "\n" + *detail;
            throw Core::APIRequestException{ response.m_statusCode, message };
        }

        // 헤더 값을 정수로 (없거나 숫자가 아니면 nullopt)
        std::optional<std::int64_t> ReadIntegerHeader(const HTTPResponse& response, std::wstring_view name)
        {
            const std::optional<std::wstring> text{ FindHeader(response, name) };
            if (!text.has_value() || text->empty()) return std::nullopt;

            std::int64_t value{};
            for (const wchar_t character : *text)
            {
                if (character < L'0' || character > L'9') return std::nullopt;
                value = value * DECIMAL_BASE + (character - L'0');
            }
            return value;
        }

        std::optional<int> ToOptionalInt(const std::optional<std::int64_t>& value)
        {
            if (!value.has_value()) return std::nullopt;
            return static_cast<int>(std::min<std::int64_t>(*value, std::numeric_limits<int>::max()));
        }

        // 경매장 검색 요청 본문 (C# JObject와 같은 키 순서)
        std::string BuildAuctionPayload(std::string_view itemName, int itemTier, int page)
        {
            return std::format(R"({{"CategoryCode":{},"ItemTier":{},"ItemName":{},"Sort":"BUY_PRICE","SortCondition":"ASC","PageNo":{}}})",
                Core::APPConstants::GEM_CATEGORY_CODE, itemTier, QuoteJSONString(itemName), page);
        }

        // 이 페이지에서 이름이 정확히 같은 매물의 즉시 구매가 최저값
        // (부분 일치 검색이라 다른 레벨도 섞여 나오고, 즉시 구매가가 없는 입찰 전용 매물은 제외)
        std::optional<std::int64_t> FindLowestBuyPrice(const std::vector<JSONValue>& items, std::string_view itemName)
        {
            std::optional<std::int64_t> lowest;
            for (const JSONValue& item : items)
            {
                if (std::string_view{ item.FindMember(KEY_NAME).GetString() } != itemName) continue;

                const JSONValue& buyPrice{ item.FindMember(KEY_AUCTION_INFO).FindMember(KEY_BUY_PRICE) };
                if (buyPrice.GetType() != JSONType::NUMBER_VALUE || buyPrice.GetNumber() <= 0.0) continue;

                const std::int64_t price{ std::llround(buyPrice.GetNumber()) };
                if (!lowest.has_value() || price < *lowest) lowest = price;
            }
            return lowest;
        }

        // 재시도 전 잠깐 대기 (중간에 중단 요청이 오면 바로 깨어남)
        void WaitBeforeRetry(std::stop_token stopToken)
        {
            std::mutex waitMutex;
            std::condition_variable_any wakeUp;
            std::unique_lock lock{ waitMutex };
            static_cast<void>(wakeUp.wait_for(lock, stopToken, std::chrono::milliseconds{ Core::APPConstants::TRANSIENT_RETRY_DELAY_MS },
                [] { return false; }));
        }

        // 검색 응답의 Items가 배열이 아니면 형식 변경으로 신고 (빈 결과로 조용히 처리되지 않게)
        void CheckItemsArray(const JSONValue& root, std::string_view context) noexcept
        {
            if (root.GetType() != JSONType::OBJECT_VALUE) return;                                 // 객체가 아닌 응답은 기존 오류 처리
            if (root.FindMember(KEY_ITEMS).GetType() == JSONType::ARRAY_VALUE) return;            // 정상 (빈 배열 포함)
            Core::APIFormatMonitor::Report(context, "Items 배열 없음");                            // 신고 (실행 중 한 번)
        }

        // 상세 응답 한 항목의 오늘 평균 거래가 (오늘 거래가 아직 없으면 가장 최근에 거래된 날의 평균가, 통계 기간 내내 없으면 nullopt)
        std::optional<double> ComputeAveragePrice(const JSONValue& entry)
        {
            for (const JSONValue& day : entry.FindMember(KEY_STATS).GetItems())   // 오늘부터 최신 날짜 순서
            {
                const double avgPrice{ day.FindMember(KEY_AVG_PRICE).GetNumber() };   // 그날 평균 거래가
                if (avgPrice > 0.0) return avgPrice;                                 // 거래가 있는 가장 최근 날
            }
            return std::nullopt;   // 통계 기간 동안 거래 없음
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

    // 단가 = 오늘 평균 거래가 (파편은 주머니 3종 중 파편 1개가 가장 싼 값, 묶음은 1개 단가)
    double WinHTTPMarketAPI::GetPrice(std::string_view itemName, bool isShard, bool isBundle)
    {
        if (isShard) return GetShardPrice(itemName);   // itemName = 주머니 공통 검색어 ("운명의 파편 주머니")

        const std::optional<std::int64_t> itemId{ FindItemId(itemName, itemName) };   // 이름 → 아이템 번호
        if (!itemId.has_value())
        {
            throw std::runtime_error{ std::format("거래소에서 '{}' 시세를 찾을 수 없습니다.", itemName) };
        }

        const std::optional<MarketAverage> average{ GetAveragePrice(*itemId) };       // 오늘 평균 거래가
        if (!average.has_value()) throw std::runtime_error{ std::format("'{}'의 유효한 시세가 없습니다.", itemName) };
        if (!isBundle) return RoundPrice(average->m_price);                           // 낱개 판매 재료

        // 하드코딩 묶음 수량 대신 응답의 실제 BundleCount 사용
        if (average->m_bundleCount <= 0) throw std::runtime_error{ std::format("'{}'의 묶음 수량 정보가 없습니다.", itemName) };
        return RoundPrice(average->m_price / static_cast<double>(average->m_bundleCount));
    }

    // 파편 1개 단가 = 주머니 3종(소 · 중 · 대)의 오늘 평균 ÷ 파편 수 중 가장 싼 값 (한 종류가 없어도 나머지로 계산)
    double WinHTTPMarketAPI::GetShardPrice(std::string_view keyword)
    {
        std::optional<double> cheapest;   // 지금까지 가장 싼 파편 1개 단가
        for (const ShardPouch& pouch : SHARD_POUCHES)
        {
            const std::optional<std::int64_t> itemId{ FindItemId(keyword, pouch.m_name) };   // 검색어 한 번으로 3종 번호를 함께 기억
            if (!itemId.has_value()) continue;                                              // 거래소에 없는 주머니는 건너뜀

            const std::optional<MarketAverage> average{ GetAveragePrice(*itemId) };         // 주머니 오늘 평균 거래가
            if (!average.has_value()) continue;                                             // 거래 기록이 없는 주머니도 건너뜀

            const double perShard{ average->m_price / pouch.m_shardCount };                // 파편 1개 단가
            if (!cheapest.has_value() || perShard < *cheapest) cheapest = perShard;
        }

        if (!cheapest.has_value())
        {
            throw std::runtime_error{ std::format("거래소에서 '{}' 시세를 찾을 수 없습니다.", keyword) };
        }
        return RoundPrice(*cheapest);
    }

    // 이름 → 거래소 아이템 번호 (한 번 찾은 번호는 기억해서 다음부터는 검색 요청을 생략)
    std::optional<std::int64_t> WinHTTPMarketAPI::FindItemId(std::string_view keyword, std::string_view itemName)
    {
        {
            std::scoped_lock lock{ m_itemIdMutex };
            if (const auto found{ m_itemIds.find(std::string{ itemName }) }; found != m_itemIds.end()) return found->second;   // 기억한 번호
        }

        // 네트워크 요청은 잠금 밖에서 (다른 스레드의 조회를 오래 막지 않도록)
        const HTTPResponse response{ SendRequest(VERB_POST, MARKET_PATH, BuildMarketPayload(keyword)) };
        EnsureSuccess(response, "거래소");
        const JSONValue root{ ParseJSONText(response.m_body) };
        CheckItemsArray(root, "거래소 검색");   // 형식 확인

        std::scoped_lock lock{ m_itemIdMutex };
        for (const JSONValue& item : root.FindMember(KEY_ITEMS).GetItems())   // 검색 결과 전부 기억 (주머니 3종처럼 한 번에)
        {
            const double id{ item.FindMember(KEY_ID).GetNumber() };   // 아이템 번호
            if (id <= 0.0) continue;                                  // 번호 없는 항목은 무시
            m_itemIds.insert_or_assign(item.FindMember(KEY_NAME).GetString(), static_cast<std::int64_t>(id));
        }

        if (const auto found{ m_itemIds.find(std::string{ itemName }) }; found != m_itemIds.end()) return found->second;
        return std::nullopt;   // 이름이 정확히 같은 아이템이 없음
    }

    // 거래소 상세 (GET /markets/items/{Id}) → 오늘 평균 거래가 + 묶음 수량
    std::optional<WinHTTPMarketAPI::MarketAverage> WinHTTPMarketAPI::GetAveragePrice(std::int64_t itemId)
    {
        const std::wstring path{ std::format(L"{}{}{}", MARKET_PATH, PATH_SEPARATOR, itemId) };   // "/markets/items/66102006"
        const HTTPResponse response{ SendRequest(VERB_GET, path, std::string{}) };
        EnsureSuccess(response, "거래소 상세");
        const JSONValue root{ ParseJSONText(response.m_body) };

        // 응답은 배열 (거래 가능 횟수별로 항목이 나뉠 수 있음) → 평균가를 낼 수 있는 첫 항목 사용
        const std::vector<JSONValue> single{ root };                                                     // 객체 하나로 온 경우 대비
        const std::vector<JSONValue>& entries{ root.GetType() == JSONType::ARRAY_VALUE ? root.GetItems() : single };
        for (const JSONValue& entry : entries)
        {
            const std::optional<double> price{ ComputeAveragePrice(entry) };   // 오늘 평균 거래가
            if (!price.has_value()) continue;
            return MarketAverage{ .m_price = *price, .m_bundleCount = static_cast<int>(entry.FindMember(KEY_BUNDLE_COUNT).GetNumber()) };
        }
        Core::APIFormatMonitor::Report("거래소 상세", "Stats 통계 없음");   // 형식이 바뀌었을 가능성 신고 (실행 중 한 번)
        return std::nullopt;
    }

    std::vector<Models::CharacterInfo> WinHTTPMarketAPI::GetRoster(std::string_view charName)
    {
        const std::string path{ std::string{ SIBLINGS_PATH_PREFIX } + EscapeDataString(charName)
                                + std::string{ SIBLINGS_PATH_SUFFIX } };

        const HTTPResponse response{ SendRequest(VERB_GET, Core::TextEncoding::UTF8ToWide(path), std::string{}) };
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
                ReadOptionalText(entry, KEY_ITEM_AVG_LEVEL),
                ReadOptionalText(entry, KEY_SERVER_NAME));   // 낙원력 트래커의 닉네임 변경 감지(같은 서버 · 직업)에 사용
        }
        return roster;
    }

    Models::CharacterArmory WinHTTPMarketAPI::GetArmory(std::string_view charName)  // 캐릭터 무기고 조회 (프로필 · 장비 · 보석 등 전체 응답을 해석)
    {
        const std::string path{ std::string{ ARMORY_PATH_PREFIX } + EscapeDataString(charName) + std::string{ ARMORY_PATH_SUFFIX } };  // 요청 경로 = 무기고 경로 + 이스케이프한 캐릭터 이름 + 뒷부분

        const HTTPResponse response{ SendRequest(VERB_GET, Core::TextEncoding::UTF8ToWide(path), std::string{}) };  // GET 요청 전송
        EnsureSuccess(response, "무기고");  // 응답 상태가 성공이 아니면 예외

#ifdef _DEBUG  // Debug 빌드에서만 적용
        // 원본 응답 저장 (실패해도 조회는 계속 → 예외를 삼킴, Debug 전용)
        try  // 저장 시도
        {
            std::ofstream dump{ Core::APPConstants::GetUserDataFilePath(ARMORY_DEBUG_DUMP_FILE_NAME), std::ios::binary | std::ios::trunc };  // 사용자 데이터 폴더의 덤프 파일을 새로 열기
            dump.write(response.m_body.data(), static_cast<std::streamsize>(response.m_body.size()));  // 응답 본문을 그대로 기록
        }
        catch (const std::exception&)  // 저장 실패는 무시하고 조회를 계속
        {
        }
#endif  // Debug 빌드 전용 구간 끝

        // 없는 닉네임은 200 + null로 옴 → ParseArmory가 "캐릭터를 찾을 수 없어요" 예외
        const JSONValue root{ response.m_body.empty() ? JSONValue{} : ParseResponseBody(response, INVALID_ARMORY_RESPONSE_MESSAGE) };  // 빈 응답이면 빈 JSON 값, 아니면 JSON 해석
        return ParseArmory(root);  // JSON 값을 무기고 모델로 해석해 반환
    }

    std::vector<Models::MarketItem> WinHTTPMarketAPI::SearchMarketItems(int categoryCode, std::string_view itemGrade, std::string_view itemName)
    {
        std::vector<Models::MarketItem> items;
        const int lastPage{ FIRST_PAGE + Core::APPConstants::MAX_MARKET_PAGES - 1 };

        for (int page{ FIRST_PAGE }; page <= lastPage; ++page)
        {
            // 다음 페이지를 넘기기 전에 호출 한도 확인 (한도를 다 쓰기 전에 멈추고 429와 같은 안내)
            if (page > FIRST_PAGE && IsRateLimitExhausted()) throw Core::APIRateLimitException{};

            const HTTPResponse response{ SendRequest(VERB_POST, MARKET_PATH, BuildMarketListPayload(categoryCode, itemGrade, itemName, page)) };
            EnsureSuccess(response, "거래소");

            JSONValue root;
            try
            {
                root = ParseJSONText(response.m_body);
            }
            catch (const std::exception&)
            {
                // HTML 오류 페이지 등이 오면 알기 쉬운 예외로
                throw Core::APIRequestException{ HTTP_STATUS_OK, std::string{ INVALID_MARKET_RESPONSE_MESSAGE } };
            }

            CheckItemsArray(root, "거래소 검색");                                                  // 형식 확인
            const std::vector<JSONValue>& pageItems{ root.FindMember(KEY_ITEMS).GetItems() };
            for (const JSONValue& item : pageItems)
            {
                items.push_back(Models::MarketItem{
                    std::string{ item.FindMember(KEY_NAME).GetString() },
                    ReadNumberOrZero(item, KEY_CURRENT_MIN_PRICE),
                    ReadNumberOrZero(item, KEY_YDAY_AVG_PRICE),
                    ReadNumberOrZero(item, KEY_RECENT_PRICE),
                    std::string{ item.FindMember(KEY_GRADE).GetString() } });
            }

            // 마지막 페이지 판정: 빈 페이지 · 한 페이지보다 적게 옴 · 전체 개수를 다 받음
            const auto pageSize{ static_cast<std::size_t>(ReadNumberOrZero(root, KEY_PAGE_SIZE)) };
            const auto totalCount{ static_cast<std::size_t>(ReadNumberOrZero(root, KEY_TOTAL_COUNT)) };
            if (pageItems.empty() || (pageSize > 0 && pageItems.size() < pageSize) || (totalCount > 0 && items.size() >= totalCount)) break;
        }
        return items;
    }

    std::vector<Models::AuctionOption> WinHTTPMarketAPI::GetAuctionOptions()
    {
        const HTTPResponse response{ SendRequest(VERB_GET, AUCTION_OPTIONS_PATH, std::string{}) };
        EnsureSuccess(response, "경매장 옵션");
        const JSONValue root{ ParseResponseBody(response, INVALID_OPTIONS_RESPONSE_MESSAGE) };

        // 묶음(EtcOptions) → 옵션(EtcSubs) → 값(EtcValues, 없을 수 있음)
        std::vector<Models::AuctionOption> options;
        for (const JSONValue& group : root.FindMember(KEY_ETC_OPTIONS).GetItems())
        {
            const int firstOption{ static_cast<int>(group.FindMember(KEY_VALUE).GetNumber()) };
            const std::string& groupName{ group.FindMember(KEY_TEXT).GetString() };

            for (const JSONValue& sub : group.FindMember(KEY_ETC_SUBS).GetItems())
            {
                Models::AuctionOption& option{ options.emplace_back(Models::AuctionOption{ firstOption,
                    static_cast<int>(sub.FindMember(KEY_VALUE).GetNumber()), groupName, sub.FindMember(KEY_TEXT).GetString(), {} }) };

                for (const JSONValue& value : sub.FindMember(KEY_ETC_VALUES).GetItems())
                {
                    const int requestValue{ static_cast<int>(value.FindMember(KEY_VALUE).GetNumber()) };
                    option.m_values.push_back(Models::AuctionOptionValue{
                        ReadOptionalText(value, KEY_DISPLAY_VALUE).value_or(std::to_string(requestValue)), requestValue });
                }
            }
        }
        return options;
    }

    Models::AccessorySearchPage WinHTTPMarketAPI::SearchAccessories(const Models::AccessorySearchQuery& query)
    {
        const HTTPResponse response{ SendRequest(VERB_POST, AUCTION_PATH, BuildAccessoryPayload(query)) };
        EnsureSuccess(response, "경매장");
        const JSONValue root{ ParseResponseBody(response, INVALID_AUCTION_RESPONSE_MESSAGE) };

        Models::AccessorySearchPage page{ {},
            static_cast<int>(ReadNumberOrZero(root, KEY_PAGE_NO)),
            static_cast<int>(ReadNumberOrZero(root, KEY_PAGE_SIZE)),
            static_cast<int>(ReadNumberOrZero(root, KEY_TOTAL_COUNT)) };
        if (page.m_page <= 0) page.m_page = query.m_page;   // 응답에 페이지 번호가 없으면 요청한 번호

        for (const JSONValue& item : root.FindMember(KEY_ITEMS).GetItems())
        {
            Models::AccessoryListing listing;
            listing.m_name = item.FindMember(KEY_NAME).GetString();
            listing.m_gradeName = item.FindMember(KEY_GRADE).GetString();
            listing.m_quality = static_cast<int>(ReadNumberOrZero(item, KEY_GRADE_QUALITY));

            for (const JSONValue& option : item.FindMember(KEY_OPTIONS).GetItems())
            {
                listing.m_options.push_back(Models::AccessoryOptionLine{ option.FindMember(KEY_TYPE).GetString(),
                    option.FindMember(KEY_OPTION_NAME).GetString(), ReadNumberOrZero(option, KEY_VALUE),
                    option.FindMember(KEY_IS_VALUE_PERCENTAGE).GetBool() });
            }

            // 즉시 구매가가 없으면(입찰 전용) nullopt
            const JSONValue& auctionInfo{ item.FindMember(KEY_AUCTION_INFO) };
            const double buyPrice{ ReadNumberOrZero(auctionInfo, KEY_BUY_PRICE) };
            if (buyPrice > 0.0) listing.m_buyPrice = std::llround(buyPrice);
            listing.m_bidStartPrice = std::llround(ReadNumberOrZero(auctionInfo, KEY_BID_START_PRICE));
            listing.m_endDate = auctionInfo.FindMember(KEY_END_DATE).GetString();
            if (const JSONValue& tradeCount{ auctionInfo.FindMember(KEY_TRADE_ALLOW_COUNT) }; tradeCount.GetType() == JSONType::NUMBER_VALUE)
            {
                listing.m_tradeAllowCount = static_cast<int>(tradeCount.GetNumber());
            }

            page.m_listings.push_back(std::move(listing));
        }
        return page;
    }

    std::optional<std::int64_t> WinHTTPMarketAPI::GetLowestBuyPrice(std::string_view itemName, int itemTier, std::stop_token stopToken)
    {
        const int lastPage{ FIRST_PAGE + Core::APPConstants::MAX_SEARCH_PAGES - 1 };

        for (int page{ FIRST_PAGE }; page <= lastPage; ++page)
        {
            // 다음 페이지를 넘기기 전에 호출 한도 확인 (페이지 넘김은 예상 호출 수에 없던 추가 요청 → 한도를 다 쓰기 전에 멈춤)
            // 429를 받은 것과 같은 예외로 알려서 스케줄러가 한도가 다시 채워질 때까지 자동 갱신을 쉬게 함
            if (page > FIRST_PAGE && IsRateLimitExhausted()) throw Core::APIRateLimitException{};

            const JSONValue root{ SearchAuctionWithRetry(itemName, itemTier, page, stopToken) };
            const std::vector<JSONValue>& items{ root.FindMember(KEY_ITEMS).GetItems() };
            if (items.empty()) return std::nullopt;   // 매물 자체가 없음

            if (const std::optional<std::int64_t> lowest{ FindLowestBuyPrice(items, itemName) }; lowest.has_value()) return lowest;

            // 마지막 페이지까지 봤는데 없음 (한 페이지보다 적게 왔으면 다음 페이지는 없음)
            const auto pageSize{ static_cast<std::size_t>(root.FindMember(KEY_PAGE_SIZE).GetNumber()) };
            if (pageSize > 0 && items.size() < pageSize) return std::nullopt;
        }
        return std::nullopt;   // 최대 페이지까지 넘겨봤는데 없음
    }

    Models::RateLimitState WinHTTPMarketAPI::GetRateLimit() const
    {
        std::scoped_lock lock{ m_rateMutex };
        return m_rateLimit;
    }

    bool WinHTTPMarketAPI::IsRateLimitExhausted() const
    {
        std::scoped_lock lock{ m_rateMutex };
        return m_rateLimit.IsExhausted(Core::LocalTime::GetUnixSeconds());
    }

    HTTPResponse WinHTTPMarketAPI::SendRequest(std::wstring_view verb, std::wstring_view path, std::string body)
    {
        HTTPResponse response{ m_client.Send(verb, path, BuildHeaders(), std::move(body)) };
        UpdateRateLimit(response);
        return response;
    }

    void WinHTTPMarketAPI::UpdateRateLimit(const HTTPResponse& response)
    {
        const Models::RateLimitHeaders headers{
            ToOptionalInt(ReadIntegerHeader(response, HEADER_RATE_LIMIT)),
            ToOptionalInt(ReadIntegerHeader(response, HEADER_RATE_REMAINING)),
            ReadIntegerHeader(response, HEADER_RATE_RESET),
            response.m_serverTime
        };

        std::scoped_lock lock{ m_rateMutex };
        m_rateLimit.ApplyResponse(headers, Core::LocalTime::GetUnixSeconds());
    }

    JSONValue WinHTTPMarketAPI::SearchAuctionWithRetry(std::string_view itemName, int itemTier, int page, std::stop_token stopToken)
    {
        for (int attempt{};; ++attempt)
        {
            if (stopToken.stop_requested()) throw Core::RequestCanceledException{};   // 다른 요청이 실패함 → 보내지 않음

            const bool canRetry{ attempt < Core::APPConstants::MAX_TRANSIENT_RETRIES };
            try
            {
                return SearchAuction(itemName, itemTier, page);
            }
            catch (const HTTPNetworkError&)
            {
                if (!canRetry || stopToken.stop_requested()) throw;   // 네트워크 끊김 · 시간 초과 → 한 번만 더
            }
            catch (const Core::APIRequestException& error)
            {
                if (!error.IsServerError() || !canRetry || stopToken.stop_requested()) throw;   // 5xx만 다시 시도
            }

            WaitBeforeRetry(stopToken);
        }
    }

    JSONValue WinHTTPMarketAPI::SearchAuction(std::string_view itemName, int itemTier, int page)
    {
        const HTTPResponse response{ SendRequest(VERB_POST, AUCTION_PATH, BuildAuctionPayload(itemName, itemTier, page)) };
        EnsureSuccess(response, "경매장");

        try
        {
            return ParseJSONText(response.m_body);
        }
        catch (const std::exception&)
        {
            // HTML 오류 페이지 등이 오면 알기 쉬운 예외로
            throw Core::APIRequestException{ HTTP_STATUS_OK, std::string{ INVALID_AUCTION_RESPONSE_MESSAGE } };
        }
    }
}