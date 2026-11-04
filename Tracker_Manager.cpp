module LostArkMultiTool.Views.TrackerManager;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.PlaceholderView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"낙원력 트래커" };
        constexpr std::wstring_view PLACEHOLDER_DESCRIPTION{ L"낙원력 기록 · 그래프 화면을 준비하고 있어요!" };
    }

    TrackerManager::TrackerManager() = default;

    TrackerManager::~TrackerManager() = default;

    std::wstring_view TrackerManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void TrackerManager::Initialize()
    {
        // 화면 단계에서 채울 부분: 낙원력 기록 불러오기
    }

    void TrackerManager::Shutdown()
    {
        // 화면 단계에서 채울 부분: 낙원력 기록 저장
    }

    void TrackerManager::OnActivated()
    {}

    void TrackerManager::OnDeactivated()
    {}

    void TrackerManager::Update(double /*deltaSeconds*/)
    {}

    void TrackerManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        DrawPlaceholder(renderer, contentArea, TAB_TITLE, PLACEHOLDER_DESCRIPTION);
    }
}