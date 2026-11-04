module LostArkMultiTool.Views.EngravingFinderManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.EngravingPriceService;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.EngravingQuote;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Services.MarketEngravingPriceService;
import LostArkMultiTool.Views.EngravingFinderView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"유각 시세 검색기" };

        // ===== 문구 =====
        constexpr std::wstring_view NOT_UPDATED_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view REFRESHING_TEXT{ L"유물 각인서 시세 조회 중..." };
        constexpr std::wstring_view STATUS_FORMAT{ L"{}종 · 마지막 갱신 {}" };
        constexpr std::wstring_view FILTERED_STATUS_FORMAT{ L"검색 결과 {}/{}종 · 마지막 갱신 {}" };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"유각: {}분마다 시세를 자동으로 갱신해요." };
        constexpr std::wstring_view AUTO_ERROR_FORMAT{ L"{}: {}" };

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TITLE{ L"유각 시세 자동 갱신 실패" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 시세 갱신은 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr std::wstring_view SEARCH_IGNORED_CHARACTERS{ L" \t" };   // 검색할 때 무시하는 글자 (띄어쓰기 차이)

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

        // 띄어쓰기를 뺀 글자 ("아드 레날린" → "아드레날린", 검색어 · 이름 비교용)
        [[nodiscard]] std::wstring RemoveSpaces(std::wstring_view text)
        {
            std::wstring compact;
            compact.reserve(text.size());
            for (const wchar_t character : text)
            {
                if (!SEARCH_IGNORED_CHARACTERS.contains(character)) compact.push_back(character);
            }
            return compact;
        }

        // 변동률이 없는 행은 정렬 방향과 상관없이 맨 아래로
        [[nodiscard]] bool CompareChange(const EngravingRow& left, const EngravingRow& right, bool isDescending) noexcept
        {
            if (left.m_dayChangeRate.has_value() != right.m_dayChangeRate.has_value()) return left.m_dayChangeRate.has_value();
            if (!left.m_dayChangeRate.has_value()) return false;
            return isDescending ? *left.m_dayChangeRate > *right.m_dayChangeRate : *left.m_dayChangeRate < *right.m_dayChangeRate;
        }

        // 매물이 없는(0) 행은 정렬 방향과 상관없이 맨 아래로
        [[nodiscard]] bool ComparePrice(const EngravingRow& left, const EngravingRow& right, bool isDescending) noexcept
        {
            const bool hasLeft{ left.m_currentMinPrice > 0.0 };
            const bool hasRight{ right.m_currentMinPrice > 0.0 };
            if (hasLeft != hasRight) return hasLeft;
            return isDescending ? left.m_currentMinPrice > right.m_currentMinPrice : left.m_currentMinPrice < right.m_currentMinPrice;
        }

        // 정렬 기준에 맞게 정렬 (같은 값이면 이름 순 → 갱신할 때마다 순서가 흔들리지 않게)
        void SortRows(std::vector<EngravingRow>& rows, EngravingSortOrder order)
        {
            std::ranges::stable_sort(rows, [](const EngravingRow& left, const EngravingRow& right) { return left.m_name < right.m_name; });
            if (order == EngravingSortOrder::SORT_NAME) return;

            std::ranges::stable_sort(rows, [order](const EngravingRow& left, const EngravingRow& right)
            {
                switch (order)
                {
                    case EngravingSortOrder::SORT_PRICE_ASC:   return ComparePrice(left, right, false);
                    case EngravingSortOrder::SORT_CHANGE_DESC: return CompareChange(left, right, true);
                    case EngravingSortOrder::SORT_CHANGE_ASC:  return CompareChange(left, right, false);
                    default:                                   return ComparePrice(left, right, true);
                }
            });
        }
    }

    EngravingFinderManager::EngravingFinderManager() = default;

    EngravingFinderManager::~EngravingFinderManager() = default;

    std::wstring_view EngravingFinderManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void EngravingFinderManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 거래소 유물 각인서 시세 (실행 중에만 보관)
        m_service = std::make_unique<Services::MarketEngravingPriceService>();

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<EngravingFinderView>(context.m_platform);
        m_view->SetOnRefresh([this] { RequestRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled)
        {
            m_isAutoRefreshWanted = isEnabled;
            SetAutoRefresh(isEnabled);
        });
        m_view->SetOnFilterChanged([this] { RefreshTable(); });

        RefreshTable();
    }

    void EngravingFinderManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoTimer.Stop();
        m_refreshTask.Wait();

        m_view.reset();
        m_service.reset();
        m_context = nullptr;
    }

    void EngravingFinderManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void EngravingFinderManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void EngravingFinderManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);

        // 조회가 끝났으면 결과 반영, 자동 갱신 주기가 되면 조회 (다른 탭 · 트레이에 숨어 있어도 진행)
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_autoTimer.Poll()) RequestRefresh(true);
    }

    void EngravingFinderManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool EngravingFinderManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape EngravingFinderManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> EngravingFinderManager::GetBackgroundStatus() const
    {
        if (!m_view || !m_view->IsAutoRefreshChecked()) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES);
    }

    // 키를 바꿨고 사용자가 자동 갱신을 켜 두었다면 다시 시작 (키 오류로 멈췄던 경우)
    void EngravingFinderManager::OnAPIKeyChanged()
    {
        if (!m_view || m_view->IsAutoRefreshChecked() || !m_isAutoRefreshWanted) return;
        SetAutoRefresh(true);
    }

    void EngravingFinderManager::RequestRefresh(bool isAuto)
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

        const Core::EngravingPriceService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([service, marketAPI]
        {
            return service->FetchQuotes(*marketAPI);
        });
    }

    // 자동 갱신 켜면 바로 한 번 + 5분마다 (계산기와 같은 방식)
    void EngravingFinderManager::SetAutoRefresh(bool isEnabled)
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
    void EngravingFinderManager::FinishRefresh()
    {
        m_view->SetRefreshEnabled(true);
        const bool isAuto{ m_isRunningAuto };
        std::optional<std::string> keyError;

        try
        {
            m_service->ApplyQuotes(m_refreshTask.TakeResult());   // 백그라운드에서 난 예외는 여기서 다시 던져짐
            m_lastAutoError.reset();
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

        RefreshTable();

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value()) HandleAPIKeyError(*keyError);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (보석 검색기와 같음)
    void EngravingFinderManager::HandleAPIKeyError(const std::string& message)
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

    // 검색어(띄어쓰기 무시)로 거르고 정렬 기준대로 표 채우기
    void EngravingFinderManager::RefreshTable()
    {
        if (!m_view) return;

        const std::vector<Models::EngravingQuote>& quotes{ m_service->GetQuotes() };
        const std::wstring keyword{ RemoveSpaces(m_view->GetSearchText()) };

        std::vector<EngravingRow> rows;
        rows.reserve(quotes.size());
        for (const Models::EngravingQuote& quote : quotes)
        {
            std::wstring name{ ToWideMessage(quote.GetName().c_str()) };
            if (!keyword.empty() && !RemoveSpaces(name).contains(keyword)) continue;

            rows.push_back(EngravingRow{ std::move(name), quote.GetCurrentMinPrice(), quote.GetDiff(),
                quote.GetYesterdayAvgPrice(), quote.GetRecentPrice(), quote.GetDayChangeRate() });
        }

        SortRows(rows, m_view->GetSortOrder());
        m_view->ShowRows(rows);

        // 상태: 조회 중이면 그대로, 아니면 종류 수 + 마지막 갱신 시각
        if (m_refreshTask.IsRunning()) return;

        const std::optional<std::int64_t>& lastUpdated{ m_service->GetLastUpdated() };
        if (!lastUpdated.has_value())
        {
            m_view->SetStatusText(std::wstring{ NOT_UPDATED_TEXT });
            return;
        }

        const std::wstring clock{ Core::LocalTime::FormatClock(*lastUpdated) };
        m_view->SetStatusText(keyword.empty() ? std::format(STATUS_FORMAT, quotes.size(), clock)
                                              : std::format(FILTERED_STATUS_FORMAT, rows.size(), quotes.size(), clock));
    }

    // 자동 갱신 오류는 내용이 바뀌었을 때만 알림 (같은 오류가 반복해서 뜨는 것 방지, 숨어 있으면 풍선 알림)
    void EngravingFinderManager::ShowAutoError(std::wstring_view title, const std::wstring& message)
    {
        if (m_lastAutoError == message) return;
        m_lastAutoError = message;
        m_context->m_notifier.ShowToast(std::format(AUTO_ERROR_FORMAT, title, message), Core::NoticeKind::NOTICE_ERROR);
    }
}
