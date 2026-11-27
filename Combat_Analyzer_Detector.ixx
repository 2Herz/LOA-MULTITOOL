export module LostArkMultiTool.Vision.CombatAnalyzerDetector;  // 전투 분석기 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

// 게임 전투 분석기(오른쪽 아래)의 "피해량 · DPS" 읽기 → 보스 체력과 합쳐 딜 지분 계산
// - 칸마다 바탕색이 달라도 값은 흰 글자 → 흰 글자만 남긴 흑백 그림을 2배로 OCR
// - 값은 같은 칸의 머리글("피해량" · "DPS") 바로 아래 글자 (칸 순서가 바뀌어도 맞게)
// - "460.99억" · "4,827.14만": OCR이 억을 익 · 역, 소수점을 쉼표로 읽기도 함 → 마지막 구분자 뒤 두 자리 + 단위면 소수
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 한 번 읽은 전투 분석기
    struct CombatReading
    {
        std::optional<std::int64_t> m_damage;   // 내 피해량 (서포터는 조력 피해량)
        std::optional<std::int64_t> m_dps;      // 초당 피해량
    };

    // 전투 분석기 인식기 (상태 없는 함수 모음)
    class CombatAnalyzerDetector
    {
    public:
        [[nodiscard]] static PixelRect GetArea(int frameWidth, int frameHeight) noexcept;                      // 찾을 영역 (오른쪽 아래)
        [[nodiscard]] static std::optional<std::int64_t> ParseAmount(std::wstring_view text);                  // "460,99익" → 46099000000
        [[nodiscard]] static std::optional<CombatReading> Parse(const TextResult& text);                       // OCR 결과 해석 (피해량 · DPS 둘 다 없으면 nullopt)
        // 프레임에서 읽기: 1배로 머리글 줄을 찾고 그 띠만 2배로 읽음 (stripCache가 있으면 찾은 띠를 기억해 다음부터 바로 읽음)
        [[nodiscard]] static std::optional<CombatReading> Detect(const FrameImage& frame, const TextRecognizer& text, std::optional<PixelRect>* stripCache = nullptr);
    };
}
