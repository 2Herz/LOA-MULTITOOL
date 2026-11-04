module LostArkMultiTool.Views.GemFinderManager;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.PlaceholderView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"보석 최저가 검색기" };
        constexpr std::wstring_view PLACEHOLDER_DESCRIPTION{ L"보석 최저가 검색 화면을 준비하고 있어요!" };
    }

    GemFinderManager::GemFinderManager() = default;

    GemFinderManager::~GemFinderManager() = default;

    std::wstring_view GemFinderManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void GemFinderManager::Initialize()
    {
        // 화면 단계에서 채울 부분: 검색 조건 · API 키 불러오기
    }

    void GemFinderManager::Shutdown()
    {
        // 화면 단계에서 채울 부분: 검색 조건 저장
    }

    void GemFinderManager::OnActivated()
    {}

    void GemFinderManager::OnDeactivated()
    {}

    void GemFinderManager::Update(double /*deltaSeconds*/)
    {}

    void GemFinderManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        DrawPlaceholder(renderer, contentArea, TAB_TITLE, PLACEHOLDER_DESCRIPTION);
    }
}