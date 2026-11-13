module LostArkMultiTool.Views.AccessoryFinderManager;

import std;
import LostArkMultiTool.Core.AccessorySearchService;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Services.AuctionAccessorySearchService;
import LostArkMultiTool.Views.AccessoryFinderView;
import LostArkMultiTool.Views.Theme;

namespace LostArkMultiTool::Views
{
    namespace
    {
        // 연마 효과 단계 → 값 목록 글자색 (상 노랑 · 중 보라 · 하 파랑, 단계가 없으면 기본색)
        [[nodiscard]] std::optional<Core::Color> GetPolishColor(const std::optional<Models::PolishGrade>& grade) noexcept
        {
            if (!grade.has_value()) return std::nullopt;
            switch (*grade)
            {
                case Models::PolishGrade::GRADE_HIGH:   return Theme::POLISH_HIGH_COLOR;
                case Models::PolishGrade::GRADE_MIDDLE: return Theme::POLISH_MIDDLE_COLOR;
                case Models::PolishGrade::GRADE_LOW:    return Theme::POLISH_LOW_COLOR;
            }
            return std::nullopt;
        }

        constexpr std::wstring_view TAB_TITLE{ L"악세 검색기" };
        constexpr int FIRST_PAGE{ 1 };

        // ===== 문구 =====
        constexpr std::wstring_view LOADING_OPTIONS_TEXT{ L"연마 효과 목록을 불러오는 중..." };
        constexpr std::wstring_view OPTIONS_READY_FORMAT{ L"연마 효과 {}종을 불러왔어요. 조건을 고르고 [검색]을 눌러 주세요." };
        constexpr std::wstring_view OPTIONS_FAILED_TEXT{ L"연마 효과 목록을 불러오지 못했어요. (옵션 없이 검색은 가능)" };
        constexpr std::wstring_view SEARCHING_TEXT{ L"경매장 검색 중..." };
        constexpr std::wstring_view SEARCH_DONE_FORMAT{ L"검색 완료 · {}" };
        constexpr std::wstring_view NO_RESULT_TEXT{ L"조건에 맞는 매물이 없어요." };
        constexpr std::wstring_view SEARCH_FAILED_TEXT{ L"검색하지 못했어요." };
        constexpr std::wstring_view PAGE_INFO_FORMAT{ L"{} / {} 페이지 (총 {}개)" };
        constexpr std::wstring_view OPTION_GROUP_FORMAT{ L"{} · {}" };   // 묶음이 여러 개일 때 "묶음 · 옵션"

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 검색은 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };
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

        // 옵션 콤보 상자 글자 (묶음이 하나뿐이면 옵션 이름만, 여러 개면 "묶음 · 옵션"으로 구분)
        [[nodiscard]] std::vector<std::wstring> MakeOptionNames(const std::vector<Models::AuctionOption>& options)
        {
            const bool hasManyGroups{ std::ranges::any_of(options, [&options](const Models::AuctionOption& option)
            {
                return option.m_firstOption != options.front().m_firstOption;
            }) };

            std::vector<std::wstring> names;
            names.reserve(options.size());
            for (const Models::AuctionOption& option : options)
            {
                const std::wstring name{ Core::TextEncoding::UTF8ToWide(option.m_name) };
                names.push_back(hasManyGroups ? std::format(OPTION_GROUP_FORMAT, Core::TextEncoding::UTF8ToWide(option.m_groupName), name) : name);
            }
            return names;
        }

        // 전체 페이지 수 (매물이 없으면 0)
        [[nodiscard]] int GetPageCount(const Models::AccessorySearchPage& page) noexcept
        {
            if (page.m_pageSize <= 0 || page.m_totalCount <= 0) return 0;
            return (page.m_totalCount + page.m_pageSize - 1) / page.m_pageSize;
        }
    }

    AccessoryFinderManager::AccessoryFinderManager() = default;

    AccessoryFinderManager::~AccessoryFinderManager() = default;

    std::wstring_view AccessoryFinderManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void AccessoryFinderManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 경매장 악세 검색 (결과는 저장하지 않음)
        m_service = std::make_unique<Services::AuctionAccessorySearchService>();

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<AccessoryFinderView>(context.m_platform);
        m_view->SetOnSearch([this] { Search(); });
        m_view->SetOnPageMove([this](int pageStep) { MovePage(pageStep); });
        m_view->SetOnOptionChanged([this](std::size_t slotIndex) { ChangeOption(slotIndex); });
        m_view->SetOnCategoryChanged([this] { ApplyCategoryOptions(); });
        UpdatePageInfo();
    }

    void AccessoryFinderManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_optionsTask.Wait();
        m_searchTask.Wait();

        m_view.reset();
        m_service.reset();
        m_context = nullptr;
    }

    // 탭을 처음 열 때 연마 효과 목록을 불러옴 (실행 직후 서버 확인 · 키 입력보다 요청이 앞서지 않게)
    void AccessoryFinderManager::OnActivated()
    {
        if (!m_view) return;
        m_view->RestoreFocus();
        RequestOptions();
    }

    void AccessoryFinderManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void AccessoryFinderManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);
        if (m_optionsTask.IsReady()) FinishOptions();
        if (m_searchTask.IsReady()) FinishSearch();
    }

    void AccessoryFinderManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool AccessoryFinderManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape AccessoryFinderManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    // 키 오류로 목록을 못 불러왔으면 새 키로 다시 시도
    void AccessoryFinderManager::OnAPIKeyChanged()
    {
        if (!m_view) return;
        m_isOptionsFailed = false;
        RequestOptions();
    }

    void AccessoryFinderManager::RequestOptions()
    {
        // 실패한 뒤에는 탭을 다시 열어도 자동으로 재시도하지 않음 (키 오류 안내 → 대화상자 닫힘 → 다시 실패가 반복되지 않게)
        if (m_isOptionsLoaded || m_isOptionsFailed || m_optionsTask.IsRunning()) return;
        if (m_context->m_marketAPI.GetRateLimit().IsExhausted(Core::LocalTime::GetUnixSeconds())) return;   // 한도가 차 있으면 다음 기회에

        m_view->SetStatusText(std::wstring{ LOADING_OPTIONS_TEXT });

        const Core::AccessorySearchService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_optionsTask.Start([service, marketAPI]
        {
            return service->FetchOptions(*marketAPI);
        });
    }

    // 목록을 받으면 모든 칸에 채움 (실패해도 옵션 없이 검색은 가능, 탭을 다시 열면 재시도)
    void AccessoryFinderManager::FinishOptions()
    {
        try
        {
            m_allOptions = m_optionsTask.TakeResult();
            m_isOptionsLoaded = true;
            ApplyCategoryOptions();
            if (!m_searchTask.IsRunning()) m_view->SetStatusText(std::format(OPTIONS_READY_FORMAT, m_options.size()));
        }
        catch (const std::exception& error)
        {
            m_isOptionsFailed = true;   // [검색]을 누르거나 키를 바꾸면 다시 시도
            m_view->SetStatusText(std::wstring{ OPTIONS_FAILED_TEXT });
            ShowRequestError(error);
        }
    }

    // 부위가 바뀌면 고른 옵션도 맞지 않으므로 모든 칸을 "선택 안 함"으로 되돌림
    void AccessoryFinderManager::ApplyCategoryOptions()
    {
        m_options = m_service->FilterOptions(m_allOptions, m_view->GetCategoryCode());
        m_view->SetOptionNames(MakeOptionNames(m_options));
    }

    void AccessoryFinderManager::ChangeOption(std::size_t slotIndex)
    {
        const std::optional<std::size_t> optionIndex{ m_view->GetSelectedOption(slotIndex) };
        if (!optionIndex.has_value() || *optionIndex >= m_options.size())
        {
            m_view->SetOptionValues(slotIndex, {});
            return;
        }

        const std::vector<Models::AuctionOptionValue>& values{ m_options[*optionIndex].m_values };
        std::vector<std::wstring> valueTexts;
        std::vector<std::optional<Core::Color>> valueColors;
        valueTexts.reserve(values.size());
        valueColors.reserve(values.size());
        for (const Models::AuctionOptionValue& value : values)
        {
            valueTexts.push_back(Core::TextEncoding::UTF8ToWide(value.m_displayText));
            valueColors.push_back(GetPolishColor(value.m_grade));
        }
        m_view->SetOptionValues(slotIndex, valueTexts, valueColors);
    }

    void AccessoryFinderManager::Search()
    {
        // 연마 효과 목록을 못 불러왔으면 검색과 함께 다시 시도 (이번 검색은 옵션 없이)
        if (m_isOptionsFailed)
        {
            m_isOptionsFailed = false;
            RequestOptions();
        }
        RequestSearch(BuildQuery());
    }

    void AccessoryFinderManager::MovePage(int pageStep)
    {
        if (!m_lastQuery.has_value()) return;

        Models::AccessorySearchQuery query{ *m_lastQuery };
        query.m_page = std::max(FIRST_PAGE, m_page.m_page + pageStep);
        RequestSearch(std::move(query));
    }

    void AccessoryFinderManager::RequestSearch(Models::AccessorySearchQuery query)
    {
        if (m_searchTask.IsRunning() || !CheckRateLimit()) return;

        m_runningQuery = query;
        m_view->SetBusy(true);
        m_view->SetStatusText(std::wstring{ SEARCHING_TEXT });

        const Core::AccessorySearchService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_searchTask.Start([service, marketAPI, query = std::move(query)]
        {
            return service->Search(*marketAPI, query);
        });
    }

    // 결과를 받으면 표 · 페이지 정보 갱신 (실패하면 이전 결과를 그대로 둠)
    void AccessoryFinderManager::FinishSearch()
    {
        m_view->SetBusy(false);

        try
        {
            m_page = m_searchTask.TakeResult();
            m_lastQuery = std::move(m_runningQuery);
            m_view->ShowListings(m_page.m_listings, m_lastQuery->m_categoryCode, m_page.m_totalCount, Core::LocalTime::FormatClock(Core::LocalTime::GetUnixSeconds()));   // 표 · 통계 카드 갱신
            m_view->SetStatusText(m_page.m_listings.empty() ? std::wstring{ NO_RESULT_TEXT }
                : std::format(SEARCH_DONE_FORMAT, Core::LocalTime::FormatClock(Core::LocalTime::GetUnixSeconds())));
        }
        catch (const std::exception& error)
        {
            m_view->SetStatusText(std::wstring{ SEARCH_FAILED_TEXT });
            ShowRequestError(error);
        }

        m_runningQuery.reset();
        UpdatePageInfo();
    }

    // 화면에서 고른 조건 (등급은 고대 고정, 옵션을 고른 칸만, 값을 고르지 않았으면 값 조건 없이)
    Models::AccessorySearchQuery AccessoryFinderManager::BuildQuery() const
    {
        Models::AccessorySearchQuery query{ m_view->GetCategoryCode(), std::string{ Core::APPConstants::ANCIENT_GRADE_NAME }, m_view->GetMinQuality(), {}, FIRST_PAGE };

        for (std::size_t slotIndex{}; slotIndex < Core::APPConstants::MAX_ACCESSORY_OPTION_FILTERS; ++slotIndex)
        {
            const std::optional<std::size_t> optionIndex{ m_view->GetSelectedOption(slotIndex) };
            if (!optionIndex.has_value() || *optionIndex >= m_options.size()) continue;

            const Models::AuctionOption& option{ m_options[*optionIndex] };
            Models::AccessoryOptionFilter& filter{ query.m_options.emplace_back(Models::AccessoryOptionFilter{ option.m_firstOption, option.m_secondOption, std::nullopt }) };

            if (const std::optional<std::size_t> valueIndex{ m_view->GetSelectedValue(slotIndex) }; valueIndex.has_value() && *valueIndex < option.m_values.size())
            {
                filter.m_minValue = option.m_values[*valueIndex].m_value;
            }
        }
        return query;
    }

    bool AccessoryFinderManager::CheckRateLimit()
    {
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        if (!rate.IsExhausted(now)) return true;

        m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE },
            std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))), Core::NoticeKind::NOTICE_INFO, nullptr);
        return false;
    }

    // 상황별 안내 (보석 · 유각 검색기의 수동 조회와 같은 방식)
    void AccessoryFinderManager::ShowRequestError(const std::exception& error)
    {
        const std::wstring message{ ToWideMessage(error.what()) };
        if (dynamic_cast<const Core::APIKeyInvalidException*>(&error) != nullptr)
        {
            HandleAPIKeyError(error.what());
        }
        else if (dynamic_cast<const Core::APIRateLimitException*>(&error) != nullptr)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, message, Core::NoticeKind::NOTICE_WARNING, nullptr);
        }
        else if (dynamic_cast<const Core::APIMaintenanceException*>(&error) != nullptr)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, message, Core::NoticeKind::NOTICE_INFO, nullptr);
        }
        else
        {
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, message, Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림
    void AccessoryFinderManager::HandleAPIKeyError(const std::string& message)
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

    // "2 / 12 페이지 (총 118개)" + 이전 · 다음 버튼 사용 여부
    void AccessoryFinderManager::UpdatePageInfo()
    {
        const int pageCount{ GetPageCount(m_page) };
        if (!m_lastQuery.has_value() || pageCount == 0)
        {
            m_view->SetPageInfo(std::wstring{}, false, false);
            return;
        }

        m_view->SetPageInfo(std::format(PAGE_INFO_FORMAT, m_page.m_page, pageCount, m_page.m_totalCount),
            m_page.m_page > FIRST_PAGE, m_page.m_page < pageCount);
    }
}
