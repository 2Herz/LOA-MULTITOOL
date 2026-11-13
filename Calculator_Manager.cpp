module LostArkMultiTool.Views.CalculatorManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.AssetCalculator;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.DataRepository;
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
import LostArkMultiTool.Models.Character;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.ItemData;
import LostArkMultiTool.Models.PriceData;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Models.UserData;
import LostArkMultiTool.Services.AssetCalculationService;
import LostArkMultiTool.Services.JSONDataRepository;
import LostArkMultiTool.Views.CalculatorView;
import LostArkMultiTool.Views.UI.LineChart;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"귀속재료 계산기" };

        constexpr std::size_t FIRST_CHARACTER_INDEX{ 0 };    // 캐릭터 선택 0번 = 레벨이 가장 높은 캐릭터
        constexpr std::size_t FIRST_MATERIAL_INDEX{ 1 };     // MAT_KEYS 0번은 구분 열
        constexpr int AMOUNT_DECIMALS{ 0 };

        // ===== 문구 =====
        constexpr std::wstring_view CHART_TITLE_TEXT{ L"날짜별 원정대 총 자산 (골드)" };   // 총자산 숫자는 카드 머리말 오른쪽에 따로 표시
        constexpr std::wstring_view SYNC_RUNNING_TEXT{ L"원정대 불러오는 중..." };          // 동기화 진행 상태
        constexpr std::wstring_view SYNC_DONE_FORMAT{ L"마지막 동기화 {}" };                 // 동기화 성공 시각
        constexpr std::wstring_view SYNC_FAILED_TEXT{ L"동기화 실패" };                      // 동기화 실패 상태
        constexpr std::wstring_view RESET_TITLE{ L"입력 초기화" };
        constexpr std::wstring_view RESET_QUESTION_FORMAT{ L"{}의 보유 수량(상위 · 하위)을 모두 0으로 되돌릴까요?\n입력한 수량이 지워져요." };
        constexpr std::wstring_view RESET_DONE_TEXT{ L"입력한 수량을 초기화했어요." };
        constexpr std::wstring_view RESET_NO_CHARACTER_TEXT{ L"먼저 캐릭터를 골라 주세요." };
        constexpr std::wstring_view CHART_EMPTY_TEXT{ L"아직 자산 기록이 없어요 · [시세 갱신] 후 기록돼요" };
        constexpr std::wstring_view TRAY_TOOLTIP_FORMAT{ L"원정대 자산: {} 골드" };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"{}분마다 시세를 자동으로 갱신해요." };

        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };   // 손상 복구 · 이전 위치 기록 가져오기
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"API 호출 한도가 부족해서 이번 시세 갱신은 건너뛰었어요.\n{}쯤 다시 시도해 주세요." };
        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"저장에 실패했어요.\n" };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view REFRESH_DONE_TEXT{ L"시세 동기화 완료!" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TEXT{ L"시세 자동 갱신 실패: " };
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };
        constexpr std::wstring_view ROSTER_NOT_FOUND_TEXT{ L"캐릭터 정보를 찾을 수 없어요." };
        constexpr std::wstring_view ROSTER_ERROR_TEXT{ L"오류 발생: " };
        constexpr std::wstring_view ROSTER_DONE_FORMAT{ L"총 {}개의 캐릭터 연동 완료!" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" \u3000" };   // 일반 공백 · 전각 공백

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

        [[nodiscard]] std::wstring_view TrimSpaces(std::wstring_view text) noexcept
        {
            const std::size_t first{ text.find_first_not_of(TRIM_CHARACTERS) };
            if (first == std::wstring_view::npos) return {};

            const std::size_t last{ text.find_last_not_of(TRIM_CHARACTERS) };
            return text.substr(first, last - first + 1);
        }

        // 캐릭터 선택에 보일 글자: "이름 (직업, Lv.1740.83)" (원본 GetDisplayName)
        [[nodiscard]] std::wstring MakeDisplayName(const std::string& name, const Models::Character& character)
        {
            const std::wstring wideName{ Core::TextEncoding::UTF8ToWide(name) };
            const std::optional<std::string>& className{ character.GetClassName() };
            if (!className.has_value() || className->empty()) return wideName;

            return std::format(L"{} ({}, Lv.{})", wideName, Core::TextEncoding::UTF8ToWide(*className), character.GetItemLevel());
        }

        // 저장 키 "yyyy-MM-dd" → 그래프 글자 "MM/dd" (형식이 다르면 그대로)
        [[nodiscard]] std::wstring ToChartDateLabel(const std::string& historyKey)
        {
            if (const auto date{ Models::UserData::ParseHistoryKey(historyKey) }; date.has_value())
            {
                return std::format(L"{:02}/{:02}", static_cast<unsigned>(date->month()), static_cast<unsigned>(date->day()));
            }
            return Core::TextEncoding::UTF8ToWide(historyKey);
        }
    }

    CalculatorManager::CalculatorManager() = default;

    CalculatorManager::~CalculatorManager() = default;

    std::wstring_view CalculatorManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void CalculatorManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 저장 파일(LostArkRosterData.json) → 시세 · 계산
        m_repository = std::make_unique<Services::JSONDataRepository>();
        m_calculator = std::make_unique<Services::AssetCalculationService>(*m_repository);
        m_calculator->AddAssetCalculatedListener([this] { RefreshView(); });

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<CalculatorView>(context.m_platform);
        m_view->SetOnSyncRoster([this](const std::wstring& mainCharacterName) { SyncRoster(mainCharacterName); });
        m_view->SetOnRefresh([this] { StartRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled) { SetAutoRefresh(isEnabled); });
        m_view->SetOnCharacterSelected([this] { SelectCharacter(); });
        m_view->SetOnResetInput([this] { ResetInput(); });
        m_view->SetOnQuantityEdited([this](MaterialTier tier, std::size_t materialIndex, double quantity) { EditQuantity(tier, materialIndex, quantity); });

        // 저장 파일이 손상돼 복구했으면 알림 (원본 Form1_Load)
        if (const std::optional<std::string>& warning{ m_repository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }

        LoadCharacterList();
        RefreshView();
    }

    void CalculatorManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoRefreshTimer.Stop();
        m_isAutoRefreshEnabled = false;
        m_refreshTask.Wait();
        m_rosterTask.Wait();

        m_view.reset();
        m_calculator.reset();
        m_repository.reset();
        m_context = nullptr;
    }

    void CalculatorManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void CalculatorManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();   // 편집 중인 칸 확정 · IME 끔
    }

    void CalculatorManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);

        // 백그라운드 작업이 끝났으면 결과 반영 (다른 탭을 보거나 트레이에 숨어 있어도 진행)
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_rosterTask.IsReady()) FinishRosterSync();
        if (m_autoRefreshTimer.Poll()) StartRefresh(true);
    }

    void CalculatorManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool CalculatorManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape CalculatorManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> CalculatorManager::GetBackgroundStatus() const
    {
        if (!m_isAutoRefreshEnabled) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES);
    }

    // 대표 캐릭터 이름으로 원정대 목록 받기 (원본 Button2_Click 앞부분)
    void CalculatorManager::SyncRoster(const std::wstring& mainCharacterName)
    {
        const std::wstring_view trimmedName{ TrimSpaces(mainCharacterName) };
        if (trimmedName.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        if (m_rosterTask.IsRunning()) return;

        m_view->SetSyncBusy(true);
        m_view->SetSyncStatus(std::wstring{ SYNC_RUNNING_TEXT });   // 조회 진행 상태 표시
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_rosterTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetRoster(name); });
    }

    // 시세 갱신 시작 (원본 RefreshMarketAsync 앞부분) — 수동 · 자동 갱신이 겹치지 않음
    void CalculatorManager::StartRefresh(bool isAuto)
    {
        if (m_refreshTask.IsRunning()) return;

        m_view->CommitEdit();   // 입력 중이던 수량부터 반영

        // 세 툴이 API 키 하나(분당 호출 한도 하나)를 함께 씀 → 보석 검색기 자동 갱신과 겹쳐도 한도를 넘지 않게 먼저 확인
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        if (!rate.CanAfford(m_calculator->GetRequestCount(), now))
        {
            const std::wstring message{ std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))) };
            if (isAuto) m_context->m_notifier.ShowToast(std::wstring{ AUTO_REFRESH_FAILED_TEXT } + message, Core::NoticeKind::NOTICE_WARNING);
            else m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, message, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        m_isAutoRefreshRun = isAuto;
        m_view->SetRefreshBusy(true);

        Core::AssetCalculator* calculator{ m_calculator.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([calculator, marketAPI] { calculator->FetchMarketPrices(*marketAPI); });
    }

    // 자동 갱신 켜면 바로 한 번 + 5분마다 (원본 CheckBox1_CheckedChanged)
    void CalculatorManager::SetAutoRefresh(bool isEnabled)
    {
        m_isAutoRefreshEnabled = isEnabled;
        if (!isEnabled)
        {
            m_autoRefreshTimer.Stop();
            return;
        }

        m_autoRefreshTimer.Start(Core::APPConstants::AUTO_REFRESH_INTERVAL_MS);
        StartRefresh(true);
    }

    // 고른 캐릭터의 수량 표시 · 잠금 → 계산 (원본 ComboBox1_SelectedIndexChanged)
    void CalculatorManager::SelectCharacter()
    {
        const std::vector<const Models::Character*> characters{ GetDisplayedCharacters() };
        if (characters.empty()) return;

        const Models::Character& character{ *characters.front() };
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
        {
            const std::string_view key{ Core::APPConstants::MAT_KEYS[index] };
            const Models::ItemData& item{ character.GetMaterial(key) };
            m_view->ShowQuantity(MaterialTier::TIER_TOP, index, item.GetTopQty(), item.GetTopQtyChange());
            m_view->ShowQuantity(MaterialTier::TIER_BOTTOM, index, item.GetBottomQty(), item.GetBottomQtyChange());
        }

        ApplyQuantityLock(character);
        if (const std::optional<std::string> name{ GetSelectedCharacterName() }; name.has_value()) m_calculator->CalculateCharacterAssets(*name);
    }

    // 보유수량 칸 하나 수정 → 저장 → 다시 계산 (원본 DataGridView1_CellValueChanged)
    void CalculatorManager::EditQuantity(MaterialTier tier, std::size_t materialIndex, double quantity)
    {
        if (materialIndex < FIRST_MATERIAL_INDEX || materialIndex >= Core::APPConstants::MAT_KEYS.size()) return;

        const std::optional<std::string> name{ GetSelectedCharacterName() };
        if (!name.has_value()) return;

        Models::Character* character{ m_repository->GetData().FindCharacter(*name) };
        if (character == nullptr) return;

        Models::ItemData& item{ character->GetMaterial(Core::APPConstants::MAT_KEYS[materialIndex]) };
        const bool isTop{ tier == MaterialTier::TIER_TOP };
        const double previousQuantity{ isTop ? item.GetTopQty() : item.GetBottomQty() };

        // 같은 수량을 다시 입력함 ("1000" → "1,000") → 지난 증감 표시를 그대로 되돌려 놓고 끝
        if (quantity == previousQuantity)
        {
            m_view->ShowQuantity(tier, materialIndex, quantity, isTop ? item.GetTopQtyChange() : item.GetBottomQtyChange());
            return;
        }

        // 바뀌기 전 수량을 직전 수량으로 남겨서 "지난 입력 대비 증감"을 보여 줌 (저장되어 다시 실행해도 유지)
        if (isTop)
        {
            item.SetPreviousTopQty(previousQuantity);
            item.SetTopQty(quantity);
        }
        else
        {
            item.SetPreviousBottomQty(previousQuantity);
            item.SetBottomQty(quantity);
        }
        m_view->ShowQuantity(tier, materialIndex, quantity, quantity - previousQuantity);

        TrySave();
        m_calculator->CalculateCharacterAssets(*name);
    }

    // 시세 갱신이 끝남 → 원정대 기록 · 완료 또는 오류 알림 (원본 RefreshMarketAsync 뒷부분)
    void CalculatorManager::FinishRefresh()
    {
        m_view->SetRefreshBusy(false);
        const bool isAuto{ m_isAutoRefreshRun };

        try
        {
            m_refreshTask.TakeResult();   // 백그라운드에서 난 예외는 여기서 다시 던져짐
            m_calculator->CalculateRosterHistory();
            if (!isAuto) m_context->m_notifier.ShowToast(std::wstring{ REFRESH_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            HandleAPIKeyError(error.what());
        }
        catch (const std::exception& error)
        {
            // 자동 갱신 오류는 작업을 막지 않게 알림만 (트레이에 숨어 있으면 풍선 알림), 직접 누른 갱신은 대화상자
            const std::wstring message{ ToWideMessage(error.what()) };
            if (isAuto) m_context->m_notifier.ShowToast(std::wstring{ AUTO_REFRESH_FAILED_TEXT } + message, Core::NoticeKind::NOTICE_ERROR);
            else m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, message, Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }

    // 원정대 목록을 받음 → 캐릭터 목록 교체 (기존 캐릭터는 수량 유지) (원본 Button2_Click 뒷부분)
    void CalculatorManager::FinishRosterSync()
    {
        m_view->SetSyncBusy(false);

        std::vector<Models::CharacterInfo> roster;
        try
        {
            roster = m_rosterTask.TakeResult();
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            m_view->SetSyncStatus(std::wstring{ SYNC_FAILED_TEXT });
            HandleAPIKeyError(error.what());
            return;
        }
        catch (const std::exception& error)
        {
            m_view->SetSyncStatus(std::wstring{ SYNC_FAILED_TEXT });
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, std::wstring{ ROSTER_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
            return;
        }

        if (roster.empty())
        {
            m_view->SetSyncStatus(std::wstring{ SYNC_FAILED_TEXT });
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ ROSTER_NOT_FOUND_TEXT }, Core::NoticeKind::NOTICE_WARNING, nullptr);
            return;
        }

        m_view->SetSyncStatus(std::format(SYNC_DONE_FORMAT, Core::LocalTime::FormatClock(Core::LocalTime::GetUnixSeconds())));   // 마지막 동기화 시각
        std::ranges::stable_sort(roster, std::ranges::greater{}, &Models::CharacterInfo::GetNumericItemLevel);

        Models::UserData& data{ m_repository->GetData() };
        std::unordered_map<std::string, Models::Character> characters;
        for (const Models::CharacterInfo& info : roster)
        {
            // 기존 캐릭터면 보유 수량은 그대로 두고 직업 · 레벨만 갱신
            const Models::Character* existing{ data.FindCharacter(info.GetCharacterName()) };
            Models::Character character{ existing != nullptr ? *existing : Models::Character{} };
            character.SetClassName(info.GetCharacterClassName());
            character.SetItemLevel(info.GetNumericItemLevel());
            characters.insert_or_assign(info.GetCharacterName(), std::move(character));
        }
        data.SetCharacters(std::move(characters));

        LoadCharacterList();

        try
        {
            if (m_calculator->HasPrices()) m_calculator->CalculateRosterHistory();
            else m_repository->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }

        m_context->m_notifier.ShowToast(std::format(ROSTER_DONE_FORMAT, roster.size()), Core::NoticeKind::NOTICE_SUCCESS);
    }

    // API 키 오류(401): 자동 갱신 끄기 → 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (원본 PromptApiKey)
    void CalculatorManager::HandleAPIKeyError(const std::string& message)
    {
        m_view->SetAutoRefreshChecked(false);
        SetAutoRefresh(false);

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

    // 캐릭터 선택 목록: 아이템 레벨 높은 순 (원정대 전체 합계는 표 맨 아래 ★ 원정대 자산 ★ 행이 맡음)
    void CalculatorManager::LoadCharacterList()
    {
        std::vector<std::pair<const std::string*, const Models::Character*>> sorted;
        for (const auto& [name, character] : m_repository->GetData().GetCharacters()) sorted.emplace_back(&name, &character);

        // 레벨이 같으면 이름순 (실행할 때마다 순서가 바뀌지 않게)
        std::ranges::sort(sorted, [](const auto& left, const auto& right)
        {
            if (left.second->GetItemLevel() != right.second->GetItemLevel()) return left.second->GetItemLevel() > right.second->GetItemLevel();
            return *left.first < *right.first;
        });

        std::vector<std::wstring> items;
        items.reserve(sorted.size());
        m_characterNames.clear();
        for (const auto& [name, character] : sorted)
        {
            m_characterNames.push_back(*name);
            items.push_back(MakeDisplayName(*name, *character));
        }

        // 기본 선택은 첫 번째 캐릭터 (캐릭터가 없으면 선택 없음)
        const std::optional<std::size_t> selectedIndex{ m_characterNames.empty() ? std::nullopt : std::optional<std::size_t>{ FIRST_CHARACTER_INDEX } };
        m_view->SetCharacterItems(std::move(items), selectedIndex);
        SelectCharacter();
    }

    // 단가 · 재료값 · 합계 · 그래프 · 트레이 글자 갱신 (원본 RefreshUI)
    void CalculatorManager::RefreshView()
    {
        if (!m_view) return;

        const auto topPrices{ m_calculator->GetTopPrices() };
        const auto bottomPrices{ m_calculator->GetBotPrices() };
        const std::vector<const Models::Character*> characters{ GetDisplayedCharacters() };

        double topTotal{};
        double bottomTotal{};
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
        {
            const std::string key{ Core::APPConstants::MAT_KEYS[index] };
            if (const auto top{ topPrices.find(key) }; top != topPrices.end())
            {
                m_view->ShowPrice(MaterialTier::TIER_TOP, index, top->second.GetCurrent(), top->second.GetDiff());
            }
            if (const auto bottom{ bottomPrices.find(key) }; bottom != bottomPrices.end())
            {
                m_view->ShowPrice(MaterialTier::TIER_BOTTOM, index, bottom->second.GetCurrent(), bottom->second.GetDiff());
            }

            // 고른 캐릭터의 재료값 (캐릭터가 없으면 건너뜀)
            if (characters.empty()) continue;

            double topValue{};
            double bottomValue{};
            for (const Models::Character* character : characters)
            {
                topValue += character->GetMaterial(key).GetCalculatedTopValue();
                bottomValue += character->GetMaterial(key).GetCalculatedBotValue();
            }
            m_view->ShowMaterialValue(MaterialTier::TIER_TOP, index, topValue);
            m_view->ShowMaterialValue(MaterialTier::TIER_BOTTOM, index, bottomValue);
            topTotal += topValue;
            bottomTotal += bottomValue;
        }
        if (!characters.empty()) m_view->ShowValueTotals(topTotal, bottomTotal);
        m_view->ShowRosterTotal(CalculateRosterTotal());   // 어떤 캐릭터를 골라도 표 맨 아래는 원정대 전체
        ShowRosterQuantities();

        const double currentTotal{ UpdateChart() };
        m_context->m_platform.SetTrayTooltip(std::format(TRAY_TOOLTIP_FORMAT, Core::NumberFormat::FormatNumber(currentTotal, AMOUNT_DECIMALS)));
    }

    // 날짜별 원정대 자산 기록 → 그래프 (가장 최근 총 자산 반환)
    double CalculatorManager::UpdateChart()
    {
        std::vector<UI::ChartPoint> points;
        double currentTotal{};
        for (const auto& [dateKey, total] : m_repository->GetData().GetHistory())
        {
            points.push_back(UI::ChartPoint{ ToChartDateLabel(dateKey), total });
            currentTotal = total;
        }

        std::wstring title{ points.empty() ? std::wstring{ CHART_EMPTY_TEXT } : std::wstring{ CHART_TITLE_TEXT } };
        m_view->SetTotalAsset(points.empty() ? std::nullopt : std::optional<double>{ currentTotal });   // 카드 머리말 오른쪽 "총자산 N G"
        m_view->SetChart(std::move(title), std::move(points));
        return currentTotal;
    }

    // 고른 캐릭터의 보유 수량(상위 · 하위)을 모두 0으로 (지워지므로 먼저 확인 → 예: 저장 · 다시 계산, 원본에 없는 새 기능)
    void CalculatorManager::ResetInput()
    {
        m_view->CommitEdit();   // 입력 중이던 칸은 확정한 뒤 지움

        const std::optional<std::string> name{ GetSelectedCharacterName() };
        if (!name.has_value())
        {
            m_context->m_notifier.ShowToast(std::wstring{ RESET_NO_CHARACTER_TEXT }, Core::NoticeKind::NOTICE_INFO);
            return;
        }

        m_context->m_notifier.AskYesNo(std::wstring{ RESET_TITLE }, std::format(RESET_QUESTION_FORMAT, Core::TextEncoding::UTF8ToWide(*name)),
            [this, name = *name](bool isYes)
        {
            if (!isYes || !m_repository || !m_view) return;   // 답하기 전에 프로그램이 종료됐거나 아니요

            Models::Character* character{ m_repository->GetData().FindCharacter(name) };
            if (character == nullptr) return;

            // 재료마다 직전 수량을 남기고 0으로 (지난 입력 대비 증감 표시와 같은 규칙)
            for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
            {
                Models::ItemData& item{ character->GetMaterial(Core::APPConstants::MAT_KEYS[index]) };
                item.SetPreviousTopQty(item.GetTopQty());
                item.SetTopQty(0.0);
                item.SetPreviousBottomQty(item.GetBottomQty());
                item.SetBottomQty(0.0);
            }

            TrySave();
            SelectCharacter();   // 표 수량 다시 표시 + 재계산 (총자산 즉시 반영)
            m_context->m_notifier.ShowToast(std::wstring{ RESET_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);
        });
    }

    // 레벨에 따라 보유수량 행 하나를 잠금 (1730 이상 = 상위만, 미만 = 하위만, 레벨 모름 = 둘 다) (원본 ApplyQtyRowLock)
    void CalculatorManager::ApplyQuantityLock(const Models::Character& character)
    {
        const bool isLevelKnown{ character.GetItemLevel() > 0.0 };
        const bool isTopTier{ character.GetItemLevel() >= Core::APPConstants::TOP_MATERIAL_MIN_LEVEL };

        m_view->SetQuantityEditable(MaterialTier::TIER_TOP, !isLevelKnown || isTopTier);
        m_view->SetQuantityEditable(MaterialTier::TIER_BOTTOM, !isLevelKnown || !isTopTier);
    }

    // 화면에 표시할 캐릭터 (고른 캐릭터 하나, 없으면 빈 목록) — 포인터는 저장소 데이터를 가리키기만 함
    std::vector<const Models::Character*> CalculatorManager::GetDisplayedCharacters() const
    {
        std::vector<const Models::Character*> characters;
        const Models::UserData& data{ m_repository->GetData() };

        if (const std::optional<std::string> name{ GetSelectedCharacterName() }; name.has_value())
        {
            if (const Models::Character* character{ data.FindCharacter(*name) }; character != nullptr) characters.push_back(character);
        }
        return characters;
    }

    // 모든 캐릭터의 상위 + 하위 재료값 합계 (시세 갱신 때 모든 캐릭터가 함께 계산되므로 저장된 계산값을 더하기만 함)
    double CalculatorManager::CalculateRosterTotal() const
    {
        double rosterTotal{};
        for (const auto& entry : m_repository->GetData().GetCharacters())
        {
            for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
            {
                const Models::ItemData& item{ entry.second.GetMaterial(Core::APPConstants::MAT_KEYS[index]) };
                rosterTotal += item.GetCalculatedTopValue() + item.GetCalculatedBotValue();
            }
        }
        return rosterTotal;
    }

    // 어떤 캐릭터를 골라도 원정대 자산 행은 원정대 전체 수량 (원정대 자산 골드와 같은 기준)
    void CalculatorManager::ShowRosterQuantities()
    {
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
        {
            double topQuantity{};
            double bottomQuantity{};
            for (const auto& entry : m_repository->GetData().GetCharacters())
            {
                const Models::ItemData& item{ entry.second.GetMaterial(Core::APPConstants::MAT_KEYS[index]) };
                topQuantity += item.GetTopQty();
                bottomQuantity += item.GetBottomQty();
            }
            m_view->ShowRosterQuantity(index, topQuantity, bottomQuantity);
        }
    }

    std::optional<std::string> CalculatorManager::GetSelectedCharacterName() const
    {
        if (!m_view) return std::nullopt;

        // 선택 번호 = 이름 번호 (목록 맨 위에 다른 항목이 없음)
        const std::optional<std::size_t> selectedIndex{ m_view->GetSelectedCharacterIndex() };
        if (!selectedIndex.has_value() || *selectedIndex >= m_characterNames.size()) return std::nullopt;   // 경계 검사
        return m_characterNames[*selectedIndex];
    }

    void CalculatorManager::TrySave()
    {
        try
        {
            m_repository->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }
}