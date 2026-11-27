export module LostArkMultiTool.Vision.OCRTextParser;  // OCR 글자 해석 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // OCR 결과 가져오기

// OCR이 읽은 글자에서 필요한 값 꺼내기 (상태 없는 함수 모음)
// - OCR은 쉼표를 마침표 · 띄어쓰기로, 한 글자를 비슷한 글자로 잘못 읽기도 함 → 숫자는 3자리 묶음 검사, 이름은 편집 거리로 맞춤
export namespace LostArkMultiTool::Vision::OCRTextParser  // OCR 해석 네임스페이스 내보내기 시작
{
    // 재료 이름이 상위 · 하위 중 어디에 들어가는지
    enum class MaterialTierHint
    {
        TIER_TOP,        // 상위 재료 (결정 · 위대한 · 상급)
        TIER_BOTTOM,     // 하위 재료
        TIER_BY_LEVEL    // 상위 · 하위 이름이 같음 (숨결 · 파편) → 캐릭터 레벨로 정함
    };

    // 툴팁 제목으로 알아볼 재료 이름 하나
    struct MaterialName
    {
        std::wstring_view m_name;   // 게임 아이템 이름 ("운명의 파괴석 결정")
        std::string_view m_key;     // 계산기 재료 키 (APPConstants::MAT_KEYS)
        MaterialTierHint m_tier;    // 상위 · 하위 구분
    };

    // 재료 이름 맞추기 결과
    struct MaterialMatch
    {
        const MaterialName* m_material{};   // 맞춘 재료 (표 항목을 가리킴)
        std::size_t m_distance{};           // 편집 거리 (0 = 정확히 같음)
    };

    [[nodiscard]] std::wstring Compact(std::wstring_view text);                                 // 띄어쓰기 · 탭 지우기 (비교용)
    [[nodiscard]] std::size_t FindDistance(std::wstring_view pattern, std::wstring_view text);  // text 안 어딘가와 pattern의 가장 작은 편집 거리 (앞뒤 글자는 무시)

    // "163,439" · "163.439" · "29,492 422" → 숫자 (처음 나온 숫자 하나, 묶음이 있으면 첫 묶음 1 ~ 3자리 · 나머지 3자리일 때만)
    [[nodiscard]] std::optional<std::int64_t> ParseGroupedNumber(std::wstring_view text);

    // keyword(띄어쓰기 무시, 편집 거리 maxDistance까지 허용)가 들어간 줄에서 keyword 바로 뒤의 숫자
    // areaWidth를 주면 줄이 OCR 영역 오른쪽 끝에 닿은 경우(숫자가 잘렸을 수 있음)는 버림
    [[nodiscard]] std::optional<std::int64_t> FindNumberAfter(const TextResult& text, std::wstring_view keyword, std::size_t maxDistance,
        std::optional<int> areaWidth = std::nullopt);

    // "전체 보유 수량: 417개" 같은 수량 줄 전용 (FindNumberAfter와 같지만 뒤의 "개"를 OCR이 "74" · "7h"로 읽은 경우를 떼어 냄)
    [[nodiscard]] std::optional<std::int64_t> FindQuantityAfter(const TextResult& text, std::wstring_view keyword, std::size_t maxDistance,
        std::optional<int> areaWidth = std::nullopt);

    // 수량 줄만 잘라 3배로 다시 읽기 (커서 주변 1배 OCR에서 keyword 줄 위치를 찾고 → 그 줄만 3배 OCR → 수량, 줄이 없거나 못 읽으면 nullopt)
    [[nodiscard]] std::optional<std::int64_t> ReadQuantityZoomed(const FrameImage& frame, const PixelRect& hoverArea, const TextResult& hoverText,
        const TextRecognizer& text, std::wstring_view keyword, std::size_t maxDistance);

    // 툴팁 수량 정하기: 3배로 다시 읽은 값을 먼저 쓰고(없으면 1배), "겹침 수량"보다 작으면 오인식으로 봄 (1배 값이 맞으면 그것, 아니면 nullopt)
    [[nodiscard]] std::optional<std::int64_t> ResolveTooltipQuantity(const FrameImage& frame, const PixelRect& hoverArea, const TextResult& hoverText,
        const TextRecognizer& text, std::wstring_view keyword, std::size_t maxDistance);

    // 수량 글자 → 숫자 (":417개" · ": 41774" → 417, ": 1 77h" → 17, 자체 검사용으로도 공개)
    [[nodiscard]] std::optional<std::int64_t> ParseQuantity(std::wstring_view text);

    [[nodiscard]] std::span<const MaterialName> GetMaterialNames() noexcept;   // 알아볼 재료 이름 표

    // 줄 하나(툴팁 제목)에서 재료 이름 찾기 (편집 거리가 작은 것 우선, 거리 1 이하끼리는 긴 이름 우선 → "결정" · "위대한" 구분)
    [[nodiscard]] std::optional<MaterialMatch> MatchMaterial(std::wstring_view line);

    // 재료 키 + 상하 구분 → 결과 값 이름 ("Dest/top") · 되돌리기
    [[nodiscard]] std::string MakeMaterialValueKey(std::string_view key, MaterialTierHint tier);
    [[nodiscard]] std::optional<std::pair<std::string, MaterialTierHint>> SplitMaterialValueKey(std::string_view valueKey);
}
