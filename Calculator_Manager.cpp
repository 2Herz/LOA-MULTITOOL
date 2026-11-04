module LostArkMultiTool.Views.CalculatorManager;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Views.PlaceholderView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"귀속재료 계산기" };
        constexpr std::wstring_view PLACEHOLDER_DESCRIPTION{ L"캐릭터별 귀속재료 표 · 그래프 화면을 준비하고 있어요!" };
    }

    CalculatorManager::CalculatorManager() = default;

    CalculatorManager::~CalculatorManager() = default;

    std::wstring_view CalculatorManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void CalculatorManager::Initialize(Core::ToolContext& /*context*/)
    {
        // 화면 단계에서 채울 부분: 공용 서비스(API · 알림) 보관, 저장 파일(LostArkRosterData.json) 불러오기
    }

    void CalculatorManager::Shutdown()
    {
        // 화면 단계에서 채울 부분: 캐릭터 · 재료 데이터 저장
    }

    void CalculatorManager::OnActivated()
    {}

    void CalculatorManager::OnDeactivated()
    {}

    void CalculatorManager::Update(double /*deltaSeconds*/)
    {}

    void CalculatorManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        DrawPlaceholder(renderer, contentArea, TAB_TITLE, PLACEHOLDER_DESCRIPTION);
    }

    bool CalculatorManager::HandleInput(const Core::InputEvent& /*event*/)
    {
        return false;   // 아직 입력을 받는 부품이 없음
    }

    Core::CursorShape CalculatorManager::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_ARROW;
    }
}