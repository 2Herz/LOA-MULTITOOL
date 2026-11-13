module LostArkMultiTool.Views.RefineBookFinderManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.RefineBookPriceService;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Models.RefineBookQuote;
import LostArkMultiTool.Services.MarketRefineBookPriceService;
import LostArkMultiTool.Views.RefineBookFinderView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"강화책 검색기" };
        constexpr double PERCENT_SCALE{ 100.0 };   // 비율 → 퍼센트

        // ===== 문구 =====
        constexpr std::wstring_view NOT_UPDATED_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view REFRESHING_TEXT{ L"야금술 · 재봉술 시세 조회 중..." };
        constexpr std::wstring_view STATUS_FORMAT{ L"마지막 갱신 {}" };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"강화책: {}분마다 시세를 자동으로 갱신해요." };
        constexpr std::wstring_view AUTO_ERROR_FORMAT{ L"{}: {}" };
        constexpr std::wstring_view REFRESH_DONE_TEXT{ L"강화책 시세를 갱신했어요." };   // 직접 갱신을 눌렀을 때 완료 알림

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TITLE{ L"강화책 시세 자동 갱신 실패" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 시세 갱신은 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };

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

        // 시세 하나 → 표 한 행 (등락 = 직전 갱신 대비, 변동률 = 전일 평균가 대비)
        [[nodiscard]] RefineBookRow MakeRow(const Models::RefineBookQuote& quote)
        {
            const Models::MarketItem& item{ quote.m_item };
            RefineBookRow row{ ToWideMessage(item.m_name.c_str()), item.m_gradeName, item.m_currentMinPrice, std::nullopt,
                item.m_yesterdayAvgPrice, item.m_recentPrice, std::nullopt };

            if (quote.m_previousMinPrice.has_value() && *quote.m_previousMinPrice > 0.0 && item.m_currentMinPrice > 0.0)
            {
                row.m_diff = item.m_currentMinPrice - *quote.m_previousMinPrice;
            }
            if (item.m_yesterdayAvgPrice > 0.0 && item.m_currentMinPrice > 0.0)
            {
                row.m_dayChangeRate = (item.m_currentMinPrice - item.m_yesterdayAvgPrice) / item.m_yesterdayAvgPrice * PERCENT_SCALE;
            }
            return row;
        }
    }

    RefineBookFinderManager::RefineBookFinderManager() = default;

    RefineBookFinderManager::~RefineBookFinderManager() = default;

    std::wstring_view RefineBookFinderManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void RefineBookFinderManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 거래소 야금술 · 재봉술 시세 (실행 중에만 보관)
        m_service = std::make_unique<Services::MarketRefineBookPriceService>();

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<RefineBookFinderView>(context.m_platform);
        m_view->SetOnRefresh([this] { RequestRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled)
        {
            m_isAutoRefreshWanted = isEnabled;
            SetAutoRefresh(isEnabled);
        });

        RefreshTables();
    }

    void RefineBookFinderManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoTimer.Stop();
        m_refreshTask.Wait();

        m_view.reset();
        m_service.reset();
        m_context = nullptr;
    }

    void RefineBookFinderManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void RefineBookFinderManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void RefineBookFinderManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);

        // 조회가 끝났으면 결과 반영, 자동 갱신 주기가 되면 조회 (다른 탭 · 트레이에 숨어 있어도 진행)
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_autoTimer.Poll()) RequestRefresh(true);
    }

    void RefineBookFinderManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool RefineBookFinderManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape RefineBookFinderManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> RefineBookFinderManager::GetBackgroundStatus() const
    {
        if (!m_view || !m_view->IsAutoRefreshChecked()) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES);
    }

    // 키를 바꿨고 사용자가 자동 갱신을 켜 두었다면 다시 시작 (키 오류로 멈췄던 경우)
    void RefineBookFinderManager::OnAPIKeyChanged()
    {
        if (!m_view || m_view->IsAutoRefreshChecked() || !m_isAutoRefreshWanted) return;
        SetAutoRefresh(true);
    }

    void RefineBookFinderManager::RequestRefresh(bool isAuto)
    {
        if (m_refreshTask.IsRunning()) return;   // 이미 조회 중이면 겹치지 않게 건너뜀

        // 남은 호출이 안전 여유분 이하면 이번 회차는 건너뜀 (다른 툴과 키를 함께 쓰므로)
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        if (rate.IsExhausted(now))
        {
            if (!isAuto)
            {
                m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE },
                    std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))), Core::NoticeKind::NOTICE_INFO, nullptr);
            }
            return;
        }

        m_isRunningAuto = isAuto;
        m_view->SetRefreshEnabled(false);
        m_view->SetStatusText(std::wstring{ REFRESHING_TEXT });

        const Core::RefineBookPriceService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([service, marketAPI]
        {
            return service->FetchQuotes(*marketAPI);
        });
    }

    // 자동 갱신 켜면 바로 한 번 + 5분마다 (계산기 · 유각 검색기와 같은 방식)
    void RefineBookFinderManager::SetAutoRefresh(bool isEnabled)
    {
        m_view->SetAutoRefreshChecked(isEnabled);
        if (!isEnabled)
        {
            m_autoTimer.Stop();
            return;
        }

        m_lastAutoError.reset();
        m_autoTimer.Start(Core::APPConstants::AUTO_REFRESH_INTERVAL_MS);
        RequestRefresh(true);
    }

    // 조회가 끝남 → 시세 반영 · 오류 처리
    void RefineBookFinderManager::FinishRefresh()
    {
        m_view->SetRefreshEnabled(true);
        const bool isAuto{ m_isRunningAuto };
        std::optional<std::string> keyError;
        bool isApplied{};   // 시세를 정상으로 받았는지

        try
        {
            m_service->ApplyQuotes(m_refreshTask.TakeResult());   // 백그라운드에서 난 예외는 여기서 다시 던져짐
            m_lastAutoError.reset();
            isApplied = true;
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            if (m_view->IsAutoRefreshChecked()) SetAutoRefresh(false);   // 잘못된 키로 계속 재시도하지 않도록 중지 (사용자 설정은 기억)
            keyError = error.what();
        }
        catch (const Core::APIRateLimitException& error)
        {
            if (isAuto) ShowAutoError(RATE_LIMIT_TITLE, ToWideMessage(error.what()));
            else m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, ToWideMessage(error.what()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }
        catch (const Core::APIMaintenanceException& error)
        {
            if (isAuto) ShowAutoError(MAINTENANCE_TITLE, ToWideMessage(error.what()));
            else m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, ToWideMessage(error.what()), Core::NoticeKind::NOTICE_INFO, nullptr);
        }
        catch (const std::exception& error)
        {
            if (isAuto) ShowAutoError(AUTO_REFRESH_FAILED_TITLE, ToWideMessage(error.what()));
            else m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, ToWideMessage(error.what()), Core::NoticeKind::NOTICE_ERROR, nullptr);
        }

        RefreshTables();
        if (isApplied && !isAuto) m_context->m_notifier.ShowToast(std::wstring{ REFRESH_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);   // 완료 토스트

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value()) HandleAPIKeyError(*keyError);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (다른 검색기와 같음)
    void RefineBookFinderManager::HandleAPIKeyError(const std::string& message)
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

    // 서비스가 정해 둔 순서 그대로 종류별로 나눠 채움
    void RefineBookFinderManager::RefreshTables()
    {
        if (!m_view) return;

        std::vector<RefineBookRow> metallurgyRows;
        std::vector<RefineBookRow> tailoringRows;
        for (const Models::RefineBookQuote& quote : m_service->GetQuotes())
        {
            (quote.m_kind == Models::RefineBookKind::BOOK_METALLURGY ? metallurgyRows : tailoringRows).push_back(MakeRow(quote));
        }
        m_view->ShowMetallurgy(metallurgyRows);
        m_view->ShowTailoring(tailoringRows);

        // 상태: 조회 중이면 그대로, 아니면 종류 수 + 마지막 갱신 시각
        if (m_refreshTask.IsRunning()) return;

        const std::optional<std::int64_t>& lastUpdated{ m_service->GetLastUpdated() };
        if (!lastUpdated.has_value())
        {
            m_view->SetStatusText(std::wstring{ NOT_UPDATED_TEXT });
            return;
        }
        m_view->SetStatusText(std::format(STATUS_FORMAT, Core::LocalTime::FormatClock(*lastUpdated)));
    }

    // 자동 갱신 오류는 내용이 바뀌었을 때만 알림 (같은 오류가 반복해서 뜨는 것 방지, 숨어 있으면 풍선 알림)
    void RefineBookFinderManager::ShowAutoError(std::wstring_view title, const std::wstring& message)
    {
        if (m_lastAutoError == message) return;
        m_lastAutoError = message;
        m_context->m_notifier.ShowToast(std::format(AUTO_ERROR_FORMAT, title, message), Core::NoticeKind::NOTICE_ERROR);
    }
}
