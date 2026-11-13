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
import LostArkMultiTool.Core.PriceHistoryRepository;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.EngravingQuote;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.PriceHistory;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Services.JSONPriceHistoryRepository;
import LostArkMultiTool.Services.MarketEngravingPriceService;
import LostArkMultiTool.Views.EngravingFinderView;
import LostArkMultiTool.Views.PriceHistoryChart;

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
        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };   // 손상된 기록 파일을 백업으로 복구했을 때
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"각인서 시세 기록을 저장하지 못했어요.\n" };
        constexpr std::wstring_view CHART_TITLE_FORMAT{ L"{} 최저가 추이" };               // 그래프 제목 (각인서 이름)
        constexpr std::wstring_view CHART_EMPTY_FORMAT{ L"{} — 아직 기록이 없어요" };      // 고른 각인서에 기록이 없을 때
        constexpr std::wstring_view CHART_HINT_TEXT{ L"표에서 각인서를 고르면 일별 최저가 그래프가 보여요" };   // 아직 아무것도 고르지 않았을 때
        constexpr std::wstring_view SEARCH_IGNORED_CHARACTERS{ L" \t" };   // 검색할 때 무시하는 글자 (띄어쓰기 차이)
        constexpr std::wstring_view REFRESH_DONE_TEXT{ L"유각 시세를 갱신했어요." };       // 수동 갱신이 끝났을 때 토스트
        constexpr std::size_t TREND_DAY_COUNT{ 14 };                                     // 추이 선에 쓰는 최근 기록 일수

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

        // 일별 최저가 기록 → 추이 선 값 (날짜순 마지막 TREND_DAY_COUNT일)
        [[nodiscard]] std::vector<double> MakeTrend(const std::map<std::string, std::int64_t>& days)
        {
            std::vector<double> trend;                                         // 결과
            const std::size_t skip{ days.size() > TREND_DAY_COUNT ? days.size() - TREND_DAY_COUNT : 0 };   // 오래된 날은 건너뜀
            std::size_t index{};                                               // 지금까지 본 날 수
            for (const auto& [dateKey, price] : days)
            {
                if (index++ < skip) continue;
                trend.push_back(static_cast<double>(price));
            }
            return trend;
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

        // 모델: 각인서별 일별 최저가 기록 (EngravingFinderData.json, 손상되면 백업으로 복구하고 안내)
        m_historyRepository = std::make_unique<Services::JSONPriceHistoryRepository>(Services::PriceHistoryFiles::ENGRAVING_FILE_NAME);
        if (const std::optional<std::string>& warning{ m_historyRepository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<EngravingFinderView>(context.m_platform);
        m_view->SetOnRefresh([this] { RequestRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled)
        {
            m_isAutoRefreshWanted = isEnabled;
            SetAutoRefresh(isEnabled);
        });
        m_view->SetOnFilterChanged([this] { RefreshTable(); });
        m_view->SetOnRowSelected([this] { SelectRow(); });   // 표 행을 고르면 그 각인서의 그래프

        RefreshTable();
    }

    void EngravingFinderManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoTimer.Stop();
        m_refreshTask.Wait();

        // 저장하지 못한 기록이 남아 있으면 마지막으로 한 번 더 저장 (종료 중이라 실패해도 알리지 않음)
        if (m_historyRepository && m_isHistoryDirty)
        {
            try
            {
                m_historyRepository->Save();
            }
            catch (const std::exception&)
            {
                // 종료 중에는 알림을 띄울 수 없어 건너뜀
            }
        }

        m_view.reset();
        m_historyRepository.reset();
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
            RecordHistory();                                       // 새 시세로 오늘의 각인서별 최저가 기록
            if (!isAuto) m_context->m_notifier.ShowToast(std::wstring{ REFRESH_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);   // 수동 갱신 완료 피드백
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

    // 전체 시세 → 통계 카드 값 (실제 시세만 사용, 계산할 수 없는 값은 비워 둠)
    EngravingStats EngravingFinderManager::BuildStats(const std::vector<Models::EngravingQuote>& quotes) const
    {
        EngravingStats stats;                                   // 결과
        stats.m_quoteCount = quotes.size();                     // 조회한 종류 수
        double rateSum{};                                       // 변동률 합
        std::size_t rateCount{};                                // 변동률을 계산할 수 있는 종류 수
        for (const Models::EngravingQuote& quote : quotes)
        {
            if (quote.GetCurrentMinPrice() > 0.0) ++stats.m_listedCount;   // 매물이 있는 종류
            if (const std::optional<double> rate{ quote.GetDayChangeRate() }; rate.has_value())
            {
                rateSum += *rate;
                ++rateCount;
            }
            if (quote.GetCurrentMinPrice() > stats.m_topPrice)             // 가장 비싼 각인서
            {
                stats.m_topPrice = quote.GetCurrentMinPrice();
                stats.m_topName = ToWideMessage(quote.GetName().c_str());
            }
        }
        if (rateCount > 0) stats.m_averageRate = rateSum / static_cast<double>(rateCount);   // 평균 변동률

        if (const std::optional<std::int64_t>& lastUpdated{ m_service->GetLastUpdated() }; lastUpdated.has_value())
        {
            stats.m_updatedText = Core::LocalTime::FormatClock(*lastUpdated);   // 마지막 갱신 시각
        }
        return stats;
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
                quote.GetYesterdayAvgPrice(), quote.GetRecentPrice(), quote.GetDayChangeRate(), quote.GetName(),
                MakeTrend(m_historyRepository->GetHistory().GetDays(quote.GetName())) });   // 추이 선 = 일별 최저가 기록
        }

        SortRows(rows, m_view->GetSortOrder());
        m_view->ShowRows(rows, quotes.size());
        m_view->SetStats(BuildStats(quotes));   // 위쪽 통계 카드 (검색과 상관없이 전체 시세 기준)

        // 표 행 순서대로 기록 키를 보관하고, 고른 각인서가 아직 표에 있으면 선택을 유지 (정렬 · 검색으로 순서가 바뀌어도)
        m_rowKeys.clear();
        for (const EngravingRow& row : rows) m_rowKeys.push_back(row.m_key);
        std::optional<std::size_t> selectedRow;   // 표에서 다시 선택할 행
        if (m_selectedKey.has_value())
        {
            if (const auto found{ std::ranges::find(m_rowKeys, *m_selectedKey) }; found != m_rowKeys.end())
            {
                selectedRow = static_cast<std::size_t>(std::distance(m_rowKeys.begin(), found));
            }
        }
        m_view->SelectRow(selectedRow);
        RefreshChart();   // 시세가 바뀌면 그래프도 새 기록으로

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

    // 조회가 성공했을 때: 매물이 있는(현재 최저가 > 0) 각인서마다 오늘(오전 6시 기준) 최저가를 기록 (그날 더 싸면 갱신)
    void EngravingFinderManager::RecordHistory()
    {
        const std::string dayKey{ Core::LocalTime::MakeDateKey(Core::LocalTime::GetGameToday()) };   // 오늘 날짜 키
        Models::PriceHistory& history{ m_historyRepository->GetHistory() };                           // 기록
        bool isChanged{};   // 새로 기록하거나 갱신한 값이 있는지

        for (const Models::EngravingQuote& quote : m_service->GetQuotes())
        {
            if (quote.GetCurrentMinPrice() <= 0.0) continue;   // 매물이 없으면 기록하지 않음
            if (history.RecordLowest(quote.GetName(), dayKey, std::llround(quote.GetCurrentMinPrice()))) isChanged = true;
        }

        // 바뀐 게 있거나 지난번 저장에 실패했으면 저장
        if (isChanged || m_isHistoryDirty) SaveHistory();
    }

    // 기록 저장 (실패하면 알리고, 다음 조회 · 종료 때 다시 시도)
    void EngravingFinderManager::SaveHistory()
    {
        try
        {
            m_historyRepository->Save();   // 임시 파일 → 교체 방식 저장
            m_isHistoryDirty = false;
        }
        catch (const std::exception& error)
        {
            m_isHistoryDirty = true;       // 다음 기회에 다시 저장
            m_context->m_notifier.ShowToast(std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()), Core::NoticeKind::NOTICE_ERROR);
        }
    }

    // 표에서 행을 골랐을 때: 그 행의 각인서가 그래프 대상
    void EngravingFinderManager::SelectRow()
    {
        const std::optional<std::size_t> row{ m_view->GetSelectedRow() };
        if (!row.has_value() || *row >= m_rowKeys.size()) return;   // 선택이 없으면 그대로 둠

        m_selectedKey = m_rowKeys[*row];
        RefreshChart();
    }

    // 고른 각인서의 일별 최저가 그래프 (고르지 않았으면 안내 문구)
    void EngravingFinderManager::RefreshChart()
    {
        if (!m_view) return;
        if (!m_selectedKey.has_value())
        {
            m_view->SetChart(std::wstring{ CHART_HINT_TEXT }, {});
            return;
        }

        const std::map<std::string, std::int64_t>& days{ m_historyRepository->GetHistory().GetDays(*m_selectedKey) };
        const std::wstring name{ ToWideMessage(m_selectedKey->c_str()) };
        m_view->SetChart(days.empty() ? std::format(CHART_EMPTY_FORMAT, name) : std::format(CHART_TITLE_FORMAT, name), BuildPriceChartPoints(days));
    }
}
