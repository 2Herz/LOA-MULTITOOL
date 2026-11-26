module LostArkMultiTool.Views.UI.Slider;  // 끌어서 값을 고르는 막대 부품 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기
import LostArkMultiTool.Views.UI.Control;  // 부품 기반 가져오기

namespace LostArkMultiTool::Views::UI  // UI 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float TRACK_HEIGHT{ 4.0f };          // 막대 두께
        constexpr float TRACK_RADIUS{ 2.0f };          // 막대 모서리
        constexpr float THUMB_RADIUS{ 7.0f };          // 손잡이 반지름
        constexpr float THUMB_HOVER_RADIUS{ 8.0f };    // 마우스를 올렸을 때 손잡이 반지름
        constexpr float FOCUS_RING_GAP{ 3.0f };        // 키보드 포커스 고리와 손잡이 사이
        constexpr float HALF{ 0.5f };                  // 절반
        constexpr float THUMB_BORDER_WIDTH{ 1.5f };    // 손잡이 테두리 두께
        constexpr double CONTINUOUS_KEY_STEPS{ 100.0 };   // 단위가 없을 때 키 한 번 = 범위의 1/100
    }

    Slider::Slider(double minimum, double maximum, double step, std::function<void(double value)> onChanged, std::function<void(double value)> onCommitted)
        : m_minimum{ std::min(minimum, maximum) }      // 작은 쪽
        , m_maximum{ std::max(minimum, maximum) }      // 큰 쪽
        , m_step{ step }                               // 단위
        , m_value{ std::min(minimum, maximum) }        // 처음엔 최솟값
        , m_onChanged{ std::move(onChanged) }          // 바뀔 때
        , m_onCommitted{ std::move(onCommitted) }      // 놓을 때
    {
    }

    double Slider::Normalize(double value) const noexcept
    {
        double clamped = std::clamp(value, m_minimum, m_maximum);                         // 범위 안으로
        if (m_step > 0.0) clamped = m_minimum + std::round((clamped - m_minimum) / m_step) * m_step;   // 단위로 맞춤
        return std::clamp(clamped, m_minimum, m_maximum);                                 // 반올림 뒤에도 범위 안
    }

    void Slider::SetValue(double value) noexcept
    {
        m_value = Normalize(value);   // 알림 없이 바꿈
    }

    double Slider::GetValue() const noexcept
    {
        return m_value;   // 지금 값
    }

    float Slider::GetTrackLeft() const noexcept
    {
        return GetBounds().m_left + THUMB_RADIUS;   // 손잡이가 테두리 밖으로 안 나가게
    }

    float Slider::GetTrackRight() const noexcept
    {
        return std::max(GetTrackLeft(), GetBounds().m_right - THUMB_RADIUS);   // 오른쪽 끝
    }

    double Slider::ValueFromX(float x) const noexcept
    {
        const float left = GetTrackLeft();                                    // 막대 시작
        const float width = GetTrackRight() - left;                           // 막대 길이
        if (width <= 0.0f) return m_minimum;                                  // 너무 좁음
        const double ratio = std::clamp(static_cast<double>((x - left) / width), 0.0, 1.0);   // 0 ~ 1
        return Normalize(m_minimum + ratio * (m_maximum - m_minimum));        // 값
    }

    void Slider::ChangeTo(double value)
    {
        const double normalized = Normalize(value);                           // 맞춘 값
        if (normalized == m_value) return;                                    // 그대로
        m_value = normalized;                                                 // 바꿈
        if (m_onChanged) m_onChanged(m_value);                                // 알림
    }

    bool Slider::IsFocusable() const noexcept
    {
        return true;   // ← → 키로 조절
    }

    Core::CursorShape Slider::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_HAND;   // 손 모양
    }

    void Slider::Render(Core::Renderer& renderer)
    {
        const Core::RectF& bounds = GetBounds();                                                          // 부품 영역
        const float centerY = bounds.m_top + bounds.GetHeight() * HALF;                                   // 세로 가운데
        const float left = GetTrackLeft();                                                                // 막대 시작
        const float right = GetTrackRight();                                                              // 막대 끝
        const double range = m_maximum - m_minimum;                                                       // 값 범위
        const float ratio = range > 0.0 ? static_cast<float>((m_value - m_minimum) / range) : 0.0f;       // 채운 비율
        const float thumbX = left + (right - left) * ratio;                                               // 손잡이 X
        const bool isEnabled = IsEnabled();                                                               // 쓸 수 있는지

        const Core::RectF track{ left, centerY - TRACK_HEIGHT * HALF, right, centerY + TRACK_HEIGHT * HALF };   // 막대
        renderer.FillRoundedRect(track, TRACK_RADIUS, Theme::GRID_LINE_COLOR);                            // 빈 막대
        const Core::RectF filled{ left, track.m_top, thumbX, track.m_bottom };                            // 채운 부분
        renderer.FillRoundedRect(filled, TRACK_RADIUS, isEnabled ? Theme::PRIMARY_BACK_COLOR : Theme::MUTED_TEXT_COLOR);   // 채운 막대

        const float radius = (IsHovered() || m_isDragging) && isEnabled ? THUMB_HOVER_RADIUS : THUMB_RADIUS;   // 손잡이 크기
        const Core::PointF thumb{ thumbX, centerY };                                                      // 손잡이 위치
        renderer.FillEllipse(thumb, radius, radius, isEnabled ? Theme::HEADER_TEXT_COLOR : Theme::MUTED_TEXT_COLOR);   // 손잡이
        renderer.DrawEllipse(thumb, radius, radius, isEnabled ? Theme::PRIMARY_BACK_COLOR : Theme::GRID_LINE_COLOR, THUMB_BORDER_WIDTH);   // 테두리
        if (IsFocused()) renderer.DrawEllipse(thumb, radius + FOCUS_RING_GAP, radius + FOCUS_RING_GAP, Theme::FOCUS_BORDER_COLOR, Theme::BORDER_WIDTH);   // 키보드 포커스
    }

    void Slider::OnMouseDown(const Core::MouseButtonEvent& event)
    {
        if (!IsEnabled() || event.m_button != Core::MouseButton::MOUSE_BUTTON_LEFT) return;   // 왼쪽 버튼만
        m_isDragging = true;                                                                // 끌기 시작
        ChangeTo(ValueFromX(event.m_x));                                                    // 누른 자리로
    }

    void Slider::OnMouseDrag(const Core::MouseMoveEvent& event)
    {
        if (!m_isDragging) return;                // 끄는 중이 아님
        ChangeTo(ValueFromX(event.m_x));          // 따라감
    }

    void Slider::OnMouseUp(const Core::MouseButtonEvent&, bool)
    {
        if (!m_isDragging) return;                // 끄는 중이 아님
        m_isDragging = false;                     // 끝
        if (m_onCommitted) m_onCommitted(m_value);   // 놓을 때 한 번 (저장)
    }

    bool Slider::OnKeyDown(const Core::KeyEvent& event)
    {
        if (!IsEnabled()) return false;                                                     // 꺼짐
        const double step = m_step > 0.0 ? m_step : (m_maximum - m_minimum) / CONTINUOUS_KEY_STEPS;   // 한 칸
        if (event.m_key == Core::Key::KEY_LEFT) ChangeTo(m_value - step);                   // 왼쪽
        else if (event.m_key == Core::Key::KEY_RIGHT) ChangeTo(m_value + step);             // 오른쪽
        else return false;                                                                  // 다른 키
        if (m_onCommitted) m_onCommitted(m_value);                                          // 키는 바로 저장
        return true;                                                                        // 처리함
    }
}
