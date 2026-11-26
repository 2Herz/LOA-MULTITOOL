module LostArkMultiTool.Views.OverlaySetupManager;  // 오버레이 · 인식 설정 탭 매니저 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Notifier;  // 알림 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.ToolContext;  // 공용 서비스 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 화면 인식 · 현재 캐릭터
import LostArkMultiTool.Views.OverlayManager;  // 오버레이 배치
import LostArkMultiTool.Views.OverlaySetupView;  // 설정 화면

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::wstring_view TAB_TITLE{ L"오버레이 · 인식 설정" };   // 탭 제목
        constexpr double REFRESH_SECONDS{ 0.25 };                         // 상태를 모으는 간격 (보이는 동안)
        constexpr double HIDDEN_REFRESH_SECONDS{ 1.0 };                   // 가려져 있을 때 모으는 간격
        constexpr std::wstring_view EDIT_FAILED_TEXT{ L"게임 창을 찾지 못했거나 오버레이가 꺼져 있어요. 게임을 창 모드나 전체 창 모드로 켜고 오버레이를 켠 뒤 다시 눌러 주세요." };
        constexpr std::wstring_view EDIT_STARTED_TEXT{ L"편집을 시작했어요. 게임 화면에서 패널 머리말을 끌어 옮기고, 끝나면 Ctrl+Shift+I를 눌러 주세요." };
        constexpr std::wstring_view SHAPES_CLEARED_TEXT{ L"기억한 이름표 모양을 모두 지웠어요. 다음에 이름표를 다시 읽어 기억해요." };
        constexpr std::wstring_view ALIASES_CLEARED_TEXT{ L"기억한 읽은 글자를 모두 지웠어요." };
        constexpr std::wstring_view RESTARTED_TEXT{ L"게임 창을 다시 찾고 현재 캐릭터를 다시 확인할게요." };
    }

    OverlaySetupManager::OverlaySetupManager() = default;    // 기본 생성
    OverlaySetupManager::~OverlaySetupManager() = default;   // 기본 소멸

    std::wstring_view OverlaySetupManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;   // 탭 제목
    }

    void OverlaySetupManager::ShowToast(std::wstring message)
    {
        if (m_context) m_context->m_notifier.ShowToast(std::move(message), Core::NoticeKind::NOTICE_INFO);   // 알림 (오버레이가 보이면 게임 위에도)
    }

    void OverlaySetupManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;                                                    // 공용 서비스
        m_view = std::make_unique<OverlaySetupView>(context.m_platform);         // 화면
        OverlaySetupCallbacks callbacks;                                         // 동작 연결
        callbacks.m_onPageChanged = [this](SetupPage) { UpdatePreview(); Refresh(); };   // 쪽이 바뀌면 미리 보기 · 상태
        callbacks.m_onAutoChanged = [this](bool isEnabled) { CalculatorManager::GetInstance().SetCharacterAutoEnabled(isEnabled); Refresh(); };
        callbacks.m_onPickCharacter = [this](std::size_t index) { CalculatorManager::GetInstance().PickCurrentCharacter(index); Refresh(); };
        callbacks.m_onRefreshCharacter = [this] { CalculatorManager::GetInstance().RefreshCurrentCharacter(); Refresh(); };
        callbacks.m_onClearShapes = [this] { CalculatorManager::GetInstance().ClearNameplateShapes(); ShowToast(std::wstring{ SHAPES_CLEARED_TEXT }); Refresh(); };
        callbacks.m_onClearAliases = [this] { CalculatorManager::GetInstance().ClearCharacterAliases(); ShowToast(std::wstring{ ALIASES_CLEARED_TEXT }); Refresh(); };
        callbacks.m_onOverlayEnabled = [this](bool isEnabled) { OverlayManager::GetInstance().SetEnabled(isEnabled); Refresh(); };
        callbacks.m_onEditing = [this](bool isEditing)                           // 편집 · 잠금
            {
                const bool isDone = OverlayManager::GetInstance().SetEditing(isEditing);   // 조작 모드
                if (isEditing) ShowToast(std::wstring{ isDone ? EDIT_STARTED_TEXT : EDIT_FAILED_TEXT });   // 결과 안내
                Refresh();
            };
        callbacks.m_onElementVisible = [this](std::string_view id, bool isVisible) { OverlayManager::GetInstance().SetElementVisible(id, isVisible); Refresh(); };
        callbacks.m_onElementAlpha = [this](std::string_view id, float alpha, bool isCommit) { OverlayManager::GetInstance().SetElementAlpha(id, alpha, isCommit); Refresh(); };
        callbacks.m_onElementReset = [this](std::string_view id) { OverlayManager::GetInstance().ResetElementPosition(id); Refresh(); };
        callbacks.m_onBackOpacity = [this](float opacity, bool isCommit) { OverlayManager::GetInstance().SetBackOpacity(opacity, isCommit); Refresh(); };
        callbacks.m_onVisionEnabled = [this](bool isEnabled) { CalculatorManager::GetInstance().SetVisionEnabled(isEnabled); UpdatePreview(); Refresh(); };
        callbacks.m_onRestartVision = [this] { CalculatorManager::GetInstance().RestartVision(); UpdatePreview(); ShowToast(std::wstring{ RESTARTED_TEXT }); Refresh(); };
        callbacks.m_onIntervalChanged = [this](int seconds) { CalculatorManager::GetInstance().SetCharacterIntervalSeconds(seconds); Refresh(); };
        m_view->SetCallbacks(std::move(callbacks));                              // 연결
    }

    void OverlaySetupManager::Shutdown()
    {
        if (m_isPreviewOn) CalculatorManager::GetInstance().SetVisionPreview(false);   // 미리 보기 끔
        m_isPreviewOn = false;                                                   // 기록
        m_view.reset();                                                          // 화면 정리
        m_context = nullptr;                                                     // 공용 서비스 놓기
    }

    void OverlaySetupManager::OnActivated()
    {
        m_isActive = true;                                                       // 보임
        UpdatePreview();                                                         // 미리 보기
        Refresh();                                                               // 바로 상태
        if (m_view) m_view->RestoreFocus();                                      // 포커스
    }

    void OverlaySetupManager::OnDeactivated()
    {
        m_isActive = false;                                                      // 가려짐
        UpdatePreview();                                                         // 미리 보기 끔
        if (m_view) m_view->SuspendFocus();                                      // 포커스 놓기
    }

    void OverlaySetupManager::UpdatePreview()
    {
        const bool isWanted = m_isActive && m_view && m_view->GetPage() == SetupPage::PAGE_VISION;   // 인식 설정 쪽이 보일 때만
        if (isWanted == m_isPreviewOn) return;                                   // 그대로
        m_isPreviewOn = isWanted;                                                // 기록
        CalculatorManager::GetInstance().SetVisionPreview(isWanted);             // 감시기에 알림
    }

    OverlaySetupSnapshot OverlaySetupManager::BuildSnapshot() const
    {
        CalculatorManager& calculator = CalculatorManager::GetInstance();        // 화면 인식 · 현재 캐릭터
        OverlayManager& overlay = OverlayManager::GetInstance();                 // 오버레이
        OverlaySetupSnapshot snapshot;                                           // 결과
        snapshot.m_character = calculator.GetCurrentCharacter();                 // 현재 캐릭터
        snapshot.m_rosterCount = calculator.GetRosterCount();                    // 원정대 수
        snapshot.m_isOverlayEnabled = overlay.IsEnabled();                       // 오버레이 켜짐
        snapshot.m_isEditing = overlay.IsEditing();                              // 편집 중
        snapshot.m_isGameFound = overlay.IsGameFound();                          // 게임 창
        snapshot.m_backOpacity = overlay.GetBackOpacity();                       // 바탕 진하기
        snapshot.m_elements = overlay.GetElements();                             // 요소들
        snapshot.m_isVisionEnabled = calculator.IsVisionEnabled();               // 화면 인식
        snapshot.m_vision = calculator.GetVisionStatus();                        // 화면 인식 상태
        snapshot.m_characterIntervalSeconds = calculator.GetCharacterIntervalSeconds();   // 확인 간격
        return snapshot;                                                         // 상태
    }

    void OverlaySetupManager::Refresh()
    {
        if (!m_view) return;                                                     // 초기화 전
        m_refreshElapsed = 0.0;                                                  // 초기화
        m_view->Show(BuildSnapshot());                                           // 화면에 넘김
    }

    void OverlaySetupManager::Update(double deltaSeconds)
    {
        if (!m_view) return;                                                     // 초기화 전
        m_view->Update(deltaSeconds);                                            // 부품 갱신
        m_refreshElapsed += deltaSeconds;                                        // 시간 누적
        if (m_refreshElapsed >= (m_isActive ? REFRESH_SECONDS : HIDDEN_REFRESH_SECONDS)) Refresh();   // 주기적으로 상태 모으기
    }

    void OverlaySetupManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);   // 그리기
    }

    bool OverlaySetupManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);   // 입력
    }

    Core::CursorShape OverlaySetupManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;   // 커서
    }
}
