module LostArkMultiTool.Views.OverlayPlaceholderTool;  // "오버레이에서 사용 중" 안내 탭 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.ToolContext;  // 공용 서비스 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::wstring_view TITLE_TEXT{ L"오버레이에서 사용 중" };   // 제목
        constexpr std::wstring_view MESSAGE_TEXT{ L"이 탭은 지금 인게임 오버레이의 미니 모드에서 쓰고 있어요.\n오버레이에서 미니 탭을 닫거나 다른 탭으로 바꾸면 여기서 다시 쓸 수 있어요." };   // 안내
        constexpr float MESSAGE_FONT_SIZE{ 15.0f };                             // 글자 크기
        constexpr Core::TextStyle MESSAGE_STYLE{ MESSAGE_FONT_SIZE, Core::FontWeight::NORMAL, Core::TextAlign::CENTER,
            Core::VerticalAlign::ALIGN_MIDDLE, Core::TextWrap::WORD_WRAP };     // 가운데 · 줄바꿈
    }

    std::wstring_view OverlayPlaceholderTool::GetTabTitle() const noexcept { return TITLE_TEXT; }   // 제목
    void OverlayPlaceholderTool::Initialize(Core::ToolContext&) {}       // 할 일 없음
    void OverlayPlaceholderTool::Shutdown() {}                           // 할 일 없음
    void OverlayPlaceholderTool::OnActivated() {}                        // 할 일 없음
    void OverlayPlaceholderTool::OnDeactivated() {}                      // 할 일 없음
    void OverlayPlaceholderTool::Update(double) {}                       // 할 일 없음
    bool OverlayPlaceholderTool::HandleInput(const Core::InputEvent&) { return false; }   // 입력 받지 않음
    Core::CursorShape OverlayPlaceholderTool::GetCursor() const noexcept { return Core::CursorShape::CURSOR_ARROW; }   // 기본 커서

    void OverlayPlaceholderTool::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        renderer.DrawString(MESSAGE_TEXT, contentArea, Theme::MUTED_TEXT_COLOR, MESSAGE_STYLE);   // 가운데 안내
    }
}
