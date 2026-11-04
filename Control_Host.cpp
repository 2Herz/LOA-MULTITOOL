module LostArkMultiTool.Views.UI.ControlHost;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    ControlHost::ControlHost() = default;

    ControlHost::~ControlHost() = default;

    void ControlHost::AddControl(std::unique_ptr<Control> control)
    {
        if (control) m_controls.push_back(std::move(control));
    }

    void ControlHost::Update(double deltaSeconds)
    {
        DropUnavailableStates();

        for (const auto& control : m_controls)
        {
            if (control->IsVisible()) control->Update(deltaSeconds);
        }
    }

    void ControlHost::Render(Core::Renderer& renderer)
    {
        for (const auto& control : m_controls)
        {
            if (control->IsVisible()) control->Render(renderer);
        }
    }

    bool ControlHost::HandleInput(const Core::InputEvent& event)
    {
        return std::visit(Core::Overloaded{
            [this](const Core::MouseMoveEvent& mouseMove) { return HandleMouseMove(mouseMove); },
            [this](const Core::MouseLeaveEvent&)
            {
                if (m_pressed == nullptr) SetHovered(nullptr);   // 누른 채 창 밖으로 나간 경우는 뗄 때까지 유지
                return false;
            },
            [this](const Core::MouseButtonEvent& mouseButton) { return HandleMouseButton(mouseButton); },
            [this](const Core::MouseWheelEvent& mouseWheel) { return HandleMouseWheel(mouseWheel); },
            [this](const Core::KeyEvent& key) { return HandleKey(key); },
            [this](const Core::TextInputEvent& textInput)
            {
                if (m_focused == nullptr) return false;
                m_focused->OnTextInput(textInput.m_text);
                return true;
            },
            [this](const Core::CompositionEvent& composition)
            {
                if (m_focused == nullptr) return false;
                m_focused->OnComposition(composition.m_text);
                return true;
            }
            }, event);
    }

    void ControlHost::SetFocus(Control* control)
    {
        if (control != nullptr && (!IsInteractive(*control) || !control->IsFocusable())) control = nullptr;
        if (control == m_focused) return;

        // 이전 부품부터 내려놓아야 IME 상태가 새 부품 기준으로 남음 (끔 → 켬 순서)
        if (m_focused != nullptr) m_focused->SetFocused(false);
        m_focused = control;
        m_suspendedFocus = nullptr;
        if (m_focused != nullptr) m_focused->SetFocused(true);
    }

    Control* ControlHost::GetFocused() const noexcept
    {
        return m_focused;
    }

    // 추가한 순서대로 포커스 이동 (끝에서 처음으로 돌아감)
    void ControlHost::FocusNext(bool isReverse)
    {
        std::vector<Control*> candidates;
        for (const auto& control : m_controls)
        {
            if (IsInteractive(*control) && control->IsFocusable()) candidates.push_back(control.get());
        }
        if (candidates.empty()) return;

        const auto current{ std::ranges::find(candidates, m_focused) };
        if (current == candidates.end())
        {
            SetFocus(isReverse ? candidates.back() : candidates.front());
            return;
        }

        const auto currentIndex{ static_cast<std::size_t>(std::distance(candidates.begin(), current)) };
        const std::size_t count{ candidates.size() };
        const std::size_t nextIndex{ isReverse ? (currentIndex + count - 1) % count : (currentIndex + 1) % count };
        SetFocus(candidates[nextIndex]);
    }

    void ControlHost::SuspendFocus()
    {
        ReleasePointer();
        SetHovered(nullptr);

        if (m_focused == nullptr) return;

        Control* previous{ m_focused };
        SetFocus(nullptr);
        m_suspendedFocus = previous;
    }

    void ControlHost::RestoreFocus()
    {
        if (m_suspendedFocus == nullptr) return;

        Control* previous{ std::exchange(m_suspendedFocus, nullptr) };
        SetFocus(previous);
    }

    Core::CursorShape ControlHost::GetCursor() const noexcept
    {
        // 누르고 있는 동안은 누른 부품 기준 (입력칸에서 드래그로 선택하다 밖으로 나가도 글자 커서 유지)
        const Control* target{ m_pressed != nullptr ? m_pressed : m_hovered };
        return target != nullptr && target->IsEnabled() ? target->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    bool ControlHost::HandleMouseMove(const Core::MouseMoveEvent& event)
    {
        if (m_pressed != nullptr)
        {
            m_pressed->OnMouseDrag(event);
            SetHovered(m_pressed->GetBounds().Contains(event.m_x, event.m_y) ? m_pressed : nullptr);
            return true;
        }

        Control* target{ FindControlAt(event.m_x, event.m_y) };
        SetHovered(target);
        return target != nullptr;
    }

    bool ControlHost::HandleMouseButton(const Core::MouseButtonEvent& event)
    {
        // 화면 부품은 왼쪽 버튼만 사용 (오른쪽 버튼 메뉴는 표 · 그래프 단계에서)
        if (event.m_button != Core::MouseButton::MOUSE_BUTTON_LEFT) return false;

        if (event.m_isPressed)
        {
            Control* target{ FindControlAt(event.m_x, event.m_y) };

            // 다른 곳을 누르면 포커스가 옮겨감 (입력칸의 한글 조합은 여기서 확정 → 버튼 클릭 전에 글자가 완성됨)
            SetFocus(target);
            if (target == nullptr) return false;

            ReleasePointer();
            m_pressed = target;
            m_pressed->SetPressed(true);
            m_pressed->OnMouseDown(event);
            return true;
        }

        if (m_pressed == nullptr) return false;

        // 콜백이 다른 부품 상태를 바꿀 수 있으므로 누름 상태를 먼저 정리한 뒤 알림
        Control* released{ std::exchange(m_pressed, nullptr) };
        released->SetPressed(false);
        released->OnMouseUp(event, released->GetBounds().Contains(event.m_x, event.m_y));
        return true;
    }

    bool ControlHost::HandleMouseWheel(const Core::MouseWheelEvent& event)
    {
        Control* target{ FindControlAt(event.m_x, event.m_y) };
        return target != nullptr && target->OnMouseWheel(event);
    }

    bool ControlHost::HandleKey(const Core::KeyEvent& event)
    {
        if (m_focused != nullptr && m_focused->OnKeyDown(event)) return true;

        // 부품이 쓰지 않은 Tab → 다음 부품으로 포커스 이동
        if (event.m_key == Core::Key::KEY_TAB && !event.m_isCtrlDown)
        {
            FocusNext(event.m_isShiftDown);
            return true;
        }
        return false;
    }

    // 맨 위(나중에 추가한) 부품부터 찾음
    Control* ControlHost::FindControlAt(float x, float y) const noexcept
    {
        for (const auto& control : std::views::reverse(m_controls))
        {
            if (IsInteractive(*control) && control->IsHitTestVisible() && control->GetBounds().Contains(x, y))
            {
                return control.get();
            }
        }
        return nullptr;
    }

    bool ControlHost::IsInteractive(const Control& control) noexcept
    {
        return control.IsVisible() && control.IsEnabled();
    }

    void ControlHost::SetHovered(Control* control)
    {
        if (control == m_hovered) return;

        if (m_hovered != nullptr) m_hovered->SetHovered(false);
        m_hovered = control;
        if (m_hovered != nullptr) m_hovered->SetHovered(true);
    }

    void ControlHost::ReleasePointer()
    {
        if (m_pressed != nullptr) std::exchange(m_pressed, nullptr)->SetPressed(false);
    }

    // 숨겨지거나 비활성화된 부품은 마우스 · 포커스 상태를 내려놓음 (보이지 않는 입력칸에 글자가 들어가지 않게)
    void ControlHost::DropUnavailableStates()
    {
        if (m_hovered != nullptr && !IsInteractive(*m_hovered)) SetHovered(nullptr);
        if (m_pressed != nullptr && !IsInteractive(*m_pressed)) ReleasePointer();
        if (m_focused != nullptr && !IsInteractive(*m_focused)) SetFocus(nullptr);
    }
}