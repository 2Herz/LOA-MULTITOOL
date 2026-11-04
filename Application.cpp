module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>   // VK_TAB

module LostArkMultiTool.Views.Application;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Views.CalculatorManager;
import LostArkMultiTool.Views.DX11Renderer;
import LostArkMultiTool.Views.GemFinderManager;
import LostArkMultiTool.Views.MainWindow;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.TrackerManager;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view WINDOW_TITLE{ L"로스트아크 멀티툴" };
        constexpr int WINDOW_WIDTH{ 1280 };    // 처음 창 안쪽 크기 (DIP)
        constexpr int WINDOW_HEIGHT{ 760 };

        // ===== 탭 막대 (DIP) =====
        constexpr float TAB_BAR_HEIGHT{ 44.0f };
        constexpr float TAB_WIDTH{ 180.0f };
        constexpr float TAB_INDICATOR_HEIGHT{ 3.0f };   // 선택한 탭 밑줄 두께
        constexpr float TAB_DIVIDER_MARGIN{ 12.0f };    // 탭 사이 구분선의 위아래 여백
        constexpr float LINE_WIDTH{ 1.0f };

        // ===== 탭 전환 애니메이션 =====
        constexpr double INDICATOR_SMOOTHING{ 14.0 };   // 클수록 밑줄이 빨리 따라감 (초당 감쇠 계수)
        constexpr float INDICATOR_SNAP_DISTANCE{ 0.001f };
        constexpr double CONTENT_FADE_SECONDS{ 0.18 };  // 새 탭 내용이 나타나는 시간
        constexpr float FULL_FADE{ 1.0f };
        constexpr float NO_FADE{ 0.0f };
        constexpr float HOVER_ALPHA{ 0.35f };           // 마우스를 올린 탭 배경 투명도
        constexpr double MAX_FRAME_SECONDS{ 0.1 };      // 창을 끌거나 멈췄다 돌아왔을 때 애니메이션이 튀지 않게

        // ===== 단축키 =====
        constexpr unsigned FIRST_TAB_KEY{ '1' };        // Ctrl+1 ~ Ctrl+3 (숫자 키의 가상 키 코드 = 문자 코드)
        constexpr std::size_t NEXT_TAB_STEP{ 1 };

        constexpr Core::TextStyle ACTIVE_TAB_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::CENTER };
        constexpr Core::TextStyle INACTIVE_TAB_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };

        // 제목 표시줄: "로스트아크 멀티툴 v0.5.0"
        [[nodiscard]] std::wstring BuildWindowTitle()
        {
            return std::wstring{ WINDOW_TITLE } + L" v" + Core::TextEncoding::UTF8ToWide(Core::APPConstants::APP_VERSION);
        }

        [[nodiscard]] Core::Color WithAlpha(Core::Color color, float alpha) noexcept
        {
            color.m_alpha = alpha;
            return color;
        }
    }

    Application::Application()
        : m_tools{ CalculatorManager::GetInstance(), GemFinderManager::GetInstance(), TrackerManager::GetInstance() }
    {
        // 창을 만드는 도중에도 WM_SIZE가 OnResize로 들어오므로, 두 unique_ptr가 먼저 비어 있는 상태로 만들어진 뒤에 생성
        m_window = std::make_unique<MainWindow>(BuildWindowTitle(), WINDOW_WIDTH, WINDOW_HEIGHT, *this);
        m_renderer = std::make_unique<DX11Renderer>(m_window->GetNativeHandle(), m_window->GetDPI());
    }

    Application::~Application()
    {
        // 예외로 Run을 빠져나온 경우에도 매니저 저장 기회를 줌 (소멸자 밖으로 예외를 던지지 않음)
        try
        {
            ShutdownTools();
        }
        catch (...)
        {
        }

        // 렌더러(스왑체인)를 먼저 놓고 창을 닫음 → 창이 닫히며 오는 메시지는 렌더러 없이 처리됨
        m_renderer.reset();
        m_window.reset();
    }

    int Application::Run(int showCommand)
    {
        using Clock = std::chrono::steady_clock;
        using Seconds = std::chrono::duration<double>;   // 초 단위 실수 (프레임 간격)

        InitializeTools();
        GetActiveTool().OnActivated();
        m_window->Show(showCommand);

        auto previousTime{ Clock::now() };
        while (m_window->ProcessMessages())
        {
            if (m_window->IsMinimized())
            {
                // 최소화 중에는 그리지 않고 메시지가 올 때까지 대기 (CPU · GPU 사용 0에 가깝게)
                m_window->WaitForMessage();
                previousTime = Clock::now();
                continue;
            }

            const auto currentTime{ Clock::now() };
            // 나노초 정수 간격 → 초 단위 실수 (명시적 변환 함수 사용: 편집기 E0289 오탐 방지, 결과는 변환 생성자와 같음)
            const double deltaSeconds{ std::chrono::duration_cast<Seconds>(currentTime - previousTime).count() };
            previousTime = currentTime;

            Update(std::min(deltaSeconds, MAX_FRAME_SECONDS));
            Render();   // EndFrame의 수직 동기화가 프레임 속도를 모니터 주사율로 맞춤
        }

        GetActiveTool().OnDeactivated();
        ShutdownTools();
        return m_window->GetExitCode();
    }

    void Application::OnResize(int pixelWidth, int pixelHeight)
    {
        if (!m_renderer) return;   // 창을 만드는 도중 (렌더러 생성 전)

        m_renderer->Resize(pixelWidth, pixelHeight);
        Render();   // 창 크기를 끄는 동안(메시지 루프가 멈춘 상태)에도 화면이 따라오게
    }

    void Application::OnDPIChanged(float dpi)
    {
        if (m_renderer) m_renderer->SetDPI(dpi);
    }

    void Application::OnMouseMove(float x, float y)
    {
        m_hoverIndex = FindTabAt(x, y);
    }

    void Application::OnMouseLeave()
    {
        m_hoverIndex.reset();
    }

    void Application::OnMouseDown(float x, float y)
    {
        if (const auto index{ FindTabAt(x, y) }) SelectTool(*index);
    }

    void Application::OnKeyDown(unsigned virtualKey, bool isCtrlDown, bool isShiftDown)
    {
        if (!isCtrlDown) return;

        // Ctrl+Tab: 다음 탭, Ctrl+Shift+Tab: 이전 탭 (끝에서 처음으로 돌아감)
        if (virtualKey == VK_TAB)
        {
            const std::size_t step{ isShiftDown ? TOOL_COUNT - NEXT_TAB_STEP : NEXT_TAB_STEP };
            SelectTool((m_activeIndex + step) % TOOL_COUNT);
            return;
        }

        // Ctrl+1 ~ Ctrl+3: 해당 탭으로 바로 이동
        if (virtualKey >= FIRST_TAB_KEY && virtualKey < FIRST_TAB_KEY + TOOL_COUNT)
        {
            SelectTool(virtualKey - FIRST_TAB_KEY);
        }
    }

    void Application::InitializeTools()
    {
        for (Core::ToolManager& tool : m_tools)
        {
            tool.Initialize();
            ++m_initializedToolCount;
        }
    }

    void Application::ShutdownTools()
    {
        // 초기화한 역순으로 정리, 하나가 실패해도 나머지는 끝까지 저장하고 첫 예외만 다시 던짐
        std::exception_ptr firstError;
        while (m_initializedToolCount > 0)
        {
            --m_initializedToolCount;
            try
            {
                m_tools[m_initializedToolCount].get().Shutdown();
            }
            catch (...)
            {
                if (!firstError) firstError = std::current_exception();
            }
        }

        if (firstError) std::rethrow_exception(firstError);
    }

    void Application::SelectTool(std::size_t index)
    {
        if (index >= TOOL_COUNT || index == m_activeIndex) return;

        GetActiveTool().OnDeactivated();
        m_activeIndex = index;
        GetActiveTool().OnActivated();

        m_contentFade = FULL_FADE;   // 새 탭 내용이 배경에서 서서히 나타나게
    }

    void Application::Update(double deltaSeconds)
    {
        // 탭 밑줄: 남은 거리의 일정 비율씩 다가감 (지수 감쇠 → 프레임 속도가 달라도 같은 빠르기)
        const float targetPosition{ static_cast<float>(m_activeIndex) };
        const float followRatio{ static_cast<float>(1.0 - std::exp(-INDICATOR_SMOOTHING * deltaSeconds)) };
        m_indicatorPosition += (targetPosition - m_indicatorPosition) * followRatio;
        if (std::abs(targetPosition - m_indicatorPosition) < INDICATOR_SNAP_DISTANCE) m_indicatorPosition = targetPosition;

        const float fadeStep{ static_cast<float>(deltaSeconds / CONTENT_FADE_SECONDS) };
        m_contentFade = std::max(NO_FADE, m_contentFade - fadeStep);

        GetActiveTool().Update(deltaSeconds);
    }

    void Application::Render()
    {
        if (!m_renderer || !m_renderer->BeginFrame()) return;

        m_renderer->Clear(Theme::BACKGROUND_COLOR);

        // 1) 선택한 탭 내용
        const Core::RectF contentRect{ GetContentRect() };
        GetActiveTool().Render(*m_renderer, contentRect);

        // 2) 탭 전환 직후: 내용 위에 배경색을 덮었다가 걷어 냄 (페이드 인)
        if (m_contentFade > NO_FADE)
        {
            m_renderer->FillRect(contentRect, WithAlpha(Theme::BACKGROUND_COLOR, m_contentFade));
        }

        // 3) 탭 막대는 내용 위에 그려서 항상 보이게
        RenderTabBar();

        m_renderer->EndFrame();
    }

    void Application::RenderTabBar()
    {
        const float width{ m_renderer->GetWidth() };
        m_renderer->FillRect(Core::RectF{ 0.0f, 0.0f, width, TAB_BAR_HEIGHT }, Theme::HEADER_BACK_COLOR);

        for (std::size_t index{}; index < TOOL_COUNT; ++index)
        {
            const Core::RectF tabRect{ GetTabRect(index) };
            const bool isActive{ index == m_activeIndex };

            if (!isActive && m_hoverIndex == index)
            {
                m_renderer->FillRect(tabRect, WithAlpha(Theme::SELECTION_BACK_COLOR, HOVER_ALPHA));
            }

            m_renderer->DrawString(m_tools[index].get().GetTabTitle(), tabRect,
                isActive ? Theme::HEADER_TEXT_COLOR : Theme::MUTED_TEXT_COLOR,
                isActive ? ACTIVE_TAB_STYLE : INACTIVE_TAB_STYLE);

            // 탭 오른쪽 구분선
            m_renderer->DrawLine(tabRect.m_right, tabRect.m_top + TAB_DIVIDER_MARGIN, tabRect.m_right, tabRect.m_bottom - TAB_DIVIDER_MARGIN,
                Theme::GRID_LINE_COLOR, LINE_WIDTH);
        }

        // 탭 막대 아래 경계선
        m_renderer->DrawLine(0.0f, TAB_BAR_HEIGHT, width, TAB_BAR_HEIGHT, Theme::GRID_LINE_COLOR, LINE_WIDTH);

        // 선택한 탭 밑줄 (m_indicatorPosition을 따라 미끄러지듯 이동)
        const float indicatorLeft{ m_indicatorPosition * TAB_WIDTH };
        m_renderer->FillRect(Core::RectF{ indicatorLeft, TAB_BAR_HEIGHT - TAB_INDICATOR_HEIGHT, indicatorLeft + TAB_WIDTH, TAB_BAR_HEIGHT },
            Theme::TOTAL_TEXT_COLOR);
    }

    Core::RectF Application::GetTabRect(std::size_t index) const noexcept
    {
        const float tabLeft{ static_cast<float>(index) * TAB_WIDTH };
        return Core::RectF{ tabLeft, 0.0f, tabLeft + TAB_WIDTH, TAB_BAR_HEIGHT };
    }

    Core::RectF Application::GetContentRect() const noexcept
    {
        return Core::RectF{ 0.0f, TAB_BAR_HEIGHT, m_renderer->GetWidth(), m_renderer->GetHeight() };
    }

    std::optional<std::size_t> Application::FindTabAt(float x, float y) const noexcept
    {
        for (std::size_t index{}; index < TOOL_COUNT; ++index)
        {
            if (GetTabRect(index).Contains(x, y)) return index;
        }
        return std::nullopt;
    }

    Core::ToolManager& Application::GetActiveTool() const noexcept
    {
        return m_tools[m_activeIndex].get();   // m_activeIndex는 SelectTool에서 TOOL_COUNT 미만만 허용
    }
}