export module LostArkMultiTool.Core.Platform;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Core
{
    // 운영체제 기능 인터페이스 (화면 부품이 Windows를 직접 부르지 않게 → 실제 구현은 Views::MainWindow)
    class Platform
    {
    public:
        virtual ~Platform();

        // 클립보드 (글자가 없으면 빈 문자열, 실패하면 false)
        [[nodiscard]] virtual std::wstring GetClipboardText() = 0;
        virtual bool SetClipboardText(std::wstring_view text) = 0;

        virtual void SetCursor(CursorShape shape) = 0;

        // 한글 입력기(IME): 글자 입력칸에 포커스가 있을 때만 켬 (단축키 · 영문 전용 칸에서는 끔)
        virtual void SetIMEEnabled(bool isEnabled) = 0;
        virtual void SetIMEPosition(const RectF& caretRect) = 0;   // 한자 변환 후보 창 위치 (DIP)
        virtual void CancelComposition() = 0;                      // 조합 중인 글자 버림 (입력칸이 직접 확정한 뒤 호출)

        // 기본 브라우저로 열기 (http · https 주소만, 실패하면 false)
        [[nodiscard]] virtual bool OpenURL(std::wstring_view url) = 0;
    };
}