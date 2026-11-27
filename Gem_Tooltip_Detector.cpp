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
        constexpr std::wstring_view BOUND_MARK{ L"귀속" };                   // "캐릭터 귀속됨"
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

    std::string GemTooltipDetector::MakeValueKey(std::size_t gemIndex, std::size_t gradeIndex)
    {
        return std::format("{}{}{}{}", VALUE_PREFIX, gemIndex, KEY_SEPARATOR, gradeIndex);   // "Gem/0/1"
    }

    std::optional<std::pair<std::size_t, std::size_t>> GemTooltipDetector::ParseValueKey(std::string_view valueKey)
    {
        if (!valueKey.starts_with(VALUE_PREFIX)) return std::nullopt;                       // 젬 값이 아님
        const std::string_view rest = valueKey.substr(VALUE_PREFIX.size());                 // "0/1"
        const std::size_t separator = rest.find(KEY_SEPARATOR);                             // 가운데
        if (separator == std::string_view::npos) return std::nullopt;                       // 모양이 다름
        std::size_t gem = 0;                                                                // 젬 번호
        std::size_t grade = 0;                                                              // 등급 번호
        const auto gemResult = std::from_chars(rest.data(), rest.data() + separator, gem, DECIMAL_BASE);   // 젬
        const auto gradeResult = std::from_chars(rest.data() + separator + 1, rest.data() + rest.size(), grade, DECIMAL_BASE);   // 등급
        if (gemResult.ec != std::errc{} || gradeResult.ec != std::errc{}) return std::nullopt;   // 숫자가 아님
        if (gem >= Models::BoundGemData::GEM_COUNT || grade >= Models::BoundGemData::GRADE_COUNT) return std::nullopt;   // 범위 밖
        return std::pair{ gem, grade };                                                      // (젬, 등급)
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

    std::optional<ScanResult> GemTooltipDetector::Detect(const FrameImage&, const DetectorContext& context)
    {
        const auto quantity = OCRTextParser::FindQuantityAfter(context.m_hoverText, QUANTITY_KEYWORD, QUANTITY_MAX_DISTANCE, context.m_hoverArea.m_width);   // 전체 보유 수량 ("개"를 숫자로 읽어도 바로잡음)
        if (!quantity) return std::nullopt;                                                 // 수량이 있는 툴팁이 아님
        std::optional<std::size_t> gem;                                                     // 젬 종류
        std::optional<std::size_t> grade;                                                   // 등급
        bool isBound = false;                                                               // 캐릭터 귀속
        for (const TextLine& line : context.m_hoverText.m_lines)                            // 줄마다
        {
            if (!gem) gem = MatchGemName(line.m_text);                                      // 제목 (위에서 처음 맞는 줄)
            if (!grade) grade = MatchGrade(line.m_text);                                    // 등급 줄
            isBound = isBound || line.m_text.contains(BOUND_MARK);                          // 귀속 표시
        }
        if (!gem || !grade || !isBound) return std::nullopt;                                // 귀속 젬 툴팁이 아님
        ScanResult result;                                                                  // 결과
        result.m_values.push_back(ScanValue{ MakeValueKey(*gem, *grade), *quantity });     // "Gem/0/1" = 수량
        return result;                                                                      // 젬 결과
    }
}
