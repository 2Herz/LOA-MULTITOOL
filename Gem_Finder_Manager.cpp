module LostArkMultiTool.Views.GemFinderManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.GemDataRepository;
import LostArkMultiTool.Core.GemPriceService;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemQuote;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Models.RefreshSettings;
import LostArkMultiTool.Services.AuctionGemPriceService;
import LostArkMultiTool.Services.JSONGemDataRepository;
import LostArkMultiTool.Services.RefreshScheduler;
import LostArkMultiTool.Views.GemFinderView;
import LostArkMultiTool.Views.UI.LineChart;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"보석 최저가 검색기" };
        constexpr int AMOUNT_DECIMALS{ 0 };

        // ===== 문구 =====
        constexpr std::wstring_view NOT_UPDATED_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view FULL_REFRESHING_TEXT{ L"전체 시세 조회 중..." };
        constexpr std::wstring_view WATCH_REFRESHING_TEXT{ L"감시 보석 조회 중..." };
        constexpr std::wstring_view FULL_NOT_YET_TEXT{ L"전체 -" };
        constexpr std::wstring_view AUTO_OFF_TEXT{ L"자동 갱신 꺼짐" };
        constexpr std::wstring_view PAUSE_RATE_LIMIT_TEXT{ L"호출 한도 초과" };
        constexpr std::wstring_view PAUSE_MAINTENANCE_TEXT{ L"API 서버 점검 중" };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"보석: 알림 보석은 {}초, 전체는 {}분마다 확인해요." };
        constexpr std::wstring_view FULL_STATUS_FORMAT{ L"전체 {}" };
        constexpr std::wstring_view WATCH_STATUS_FORMAT{ L" · 감시 {}" };
        constexpr std::wstring_view PAUSED_FORMAT{ L"⏸ {} ({} 이후 재시도)" };
        constexpr std::wstring_view RATE_PROTECT_FORMAT{ L"⏸ 호출 한도 보호 중 ({} 이후 재개)" };
        constexpr std::wstring_view REMAINING_CALLS_FORMAT{ L"남은 호출 {}/{} · " };
        constexpr std::wstring_view ESTIMATE_FORMAT{ L"예상 분당 약 {}회" };
        constexpr std::wstring_view AUTO_ERROR_FORMAT{ L"{}: {}" };

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };   // 손상 복구 · 이전 위치 기록 가져오기
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"저장에 실패했어요.\n" };
        constexpr std::wstring_view PRICE_SAVE_ERROR_TEXT{ L"시세는 갱신됐지만 파일 저장에 실패했어요.\n" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TITLE{ L"시세 자동 갱신 실패" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 잠시 쉬고 있어요.\n{} 이후 다시 시도해 주세요." };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view EMPTY_TARGET_TEXT{ L"목표가를 입력해 주세요." };
        constexpr std::wstring_view NO_ALARM_SELECTED_TEXT{ L"삭제할 알림을 목록에서 먼저 선택해 주세요." };
        constexpr std::wstring_view REMOVE_ALARM_TITLE{ L"알림 삭제" };
        constexpr std::wstring_view REMOVE_ALARM_FORMAT{ L"'{}' 알림 (목표가 {})을 삭제할까요?" };
        constexpr std::wstring_view REACHED_TITLE{ L"💎 목표가 도달" };
        constexpr std::wstring_view REACHED_LINE_FORMAT{ L"{}: {} 골드 (목표 {})" };
        constexpr std::wstring_view HIDDEN_ALARMS_FORMAT{ L"외 {}개 (창을 열어 확인)" };
        constexpr std::wstring_view CHART_TITLE_FORMAT{ L"{} 최저가 추이" };
        constexpr std::wstring_view CHART_EMPTY_FORMAT{ L"{} — 아직 기록이 없어요" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr wchar_t LINE_BREAK{ L'\n' };

        // ===== 날짜 키 "yyyy-MM-dd" → 그래프 글자 "MM/dd" =====
        constexpr std::size_t DATE_KEY_LENGTH{ 10 };
        constexpr std::size_t DATE_KEY_MONTH_POSITION{ 5 };
        constexpr std::size_t DATE_KEY_DAY_POSITION{ 8 };
        constexpr std::size_t DATE_PART_LENGTH{ 2 };
        constexpr wchar_t CHART_DATE_SEPARATOR{ L'/' };

        // 예외 메시지(UTF-8) → 화면 문구 (변환할 수 없으면 기본 문구)
        [[nodiscard]] std::wstring ToWideMessage(const char* message)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(message);
            }
            catch (const std::exception&)
            {
                return std::wstring{ UNKNOWN_ERROR_TEXT };
            }
        }

        [[nodiscard]] std::wstring FormatAmount(std::int64_t value)
        {
            return Core::NumberFormat::FormatNumber(static_cast<double>(value), AMOUNT_DECIMALS);
        }

        [[nodiscard]] std::wstring ToDisplayName(const Models::GemSelection& gem)
        {
            return Core::TextEncoding::UTF8ToWide(Models::GemCatalog::MakeDisplayName(gem));
        }

        // 저장 키 "yyyy-MM-dd" → 그래프 표시 "MM/dd" (형식이 다르면 그대로)
        [[nodiscard]] std::wstring ToChartDateLabel(const std::string& dateKey)
        {
            const std::wstring wideKey{ Core::TextEncoding::UTF8ToWide(dateKey) };
            if (wideKey.size() != DATE_KEY_LENGTH) return wideKey;

            std::wstring label{ wideKey.substr(DATE_KEY_MONTH_POSITION, DATE_PART_LENGTH) };
            label.push_back(CHART_DATE_SEPARATOR);
            label += wideKey.substr(DATE_KEY_DAY_POSITION, DATE_PART_LENGTH);
            return label;
        }

        [[nodiscard]] std::wstring_view GetPauseText(Services::PauseReason reason) noexcept
        {
            return reason == Services::PauseReason::PAUSE_MAINTENANCE ? PAUSE_MAINTENANCE_TEXT : PAUSE_RATE_LIMIT_TEXT;
        }
    }

    GemFinderManager::GemFinderManager() = default;

    GemFinderManager::~GemFinderManager() = default;

    std::wstring_view GemFinderManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void GemFinderManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 저장 파일(GemFinderData.json) → 시세 · 알림 · 기록
        m_repository = std::make_unique<Services::JSONGemDataRepository>();
        m_service = std::make_unique<Services::AuctionGemPriceService>(*m_repository);

        // 화면: 저장된 주기로 채우고 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<GemFinderView>(context.m_platform);
        const Models::RefreshSettings settings{ m_service->GetSettings() };
        m_view->SetIntervals(settings.GetWatchIntervalSeconds(), settings.GetFullIntervalMinutes());

        m_view->SetOnRefresh([this] { RequestRefresh(Services::RefreshMode::REFRESH_FULL, false); });   // 수동 갱신은 항상 전체
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled) { SetAutoRefresh(isEnabled, true); });
        m_view->SetOnIntervalChanged([this] { ChangeIntervals(); });
        m_view->SetOnPriceCellClicked([this](const Models::GemSelection& gem) { SelectGem(gem); });
        m_view->SetOnAlarmSelected([this](std::size_t alarmIndex) { SelectAlarm(alarmIndex); });
        m_view->SetOnAddAlarm([this] { AddAlarm(); });
        m_view->SetOnRemoveAlarm([this] { RemoveAlarm(); });
        m_view->SetOnChartGemChanged([this] { RefreshChart(); });

        // 저장 파일이 손상돼 복구했으면 알림 (원본 MainForm_Load)
        if (const std::optional<std::string>& warning{ m_repository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }

        // 지난번에 자동 갱신을 켜 둔 채 종료했으면 첫 프레임에 다시 켬 (실행 직후 서버 확인 · 키 입력보다 요청이 앞서지 않게)
        m_isAutoRestorePending = settings.IsAutoRefreshEnabled();
        RefreshView();
    }

    void GemFinderManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoTimer.Stop();
        m_refreshTask.Wait();

        m_view.reset();
        m_service.reset();
        m_repository.reset();
        m_context = nullptr;
    }

    void GemFinderManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void GemFinderManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void GemFinderManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);

        if (m_isAutoRestorePending)
        {
            m_isAutoRestorePending = false;
            SetAutoRefresh(true, false);   // 켜 둔 설정을 되살리는 것이라 다시 저장하지 않음
        }

        // 조회가 끝났으면 결과 반영, 감시 주기가 되면 스케줄러가 전체 / 감시를 결정 (다른 탭 · 트레이에 숨어 있어도 진행)
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_autoTimer.Poll())
        {
            const Services::RefreshMode mode{ m_scheduler.OnTick(m_view->GetWatchIntervalSeconds(), m_view->GetFullIntervalMinutes()) };
            RequestRefresh(mode, true);
        }
    }

    void GemFinderManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool GemFinderManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape GemFinderManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> GemFinderManager::GetBackgroundStatus() const
    {
        if (!m_view || !m_view->IsAutoRefreshChecked()) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, m_view->GetWatchIntervalSeconds(), m_view->GetFullIntervalMinutes());
    }

    // 키를 바꿨고 자동 갱신 설정이 켜져 있었다면 다시 시작 (원본 ChangeApiKey 뒷부분)
    void GemFinderManager::OnAPIKeyChanged()
    {
        if (!m_view || m_view->IsAutoRefreshChecked() || !m_service->GetSettings().IsAutoRefreshEnabled()) return;
        SetAutoRefresh(true, false);
    }

    void GemFinderManager::RequestRefresh(Services::RefreshMode requestedMode, bool isAuto)
    {
        // 이미 조회 중이면: 전체 갱신은 끝난 뒤 바로 실행하도록 예약, 감시 갱신은 건너뜀
        if (m_refreshTask.IsRunning())
        {
            if (requestedMode == Services::RefreshMode::REFRESH_FULL)
            {
                // 수동 요청이 하나라도 섞이면 수동으로 취급 (자동 갱신을 꺼도 실행)
                m_isPendingFullAuto = m_isFullRefreshPending ? (m_isPendingFullAuto && isAuto) : isAuto;
                m_isFullRefreshPending = true;
            }
            return;
        }

        // 실행 여부 · 실제 갱신 종류는 스케줄러가 판단
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        const std::vector<Models::GemSelection> allTargets{ m_service->GetAllTargets() };
        const std::vector<Models::GemSelection> watchedTargets{ m_service->GetWatchedTargets() };
        const Services::RefreshPlan plan{ m_scheduler.Plan(requestedMode, isAuto, allTargets.size(), watchedTargets.size(), rate, now) };

        if (plan.m_decision != Services::RefreshDecision::DECISION_RUN)
        {
            UpdateRateLimitText();
            if (!isAuto && plan.m_decision == Services::RefreshDecision::DECISION_SKIP_RATE_LIMIT)
            {
                const std::optional<std::int64_t> resetAt{ rate.GetResetAtLocal() };
                m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE },
                    std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(resetAt.value_or(now))), Core::NoticeKind::NOTICE_INFO, nullptr);
            }
            return;
        }

        // 전체가 부족하면 감시로 바뀌어 있을 수 있음
        m_runningMode = plan.m_mode;
        m_isRunningAuto = isAuto;
        m_scheduler.MarkStarted(plan.m_mode);

        const bool isFull{ plan.m_mode == Services::RefreshMode::REFRESH_FULL };
        m_view->SetRefreshEnabled(false);
        m_view->SetStatusText(std::wstring{ isFull ? FULL_REFRESHING_TEXT : WATCH_REFRESHING_TEXT });

        const Core::GemPriceService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([service, marketAPI, targets = isFull ? allTargets : watchedTargets]
        {
            return service->FetchPrices(*marketAPI, targets);
        });
    }

    // 자동 갱신 켜면 바로 전체 한 번 + 감시 주기마다 (원본 ChkAutoRefresh_CheckedChanged)
    void GemFinderManager::SetAutoRefresh(bool isEnabled, bool isPersisted)
    {
        m_view->SetAutoRefreshChecked(isEnabled);
        if (isPersisted) SaveSettings();   // 켜 둔 상태를 기억 (코드로 바꾼 경우는 저장 안 함)

        if (!isEnabled)
        {
            m_autoTimer.Stop();
            if (m_isPendingFullAuto) m_isFullRefreshPending = false;   // 자동으로 예약된 전체 갱신은 취소 (수동 요청은 유지)
            UpdateRateLimitText();
            return;
        }

        m_lastAutoError.reset();
        m_scheduler.Restart();
        m_autoTimer.Start(m_view->GetWatchIntervalSeconds() * Core::APPConstants::MS_PER_SECOND);
        UpdateRateLimitText();
        RequestRefresh(Services::RefreshMode::REFRESH_FULL, true);   // 켜자마자 전체 한 번
    }

    // 주기를 바꾸면 타이머에 바로 적용하고 설정 저장
    void GemFinderManager::ChangeIntervals()
    {
        if (m_autoTimer.IsRunning()) m_autoTimer.Start(m_view->GetWatchIntervalSeconds() * Core::APPConstants::MS_PER_SECOND);
        UpdateRateLimitText();
        SaveSettings();
    }

    void GemFinderManager::SaveSettings()
    {
        Models::RefreshSettings settings{ m_service->GetSettings() };
        settings.SetWatchIntervalSeconds(m_view->GetWatchIntervalSeconds());
        settings.SetFullIntervalMinutes(m_view->GetFullIntervalMinutes());
        settings.SetAutoRefreshEnabled(m_view->IsAutoRefreshChecked());

        try
        {
            m_service->UpdateSettings(settings);
        }
        catch (const std::exception& error)
        {
            ShowSaveError(error);
        }
    }

    // 조회가 끝남 → 시세 반영 · 알림 · 오류 처리 (원본 RefreshPricesAsync 뒷부분)
    void GemFinderManager::FinishRefresh()
    {
        m_view->SetRefreshEnabled(true);
        const bool isAuto{ m_isRunningAuto };
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        std::optional<std::string> keyError;

        try
        {
            const std::vector<Core::FetchedPrice> prices{ m_refreshTask.TakeResult() };   // 백그라운드에서 난 예외는 여기서 다시 던져짐
            const Core::RefreshResult result{ m_service->ApplyPrices(prices, m_runningMode == Services::RefreshMode::REFRESH_FULL) };

            NotifyAlarms(result.m_reachedAlarms);   // 저장 실패와 상관없이 알림은 항상 표시
            m_scheduler.ClearPause();               // 조회 성공 → 일시정지 해제

            if (result.m_saveError.has_value())
            {
                // 시세는 정상 반영됨 → "갱신 실패"가 아니라 "저장 실패"로 안내
                const std::wstring message{ std::wstring{ PRICE_SAVE_ERROR_TEXT } + ToWideMessage(result.m_saveError->c_str()) };
                if (isAuto) ShowAutoError(SAVE_ERROR_TITLE, message);
                else m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, message, Core::NoticeKind::NOTICE_WARNING, nullptr);
            }
            else
            {
                m_lastAutoError.reset();   // 완전히 성공하면 오류 표시 기록 초기화
            }
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            SetAutoRefresh(false, false);   // 잘못된 키로 계속 재시도하지 않도록 중지 (켜 둔 설정은 유지 → 키를 바꾸면 다시 켜짐)
            keyError = error.what();
        }
        catch (const Core::APIRateLimitException& error)
        {
            m_scheduler.PauseForRateLimit(m_context->m_marketAPI.GetRateLimit(), now);
            if (!isAuto) m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, ToWideMessage(error.what()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }
        catch (const Core::APIMaintenanceException& error)
        {
            m_scheduler.PauseForMaintenance(now);
            if (isAuto) ShowAutoError(MAINTENANCE_TITLE, ToWideMessage(error.what()));
            else m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, ToWideMessage(error.what()), Core::NoticeKind::NOTICE_INFO, nullptr);
        }
        catch (const std::exception& error)
        {
            if (isAuto) ShowAutoError(AUTO_REFRESH_FAILED_TITLE, ToWideMessage(error.what()));
            else m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, ToWideMessage(error.what()), Core::NoticeKind::NOTICE_ERROR, nullptr);
        }

        RefreshView();

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value())
        {
            m_isFullRefreshPending = false;
            HandleAPIKeyError(*keyError);
            return;
        }

        // 예약된 전체 갱신이 있으면 바로 실행
        if (!m_isFullRefreshPending) return;

        const bool pendingIsAuto{ m_isPendingFullAuto };
        m_isFullRefreshPending = false;
        m_isPendingFullAuto = true;
        if (!pendingIsAuto || m_view->IsAutoRefreshChecked()) RequestRefresh(Services::RefreshMode::REFRESH_FULL, pendingIsAuto);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (원본 PromptApiKey)
    void GemFinderManager::HandleAPIKeyError(const std::string& message)
    {
        const std::wstring reason{ ToWideMessage(message.c_str()) };
        if (!m_context->m_platform.IsWindowShown())
        {
            m_context->m_notifier.ShowToast(reason, Core::NoticeKind::NOTICE_ERROR);
            return;
        }

        m_context->m_notifier.AskYesNo(std::wstring{ API_KEY_ERROR_TITLE }, reason + std::wstring{ API_KEY_RETRY_TEXT },
            [openAPIKeyDialog = m_context->m_openAPIKeyDialog](bool isYes)
        {
            if (isYes && openAPIKeyDialog) openAPIKeyDialog();
        });
    }

    // 최저가 표의 칸을 누르면 해당 보석을 그래프와 알림 입력칸에 바로 선택 + 현재가를 목표가에 미리 채움
    void GemFinderManager::SelectGem(const Models::GemSelection& gem)
    {
        m_view->SelectChartGem(gem);
        m_view->SelectAlarmGem(gem);
        RefreshChart();

        const auto quote{ m_service->GetQuotes().find(Models::GemCatalog::MakeKey(gem)) };
        if (quote != m_service->GetQuotes().end() && quote->second.GetPrice().has_value()) m_view->SetTargetPrice(*quote->second.GetPrice());
    }

    // 알림 목록의 선택이 바뀔 때마다 (마우스 · 키보드 모두) 삭제 대상 · 입력칸 · 그래프를 같은 보석으로
    void GemFinderManager::SelectAlarm(std::size_t alarmIndex)
    {
        const std::vector<Models::PriceAlarm>& alarms{ m_service->GetAlarms() };
        if (alarmIndex >= alarms.size()) return;

        const Models::PriceAlarm& alarm{ alarms[alarmIndex] };
        m_selectedAlarmKey = alarm.GetGemKey();
        m_view->SelectAlarmGem(alarm.GetGem());
        m_view->SelectChartGem(alarm.GetGem());
        m_view->SetTargetPrice(alarm.GetTargetPrice());
        RefreshChart();
    }

    // 알림 추가 / 목표가 변경 (같은 보석이면 목표가만 변경)
    void GemFinderManager::AddAlarm()
    {
        const std::optional<Models::GemSelection> gem{ m_view->GetAlarmGem() };
        if (!gem.has_value()) return;

        const std::int64_t targetPrice{ m_view->GetTargetPrice() };
        if (targetPrice <= 0)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_TARGET_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        try
        {
            m_service->SetAlarm(*gem, targetPrice);
            m_selectedAlarmKey = Models::GemCatalog::MakeKey(*gem);   // 방금 추가한 알림을 선택 상태로
        }
        catch (const std::exception& error)
        {
            ShowSaveError(error);   // 저장 실패 시 서비스가 원래대로 되돌림
        }

        m_view->SetTargetPrice(targetPrice);   // 입력한 값을 콤마 형식으로 다시 표시
        RefreshAlarmList();
        UpdateRateLimitText();   // 알림 수가 바뀌면 예상 호출량도 바뀜
    }

    // 선택한 알림만, 확인을 받은 뒤 삭제
    void GemFinderManager::RemoveAlarm()
    {
        const std::vector<Models::PriceAlarm>& alarms{ m_service->GetAlarms() };
        const auto selected{ std::find_if(alarms.begin(), alarms.end(), [this](const Models::PriceAlarm& alarm)
        {
            return m_selectedAlarmKey.has_value() && alarm.GetGemKey() == *m_selectedAlarmKey;
        }) };

        if (selected == alarms.end())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ NO_ALARM_SELECTED_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        const std::wstring question{ std::format(REMOVE_ALARM_FORMAT, ToDisplayName(selected->GetGem()), FormatAmount(selected->GetTargetPrice())) };
        m_context->m_notifier.AskYesNo(std::wstring{ REMOVE_ALARM_TITLE }, question, [this, gemKey = selected->GetGemKey()](bool isYes)
        {
            if (!isYes || !m_service) return;   // 답하기 전에 프로그램이 닫혔으면 무시

            try
            {
                m_service->RemoveAlarm(gemKey);
                m_selectedAlarmKey.reset();
            }
            catch (const std::exception& error)
            {
                ShowSaveError(error);   // 저장 실패 시 서비스가 원래 위치에 되돌림
            }

            RefreshAlarmList();
            UpdateRateLimitText();
        });
    }

    void GemFinderManager::RefreshView()
    {
        if (!m_view) return;

        RefreshPriceGrid();
        RefreshAlarmList();
        RefreshChart();
        UpdateStatusText();
        UpdateRateLimitText();
    }

    void GemFinderManager::RefreshPriceGrid()
    {
        for (const auto& entry : m_service->GetQuotes()) m_view->ShowQuote(entry.second);
    }

    // 알림 목록: 상태는 위에서부터 순서대로 판정 (범위 밖 → 확인 전 → 도달 → 대기, 원본 FillAlarmRow)
    void GemFinderManager::RefreshAlarmList()
    {
        const std::vector<Models::PriceAlarm>& alarms{ m_service->GetAlarms() };
        const std::map<std::string, Models::GemQuote>& quotes{ m_service->GetQuotes() };

        std::vector<AlarmRow> rows;
        rows.reserve(alarms.size());
        std::optional<std::size_t> selectedIndex;
        for (const Models::PriceAlarm& alarm : alarms)
        {
            const std::string gemKey{ alarm.GetGemKey() };
            const auto quote{ quotes.find(gemKey) };
            const bool isInRange{ quote != quotes.end() };
            const bool isQueried{ isInRange && quote->second.IsQueried() };   // 지난 실행 상태를 그대로 보여 주지 않음

            AlarmState state{ AlarmState::ALARM_WAITING };
            if (!isInRange) state = AlarmState::ALARM_OUT_OF_RANGE;
            else if (!isQueried) state = AlarmState::ALARM_NOT_CHECKED;
            else if (alarm.IsTriggered()) state = AlarmState::ALARM_REACHED;

            if (m_selectedAlarmKey == gemKey) selectedIndex = rows.size();
            rows.push_back(AlarmRow{ ToDisplayName(alarm.GetGem()), alarm.GetTargetPrice(),
                isInRange ? quote->second.GetPrice() : std::nullopt, state });
        }

        if (!selectedIndex.has_value()) m_selectedAlarmKey.reset();   // 목록에서 사라진 알림이면 선택 해제
        m_view->ShowAlarms(rows);
        m_view->SelectAlarm(selectedIndex);
    }

    // 선택한 보석의 일별 최저가 그래프
    void GemFinderManager::RefreshChart()
    {
        const std::optional<Models::GemSelection> gem{ m_view->GetChartGem() };
        if (!gem.has_value())
        {
            m_view->SetChart(std::wstring{}, {});
            return;
        }

        const std::map<std::string, std::int64_t>& history{ m_service->GetHistory(Models::GemCatalog::MakeKey(*gem)) };
        std::vector<UI::ChartPoint> points;
        points.reserve(history.size());
        for (const auto& [dateKey, price] : history) points.push_back(UI::ChartPoint{ ToChartDateLabel(dateKey), static_cast<double>(price) });

        const std::wstring name{ ToDisplayName(*gem) };
        m_view->SetChart(history.empty() ? std::format(CHART_EMPTY_FORMAT, name) : std::format(CHART_TITLE_FORMAT, name), std::move(points));
    }

    // 전체 · 감시 갱신 시각을 따로 표시
    void GemFinderManager::UpdateStatusText()
    {
        const std::optional<std::int64_t>& lastFull{ m_service->GetLastFullUpdated() };
        const std::optional<std::int64_t>& lastWatch{ m_service->GetLastWatchUpdated() };
        if (!lastFull.has_value() && !lastWatch.has_value())
        {
            m_view->SetStatusText(std::wstring{ NOT_UPDATED_TEXT });
            return;
        }

        std::wstring text{ lastFull.has_value() ? std::format(FULL_STATUS_FORMAT, Core::LocalTime::FormatClock(*lastFull)) : std::wstring{ FULL_NOT_YET_TEXT } };
        if (lastWatch.has_value()) text += std::format(WATCH_STATUS_FORMAT, Core::LocalTime::FormatClock(*lastWatch));
        m_view->SetStatusText(std::move(text));
    }

    // 남은 호출 수 + 예상 호출량 + 일시정지 안내
    void GemFinderManager::UpdateRateLimitText()
    {
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        const bool isAutoEnabled{ m_view->IsAutoRefreshChecked() };

        // 429 / 점검으로 자동 갱신을 쉬는 중
        if (isAutoEnabled && m_scheduler.IsPaused(now) && m_scheduler.GetPausedUntil().has_value())
        {
            m_view->SetRateLimitText(std::format(PAUSED_FORMAT, GetPauseText(m_scheduler.GetPauseReason()),
                Core::LocalTime::FormatClock(*m_scheduler.GetPausedUntil())), RateLimitLevel::RATE_ALERT);
            return;
        }

        // 남은 호출이 안전 여유분 이하
        if (rate.IsExhausted(now))
        {
            m_view->SetRateLimitText(std::format(RATE_PROTECT_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))),
                RateLimitLevel::RATE_ALERT);
            return;
        }

        // 남은 호출 (아직 응답을 받은 적이 없으면 생략)
        std::wstring text;
        if (rate.GetRemaining().has_value())
        {
            text = std::format(REMAINING_CALLS_FORMAT, *rate.GetRemaining(), rate.GetLimit().value_or(Core::APPConstants::RATE_LIMIT_PER_MINUTE));
        }

        const int estimated{ Services::RefreshScheduler::EstimateRequestsPerMinute(m_service->GetAllTargets().size(),
            m_service->GetWatchedTargets().size(), m_view->GetWatchIntervalSeconds(), m_view->GetFullIntervalMinutes()) };
        text += isAutoEnabled ? std::format(ESTIMATE_FORMAT, estimated) : std::wstring{ AUTO_OFF_TEXT };

        // 예상치가 한도에 가까우면 주황색으로 경고
        const bool isNearLimit{ isAutoEnabled && estimated > Core::APPConstants::RATE_LIMIT_PER_MINUTE - Core::APPConstants::RATE_LIMIT_SAFETY_MARGIN };
        m_view->SetRateLimitText(std::move(text), isNearLimit ? RateLimitLevel::RATE_CAUTION : RateLimitLevel::RATE_NORMAL);
    }

    // 새로 도달한 알림을 트레이 풍선으로 한 번에 (게임 중에도 보이게 창 상태와 상관없이, 원본과 같음)
    void GemFinderManager::NotifyAlarms(const std::vector<Core::ReachedAlarm>& reached)
    {
        if (reached.empty()) return;

        // 풍선 글자 수 제한 때문에 앞의 몇 줄만 보여 주고 나머지는 개수로 표시
        std::wstring text;
        const std::size_t shownCount{ std::min(reached.size(), Core::APPConstants::MAX_BALLOON_LINES) };
        for (std::size_t index{}; index < shownCount; ++index)
        {
            const Core::ReachedAlarm& alarm{ reached[index] };
            if (!text.empty()) text.push_back(LINE_BREAK);
            text += std::format(REACHED_LINE_FORMAT, ToWideMessage(alarm.m_displayName.c_str()), FormatAmount(alarm.m_price), FormatAmount(alarm.m_targetPrice));
        }
        if (const std::size_t hiddenCount{ reached.size() - shownCount }; hiddenCount > 0)
        {
            text.push_back(LINE_BREAK);
            text += std::format(HIDDEN_ALARMS_FORMAT, hiddenCount);
        }

        m_context->m_platform.ShowTrayBalloon(REACHED_TITLE, text, Core::NoticeKind::NOTICE_SUCCESS);
    }

    // 자동 갱신 오류는 내용이 바뀌었을 때만 알림 (같은 오류가 반복해서 뜨는 것 방지, 숨어 있으면 풍선 알림)
    void GemFinderManager::ShowAutoError(std::wstring_view title, const std::wstring& message)
    {
        if (m_lastAutoError == message) return;
        m_lastAutoError = message;
        m_context->m_notifier.ShowToast(std::format(AUTO_ERROR_FORMAT, title, message), Core::NoticeKind::NOTICE_ERROR);
    }

    void GemFinderManager::ShowSaveError(const std::exception& error)
    {
        m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
            Core::NoticeKind::NOTICE_ERROR, nullptr);
    }
}