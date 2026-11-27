export module LostArkMultiTool.Vision.BossHealthDetector;  // 보스 체력바 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

// 화면 위쪽 가운데 보스 체력바에서 "현재 체력 / 최대 체력"과 남은 줄 수("X 100") 읽기
// - 체력 숫자는 주황 · 빨강 · 보라 등 줄마다 색이 바뀌는 막대 위의 흰 글자 → 흰 픽셀만 남긴 흑백 그림으로 바꿔 OCR (그대로 읽으면 자주 틀림)
// - OCR이 쉼표를 마침표 · 띄어쓰기로 읽어도 '/' 양쪽 숫자만 이어 붙여 계산 ('이' · 'O'는 0으로)
// - 줄 수("X 6")는 글자가 작아 체력 줄 오른쪽만 따로 잘라 3배로 다시 읽음
// - 한 장에서 최대 체력을 틀리게 읽을 때가 있어서(쉼표를 1로), 여러 번 읽은 최대 체력을 투표로 정하는 추적기를 같이 씀
// 감시 스레드가 커서와 상관없이 짧은 주기로 부름 (보스가 없으면 nullopt)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 한 번 읽은 보스 체력
    struct BossHealthReading
    {
        std::int64_t m_current{};     // 현재 체력
        std::int64_t m_maximum{};     // 최대 체력
        std::optional<int> m_lines;   // 남은 줄 수 ("X 100" → 100, 못 읽으면 nullopt)
        std::optional<PixelRect> m_textBounds;   // 체력 글자 줄 위치 (프레임 기준, 줄 수를 다시 읽을 때 씀)
        std::optional<std::array<std::uint8_t, 3>> m_barColor;   // 체력 숫자 뒤 막대 색 (R · G · B, 오버레이 덮개를 게임 색으로 칠할 때)
    };

    // 여러 번 읽은 보스 체력을 모아 믿을 만한 값만 내는 추적기 (감시 스레드 전용)
    // - 최대 체력: 최근 몇 번 읽은 값 중 가장 많이 나온 값 (두 번 이상, 같으면 최근 값) → 한 장 오인식은 고쳐짐
    // - 현재 체력이 정한 최대 체력보다 크면 버림 (자릿수 오인식)
    // - 줄 수를 못 읽은 번에는 지난 줄 수를 씀
    class BossHealthTracker
    {
    public:
        static constexpr std::size_t VOTE_WINDOW{ 4 };   // 최대 체력 투표에 쓰는 최근 읽기 수
        static constexpr int MIN_VOTES{ 2 };             // 이만큼 같아야 최대 체력으로 정함

        [[nodiscard]] std::optional<BossHealthReading> Feed(BossHealthReading reading);   // 한 번 읽은 값 넣기 → 믿을 만하면 고친 값
        void Reset() noexcept;                                                            // 보스가 사라졌을 때

    private:
        std::deque<std::int64_t> m_maxima;    // 최근 최대 체력들 (오래된 것이 앞)
        std::optional<std::int64_t> m_confirmed;   // 정한 최대 체력
        std::optional<int> m_lastLines;       // 지난 줄 수
    };

    // 보스 체력바 인식기 (상태 없는 함수 모음)
    class BossHealthDetector
    {
    public:
        static constexpr int MASK_PADDING{ 20 };   // 흑백 그림 둘레 여백 (OCR이 가장자리 글자를 잘 읽게, OCR 위치를 프레임 기준으로 바꿀 때 뺌)
        [[nodiscard]] static PixelRect GetBarArea(int frameWidth, int frameHeight) noexcept;              // 체력바를 찾을 영역 (위쪽 가운데)
        [[nodiscard]] static FrameImage MakeTextMask(const FrameImage& image);                            // 흰 글자만 검게, 나머지는 흰 바탕 (여백 포함)
        [[nodiscard]] static std::optional<BossHealthReading> Parse(const TextResult& text);              // OCR 결과 해석
        [[nodiscard]] static std::optional<std::int64_t> ParseDigits(std::wstring_view text) noexcept;   // 숫자만 이어 붙이기 ("83.1 19.1 20.458" → 83119120458)
        [[nodiscard]] static std::optional<BossHealthReading> Detect(const FrameImage& frame, const TextRecognizer& text);   // 프레임에서 읽기
    };
}
