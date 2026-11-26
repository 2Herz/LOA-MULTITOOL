export module LostArkMultiTool.Views.UI.Slider;  // 끌어서 값을 고르는 막대 부품 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.UI.Control;  // 부품 기반 가져오기

// 가로 막대 + 손잡이로 최솟값 ~ 최댓값 사이 값을 고르는 부품 (오버레이 요소별 투명도 등)
// - 막대를 누르면 그 자리로, 손잡이를 끌면 따라감, ← → 키로 한 칸씩
// - 끄는 동안 onChanged(값)를 계속 부르고, 놓을 때 onCommitted(값)를 한 번 부름 (저장은 놓을 때만)
export namespace LostArkMultiTool::Views::UI  // UI 네임스페이스 내보내기 시작
{
    // 값 막대
    class Slider final : public Control
    {
    public:
        // minimum ~ maximum 사이를 step 단위로 (step이 0 이하이면 연속)
        Slider(double minimum, double maximum, double step, std::function<void(double value)> onChanged, std::function<void(double value)> onCommitted);

        void SetValue(double value) noexcept;                   // 코드에서 바꿈 (알림 없음, 범위 · 단위로 맞춤)
        [[nodiscard]] double GetValue() const noexcept;         // 지금 값
        [[nodiscard]] double ValueFromX(float x) const noexcept;   // 막대 위 X → 값 (자체 검사용으로도 공개)

        [[nodiscard]] bool IsFocusable() const noexcept override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;
        void Render(Core::Renderer& renderer) override;
        void OnMouseDown(const Core::MouseButtonEvent& event) override;
        void OnMouseDrag(const Core::MouseMoveEvent& event) override;
        void OnMouseUp(const Core::MouseButtonEvent& event, bool isInside) override;
        [[nodiscard]] bool OnKeyDown(const Core::KeyEvent& event) override;

    private:
        [[nodiscard]] double Normalize(double value) const noexcept;   // 범위 · 단위로 맞춘 값
        [[nodiscard]] float GetTrackLeft() const noexcept;             // 막대 왼쪽 끝 (손잡이 반지름만큼 안쪽)
        [[nodiscard]] float GetTrackRight() const noexcept;            // 막대 오른쪽 끝
        void ChangeTo(double value);                                   // 값 바꾸고 onChanged 알림 (바뀌었을 때만)

        double m_minimum;                              // 최솟값
        double m_maximum;                              // 최댓값
        double m_step;                                 // 단위
        double m_value;                                // 지금 값
        bool m_isDragging{};                           // 끄는 중
        std::function<void(double)> m_onChanged;       // 바뀔 때마다
        std::function<void(double)> m_onCommitted;     // 놓을 때 · 키로 바꿀 때
    };
}
