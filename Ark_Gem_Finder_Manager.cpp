module LostArkMultiTool.Views.ArkGemFinderManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.BoundGemRepository;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.PriceHistoryRepository;
import LostArkMultiTool.Core.ArkGemPriceService;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.BoundGemData;
import LostArkMultiTool.Models.BoundGemRosterData;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.PriceHistory;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Models.ArkGemQuote;
import LostArkMultiTool.Services.JSONBoundGemRepository;
import LostArkMultiTool.Services.JSONPriceHistoryRepository;
import LostArkMultiTool.Services.MarketArkGemPriceService;
import LostArkMultiTool.Views.ArkGemFinderView;
import LostArkMultiTool.Views.PriceHistoryChart;
import LostArkMultiTool.Views.UI.LineChart;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"젬 시세 검색기" };
        constexpr double PERCENT_SCALE{ 100.0 };   // 비율 → 퍼센트

        // ===== 문구 =====
        constexpr std::wstring_view NOT_UPDATED_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view REFRESHING_TEXT{ L"질서 · 혼돈의 젬 시세 조회 중..." };
        constexpr std::wstring_view STATUS_FORMAT{ L"마지막 갱신 {}" };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"젬: {}분마다 시세를 자동으로 갱신해요." };
        constexpr std::wstring_view AUTO_ERROR_FORMAT{ L"{}: {}" };
        constexpr std::wstring_view REFRESH_DONE_TEXT{ L"젬 시세를 갱신했어요." };   // 직접 갱신을 눌렀을 때 완료 알림
        constexpr std::size_t TREND_DAYS{ 14 };   // 표 추이 선에 쓰는 최근 기록 일수

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TITLE{ L"젬 시세 자동 갱신 실패" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 시세 갱신은 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"귀속 젬 수량을 저장하지 못했어요.\n" };
        constexpr std::wstring_view PRICE_SAVE_ERROR_TEXT{ L"젬 시세 기록을 저장하지 못했어요.\n" };
        constexpr std::wstring_view PRICE_CHART_TITLE_FORMAT{ L"{} 최저가 추이" };               // 시세 그래프 제목 (젬 이름)
        constexpr std::wstring_view PRICE_CHART_EMPTY_FORMAT{ L"{} — 아직 기록이 없어요" };      // 고른 젬에 기록이 없을 때
        constexpr std::wstring_view PRICE_CHART_HINT_TEXT{ L"젬 표에서 행을 고르면 일별 최저가 그래프가 보여요" };   // 아직 아무것도 고르지 않았을 때
        constexpr std::wstring_view IGNORED_CHARACTERS{ L" \t" };   // 이름을 비교할 때 무시하는 글자

        // ===== 원정대 동기화 · 캐릭터 선택 =====
        constexpr std::size_t ROSTER_ALL_INDEX{ 0 };          // 캐릭터 선택 0번 = ★ 원정대 전체 ★
        constexpr std::size_t FIRST_CHARACTER_INDEX{ 1 };     // 1번부터 캐릭터 (레벨 높은 순)
        constexpr std::wstring_view ROSTER_ALL_TEXT{ L"★ 원정대 전체 ★" };
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };
        constexpr std::wstring_view ROSTER_NOT_FOUND_TEXT{ L"캐릭터 정보를 찾을 수 없어요." };
        constexpr double MIN_GEM_ITEM_LEVEL{ 1700.0 };   // 귀속 젬 계산기에 불러올 최소 아이템 레벨
        constexpr std::wstring_view NO_GEM_CHARACTER_FORMAT{ L"아이템 레벨 {} 이상인 캐릭터가 없어요." };
        constexpr std::wstring_view ROSTER_ERROR_TEXT{ L"오류 발생: " };
        constexpr std::wstring_view ROSTER_DONE_FORMAT{ L"총 {}개의 캐릭터 연동 완료!" };
        constexpr std::wstring_view UNASSIGNED_MOVED_FORMAT{ L"예전에 입력한 귀속 젬 수량을 {}에게 옮겼어요." };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };   // 일반 공백 · 전각 공백

        // ===== 그래프 =====
        constexpr std::wstring_view CHART_TITLE_FORMAT{ L"원정대 귀속 젬 총 가치: {} 골드" };
        constexpr std::wstring_view CHART_EMPTY_TEXT{ L"아직 기록이 없어요 · [시세 갱신] 후 기록돼요" };
        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr std::size_t DATE_KEY_LENGTH{ 10 };   // "yyyy-MM-dd"
        constexpr std::size_t MONTH_OFFSET{ 5 };
        constexpr std::size_t DAY_OFFSET{ 8 };
        constexpr std::size_t TWO_DIGITS{ 2 };

        // 저장 키 "yyyy-MM-dd" → 그래프 글자 "MM/dd" (형식이 다르면 그대로)
        [[nodiscard]] std::wstring ToChartDateLabel(const std::string& dateKey)
        {
            const std::wstring wideKey{ Core::TextEncoding::UTF8ToWide(dateKey) };
            if (wideKey.size() != DATE_KEY_LENGTH) return wideKey;
            return wideKey.substr(MONTH_OFFSET, TWO_DIGITS) + L"/" + wideKey.substr(DAY_OFFSET, TWO_DIGITS);
        }

        [[nodiscard]] std::wstring_view TrimSpaces(std::wstring_view text) noexcept
        {
            const std::size_t first{ text.find_first_not_of(TRIM_CHARACTERS) };
            if (first == std::wstring_view::npos) return {};

            const std::size_t last{ text.find_last_not_of(TRIM_CHARACTERS) };
            return text.substr(first, last - first + 1);
        }

        // 띄어쓰기를 뺀 이름 ("질서의 젬 : 안정" → "질서의젬:안정")
        [[nodiscard]] std::wstring RemoveSpaces(std::wstring_view text)
        {
            std::wstring compact;
            compact.reserve(text.size());
            for (const wchar_t character : text)
            {
                if (!IGNORED_CHARACTERS.contains(character)) compact.push_back(character);
            }
            return compact;
        }

        // 계산기 단가 = 현재 최저가 · 최근 거래가 · 전일 평균가 중 0보다 큰 가장 낮은 값 (모두 없으면 0)
        // 고급 젬처럼 거래가 드문 품목은 한 번의 비싼 거래(예: 99,999)가 최근 거래가 · 전일 평균가로 그대로 남으므로
        // 지금 바로 살 수 있는 값(현재 최저가)을 넘지 않게 가장 낮은 값을 씀
        [[nodiscard]] double GetUnitPrice(const Models::MarketItem& item) noexcept
        {
            double unitPrice{};
            for (const double price : { item.m_currentMinPrice, item.m_recentPrice, item.m_yesterdayAvgPrice })
            {
                if (price > 0.0 && (unitPrice <= 0.0 || price < unitPrice)) unitPrice = price;
            }
            return unitPrice;
        }

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

        // 일별 최저가 기록(날짜 오름차순) → 최근 TREND_DAYS일의 값 (추이 선용)
        [[nodiscard]] std::vector<double> BuildTrend(const std::map<std::string, std::int64_t>& days)
        {
            std::vector<double> trend;
            trend.reserve(days.size());
            for (const auto& [dateKey, price] : days) trend.push_back(static_cast<double>(price));   // 날짜 순서 그대로
            if (trend.size() > TREND_DAYS) trend.erase(trend.begin(), trend.end() - static_cast<std::ptrdiff_t>(TREND_DAYS));   // 최근 것만 남김
            return trend;
        }

        // 시세 하나 → 표 한 행 (등락 = 직전 갱신 대비, 변동률 = 전일 평균가 대비)
        [[nodiscard]] ArkGemRow MakeRow(const Models::ArkGemQuote& quote)
        {
            const Models::MarketItem& item{ quote.m_item };
            ArkGemRow row{ ToWideMessage(item.m_name.c_str()), item.m_gradeName, item.m_currentMinPrice, std::nullopt,
                item.m_yesterdayAvgPrice, item.m_recentPrice, std::nullopt, item.m_name };

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

    ArkGemFinderManager::ArkGemFinderManager() = default;

    ArkGemFinderManager::~ArkGemFinderManager() = default;

    std::wstring_view ArkGemFinderManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void ArkGemFinderManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 거래소 질서 · 혼돈의 젬 시세 (실행 중에만 보관) + 귀속 젬 보유 수량 (BoundGemData.json)
        m_service = std::make_unique<Services::MarketArkGemPriceService>();
        m_boundGemRepository = std::make_unique<Services::JSONBoundGemRepository>();

        // 모델: 젬별 일별 최저가 기록 (ArkGemFinderData.json, 손상되면 백업으로 복구하고 안내)
        m_priceHistoryRepository = std::make_unique<Services::JSONPriceHistoryRepository>(Services::PriceHistoryFiles::ARK_GEM_FILE_NAME);
        if (const std::optional<std::string>& warning{ m_priceHistoryRepository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }
        if (const std::optional<std::string>& warning{ m_boundGemRepository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<ArkGemFinderView>(context.m_platform);
        m_view->SetOnRefresh([this] { RequestRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled)
        {
            m_isAutoRefreshWanted = isEnabled;
            SetAutoRefresh(isEnabled);
        });
        m_view->SetOnQuantityEdited([this](std::size_t gemIndex, std::size_t gradeIndex, const std::wstring& text)
        {
            ChangeQuantity(gemIndex, gradeIndex, text);
        });
        m_view->SetOnSyncRoster([this](const std::wstring& mainCharacterName) { SyncRoster(mainCharacterName); });
        m_view->SetOnCharacterSelected([this] { SelectCharacter(); });
        m_view->SetOnVisionChanged([this](bool isEnabled) { if (m_setVisionEnabled) m_setVisionEnabled(isEnabled); });   // 귀속재료 계산기 [화면 인식]과 같은 설정
        m_view->SetOnPriceRowSelected([this](bool isOrder, std::size_t row) { SelectPriceRow(isOrder, row); });   // 시세 표 행을 고르면 그 젬의 그래프

        LoadCharacterList();
        RefreshTables();
        UpdateChart();
        RefreshPriceChart();
    }

    void ArkGemFinderManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoTimer.Stop();
        m_refreshTask.Wait();
        m_rosterTask.Wait();

        // 저장하지 못한 시세 기록이 남아 있으면 마지막으로 한 번 더 저장 (종료 중이라 실패해도 알리지 않음)
        if (m_priceHistoryRepository && m_isPriceHistoryDirty)
        {
            try
            {
                m_priceHistoryRepository->Save();
            }
            catch (const std::exception&)
            {
                // 종료 중에는 알림을 띄울 수 없어 건너뜀
            }
        }

        m_view.reset();
        m_priceHistoryRepository.reset();
        m_boundGemRepository.reset();
        m_service.reset();
        m_context = nullptr;
    }

    void ArkGemFinderManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void ArkGemFinderManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void ArkGemFinderManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);
        if (m_isVisionEnabled) m_view->SetVisionChecked(m_isVisionEnabled());   // [화면 인식] 표시 맞추기 (계산기에서 바꿔도 같이)

        // 조회가 끝났으면 결과 반영, 자동 갱신 주기가 되면 조회 (다른 탭 · 트레이에 숨어 있어도 진행)
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_rosterTask.IsReady()) FinishRosterSync();
        if (m_autoTimer.Poll()) RequestRefresh(true);
    }

    void ArkGemFinderManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool ArkGemFinderManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape ArkGemFinderManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> ArkGemFinderManager::GetBackgroundStatus() const
    {
        if (!m_view || !m_view->IsAutoRefreshChecked()) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES);
    }

    // 키를 바꿨고 사용자가 자동 갱신을 켜 두었다면 다시 시작 (키 오류로 멈췄던 경우)
    void ArkGemFinderManager::OnAPIKeyChanged()
    {
        if (!m_view || m_view->IsAutoRefreshChecked() || !m_isAutoRefreshWanted) return;
        SetAutoRefresh(true);
    }

    void ArkGemFinderManager::RequestRefresh(bool isAuto)
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

        const Core::ArkGemPriceService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([service, marketAPI]
        {
            return service->FetchQuotes(*marketAPI);
        });
    }

    // 자동 갱신 켜면 바로 한 번 + 5분마다 (계산기 · 유각 검색기와 같은 방식)
    void ArkGemFinderManager::SetAutoRefresh(bool isEnabled)
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
    void ArkGemFinderManager::FinishRefresh()
    {
        m_view->SetRefreshEnabled(true);
        const bool isAuto{ m_isRunningAuto };
        std::optional<std::string> keyError;
        bool isApplied{};

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

        if (isApplied) RecordPriceHistory();   // 새 시세의 젬별 최저가를 오늘 기록으로 (표 추이 선에 오늘 값이 들어가도록 먼저)
        RefreshTables();
        if (isApplied) RecordHistory();        // 새 시세로 계산한 원정대 총 가치를 오늘 기록으로
        if (isApplied && !isAuto) m_context->m_notifier.ShowToast(std::wstring{ REFRESH_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);   // 완료 토스트

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value()) HandleAPIKeyError(*keyError);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (다른 검색기와 같음)
    void ArkGemFinderManager::HandleAPIKeyError(const std::string& message)
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
    void ArkGemFinderManager::RefreshTables()
    {
        if (!m_view) return;

        std::vector<ArkGemRow> orderRows;
        std::vector<ArkGemRow> chaosRows;
        for (const Models::ArkGemQuote& quote : m_service->GetQuotes())
        {
            if (quote.m_item.m_gradeName != Core::APPConstants::EPIC_GRADE_NAME) continue;   // 시세 표는 영웅만 (다른 등급은 계산기 단가로만 사용)
            ArkGemRow row{ MakeRow(quote) };
            row.m_trend = BuildTrend(m_priceHistoryRepository->GetHistory().GetDays(row.m_key));   // 일별 최저가 기록 → 추이 선
            (quote.m_kind == Models::ArkGemKind::GEM_ORDER ? orderRows : chaosRows).push_back(std::move(row));
        }
        m_view->ShowOrder(orderRows);
        m_view->ShowChaos(chaosRows);

        // 표 행 순서대로 기록 키를 보관하고, 고른 젬이 아직 표에 있으면 선택을 유지
        m_orderKeys.clear();
        for (const ArkGemRow& row : orderRows) m_orderKeys.push_back(row.m_key);
        m_chaosKeys.clear();
        for (const ArkGemRow& row : chaosRows) m_chaosKeys.push_back(row.m_key);

        std::optional<std::size_t> orderSelected;   // 질서 표에서 다시 선택할 행
        std::optional<std::size_t> chaosSelected;   // 혼돈 표에서 다시 선택할 행
        if (m_selectedPriceKey.has_value())
        {
            if (const auto found{ std::ranges::find(m_orderKeys, *m_selectedPriceKey) }; found != m_orderKeys.end())
            {
                orderSelected = static_cast<std::size_t>(std::distance(m_orderKeys.begin(), found));
            }
            else if (const auto chaosFound{ std::ranges::find(m_chaosKeys, *m_selectedPriceKey) }; chaosFound != m_chaosKeys.end())
            {
                chaosSelected = static_cast<std::size_t>(std::distance(m_chaosKeys.begin(), chaosFound));
            }
        }
        m_view->SelectOrderRow(orderSelected);
        m_view->SelectChaosRow(chaosSelected);
        RefreshPriceChart();   // 시세가 바뀌면 그래프도 새 기록으로
        RefreshBoundGems();

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

    // 젬 · 등급마다 시세 찾기 (이름은 띄어쓰기를 무시하고 비교) → 고른 대상의 수량 × 단가 + 원정대 총 가치
    void ArkGemFinderManager::RefreshBoundGems()
    {
        if (!m_view) return;

        const Models::BoundGemRosterData& roster{ m_boundGemRepository->GetData() };
        const std::vector<Models::ArkGemQuote>& quotes{ m_service->GetQuotes() };

        // 고른 캐릭터의 수량 (원정대 전체 · 선택 없음이면 원정대 합계)
        const Models::BoundGemData rosterTotal{ roster.GetTotal() };
        const Models::BoundGemCharacter* selected{ GetSelectedCharacter() };
        const Models::BoundGemData& data{ selected != nullptr ? selected->m_quantities : rosterTotal };

        std::vector<BoundGemRow> rows;
        rows.reserve(Models::BoundGemData::GEM_COUNT);
        std::array<int, Models::BoundGemData::GRADE_COUNT> rosterQuantities{};
        double rosterValue{};
        for (std::size_t gemIndex{}; gemIndex < Models::BoundGemData::GEM_COUNT; ++gemIndex)
        {
            const std::wstring gemName{ RemoveSpaces(Core::TextEncoding::UTF8ToWide(Models::BoundGemData::GEM_NAMES[gemIndex])) };
            BoundGemRow& row{ rows.emplace_back() };

            for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
            {
                const auto quote{ std::ranges::find_if(quotes, [&gemName, gradeIndex](const Models::ArkGemQuote& candidate)
                {
                    return candidate.m_item.m_gradeName == Models::BoundGemData::GRADE_NAMES[gradeIndex]
                        && RemoveSpaces(ToWideMessage(candidate.m_item.m_name.c_str())) == gemName;
                }) };

                const double unitPrice{ quote != quotes.end() ? GetUnitPrice(quote->m_item) : 0.0 };
                row.m_quantities[gradeIndex] = data.GetQuantity(gemIndex, gradeIndex);
                row.m_unitPrices[gradeIndex] = unitPrice;
                row.m_value += static_cast<double>(row.m_quantities[gradeIndex]) * unitPrice;

                // 원정대 총 가치 (같은 단가로)
                const int rosterQuantity{ rosterTotal.GetQuantity(gemIndex, gradeIndex) };
                rosterQuantities[gradeIndex] += rosterQuantity;
                rosterValue += static_cast<double>(rosterQuantity) * unitPrice;
            }
        }
        m_view->ShowBoundGems(rows);
        m_view->ShowRosterTotal(rosterQuantities, rosterValue);
        m_rosterValue = rosterValue;
    }

    void ArkGemFinderManager::RecordHistory()
    {
        // 시세 없이 0골드가 기록되지 않게 (한 번이라도 조회했을 때만)
        if (!m_service->GetLastUpdated().has_value()) return;

        // 그래프의 하루는 오전 6시부터 (귀속재료 계산기와 같은 기준)
        m_boundGemRepository->GetData().RecordHistory(Core::LocalTime::MakeDateKey(Core::LocalTime::GetGameToday()), m_rosterValue);
        TrySave();
        UpdateChart();
    }

    // 날짜별 원정대 귀속 젬 총 가치 → 그래프 (제목 = 가장 최근 값)
    void ArkGemFinderManager::UpdateChart()
    {
        if (!m_view) return;

        const std::map<std::string, double>& history{ m_boundGemRepository->GetData().GetHistory() };
        std::vector<UI::ChartPoint> points;
        points.reserve(history.size());
        for (const auto& [dateKey, value] : history) points.push_back(UI::ChartPoint{ ToChartDateLabel(dateKey), value });

        if (points.empty())
        {
            m_view->SetChart(std::wstring{ CHART_EMPTY_TEXT }, {});
            return;
        }
        const std::wstring title{ std::format(CHART_TITLE_FORMAT, Core::NumberFormat::FormatNumber(points.back().m_value, AMOUNT_DECIMALS)) };
        m_view->SetChart(title, std::move(points));
    }

    // 대표 캐릭터 이름으로 원정대 목록 받기 (귀속재료 계산기와 같은 방식)
    void ArkGemFinderManager::SyncRoster(const std::wstring& mainCharacterName)
    {
        const std::wstring_view trimmedName{ TrimSpaces(mainCharacterName) };
        if (trimmedName.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        if (m_rosterTask.IsRunning()) return;

        m_view->SetSyncBusy(true);
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_rosterTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetRoster(name); });
    }

    // 원정대 목록을 받음 → 캐릭터 목록 교체 (기존 캐릭터는 수량 유지) → 저장
    void ArkGemFinderManager::FinishRosterSync()
    {
        m_view->SetSyncBusy(false);

        std::vector<Models::CharacterInfo> roster;
        try
        {
            roster = m_rosterTask.TakeResult();
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            HandleAPIKeyError(error.what());
            return;
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, std::wstring{ ROSTER_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
            return;
        }

        if (roster.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ ROSTER_NOT_FOUND_TEXT }, Core::NoticeKind::NOTICE_WARNING, nullptr);
            return;
        }

        // 젬은 아이템 레벨 1700 이상 캐릭터만 (미만 캐릭터는 목록에서 뺌)
        std::erase_if(roster, [](const Models::CharacterInfo& info) { return info.GetNumericItemLevel() < MIN_GEM_ITEM_LEVEL; });
        if (roster.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::format(NO_GEM_CHARACTER_FORMAT, MIN_GEM_ITEM_LEVEL),
                Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        m_view->CommitEdit();   // 입력 중이던 수량부터 반영
        Models::BoundGemRosterData& data{ m_boundGemRepository->GetData() };
        const bool isUnassignedMoved{ data.SetRoster(roster) };

        LoadCharacterList();
        TrySave();
        RecordHistory();   // 원정대가 바뀌면 총 가치도 바뀜 (시세가 있을 때만 기록)

        m_context->m_notifier.ShowToast(std::format(ROSTER_DONE_FORMAT, roster.size()), Core::NoticeKind::NOTICE_SUCCESS);
        if (isUnassignedMoved && !data.GetCharacters().empty())
        {
            m_context->m_notifier.ShowToast(std::format(UNASSIGNED_MOVED_FORMAT, ToWideMessage(data.GetCharacters().front().m_name.c_str())),
                Core::NoticeKind::NOTICE_INFO);
        }
    }

    // 캐릭터 선택 목록: 0번 ★ 원정대 전체 ★ + 레벨 높은 순 캐릭터 ("이름 (직업, Lv.1740.83)")
    void ArkGemFinderManager::LoadCharacterList()
    {
        const std::vector<Models::BoundGemCharacter>& characters{ m_boundGemRepository->GetData().GetCharacters() };

        std::vector<std::wstring> items;
        items.reserve(characters.size() + FIRST_CHARACTER_INDEX);
        items.emplace_back(ROSTER_ALL_TEXT);
        m_characterNames.clear();
        m_characterNames.reserve(characters.size());
        for (const Models::BoundGemCharacter& character : characters)
        {
            m_characterNames.push_back(character.m_name);
            std::wstring item{ ToWideMessage(character.m_name.c_str()) };
            if (character.m_className.has_value() && !character.m_className->empty())
            {
                item = std::format(L"{} ({}, Lv.{})", item, ToWideMessage(character.m_className->c_str()), character.m_itemLevel);
            }
            items.push_back(std::move(item));
        }

        // 기본 선택: 캐릭터가 있으면 첫 캐릭터, 없으면 원정대 전체
        m_view->SetCharacterItems(std::move(items), m_characterNames.empty() ? ROSTER_ALL_INDEX : FIRST_CHARACTER_INDEX);
        SelectCharacter();
    }

    // 캐릭터를 고르면 수량 입력 가능, 원정대 전체는 합계라 읽기 전용
    void ArkGemFinderManager::SelectCharacter()
    {
        if (!m_view) return;
        m_view->SetQuantityEditable(GetSelectedCharacter() != nullptr);
        RefreshBoundGems();
    }

    Models::BoundGemCharacter* ArkGemFinderManager::GetSelectedCharacter()
    {
        if (!m_view) return nullptr;

        // 선택 번호 - 1 = 이름 번호 (0번은 원정대 전체)
        const std::optional<std::size_t> selectedIndex{ m_view->GetSelectedCharacterIndex() };
        if (!selectedIndex.has_value() || *selectedIndex < FIRST_CHARACTER_INDEX) return nullptr;

        const std::size_t nameIndex{ *selectedIndex - FIRST_CHARACTER_INDEX };
        if (nameIndex >= m_characterNames.size()) return nullptr;   // 경계 검사
        return m_boundGemRepository->GetData().FindCharacter(m_characterNames[nameIndex]);
    }

    void ArkGemFinderManager::TrySave()
    {
        try
        {
            m_boundGemRepository->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }

    // 입력한 글자를 수량으로 (0 ~ 최대값) → 고른 캐릭터에 저장 (실패하면 되돌리고 안내) → 다시 계산
    void ArkGemFinderManager::SetVisionControl(std::function<bool()> isEnabled, std::function<void(bool)> setEnabled)
    {
        m_isVisionEnabled = std::move(isEnabled);     // 상태
        m_setVisionEnabled = std::move(setEnabled);   // 켜기 · 끄기
    }

    // 화면에서 읽은 귀속 젬 수량 → 그 캐릭터 칸 (손 입력과 같은 저장 · 다시 계산 · 그래프 기록, 계산기 선택도 그 캐릭터로)
    ArkGemFinderManager::GemScanOutcome ArkGemFinderManager::ApplyScannedGem(const std::string& characterName, std::size_t gemIndex, std::size_t gradeIndex,
        Models::GemStockKind kind, int quantity, std::string_view processedKey)
    {
        if (!m_boundGemRepository || !m_view) return GemScanOutcome{};                                // 초기화 전
        Models::BoundGemCharacter* character{ m_boundGemRepository->GetData().FindCharacter(characterName) };   // 캐릭터
        if (character == nullptr) return GemScanOutcome{};                                           // 원정대 동기화 전
        const int previous{ character->m_quantities.GetQuantity(gemIndex, gradeIndex) };            // 지금 수량
        const Models::GemScanSlot previousSlot{ character->m_quantities.GetScanSlot(gemIndex, gradeIndex) };   // 지금 내역 (되돌리기용)
        if (const auto found{ std::ranges::find(m_characterNames, characterName) }; found != m_characterNames.end())   // 계산기에서도 그 캐릭터로
        {
            const std::size_t index{ static_cast<std::size_t>(found - m_characterNames.begin()) + FIRST_CHARACTER_INDEX };   // 선택 번호
            if (m_view->GetSelectedCharacterIndex() != index) { m_view->SelectCharacterIndex(index); SelectCharacter(); }
        }
        const int total{ character->m_quantities.ApplyScan(gemIndex, gradeIndex, kind, quantity, processedKey, Core::APPConstants::MAX_BOUND_GEM_COUNT) };   // 내역 반영 → 합계
        if (character->m_quantities.GetScanSlot(gemIndex, gradeIndex) == previousSlot && previous == total)
            return GemScanOutcome{ GemScanResult::GEM_UNCHANGED, total };                            // 같은 내역 · 수량
        try
        {
            m_boundGemRepository->Save();                                                            // 저장 (내역이 바뀌면 수량이 같아도 저장)
        }
        catch (const std::exception&)
        {
            character->m_quantities.SetQuantity(gemIndex, gradeIndex, previous);                    // 되돌림
            character->m_quantities.SetScanSlot(gemIndex, gradeIndex, previousSlot);
            return GemScanOutcome{ GemScanResult::GEM_SAVE_FAILED, previous };
        }
        if (previous == total) return GemScanOutcome{ GemScanResult::GEM_UNCHANGED, total };         // 내역만 바뀜 → 조용히
        RefreshBoundGems();                                                                           // 표 · 가치
        RecordHistory();                                                                              // 오늘 가치 기록
        return GemScanOutcome{ GemScanResult::GEM_APPLIED, total };                                   // 넣음
    }

    void ArkGemFinderManager::ChangeQuantity(std::size_t gemIndex, std::size_t gradeIndex, const std::wstring& text)
    {
        Models::BoundGemCharacter* character{ GetSelectedCharacter() };
        if (character == nullptr) return;   // 원정대 전체는 입력할 수 없음

        Models::BoundGemData& data{ character->m_quantities };
        const int previous{ data.GetQuantity(gemIndex, gradeIndex) };
        const Models::GemScanSlot previousSlot{ data.GetScanSlot(gemIndex, gradeIndex) };   // 화면 인식 내역 (되돌리기용)
        const double parsed{ std::clamp(Core::NumberFormat::ParseNumber(text), 0.0, static_cast<double>(Core::APPConstants::MAX_BOUND_GEM_COUNT)) };

        data.SetQuantity(gemIndex, gradeIndex, static_cast<int>(parsed));
        if (static_cast<int>(parsed) != previous) data.ClearScan(gemIndex, gradeIndex);   // 손으로 바꾸면 화면 인식 내역은 다시 시작
        bool isSaved{ true };
        try
        {
            m_boundGemRepository->Save();
        }
        catch (const std::exception& error)
        {
            isSaved = false;
            data.SetQuantity(gemIndex, gradeIndex, previous);
            data.SetScanSlot(gemIndex, gradeIndex, previousSlot);   // 내역도 되돌림
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
        RefreshBoundGems();

        // 수량이 바뀌면 오늘 가치도 바뀜 → 그래프 기록 갱신 (저장이 실패했으면 같은 안내를 두 번 띄우지 않게 건너뜀)
        if (isSaved) RecordHistory();
    }

    // 자동 갱신 오류는 내용이 바뀌었을 때만 알림 (같은 오류가 반복해서 뜨는 것 방지, 숨어 있으면 풍선 알림)
    void ArkGemFinderManager::ShowAutoError(std::wstring_view title, const std::wstring& message)
    {
        if (m_lastAutoError == message) return;
        m_lastAutoError = message;
        m_context->m_notifier.ShowToast(std::format(AUTO_ERROR_FORMAT, title, message), Core::NoticeKind::NOTICE_ERROR);
    }

    // 조회가 성공했을 때: 표에 보이는(영웅 등급) 젬 중 매물이 있는(현재 최저가 > 0) 젬마다 오늘(오전 6시 기준) 최저가를 기록 (그날 더 싸면 갱신)
    void ArkGemFinderManager::RecordPriceHistory()
    {
        const std::string dayKey{ Core::LocalTime::MakeDateKey(Core::LocalTime::GetGameToday()) };   // 오늘 날짜 키
        Models::PriceHistory& history{ m_priceHistoryRepository->GetHistory() };                      // 기록
        bool isChanged{};   // 새로 기록하거나 갱신한 값이 있는지

        for (const Models::ArkGemQuote& quote : m_service->GetQuotes())
        {
            if (quote.m_item.m_gradeName != Core::APPConstants::EPIC_GRADE_NAME) continue;   // 시세 표와 같은 영웅 등급만 (등급마다 이름이 같아 키가 겹치지 않게)
            if (quote.m_item.m_currentMinPrice <= 0.0) continue;                              // 매물이 없으면 기록하지 않음
            if (history.RecordLowest(quote.m_item.m_name, dayKey, std::llround(quote.m_item.m_currentMinPrice))) isChanged = true;
        }

        // 바뀐 게 있거나 지난번 저장에 실패했으면 저장
        if (isChanged || m_isPriceHistoryDirty) SavePriceHistory();
    }

    // 시세 기록 저장 (실패하면 알리고, 다음 조회 · 종료 때 다시 시도)
    void ArkGemFinderManager::SavePriceHistory()
    {
        try
        {
            m_priceHistoryRepository->Save();   // 임시 파일 → 교체 방식 저장
            m_isPriceHistoryDirty = false;
        }
        catch (const std::exception& error)
        {
            m_isPriceHistoryDirty = true;       // 다음 기회에 다시 저장
            m_context->m_notifier.ShowToast(std::wstring{ PRICE_SAVE_ERROR_TEXT } + ToWideMessage(error.what()), Core::NoticeKind::NOTICE_ERROR);
        }
    }

    // 시세 표에서 행을 골랐을 때: 그 행의 젬이 그래프 대상
    void ArkGemFinderManager::SelectPriceRow(bool isOrder, std::size_t row)
    {
        const std::vector<std::string>& keys{ isOrder ? m_orderKeys : m_chaosKeys };
        if (row >= keys.size()) return;   // 범위를 벗어나면 무시

        m_selectedPriceKey = keys[row];
        RefreshPriceChart();
    }

    // 고른 젬의 일별 최저가 그래프 (고르지 않았으면 안내 문구)
    void ArkGemFinderManager::RefreshPriceChart()
    {
        if (!m_view) return;
        if (!m_selectedPriceKey.has_value())
        {
            m_view->SetPriceChart(std::wstring{ PRICE_CHART_HINT_TEXT }, {});
            return;
        }

        const std::map<std::string, std::int64_t>& days{ m_priceHistoryRepository->GetHistory().GetDays(*m_selectedPriceKey) };
        const std::wstring name{ ToWideMessage(m_selectedPriceKey->c_str()) };
        m_view->SetPriceChart(days.empty() ? std::format(PRICE_CHART_EMPTY_FORMAT, name) : std::format(PRICE_CHART_TITLE_FORMAT, name), BuildPriceChartPoints(days));
    }
}
