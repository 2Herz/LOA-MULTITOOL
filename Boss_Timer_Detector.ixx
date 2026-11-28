export module LostArkMultiTool.Vision.BossTimerDetector;  // 보스 남은 시간 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

// 보스전 남은 시간 읽기
// - 레이드: 보스 체력바 왼쪽 아래 "광폭화까지 00:17:54" (노란 글자 → 흰 · 노란 글자만 남겨 2배 OCR)
// - 가디언 토벌: 왼쪽 위 퀘스트 칸의 "19 : 13" (흰 글자 → 흰 글자만 남겨 2배 OCR)
// - 시계 글자는 OCR이 ":"를 ";"로, 숫자 사이에 "-" · "," 같은 잡음을 넣기도 해서 숫자만 모아 해석
// - 화면 타이머를 못 읽을 때를 위해 2부 엔드 콘텐츠(지평의 성당 · 세르카 · 벨가르딘) 관문별 광폭화 시간 표 (나무위키 기준)
// - 인원 자동 인식: 가디언 토벌(…의 잔영 n단계 · 고정 4종) · 지평의 성당 · 세르카 = 4인, 카제로스 레이드(4막 · 종막) · 벨가르딘 = 8인, 목록에 없는 콘텐츠는 모름
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 한 번 읽은 남은 시간
    struct BossTimerReading
    {
        int m_seconds{};      // 남은 초
        bool m_isEnrage{};    // 레이드 광폭화 타이머인지 (아니면 가디언 토벌 제한 시간)
    };

    // 보스 남은 시간 인식기 (상태 없는 함수 모음)
    class BossTimerDetector
    {
    public:
        [[nodiscard]] static PixelRect GetEnrageArea(int frameWidth, int frameHeight) noexcept;     // 레이드 "광폭화까지" 영역 (체력바 왼쪽 아래)
        [[nodiscard]] static PixelRect GetGuardianArea(int frameWidth, int frameHeight) noexcept;   // 가디언 토벌 타이머 영역 (왼쪽 위)
        // 시계 글자 → 초 ("00:17:54" → 1074, "19 : 13" → 1153, "19 : 4-7" → 1187, 시계가 아니면 nullopt)
        [[nodiscard]] static std::optional<int> ParseClock(std::wstring_view text);
        // 보스 이름 → 광폭화 시간 (초, 표에 없으면 nullopt)
        [[nodiscard]] static std::optional<int> FindEnrageSeconds(std::wstring_view bossName);
        // 프레임에서 남은 시간 읽기 (레이드 광폭화 → 가디언 토벌 순서, 둘 다 없으면 nullopt)
        [[nodiscard]] static std::optional<BossTimerReading> Detect(const FrameImage& frame, const TextRecognizer& text);
        // 체력 숫자 줄 위의 보스 이름을 읽어 광폭화 시간 찾기 (화면 타이머를 못 읽을 때, textBounds: 체력 숫자 줄 위치)
        [[nodiscard]] static std::optional<int> ReadEnrageFromName(const FrameImage& frame, const TextRecognizer& text, const PixelRect& textBounds);
        // 보스 이름 → 레이드 인원 (카제로스 레이드 · 벨가르딘 = 8, 세르카 · 지평의 성당 = 4, 목록에 없으면 nullopt)
        [[nodiscard]] static std::optional<int> FindRaidSize(std::wstring_view bossName);
        // 가디언 토벌 줄인지 (왼쪽 위 목표 가디언 아래 "…의 잔영 n단계" → true, 잔영을 진영 · 한영으로 읽어도, 고정 가디언 4종 이름도)
        [[nodiscard]] static bool IsGuardianStage(std::wstring_view line);
        // 지금 콘텐츠의 인원 (가디언 토벌 4 → 보스 이름 4 · 8, 모르면 nullopt, textBounds: 체력 숫자 줄 위치)
        [[nodiscard]] static std::optional<int> DetectRaidSize(const FrameImage& frame, const TextRecognizer& text, const std::optional<PixelRect>& textBounds);
        // 보스 이름 줄 그림 → 인원 (1배 · 2배로 읽어 목록과 맞춤, 자체 검사용으로도 공개)
        [[nodiscard]] static std::optional<int> ReadRaidSizeFromName(const FrameImage& nameArea, const TextRecognizer& text);
    };
}
