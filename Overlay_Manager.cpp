module LostArkMultiTool.Views.OverlayManager;  // 인게임 오버레이 매니저 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Notifier;  // 알림 종류 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.ToolManager;  // 탭 매니저 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기
import LostArkMultiTool.Vision.GameWindowFinder;  // 게임 창 찾기 가져오기
import LostArkMultiTool.Views.BossPanel;  // 보스 체력 패널
import LostArkMultiTool.Views.CalculatorManager;  // 재료 요약 · 현재 캐릭터 · 보스 체력
import LostArkMultiTool.Views.CharacterPanel;  // 현재 캐릭터 패널
import LostArkMultiTool.Views.DamagePanel;  // 딜 지분 패널
import LostArkMultiTool.Views.DX11Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.MaterialPanel;  // 재료 패널
import LostArkMultiTool.Views.MiniToolPanel;  // 탭 미니 모드 패널
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반
import LostArkMultiTool.Views.OverlaySettings;  // 설정
import LostArkMultiTool.Views.OverlayWindow;  // 오버레이 창
import LostArkMultiTool.Views.ParadisePanel;  // 낙원력 패널
import LostArkMultiTool.Views.Theme;  // 색
import LostArkMultiTool.Views.TrackerManager;  // 낙원력 요약
import LostArkMultiTool.Views.UI.ToastHost;  // 토스트

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr int HOTKEY_TOGGLE_OVERLAY{ 1 };            // 단축키 번호: 오버레이 켜기 · 끄기
        constexpr int HOTKEY_TOGGLE_INTERACT{ 2 };           // 단축키 번호: 조작 모드
        constexpr int HOTKEY_REFRESH_PRICES{ 3 };            // 단축키 번호: 시세 갱신
        constexpr unsigned KEY_F5{ 0x74 };                   // F5 키 (VK_F5)
        constexpr float BOSS_PANEL_TOP_RATIO{ 0.15f };       // 보스 체력 패널 위쪽 (게임 체력바 바로 아래, 화면 세로 비율)
        constexpr unsigned MODIFIER_CONTROL{ 0x0002 };       // MOD_CONTROL (Ctrl)
        constexpr unsigned MODIFIER_SHIFT{ 0x0004 };         // MOD_SHIFT (Shift)
        constexpr unsigned KEY_O{ 'O' };                     // O 키 (가상 키 = 대문자)
        constexpr unsigned KEY_I{ 'I' };                     // I 키
        constexpr double FIND_INTERVAL_SECONDS{ 1.0 };       // 게임 창을 다시 찾는 간격 (못 찾았을 때)
        constexpr double RENDER_INTERVAL_SECONDS{ 1.0 / 20.0 };   // 평소 그리는 간격 (초당 20번이면 충분, 게임 성능 보호)
        constexpr float PANEL_MARGIN{ 16.0f };               // 화면 가장자리와 패널 사이
        constexpr float PANEL_GAP{ 12.0f };                  // 패널 사이
        constexpr float PANEL_TOP{ 110.0f };                 // 첫 패널 위쪽 (게임 위쪽 재화 줄 · 미니맵 아래)
        constexpr float CHARACTER_PANEL_TOP{ 64.0f };        // 현재 캐릭터 패널 위쪽 (왼쪽 위 아이콘 줄 아래)
        constexpr float OPACITY_STEP{ 0.1f };                // 불투명도 한 단계
        constexpr float TOOLBAR_TOP{ 8.0f };                 // 도구 막대 위쪽
        constexpr float TOOLBAR_HEIGHT{ 30.0f };             // 도구 막대 높이
        constexpr float TOOLBAR_BUTTON_PADDING{ 14.0f };     // 단추 글자 좌우 여백
        constexpr float TOOLBAR_GAP{ 6.0f };                 // 단추 사이
        constexpr float TOOLBAR_RADIUS{ 6.0f };              // 단추 모서리
        constexpr float TOOLBAR_FONT_SIZE{ 12.0f };          // 단추 글자 크기
        constexpr float TOOLBAR_HINT_HEIGHT{ 20.0f };        // 단추 아래 안내 줄 높이
        constexpr float TOOLBAR_BACK_ALPHA{ 0.92f };         // 단추 바탕 불투명도
        constexpr float ACTIVE_BACK_ALPHA{ 0.45f };          // 켜진 단추 바탕 불투명도
        constexpr float BORDER_WIDTH{ 1.0f };                // 테두리 두께
        constexpr float HALF{ 0.5f };                        // 절반
        constexpr float BOTH_SIDES{ 2.0f };                  // 좌우 양쪽
        constexpr Core::Color TRANSPARENT_COLOR{ 0.0f, 0.0f, 0.0f, 0.0f };   // 완전 투명 (게임 화면이 비침)
        constexpr Core::TextStyle TOOLBAR_STYLE{ TOOLBAR_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };   // 단추 글꼴
        constexpr Core::TextStyle HINT_STYLE{ TOOLBAR_FONT_SIZE, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };    // 안내 글꼴
        constexpr std::wstring_view TOOLBAR_CHARACTER_TEXT{ L"현재 캐릭터" };   // 단추 글자
        constexpr std::wstring_view TOOLBAR_DAMAGE_TEXT{ L"딜 지분" };          // 딜 지분 단추 글자
        constexpr float DAMAGE_PANEL_BOTTOM_RATIO{ 0.74f };  // 딜 지분 패널 아래쪽 (게임 전투 분석기 바로 위, 화면 세로 비율)
        constexpr std::wstring_view TOOLBAR_BOSS_TEXT{ L"보스 체력" };
        constexpr std::wstring_view REFRESH_HOTKEY_FAILED_TEXT{ L"시세 갱신 단축키(Ctrl+Shift+F5)를 다른 프로그램이 쓰고 있어요. 그 프로그램의 단축키를 바꾸거나 계산기의 [시세 갱신]을 눌러 주세요." };
        constexpr std::wstring_view TOOLBAR_MATERIAL_TEXT{ L"재료 시세" };
        constexpr std::wstring_view TOOLBAR_PARADISE_TEXT{ L"낙원력" };
        constexpr std::wstring_view TOOLBAR_MINI_TEXT{ L"미니 탭" };
        constexpr std::wstring_view TOOLBAR_OPACITY_DOWN_TEXT{ L"투명하게" };
        constexpr std::wstring_view TOOLBAR_OPACITY_UP_TEXT{ L"진하게" };
        constexpr std::wstring_view TOOLBAR_DONE_TEXT{ L"조작 끝" };
        constexpr std::wstring_view TOOLBAR_HINT_TEXT{ L"조작 모드 · 패널 머리말을 끌어 옮기고, 미니 탭은 오른쪽 아래로 크기를 바꿔요 · Ctrl+Shift+I 조작 끝 · Ctrl+Shift+O 오버레이 끄기" };
        constexpr std::wstring_view HOTKEY_FAILED_TEXT{ L"오버레이 단축키(Ctrl+Shift+O · Ctrl+Shift+I)를 다른 프로그램이 쓰고 있어요. 메뉴의 [인게임 오버레이] 단추로 켜 주세요." };
        constexpr std::wstring_view SAVE_FAILED_TEXT{ L"오버레이 설정을 저장하지 못했어요." };
    }

    OverlayManager::OverlayManager() = default;    // 기본 생성
    OverlayManager::~OverlayManager() = default;   // 기본 소멸

    void OverlayManager::Initialize(std::span<const std::reference_wrapper<Core::ToolManager>> tools)
    {
        m_tools.assign(tools.begin(), tools.end());                                      // 미니 모드 탭들
        m_settings = std::make_unique<OverlaySettings>();                                // 설정 읽기
        m_window = std::make_unique<OverlayWindow>();                                    // 숨은 투명 창
        m_renderer = std::make_unique<DX11Renderer>(m_window->GetNativeHandle(), m_window->GetDPI(), SurfaceMode::SURFACE_TRANSPARENT);   // 투명 렌더러

        m_window->SetOnInput([this](const Core::InputEvent& event) { HandleInput(event); });                    // 입력
        m_window->SetOnHotkey([this](int hotkeyId) { HandleHotkey(hotkeyId); });                                // 단축키
        m_window->SetOnResize([this](int width, int height) { if (m_renderer) m_renderer->Resize(width, height); m_isDirty = true; });   // 크기
        m_window->SetOnDPIChanged([this](float dpi) { if (m_renderer) m_renderer->SetDPI(dpi); m_isDirty = true; });                    // 배율

        const bool isOverlayKey = m_window->RegisterHotkey(HOTKEY_TOGGLE_OVERLAY, MODIFIER_CONTROL | MODIFIER_SHIFT, KEY_O);    // Ctrl+Shift+O
        const bool isInteractKey = m_window->RegisterHotkey(HOTKEY_TOGGLE_INTERACT, MODIFIER_CONTROL | MODIFIER_SHIFT, KEY_I);  // Ctrl+Shift+I
        if (!isOverlayKey || !isInteractKey) m_pendingNotice = std::wstring{ HOTKEY_FAILED_TEXT };                              // 등록 실패 안내
        else if (!m_window->RegisterHotkey(HOTKEY_REFRESH_PRICES, MODIFIER_CONTROL | MODIFIER_SHIFT, KEY_F5))                   // Ctrl+Shift+F5
            m_pendingNotice = std::wstring{ REFRESH_HOTKEY_FAILED_TEXT };                                                       // 등록 실패 안내
        [[maybe_unused]] const bool isCaptureApplied = m_window->SetCaptureHidden(m_settings->IsCaptureHidden());              // 캡처 · 방송에서 숨기기 (옛 윈도우면 그대로)

        // 패널들 (컴포넌트): 탭 미니 모드(큰 패널이라 맨 아래에 그림), 재료 시세 · 총자산, 낙원력 주간 현황
        if (!m_tools.empty())
        {
            auto mini = std::make_unique<MiniToolPanel>(std::span<const std::reference_wrapper<Core::ToolManager>>{ m_tools },
                m_settings->GetMiniToolIndex(), [this](std::size_t index) { m_settings->SetMiniToolIndex(index); SaveSettings(); });   // 탭을 바꾸면 저장
            mini->SetVisible(false);                                                     // 처음엔 숨김 (도구 막대로 켬)
            m_panels.push_back(std::move(mini));                                         // 추가
        }
        m_panels.push_back(std::make_unique<MaterialPanel>([] { return CalculatorManager::GetInstance().GetOverlaySummary(); }));
        m_panels.push_back(std::make_unique<ParadisePanel>([] { return TrackerManager::GetInstance().GetOverlaySummary(); }));
        m_panels.push_back(std::make_unique<BossPanel>([] { return CalculatorManager::GetInstance().GetBossHealth(); }));   // 보스 체력
        auto damage = std::make_unique<DamagePanel>(
            [] { const auto& calculator = CalculatorManager::GetInstance(); return DamageShareSource{ calculator.GetBossHealth(), calculator.GetCombatStats() }; },   // 보스 체력 · 전투 분석기
            [this] { return m_settings->GetRaidSize(); },                                       // 8인 · 4인
            [this](int raidSize) { m_settings->SetRaidSize(raidSize); SaveSettings(); });       // 바꾸면 저장
        damage->SetVisible(false);                                                       // 처음엔 숨김 (도구 막대로 켬)
        m_panels.push_back(std::move(damage));                                           // 딜 지분
        m_panels.push_back(std::make_unique<CharacterPanel>(
            [] { return CalculatorManager::GetInstance().GetCurrentCharacter(); },                        // 현재 캐릭터 요약
            [](std::size_t index) { CalculatorManager::GetInstance().PickCurrentCharacter(index); },      // [변경]으로 고름
            [] { CalculatorManager::GetInstance().RefreshCurrentCharacter(); }));                         // [갱신]
        LoadPanelStates();                                                               // 저장된 위치 · 표시

        // 조작 모드 도구 막대
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_CHARACTER_TEXT },
            [this] { if (auto* panel = FindPanel(CharacterPanel::PANEL_ID)) { panel->SetVisible(!panel->IsVisible()); SavePanelStates(); } },
            [this] { const auto* panel = FindPanel(CharacterPanel::PANEL_ID); return panel != nullptr && panel->IsVisible(); } });  // 현재 캐릭터 패널 켜기 · 끄기
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_BOSS_TEXT },
            [this] { if (auto* panel = FindPanel(BossPanel::PANEL_ID)) { panel->SetVisible(!panel->IsVisible()); SavePanelStates(); } },
            [this] { const auto* panel = FindPanel(BossPanel::PANEL_ID); return panel != nullptr && panel->IsVisible(); } });   // 보스 체력 패널 켜기 · 끄기
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_DAMAGE_TEXT },
            [this] { if (auto* panel = FindPanel(DamagePanel::PANEL_ID)) { panel->SetVisible(!panel->IsVisible()); SavePanelStates(); } },
            [this] { const auto* panel = FindPanel(DamagePanel::PANEL_ID); return panel != nullptr && panel->IsVisible(); } });  // 딜 지분 패널 켜기 · 끄기
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_MATERIAL_TEXT },
            [this] { if (auto* panel = FindPanel(MaterialPanel::PANEL_ID)) { panel->SetVisible(!panel->IsVisible()); SavePanelStates(); } },
            [this] { const auto* panel = FindPanel(MaterialPanel::PANEL_ID); return panel != nullptr && panel->IsVisible(); } });   // 재료 패널 켜기 · 끄기
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_PARADISE_TEXT },
            [this] { if (auto* panel = FindPanel(ParadisePanel::PANEL_ID)) { panel->SetVisible(!panel->IsVisible()); SavePanelStates(); } },
            [this] { const auto* panel = FindPanel(ParadisePanel::PANEL_ID); return panel != nullptr && panel->IsVisible(); } });  // 낙원력 패널 켜기 · 끄기
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_MINI_TEXT },
            [this] { if (auto* panel = FindPanel(MiniToolPanel::PANEL_ID)) { panel->SetVisible(!panel->IsVisible()); SavePanelStates(); } },
            [this] { const auto* panel = FindPanel(MiniToolPanel::PANEL_ID); return panel != nullptr && panel->IsVisible(); } });  // 미니 모드 켜기 · 끄기
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_OPACITY_DOWN_TEXT },
            [this] { m_settings->SetOpacity(m_settings->GetOpacity() - OPACITY_STEP); SaveSettings(); }, {} });              // 더 투명하게
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_OPACITY_UP_TEXT },
            [this] { m_settings->SetOpacity(m_settings->GetOpacity() + OPACITY_STEP); SaveSettings(); }, {} });              // 더 진하게
        m_toolbar.push_back(ToolbarButton{ std::wstring{ TOOLBAR_DONE_TEXT }, [this] { SetInteractive(false); }, {} });      // 조작 끝
    }

    void OverlayManager::Shutdown()
    {
        if (m_settings) SavePanelStates();   // 마지막 위치 저장
        m_panels.clear();                    // 패널 정리 (미니 모드 탭 참조를 먼저 놓음)
        m_renderer.reset();                  // 렌더러 (스왑체인 · 합성) 먼저
        m_window.reset();                    // 창
        m_settings.reset();                  // 설정
        m_tools.clear();                     // 탭 목록
    }

    bool OverlayManager::IsEnabled() const noexcept
    {
        return m_settings && m_settings->IsEnabled();   // 켜짐
    }

    void OverlayManager::SetEnabled(bool isEnabled)
    {
        if (!m_settings || m_settings->IsEnabled() == isEnabled) return;   // 그대로
        m_settings->SetEnabled(isEnabled);                                 // 바꾸기
        if (!isEnabled) SetInteractive(false);                             // 끄면 조작 모드도 끝
        SaveSettings();                                                    // 저장
        m_isDirty = true;                                                  // 다시 그리기
        if (m_onEnabledChanged) m_onEnabledChanged(isEnabled);             // 알림 (메뉴 단추 글자)
    }

    bool OverlayManager::IsVisible() const noexcept
    {
        return m_isVisible;   // 지금 보이는지
    }

    std::optional<std::size_t> OverlayManager::GetMiniToolIndex() const noexcept
    {
        if (!m_isVisible) return std::nullopt;                                             // 오버레이가 안 보이면 메인 창에서 그대로 씀
        const auto* mini = dynamic_cast<const MiniToolPanel*>(FindPanel(MiniToolPanel::PANEL_ID));   // 미니 패널
        if (mini == nullptr || !mini->IsVisible()) return std::nullopt;                    // 미니 모드 꺼짐
        return mini->GetToolIndex();                                                       // 띄운 탭
    }

    bool OverlayManager::ShowToast(const std::wstring& message, Core::NoticeKind kind)
    {
        if (!m_isVisible) return false;   // 게임 위에 안 보이면 메인 창 방식 그대로
        m_toasts.Add(message, kind);      // 게임 위 토스트
        m_isDirty = true;                 // 다시 그리기
        return true;                      // 띄움
    }

    std::optional<std::wstring> OverlayManager::TakeNotice()
    {
        return std::exchange(m_pendingNotice, std::nullopt);   // 한 번만 넘김
    }

    void OverlayManager::SetOnEnabledChanged(std::function<void(bool isEnabled)> onEnabledChanged)
    {
        m_onEnabledChanged = std::move(onEnabledChanged);   // 알림 연결
    }

    void OverlayManager::SetOnRefreshHotkey(std::function<void()> onRefreshHotkey)
    {
        m_onRefreshHotkey = std::move(onRefreshHotkey);     // 알림 연결
    }

    bool OverlayManager::IsCaptureHidden() const noexcept
    {
        return m_settings && m_settings->IsCaptureHidden(); // 캡처 숨기기
    }

    void OverlayManager::SetCaptureHidden(bool isHidden)
    {
        if (!m_settings || !m_window) return;               // 초기화 전
        m_settings->SetCaptureHidden(isHidden);             // 기록
        [[maybe_unused]] const bool isApplied = m_window->SetCaptureHidden(isHidden);   // 창에 반영
        SaveSettings();                                     // 저장
    }

    std::vector<OverlayElementInfo> OverlayManager::GetElements() const
    {
        std::vector<OverlayElementInfo> elements;                              // 결과
        elements.reserve(m_panels.size());                                     // 자리 확보
        for (const auto& panel : m_panels)                                     // 패널마다
            elements.push_back(OverlayElementInfo{ panel->GetId(), panel->GetTitle(), panel->IsVisible(), panel->GetAlpha(), panel->GetBounds() });
        return elements;                                                       // 목록
    }

    void OverlayManager::SetElementVisible(std::string_view id, bool isVisible)
    {
        OverlayPanel* panel = FindPanel(id);                                   // 패널
        if (panel == nullptr || panel->IsVisible() == isVisible) return;       // 없거나 그대로
        panel->SetVisible(isVisible);                                          // 바꾸기
        SavePanelStates();                                                     // 저장
    }

    void OverlayManager::SetElementAlpha(std::string_view id, float alpha, bool isCommit)
    {
        OverlayPanel* panel = FindPanel(id);                                   // 패널
        if (panel == nullptr) return;                                          // 없음
        panel->SetAlpha(alpha);                                                // 바꾸기 (범위는 패널이 맞춤)
        m_isDirty = true;                                                      // 다시 그리기
        if (isCommit) SavePanelStates();                                       // 놓을 때 저장
    }

    void OverlayManager::ResetElementPosition(std::string_view id)
    {
        OverlayPanel* panel = FindPanel(id);                                   // 패널
        if (panel == nullptr || !m_settings) return;                           // 없음
        const float alpha = panel->GetAlpha();                                 // 투명도는 그대로 둠
        const bool isVisible = panel->IsVisible();                             // 표시도 그대로
        m_settings->RemovePanel(id);                                           // 저장된 위치 지움
        m_isPlaced = false;                                                    // 다음에 보일 때 기본 위치로 놓음
        if (GetOverlayArea().GetWidth() > 0.0f) PlaceDefaultPanels();          // 지금 크기를 알면 바로 놓음 (저장 포함)
        panel->SetAlpha(alpha);                                                // 그대로
        panel->SetVisible(isVisible);
        if (m_isPlaced) SavePanelStates();                                     // 놓았으면 저장
        m_isDirty = true;                                                      // 다시 그리기
    }

    float OverlayManager::GetBackOpacity() const noexcept
    {
        return m_settings ? m_settings->GetOpacity() : OverlaySettings::DEFAULT_OPACITY;   // 바탕 진하기
    }

    void OverlayManager::SetBackOpacity(float opacity, bool isCommit)
    {
        if (!m_settings) return;                                               // 초기화 전
        m_settings->SetOpacity(opacity);                                       // 바꾸기 (범위는 설정이 맞춤)
        m_isDirty = true;                                                      // 다시 그리기
        if (isCommit) SaveSettings();                                          // 놓을 때 저장
    }

    bool OverlayManager::IsEditing() const noexcept
    {
        return m_window && m_window->IsInteractive();                          // 조작 모드
    }

    bool OverlayManager::SetEditing(bool isEditing)
    {
        if (!m_window) return false;                                           // 초기화 전
        if (!isEditing) { SetInteractive(false); return true; }                // 잠금 (게임으로 돌아감)
        if (!IsEnabled() || m_gameWindow == nullptr) return false;             // 오버레이가 꺼졌거나 게임이 없음
        SetInteractive(true);                                                  // 편집 (오버레이 창이 맨 앞으로 → 게임 위에 보임)
        return true;                                                           // 시작함
    }

    bool OverlayManager::IsGameFound() const noexcept
    {
        return m_gameWindow != nullptr;                                        // 따라가는 게임 창
    }

    void OverlayManager::HandleHotkey(int hotkeyId)
    {
        if (hotkeyId == HOTKEY_TOGGLE_OVERLAY) SetEnabled(!IsEnabled());                             // 켜기 · 끄기
        else if (hotkeyId == HOTKEY_TOGGLE_INTERACT && IsEnabled()) SetInteractive(!m_window->IsInteractive());   // 조작 모드
        else if (hotkeyId == HOTKEY_REFRESH_PRICES && m_onRefreshHotkey) m_onRefreshHotkey();                       // 시세 갱신
    }

    void OverlayManager::SetInteractive(bool isInteractive)
    {
        if (!m_window || m_window->IsInteractive() == isInteractive) return;   // 그대로
        m_window->SetInteractive(isInteractive);                               // 창 스타일 바꾸기
        if (isInteractive) m_window->Activate();                               // 키보드 입력을 받게 맨 앞으로
        else OverlayWindow::ActivateWindow(m_gameWindow);                      // 게임으로 포커스 돌려주기
        m_cursor = Core::CursorShape::CURSOR_ARROW;                            // 커서 초기화
        m_window->SetCursor(m_cursor);                                         // 반영
        m_isDirty = true;                                                      // 다시 그리기
    }

    void OverlayManager::TrackGameWindow()
    {
        std::optional<Vision::GameWindowInfo> info = m_gameWindow ? Vision::GameWindowFinder::Refresh(m_gameWindow) : std::nullopt;   // 지난 창 (매 프레임 가볍게)
        if (!info && m_findElapsed >= FIND_INTERVAL_SECONDS)                   // 못 찾았으면 가끔 새로 찾기 (창 목록 훑기는 무거움)
        {
            m_findElapsed = 0.0;                                               // 초기화
            info = Vision::GameWindowFinder::Find();                           // 새로 찾기
        }
        m_gameWindow = info ? info->m_handle : nullptr;                        // 기록

        const bool isGameShown = info && !info->m_isMinimized && info->m_clientArea.m_width > 0;   // 게임 화면이 보이는지
        const bool isFront = info && (info->m_isForeground || m_window->IsForeground());           // 게임 또는 오버레이가 맨 앞
        const bool shouldShow = IsEnabled() && isGameShown && isFront;         // 보일 조건
        if (shouldShow)
        {
            m_window->Place(info->m_clientArea);                               // 게임 화면에 맞춤
            if (!m_isVisible) m_isDirty = true;                                // 새로 보이면 바로 그림
            m_window->Show();                                                  // 보이기
        }
        else
        {
            if (m_window->IsInteractive()) m_window->SetInteractive(false);    // 숨으면 조작 모드도 끝 (포커스는 건드리지 않음)
            m_window->Hide();                                                  // 숨기기
        }
        m_isVisible = shouldShow;                                              // 기록
    }

    void OverlayManager::Update(double deltaSeconds)
    {
        if (!m_window) return;                                                 // 초기화 전
        m_findElapsed += deltaSeconds;                                         // 시간 누적
        m_renderElapsed += deltaSeconds;
        TrackGameWindow();                                                     // 게임 창 따라가기
        const auto* damagePanel = FindPanel(DamagePanel::PANEL_ID);            // 딜 지분 패널
        CalculatorManager::GetInstance().SetCombatScan(m_isVisible && damagePanel != nullptr && damagePanel->IsVisible());   // 보일 때만 전투 분석기 읽기
        if (!m_isVisible) return;                                              // 안 보이면 패널 갱신도 쉼

        if (!m_isPlaced) PlaceDefaultPanels();                                 // 처음 보일 때 기본 위치
        if (auto* boss = dynamic_cast<BossPanel*>(FindPanel(BossPanel::PANEL_ID)))   // 보스 체력: 게임 픽셀 → DIP (체력바에 고정)
            boss->SetPixelScale(OverlayWindow::DEFAULT_DPI_VALUE / m_window->GetDPI());
        const Core::RectF area = GetOverlayArea();                             // 오버레이 영역
        bool isChanged = false;                                                // 저장할 변화
        for (const auto& panel : m_panels)                                     // 패널마다
        {
            panel->Update(deltaSeconds);                                       // 내용 갱신
            panel->ClampInto(area);                                            // 화면 밖으로 안 나가게
            isChanged = panel->IsChanged() || isChanged;                       // 옮김 · 닫음
        }
        if (isChanged) SavePanelStates();                                      // 저장
        m_toasts.Update(deltaSeconds);                                         // 토스트 시간
    }

    void OverlayManager::Render()
    {
        if (!m_isVisible || !m_renderer) return;                               // 안 보이면 안 그림
        const bool isInteractive = m_window->IsInteractive();                  // 조작 모드
        if (!isInteractive && !m_isDirty && m_renderElapsed < RENDER_INTERVAL_SECONDS) return;   // 평소엔 초당 20번까지만
        if (!m_renderer->BeginFrame()) return;                                 // 그릴 준비 안 됨
        m_renderElapsed = 0.0;                                                 // 초기화
        m_isDirty = false;                                                     // 그림

        m_renderer->Clear(TRANSPARENT_COLOR);                                  // 전부 투명 (게임 화면이 비침)
        for (const auto& panel : m_panels)                                     // 패널마다 (뒤가 위)
            if (panel->IsVisible() && (isInteractive || panel->IsContentAvailable())) panel->Render(*m_renderer, m_settings->GetOpacity(), isInteractive);   // 보일 내용이 없으면 평소엔 안 그림
        if (isInteractive) RenderToolbar(*m_renderer);                         // 조작 모드 도구 막대
        m_toasts.Render(*m_renderer, GetOverlayArea());                        // 토스트 (오른쪽 아래)
        m_renderer->EndFrame();                                                // 화면에 내보내기
    }

    void OverlayManager::HandleInput(const Core::InputEvent& event)
    {
        if (!m_window || !m_window->IsInteractive()) return;                   // 평소엔 입력 없음 (클릭 통과)
        m_isDirty = true;                                                      // 입력이 있으면 다시 그림
        if (HandleToolbarInput(event)) return;                                 // 도구 막대
        for (auto panel = m_panels.rbegin(); panel != m_panels.rend(); ++panel)   // 위에 그려진 패널부터
        {
            if (!(*panel)->IsVisible()) continue;                              // 숨은 패널 건너뜀
            if ((*panel)->HandleInput(event, true))                            // 처리하면
            {
                m_cursor = (*panel)->GetCursor();                              // 커서
                m_window->SetCursor(m_cursor);                                 // 반영
                return;                                                        // 끝
            }
        }
        if (std::holds_alternative<Core::MouseMoveEvent>(event))               // 빈 곳 위 이동
        {
            m_cursor = Core::CursorShape::CURSOR_ARROW;                        // 기본 커서
            m_window->SetCursor(m_cursor);
        }
    }

    Core::RectF OverlayManager::GetOverlayArea() const
    {
        if (!m_renderer) return {};                                            // 렌더러 없음
        return Core::RectF{ 0.0f, 0.0f, m_renderer->GetWidth(), m_renderer->GetHeight() };   // 전체 (DIP)
    }

    OverlayPanel* OverlayManager::FindPanel(std::string_view id) const
    {
        const auto found = std::ranges::find_if(m_panels, [id](const auto& panel) { return panel->GetId() == id; });   // 이름으로
        return found == m_panels.end() ? nullptr : found->get();               // 없으면 nullptr
    }

    void OverlayManager::LoadPanelStates()
    {
        bool hasAllPositions = true;                                           // 모든 패널 위치가 저장돼 있는지
        for (const auto& panel : m_panels)                                     // 패널마다
        {
            const auto state = m_settings->FindPanel(panel->GetId());          // 저장 상태
            if (!state) { hasAllPositions = false; continue; }                 // 처음 → 기본 위치 (보임)
            const Core::SizeF size = panel->GetDefaultSize();                  // 기본 크기
            const float width = state->m_width > 0.0f ? state->m_width : size.m_width;     // 저장 가로 (없으면 기본)
            const float height = state->m_height > 0.0f ? state->m_height : size.m_height; // 저장 세로
            panel->SetBounds(Core::RectF{ state->m_x, state->m_y, state->m_x + width, state->m_y + height });   // 위치
            panel->SetVisible(state->m_isVisible);                             // 표시
            panel->SetAlpha(state->m_alpha);                                   // 요소 투명도
        }
        m_isPlaced = hasAllPositions;                                          // 모두 있으면 기본 배치 불필요
    }

    void OverlayManager::PlaceDefaultPanels()
    {
        const Core::RectF area = GetOverlayArea();                             // 오버레이 영역
        if (area.GetWidth() <= 0.0f) return;                                   // 아직 크기 없음
        float top = PANEL_TOP;                                                 // 쌓을 위치
        for (const auto& panel : m_panels)                                     // 패널마다
        {
            if (m_settings->FindPanel(panel->GetId())) continue;               // 저장된 위치가 있으면 그대로
            const Core::SizeF size = panel->GetDefaultSize();                  // 기본 크기
            if (panel->GetId() == MiniToolPanel::PANEL_ID)                     // 미니 모드는 화면 가운데
            {
                const float centerLeft = area.m_left + (area.GetWidth() - size.m_width) * HALF;     // 가로 가운데
                const float centerTop = area.m_top + (area.GetHeight() - size.m_height) * HALF;     // 세로 가운데
                panel->SetBounds(Core::RectF{ centerLeft, centerTop, centerLeft + size.m_width, centerTop + size.m_height });
                continue;                                                      // 오른쪽 쌓기에서 뺌
            }
            if (panel->GetId() == BossPanel::PANEL_ID)                         // 보스 체력은 게임 체력바 바로 아래 가운데
            {
                const float bossLeft = area.m_left + (area.GetWidth() - size.m_width) * HALF;   // 가로 가운데
                const float bossTop = area.m_top + area.GetHeight() * BOSS_PANEL_TOP_RATIO;     // 체력바 아래
                panel->SetBounds(Core::RectF{ bossLeft, bossTop, bossLeft + size.m_width, bossTop + size.m_height });
                continue;                                                      // 오른쪽 쌓기에서 뺌
            }
            if (panel->GetId() == DamagePanel::PANEL_ID)                       // 딜 지분은 오른쪽 아래 (게임 전투 분석기 바로 위)
            {
                const float left = area.m_right - PANEL_MARGIN - size.m_width; // 오른쪽 정렬
                const float bottom = area.m_top + area.GetHeight() * DAMAGE_PANEL_BOTTOM_RATIO;   // 전투 분석기 위
                panel->SetBounds(Core::RectF{ left, bottom - size.m_height, left + size.m_width, bottom });
                continue;                                                      // 오른쪽 쌓기에서 뺌
            }
            if (panel->GetId() == CharacterPanel::PANEL_ID)                    // 현재 캐릭터는 왼쪽 위
            {
                const float left = area.m_left + PANEL_MARGIN;                 // 왼쪽 여백
                const float characterTop = area.m_top + CHARACTER_PANEL_TOP;   // 아이콘 줄 아래
                panel->SetBounds(Core::RectF{ left, characterTop, left + size.m_width, characterTop + size.m_height });
                continue;                                                      // 오른쪽 쌓기에서 뺌
            }
            const float left = area.m_right - PANEL_MARGIN - size.m_width;     // 오른쪽 정렬
            panel->SetBounds(Core::RectF{ left, top, left + size.m_width, top + size.m_height });   // 위치
            top += size.m_height + PANEL_GAP;                                  // 다음 패널 아래로
        }
        m_isPlaced = true;                                                     // 배치함
        SavePanelStates();                                                     // 저장
    }

    void OverlayManager::SavePanelStates()
    {
        for (const auto& panel : m_panels)                                     // 패널마다
        {
            const Core::RectF& bounds = panel->GetBounds();                    // 위치 · 크기
            m_settings->SetPanel(OverlayPanelState{ panel->GetId(), panel->IsVisible(), bounds.m_left, bounds.m_top,
                bounds.GetWidth(), bounds.GetHeight(), panel->GetAlpha() });   // 상태
        }
        SaveSettings();                                                        // 파일 저장
    }

    void OverlayManager::SaveSettings()
    {
        try
        {
            m_settings->Save();                                                // 저장
        }
        catch (const std::exception&)
        {
            m_pendingNotice = std::wstring{ SAVE_FAILED_TEXT };                // 메인 창 안내
        }
        m_isDirty = true;                                                      // 다시 그리기 (불투명도 등)
    }

    void OverlayManager::RenderToolbar(Core::Renderer& renderer)
    {
        const Core::RectF area = GetOverlayArea();                             // 오버레이 영역
        float totalWidth = 0.0f;                                               // 단추 전체 가로
        std::vector<float> widths;                                             // 단추별 가로
        for (const ToolbarButton& button : m_toolbar)                          // 단추마다
        {
            const float width = renderer.MeasureString(button.m_text, TOOLBAR_STYLE, area.GetWidth()).m_width + TOOLBAR_BUTTON_PADDING * BOTH_SIDES;   // 글자 + 여백
            widths.push_back(width);                                           // 기록
            totalWidth += width + TOOLBAR_GAP;                                 // 누적
        }
        float left = (area.GetWidth() - (totalWidth - TOOLBAR_GAP)) * HALF;    // 가운데 정렬
        for (std::size_t index = 0; index < m_toolbar.size(); ++index)         // 단추마다
        {
            ToolbarButton& button = m_toolbar[index];                          // 단추
            button.m_bounds = Core::RectF{ left, TOOLBAR_TOP, left + widths[index], TOOLBAR_TOP + TOOLBAR_HEIGHT };   // 위치
            const bool isActive = button.m_isActive && button.m_isActive();    // 켜진 상태
            renderer.FillRoundedRect(button.m_bounds, TOOLBAR_RADIUS, Core::WithAlpha(Theme::PANEL_BACK_COLOR, TOOLBAR_BACK_ALPHA));   // 바탕
            if (isActive) renderer.FillRoundedRect(button.m_bounds, TOOLBAR_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, ACTIVE_BACK_ALPHA));   // 켜짐 표시
            renderer.DrawRoundedRect(button.m_bounds, TOOLBAR_RADIUS, Theme::ACCENT_COLOR, BORDER_WIDTH);   // 테두리
            renderer.DrawString(button.m_text, button.m_bounds, Theme::TEXT_COLOR, TOOLBAR_STYLE);          // 글자
            left += widths[index] + TOOLBAR_GAP;                               // 다음 단추
        }
        const float hintTop = TOOLBAR_TOP + TOOLBAR_HEIGHT + TOOLBAR_GAP;      // 안내 줄 위쪽
        const Core::RectF hint{ area.m_left, hintTop, area.m_right, hintTop + TOOLBAR_HINT_HEIGHT };   // 안내 줄
        renderer.DrawString(TOOLBAR_HINT_TEXT, hint, Theme::ACCENT_COLOR, HINT_STYLE);            // 안내
    }

    bool OverlayManager::HandleToolbarInput(const Core::InputEvent& event)
    {
        const auto* button = std::get_if<Core::MouseButtonEvent>(&event);      // 마우스 버튼만
        if (button == nullptr || button->m_button != Core::MouseButton::MOUSE_BUTTON_LEFT) return false;   // 왼쪽 버튼만
        for (ToolbarButton& item : m_toolbar)                                  // 단추마다
        {
            if (!item.m_bounds.Contains(button->m_x, button->m_y)) continue;   // 다른 단추
            if (!button->m_isPressed && item.m_onClick) item.m_onClick();      // 뗄 때 실행
            return true;                                                       // 단추 위 입력은 삼킴
        }
        return false;                                                          // 단추 밖
    }
}
