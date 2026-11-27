module LostArkMultiTool.Vision.GemTooltipDetector;  // 귀속 젬 툴팁 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // UTF-8 → 넓은 글자 가져오기
import LostArkMultiTool.Models.BoundGemData;  // 젬 이름 · 등급 이름 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // OCR 글자 해석 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // OCR 결과 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr std::wstring_view QUANTITY_KEYWORD{ L"전체 보유 수량" };   // 수량 줄 글자
        constexpr std::size_t QUANTITY_MAX_DISTANCE{ 2 };                    // 수량 줄 글자 오인식 허용
        constexpr std::wstring_view BOUND_MARK{ L"귀속" };                   // "캐릭터 귀속됨" (없으면 거래 가능 젬)
        constexpr std::wstring_view PROCESS_MARK{ L"가공" };                 // 가공 정보 ("젬 가공 가능 횟수")
        constexpr std::array<std::wstring_view, 5> INFO_MARKS{ L"포인트", L"횟수", L"의지력", L"Lv", L"%" };   // 가공 젬 구분에 쓰는 정보 줄 (창고 칸 숫자는 빼려고)
        constexpr char KIND_BOUND{ 'B' };                                    // 귀속 묶음
        constexpr char KIND_TRADABLE{ 'T' };                                 // 거래 가능 묶음
        constexpr char KIND_PROCESSED{ 'P' };                                // 가공 젬
        constexpr char PROCESSED_GROUP_SEPARATOR{ '.' };                     // 구분 글자 안 줄 구분
        constexpr int PROCESSED_COUNT{ 1 };                                  // 가공 젬은 한 개씩
        constexpr std::wstring_view GRADE_LINE_TEXT{ L"아크그리드젬" };       // 등급 줄 (띄어쓰기 없이)
        constexpr std::size_t GRADE_LINE_MAX_DISTANCE{ 2 };                  // 등급 줄 오인식 허용
        constexpr std::size_t NAME_MAX_DISTANCE{ 1 };                        // 젬 이름 한 글자 오인식 허용 (이름끼리는 두 글자 이상 다름)
        constexpr wchar_t NAME_SEPARATOR{ L':' };                            // 이름 가운데 ':' (OCR이 다르게 읽기도 해서 빼고 비교)
        constexpr char KEY_SEPARATOR{ '/' };                                 // 값 이름 구분 글자
        constexpr int DECIMAL_BASE{ 10 };                                    // 십진수

        // 비교용 글자: 띄어쓰기 · ':' 빼기
        [[nodiscard]] std::wstring Normalize(std::wstring_view text)
        {
            std::wstring compact = OCRTextParser::Compact(text);              // 띄어쓰기 지움
            std::erase(compact, NAME_SEPARATOR);                              // ':' 지움
            return compact;                                                   // 비교용
        }
    }

    std::string_view GemTooltipDetector::GetScreenName() const noexcept
    {
        return SCREEN_NAME;   // "gem_tooltip"
    }

    std::string GemTooltipDetector::MakeValueKey(std::size_t gemIndex, std::size_t gradeIndex, Models::GemStockKind kind, std::string_view processedKey)
    {
        const char kindMark = kind == Models::GemStockKind::STOCK_BOUND ? KIND_BOUND
            : kind == Models::GemStockKind::STOCK_TRADABLE ? KIND_TRADABLE : KIND_PROCESSED;          // 종류 글자
        std::string key = std::format("{}{}{}{}{}{}", VALUE_PREFIX, gemIndex, KEY_SEPARATOR, gradeIndex, KEY_SEPARATOR, kindMark);   // "Gem/0/1/B"
        if (kind == Models::GemStockKind::STOCK_PROCESSED) { key += KEY_SEPARATOR; key += processedKey; }   // "Gem/0/1/P/4.99"
        return key;
    }

    std::string GemTooltipDetector::MakeProcessedKey(const TextResult& text)
    {
        std::string key;                                                                    // 결과
        for (const TextLine& line : text.m_lines)                                           // 줄마다
        {
            if (std::ranges::none_of(INFO_MARKS, [&line](std::wstring_view mark) { return line.m_text.contains(mark); })) continue;   // 정보 줄만
            std::string digits;                                                             // 이 줄 숫자
            for (wchar_t character : line.m_text)
                if (character >= L'0' && character <= L'9') digits += static_cast<char>(character);   // 숫자만 (OCR 띄어쓰기 · 점 차이에 흔들리지 않게)
            if (digits.empty()) continue;                                                   // 숫자 없는 줄
            if (!key.empty()) key += PROCESSED_GROUP_SEPARATOR;                             // 줄 구분
            key += digits;                                                                  // 덧붙임
        }
        return key;                                                                         // "4.99.9101.1.1.013…"
    }

    std::optional<GemValueKey> GemTooltipDetector::ParseValueKey(std::string_view valueKey)
    {
        if (!valueKey.starts_with(VALUE_PREFIX)) return std::nullopt;                       // 젬 값이 아님
        const std::string_view rest = valueKey.substr(VALUE_PREFIX.size());                 // "0/1/B"
        const std::size_t separator = rest.find(KEY_SEPARATOR);                             // 젬 뒤
        if (separator == std::string_view::npos) return std::nullopt;                       // 모양이 다름
        const std::size_t kindSeparator = rest.find(KEY_SEPARATOR, separator + 1);          // 등급 뒤 (없으면 귀속)
        const std::size_t gradeEnd = kindSeparator == std::string_view::npos ? rest.size() : kindSeparator;   // 등급 끝
        GemValueKey key;                                                                    // 결과
        const auto gemResult = std::from_chars(rest.data(), rest.data() + separator, key.m_gemIndex, DECIMAL_BASE);   // 젬
        const auto gradeResult = std::from_chars(rest.data() + separator + 1, rest.data() + gradeEnd, key.m_gradeIndex, DECIMAL_BASE);   // 등급
        if (gemResult.ec != std::errc{} || gradeResult.ec != std::errc{} || gradeResult.ptr != rest.data() + gradeEnd) return std::nullopt;   // 숫자가 아님
        if (key.m_gemIndex >= Models::BoundGemData::GEM_COUNT || key.m_gradeIndex >= Models::BoundGemData::GRADE_COUNT) return std::nullopt;   // 범위 밖
        if (kindSeparator == std::string_view::npos) return key;                            // 예전 모양 "Gem/0/1" → 귀속
        const std::string_view kindText = rest.substr(kindSeparator + 1);                   // "B" · "T" · "P/구분"
        if (kindText.size() == 1 && kindText.front() == KIND_BOUND) return key;            // 귀속
        if (kindText.size() == 1 && kindText.front() == KIND_TRADABLE) { key.m_kind = Models::GemStockKind::STOCK_TRADABLE; return key; }   // 거래 가능
        if (kindText.size() > 2 && kindText.front() == KIND_PROCESSED && kindText[1] == KEY_SEPARATOR)   // 가공 젬
        {
            key.m_kind = Models::GemStockKind::STOCK_PROCESSED;
            key.m_processedKey = std::string{ kindText.substr(2) };                         // 구분 글자
            return key;
        }
        return std::nullopt;                                                                // 모르는 종류
    }

    std::optional<std::size_t> GemTooltipDetector::MatchGemName(std::wstring_view line)
    {
        const std::wstring compact = Normalize(line);                                       // 비교용 줄
        std::optional<std::size_t> best;                                                    // 가장 잘 맞는 젬
        std::size_t bestDistance = NAME_MAX_DISTANCE + 1;                                   // 그 거리
        for (std::size_t index = 0; index < Models::BoundGemData::GEM_COUNT; ++index)       // 젬마다
        {
            const std::wstring name = Normalize(Core::TextEncoding::UTF8ToWide(Models::BoundGemData::GEM_NAMES[index]));   // "질서의젬안정"
            const std::size_t distance = OCRTextParser::FindDistance(name, compact);       // 줄 안 이름과의 거리
            if (distance < bestDistance) { bestDistance = distance; best = index; }        // 더 잘 맞음
        }
        return best;                                                                        // 젬 번호 (없으면 nullopt)
    }

    std::optional<std::size_t> GemTooltipDetector::MatchGrade(std::wstring_view line)
    {
        const std::wstring compact = OCRTextParser::Compact(line);                          // 띄어쓰기 없이
        if (OCRTextParser::FindDistance(GRADE_LINE_TEXT, compact) > GRADE_LINE_MAX_DISTANCE) return std::nullopt;   // 등급 줄이 아님
        for (std::size_t index = 0; index < Models::BoundGemData::GRADE_COUNT; ++index)     // 등급마다
            if (compact.contains(Core::TextEncoding::UTF8ToWide(Models::BoundGemData::GRADE_NAMES[index]))) return index;   // "희귀"
        return std::nullopt;                                                                // 등급 글자 없음
    }

    std::optional<ScanResult> GemTooltipDetector::Detect(const FrameImage& frame, const DetectorContext& context)
    {
        const auto quantity = OCRTextParser::ResolveTooltipQuantity(frame, context.m_hoverArea, context.m_hoverText, context.m_text,
            QUANTITY_KEYWORD, QUANTITY_MAX_DISTANCE);                                       // 전체 보유 수량 (그 줄만 3배로 다시 읽음 · 겹침 수량 대조)
        std::optional<std::size_t> gem;                                                     // 젬 종류
        std::optional<std::size_t> grade;                                                   // 등급
        bool isBound = false;                                                               // 캐릭터 귀속
        bool isProcessing = false;                                                          // 가공 정보
        for (const TextLine& line : context.m_hoverText.m_lines)                            // 줄마다
        {
            if (!gem) gem = MatchGemName(line.m_text);                                      // 제목 (위에서 처음 맞는 줄)
            if (!grade) grade = MatchGrade(line.m_text);                                    // 등급 줄
            isBound = isBound || line.m_text.contains(BOUND_MARK);                          // 귀속 표시
            isProcessing = isProcessing || line.m_text.contains(PROCESS_MARK);              // 가공 표시
        }
        if (!gem || !grade) return std::nullopt;                                            // 젬 툴팁이 아님
        ScanResult result;                                                                  // 결과
        if (quantity)                                                                       // 묶음 젬 → 전체 보유 수량
        {
            const auto kind = isBound ? Models::GemStockKind::STOCK_BOUND : Models::GemStockKind::STOCK_TRADABLE;   // 귀속 · 거래 가능
            result.m_values.push_back(ScanValue{ MakeValueKey(*gem, *grade, kind), *quantity });   // "Gem/0/1/B" = 수량
            return result;
        }
        if (!isProcessing) return std::nullopt;                                             // 수량도 가공 정보도 없음 (거래소 목록 등)
        const std::string processedKey = MakeProcessedKey(context.m_hoverText);             // 이 젬 구분 글자
        if (processedKey.empty()) return std::nullopt;                                      // 정보 줄을 못 읽음
        result.m_values.push_back(ScanValue{ MakeValueKey(*gem, *grade, Models::GemStockKind::STOCK_PROCESSED, processedKey), PROCESSED_COUNT });   // 가공 젬 한 개
        return result;                                                                      // 젬 결과
    }
}
