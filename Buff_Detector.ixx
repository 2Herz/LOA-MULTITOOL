export module LostArkMultiTool.Vision.BuffDetector;  // 버프 타이머 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.BuffTemplates;  // 버프 종류 · 템플릿 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기

// 보스 체력바 아래 디버프 줄 · 스킬 바 위 내 버프 줄에서 암흑 수류탄 · 아드로핀 · 서폿 버프 4종과 남은 시간 읽기
// 1) 아이콘 찾기: 테두리 색(디버프 빨강, 버프 초록 · 서폿 금색)으로 된 정사각형 → 가로 테두리 한 줄 + 양옆 세로 테두리 75% 이상
// 2) 종류: 테두리 안쪽을 16x16으로 줄여(상자 평균) 템플릿과 정규화 상관계수 비교 (0.80 이상)
// 3) 남은 시간: 아이콘 아래 글자 줄에서 밝고 둘레가 어두운 픽셀(글자 + 그림자)만 남기고
//    열 단위로 나눠 숫자 · 소수점 · 단위(초 · 분) 글자 모양을 다이스 계수로 비교 (OCR보다 정확 · 빠름, 1080p 기준 크기로 맞춤)
// 화면 전체를 한 번 훑는 가벼운 픽셀 계산이라 감시 스레드가 짧은 주기로 불러도 됨 (OCR 없음)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 알아본 버프 아이콘 하나
    struct BuffIcon
    {
        BuffKind m_kind{};                 // 종류
        PixelRect m_bounds;                // 아이콘 위치 (프레임 기준, 테두리 포함)
        bool m_isDebuff{};                 // 보스 디버프 줄인지
        double m_score{};                  // 템플릿 일치 점수 (0 ~ 1)
        std::optional<double> m_seconds;   // 남은 시간 (초, 못 읽으면 nullopt)
    };

    // 한 장에서 읽은 결과
    struct BuffReading
    {
        std::vector<BuffIcon> m_icons;     // 알아본 아이콘들 (모르는 아이콘은 뺌)
    };

    // 버프 타이머 인식기 (상태 없는 함수 모음)
    class BuffDetector
    {
    public:
        static constexpr double MIN_ICON_SCORE{ 0.80 };   // 종류 점수 하한 (샘플: 맞는 아이콘 1.00, 다른 아이콘 0.60 이하)

        [[nodiscard]] static PixelRect GetDebuffArea(int frameWidth, int frameHeight) noexcept;   // 보스 디버프 줄을 찾을 영역 (위쪽 가운데)
        [[nodiscard]] static PixelRect GetBuffArea(int frameWidth, int frameHeight) noexcept;     // 내 버프 줄을 찾을 영역 (아래쪽 가운데)
        // 아이콘 테두리 찾기 (isDebuff: 빨간 테두리, 아니면 초록 · 금색 테두리), 왼쪽부터
        [[nodiscard]] static std::vector<PixelRect> FindIcons(const FrameImage& frame, const PixelRect& area, bool isDebuff);
        // 아이콘 종류 (템플릿 점수가 가장 높은 종류와 점수, 점수가 하한보다 낮으면 nullopt)
        [[nodiscard]] static std::optional<std::pair<BuffKind, double>> Identify(const FrameImage& frame, const PixelRect& icon);
        // 아이콘 아래 남은 시간 글자 ("18s" · "0.7s" · "5m", 못 읽으면 nullopt, 자체 검사용으로도 공개)
        [[nodiscard]] static std::optional<std::string> ReadText(const FrameImage& frame, const PixelRect& icon);
        // 글자 → 초 ("18s" → 18, "0.7s" → 0.7, "5m" → 300, 형식이 틀리면 nullopt)
        [[nodiscard]] static std::optional<double> ParseSeconds(std::string_view text) noexcept;
        // 프레임 한 장 읽기 (디버프 줄 + 내 버프 줄)
        [[nodiscard]] static BuffReading Detect(const FrameImage& frame);
    };
}
