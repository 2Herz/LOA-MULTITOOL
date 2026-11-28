export module LostArkMultiTool.Vision.BossTimerDetector;  // 보스 남은 시간 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

// 보스전 남은 시간 읽기
// - 레이드: 보스 체력바 왼쪽 아래 "광폭화까지 00:17:54" (노란 글자 → 흰 · 노란 글자만 남겨 2배 OCR)
// - 가디언 토벌: 왼쪽 위 퀘스트 칸의 "19 : 13" (흰 글자 → 흰 글자만 남겨 2배 OCR)
// - 시계 글자는 OCR이 ":"를 ";"로, 숫자 사이에 "-" · "," 같은 잡음을 넣기도 해서 숫자만 모아 해석
// - 화면 타이머를 못 읽을 때를 위해 2부 엔드 콘텐츠(지평의 성당 · 세르카 · 벨가르딘) 관문별 광폭화 시간 표 (나무위키 기준)
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
    };
}
