export module LostArkMultiTool.Views.PlaceholderView;

import std;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Views
{
    // 아직 화면을 만들지 않은 탭의 내용 영역 가운데에 제목 · 설명을 표시
    void DrawPlaceholder(Core::Renderer& renderer, const Core::RectF& contentArea, std::wstring_view title, std::wstring_view description);
}