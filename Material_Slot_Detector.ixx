export module LostArkMultiTool.Vision.MaterialSlotDetector;  // 창고 · 소지품 슬롯 재료 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 결과 형식 가져오기

// 창고 · 소지품이 열려 있으면 마우스를 올리지 않아도 슬롯 아이콘 + 오른쪽 위 숫자로 재료 수량 읽기
// - 대상: 아비도스 융화 재료 · 용암의 숨결 · 빙하의 숨결 (파괴석 · 수호석 · 돌파석은 9999+로 넘쳐서 툴팁으로 읽음)
// - 아이콘 찾기: 화면을 4배 줄여 후보를 빠르게 찾고(NCC), 원래 크기에서 다시 맞춰 보고 평균 색까지 비교
// - 숫자 읽기: 흰 글자만 남겨 글자마다 잘라 게임 숫자 모양과 비교 (OCR보다 작은 글자에 정확)
// - 같은 재료 슬롯이 여러 개 보이면 더함, "9999+"처럼 넘친 슬롯은 건너뜀
// - 1080p 게임 화면(UI 크기 기본) 기준 템플릿
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 슬롯 하나를 읽은 결과
    struct SlotReading
    {
        std::string m_valueKey;        // 값 이름 ("Yong/level")
        std::int64_t m_quantity{};     // 슬롯 숫자
        PixelRect m_iconBounds;        // 아이콘 템플릿이 맞은 위치
        double m_score{};              // 아이콘 일치 점수
    };

    // 슬롯 재료 인식기 (전략 구현, 감시기가 커서와 상관없이 주기적으로 부름)
    class MaterialSlotDetector final : public ScreenDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "material_slot" };   // 화면 이름 (확정 · 결과 구분)

        [[nodiscard]] std::string_view GetScreenName() const noexcept override;   // 담당 화면 이름
        [[nodiscard]] std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) override;   // 재료별 합계 (없으면 nullopt)

        [[nodiscard]] static std::vector<SlotReading> FindSlots(const FrameImage& frame);   // 보이는 대상 슬롯 모두
        [[nodiscard]] static std::optional<ScanResult> DetectFrame(const FrameImage& frame);// 재료별 합계 (자체 검사용으로도 공개)
        [[nodiscard]] static std::optional<std::string> ReadCount(const FrameImage& band);  // 숫자 띠 → 글자 ("2720" · "9999+", 못 읽으면 nullopt)
    };
}
