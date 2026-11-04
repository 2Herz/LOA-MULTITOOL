module LostArkMultiTool.Views.APIKeyDialog;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view API_KEY_PAGE_URL{ L"https://developer-lostark.game.onstove.com/" };
        constexpr std::size_t API_KEY_MAX_LENGTH{ 2048 };   // 로스트아크 API 키(JWT)는 수백 글자

        // ===== 문구 =====
        constexpr std::wstring_view FIRST_RUN_TITLE{ L"초기 설정 - API 키 입력" };
        constexpr std::wstring_view CHANGE_TITLE{ L"API 키 변경" };
        constexpr std::wstring_view INFO_TEXT{ L"원활한 시세 동기화를 위해 스토브 개발자 센터의 API 키를 입력해 주세요." };
        constexpr std::wstring_view LINK_TEXT{ L"▶ API 키 발급받는 곳 (클릭)" };
        constexpr std::wstring_view PLACEHOLDER_TEXT{ L"API 키를 붙여 넣어 주세요 (Ctrl+V)" };
        constexpr std::wstring_view HINT_TEXT{ L"API 키는 이 PC의 Windows 계정으로 암호화해서 저장돼요. (세 툴 공용)" };
        constexpr std::wstring_view EMPTY_KEY_TEXT{ L"API 키를 입력해 주세요!" };
        constexpr std::wstring_view FIRST_RUN_SAVE_TEXT{ L"저장 및 시작" };
        constexpr std::wstring_view FIRST_RUN_CANCEL_TEXT{ L"종료" };
        constexpr std::wstring_view CHANGE_SAVE_TEXT{ L"저장" };
        constexpr std::wstring_view CHANGE_CANCEL_TEXT{ L"취소" };
        constexpr std::wstring_view BROWSER_ERROR_TITLE{ L"알림" };
        constexpr std::wstring_view BROWSER_ERROR_TEXT{ L"브라우저를 열지 못했어요.\n아래 주소를 직접 열어 주세요!\n" };

        // ===== 배치 (DIP) =====
        constexpr float DIALOG_WIDTH{ 520.0f };
        constexpr float LINK_HEIGHT{ 22.0f };
        constexpr float LINK_WIDTH{ 220.0f };
        constexpr float STATUS_HEIGHT{ 18.0f };
        constexpr float SAVE_BUTTON_WIDTH{ 120.0f };
        constexpr float CANCEL_BUTTON_WIDTH{ 96.0f };
        constexpr float INFO_LINK_GAP{ 6.0f };
        constexpr float LINK_BOX_GAP{ 12.0f };
        constexpr float BOX_STATUS_GAP{ 6.0f };
        constexpr float STATUS_BUTTON_GAP{ 18.0f };
        constexpr float BUTTON_GAP{ 8.0f };

        constexpr Core::TextStyle INFO_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP, Core::TextWrap::WORD_WRAP };
        constexpr Core::TextStyle STATUS_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };

        constexpr std::wstring_view TRIM_CHARACTERS{ L" \u3000" };   // 일반 공백 · 전각 공백

        // 앞뒤 공백 제거 (붙여 넣을 때 딸려 온 공백)
        [[nodiscard]] std::wstring_view TrimSpaces(std::wstring_view text) noexcept
        {
            const std::size_t first{ text.find_first_not_of(TRIM_CHARACTERS) };
            if (first == std::wstring_view::npos) return {};

            const std::size_t last{ text.find_last_not_of(TRIM_CHARACTERS) };
            return text.substr(first, last - first + 1);
        }
    }

    APIKeyDialog::APIKeyDialog(Core::Platform& platform, Core::Notifier& notifier, APIKeyDialogMode mode, std::wstring currentKey,
        std::function<bool(const std::string& apiKey)> onSave, std::function<void()> onCancel)
        : UI::Dialog{ std::wstring{ mode == APIKeyDialogMode::API_KEY_FIRST_RUN ? FIRST_RUN_TITLE : CHANGE_TITLE },
            Core::NoticeKind::NOTICE_INFO, DIALOG_WIDTH }
        , m_platform{ platform }
        , m_notifier{ notifier }
        , m_mode{ mode }
        , m_infoLabel{ GetHost().Emplace<UI::Label>(std::wstring{ INFO_TEXT }, INFO_STYLE, Theme::HEADER_TEXT_COLOR) }
    , m_linkButton{ GetHost().Emplace<UI::Button>(std::wstring{ LINK_TEXT }, UI::ButtonStyle::BUTTON_LINK, [this] { OpenKeyPage(); }) }
        , m_keyBox{ GetHost().Emplace<UI::TextBox>(platform, std::wstring{ PLACEHOLDER_TEXT }) }
        , m_statusLabel{ GetHost().Emplace<UI::Label>(std::wstring{ HINT_TEXT }, STATUS_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_saveButton{ GetHost().Emplace<UI::Button>(
            std::wstring{ mode == APIKeyDialogMode::API_KEY_FIRST_RUN ? FIRST_RUN_SAVE_TEXT : CHANGE_SAVE_TEXT },
            UI::ButtonStyle::BUTTON_PRIMARY, [this] { Save(); }) }
        , m_cancelButton{ GetHost().Emplace<UI::Button>(
            std::wstring{ mode == APIKeyDialogMode::API_KEY_FIRST_RUN ? FIRST_RUN_CANCEL_TEXT : CHANGE_CANCEL_TEXT },
            UI::ButtonStyle::BUTTON_SECONDARY, [this] { Cancel(); }) }
        , m_onSave{ std::move(onSave) }
        , m_onCancel{ std::move(onCancel) }
    {
        // API 키는 영문 · 숫자 · 기호뿐 → 한/영 상태와 상관없이 영문으로 입력되게 IME를 끔
        m_keyBox.SetIMEAllowed(false);
        m_keyBox.SetMaxLength(API_KEY_MAX_LENGTH);
        m_keyBox.SetText(std::move(currentKey));   // 재입력이면 기존 키를 채워서 표시 (원본과 같음)
        m_keyBox.SetOnTextChanged([this] { ShowHint(); });
        m_keyBox.SetOnSubmit([this] { Save(); });   // Enter = 저장
    }

    float APIKeyDialog::MeasureContentHeight(Core::Renderer& renderer, float contentWidth)
    {
        m_infoHeight = m_infoLabel.MeasureHeight(renderer, contentWidth);
        return m_infoHeight + INFO_LINK_GAP + LINK_HEIGHT + LINK_BOX_GAP + Theme::CONTROL_HEIGHT
            + BOX_STATUS_GAP + STATUS_HEIGHT + STATUS_BUTTON_GAP + Theme::CONTROL_HEIGHT;
    }

    // 안내 → 링크 → 입력칸 → 안내/오류 → 버튼 (위에서 아래로)
    void APIKeyDialog::LayoutContent(const Core::RectF& contentArea)
    {
        float top{ contentArea.m_top };
        m_infoLabel.SetBounds(Core::RectF{ contentArea.m_left, top, contentArea.m_right, top + m_infoHeight });

        top += m_infoHeight + INFO_LINK_GAP;
        m_linkButton.SetBounds(Core::RectF{ contentArea.m_left, top, contentArea.m_left + LINK_WIDTH, top + LINK_HEIGHT });

        top += LINK_HEIGHT + LINK_BOX_GAP;
        m_keyBox.SetBounds(Core::RectF{ contentArea.m_left, top, contentArea.m_right, top + Theme::CONTROL_HEIGHT });

        top += Theme::CONTROL_HEIGHT + BOX_STATUS_GAP;
        m_statusLabel.SetBounds(Core::RectF{ contentArea.m_left, top, contentArea.m_right, top + STATUS_HEIGHT });

        const float buttonTop{ contentArea.m_bottom - Theme::CONTROL_HEIGHT };
        const float cancelLeft{ contentArea.m_right - CANCEL_BUTTON_WIDTH };
        m_cancelButton.SetBounds(Core::RectF{ cancelLeft, buttonTop, contentArea.m_right, contentArea.m_bottom });

        const float saveRight{ cancelLeft - BUTTON_GAP };
        m_saveButton.SetBounds(Core::RectF{ saveRight - SAVE_BUTTON_WIDTH, buttonTop, saveRight, contentArea.m_bottom });
    }

    // 키 입력칸부터 (기존 키가 있으면 전체 선택 → 바로 붙여 넣으면 교체)
    void APIKeyDialog::FocusInitialControl()
    {
        GetHost().SetFocus(&m_keyBox);
        m_keyBox.SelectAll();
    }

    void APIKeyDialog::OnSubmit()
    {
        Save();
    }

    // Esc: 키 변경은 취소, 처음 실행은 무시 (실수로 프로그램이 꺼지지 않게 [종료]를 직접 눌러야 함)
    void APIKeyDialog::OnCancel()
    {
        if (m_mode == APIKeyDialogMode::API_KEY_CHANGE) Cancel();
    }

    void APIKeyDialog::Save()
    {
        if (IsClosed()) return;

        m_keyBox.CommitComposition();
        const std::wstring_view trimmedKey{ TrimSpaces(m_keyBox.GetText()) };
        if (trimmedKey.empty())
        {
            ShowError(std::wstring{ EMPTY_KEY_TEXT });
            GetHost().SetFocus(&m_keyBox);
            return;
        }

        const std::string apiKey{ Core::TextEncoding::WideToUTF8(trimmedKey) };
        if (m_onSave && !m_onSave(apiKey)) return;   // 저장 실패 → 창을 열어 둔 채 다시 시도할 수 있게

        Close();
    }

    void APIKeyDialog::Cancel()
    {
        if (IsClosed()) return;

        Close();
        if (m_onCancel) m_onCancel();
    }

    // 발급 페이지 열기 (기본 브라우저가 없거나 막힌 경우 주소를 안내 → 직접 열 수 있게)
    void APIKeyDialog::OpenKeyPage()
    {
        if (m_platform.OpenURL(API_KEY_PAGE_URL)) return;

        m_notifier.ShowMessage(std::wstring{ BROWSER_ERROR_TITLE }, std::wstring{ BROWSER_ERROR_TEXT } + std::wstring{ API_KEY_PAGE_URL },
            Core::NoticeKind::NOTICE_WARNING, nullptr);
    }

    void APIKeyDialog::ShowHint()
    {
        m_statusLabel.SetText(std::wstring{ HINT_TEXT });
        m_statusLabel.SetColor(Theme::MUTED_TEXT_COLOR);
    }

    void APIKeyDialog::ShowError(std::wstring message)
    {
        m_statusLabel.SetText(std::move(message));
        m_statusLabel.SetColor(Theme::NOTICE_ERROR_COLOR);
    }
}