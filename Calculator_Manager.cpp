module LostArkMultiTool.Views.CalculatorManager;

import std;
import LostArkMultiTool.Core.Renderer;
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

    void CalculatorManager::Initialize()
    {
        // 화면 단계에서 채울 부분: 저장 파일(LostArkRosterData.json) · API 키 불러오기
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
}