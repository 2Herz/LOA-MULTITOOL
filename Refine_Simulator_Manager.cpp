module LostArkMultiTool.Views.RefineSimulatorManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.RefineSimulationService;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.RefinePlan;
import LostArkMultiTool.Services.MarketRefineSimulationService;
import LostArkMultiTool.Services.RefineCalculator;
import LostArkMultiTool.Views.RefineSimulatorView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        using Models::RefineData::EquipmentState;
        using Models::RefineData::EquipmentTier;
        using Models::RefineData::RefineMaterial;

        constexpr std::wstring_view TAB_TITLE{ L"재련 시뮬레이터" };

        // ===== 선택지 =====
        constexpr std::array<std::wstring_view, 3> START_PRESET_ITEMS{ L"1700 (에기르 +18 / 상급 20)", L"1730 (세르카 +11)", L"1750 (세르카 +15)" };
        constexpr std::array<Core::RefineStartPreset, 3> START_PRESETS{ Core::RefineStartPreset::PRESET_1700, Core::RefineStartPreset::PRESET_1730, Core::RefineStartPreset::PRESET_1750 };
        constexpr std::array<std::wstring_view, 2> TARGET_MODE_ITEMS{ L"아이템 레벨", L"부위별 단계" };
        constexpr std::size_t TARGET_MODE_ITEM_LEVEL_INDEX{ 0 };
        constexpr std::size_t TARGET_MODE_STAGE_INDEX{ 1 };
        constexpr std::array<std::wstring_view, Models::REFINE_SLOT_COUNT> SLOT_NAMES{ L"무기", L"투구", L"어깨", L"상의", L"하의", L"장갑" };
        constexpr std::size_t ARMOR_SLOT_COUNT{ Models::REFINE_SLOT_COUNT - 1 };   // 무기를 뺀 방어구 부위 수

        // ===== 입력 범위 =====
        constexpr int MIN_TARGET_ITEM_LEVEL{ 1600 };
        constexpr int MAX_TARGET_ITEM_LEVEL{ 1800 };
        constexpr EquipmentState DEFAULT_SERCA_STATE{ EquipmentTier::TIER_SERCA, Models::RefineData::MIN_SERCA_ENHANCEMENT, 0 };
        constexpr EquipmentState DEFAULT_AEGIR_STATE{ EquipmentTier::TIER_AEGIR, Models::RefineData::INHERIT_ENHANCEMENT, Models::RefineData::INHERIT_ADVANCED_ENHANCEMENT };

        // ===== 숫자 표시 =====
        constexpr double TEN_THOUSAND{ 10'000.0 };       // "만" 단위
        constexpr int MAN_DECIMALS{ 1 };
        constexpr int GOLD_DECIMALS{ 0 };
        constexpr int LEVEL_DECIMALS{ 2 };
        constexpr double SMALL_PRICE_LIMIT{ 1.0 };       // 1골드 미만 가격은 소수 넷째 자리까지 (파편 · 수호석)
        constexpr double MIDDLE_PRICE_LIMIT{ 100.0 };    // 100골드 미만은 소수 둘째 자리까지
        constexpr int SMALL_PRICE_DECIMALS{ 4 };
        constexpr int MIDDLE_PRICE_DECIMALS{ 2 };
        constexpr double SMALL_AMOUNT_LIMIT{ 10.0 };     // 10개 미만 기대 소모는 소수 첫째 자리까지 (책 · 숨결)
        constexpr int SMALL_AMOUNT_DECIMALS{ 1 };
        constexpr double MIN_SHOWN_AMOUNT{ 0.05 };       // 이보다 적은 기대 소모는 "-"

        // ===== 문구 =====
        constexpr std::wstring_view TIER_AEGIR_TEXT{ L"에기르" };
        constexpr std::wstring_view TIER_SERCA_TEXT{ L"세르카" };
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view NO_CHANGE_TEXT{ L"변화 없음" };
        constexpr std::wstring_view STAGE_CHANGE_FORMAT{ L"+{}단계" };
        constexpr std::wstring_view ADVANCED_CHANGE_FORMAT{ L"상급 +{}" };
        constexpr std::wstring_view INHERIT_CHANGE_FORMAT{ L"계승 → 세르카 +{}" };
        constexpr std::wstring_view CHANGE_SEPARATOR{ L" · " };
        constexpr std::wstring_view SLOT_COST_FORMAT{ L"{} · {}회" };
        constexpr std::wstring_view GOLD_FORMAT{ L"{} G" };
        constexpr std::wstring_view MAN_GOLD_FORMAT{ L"{}만 G" };
        constexpr std::wstring_view TRADE_TEXT{ L"거래" };
        constexpr std::wstring_view BOUND_TEXT{ L"귀속" };
        constexpr std::wstring_view GOLD_TRADE_TEXT{ L"일반 골드" };
        constexpr std::wstring_view GOLD_BOUND_TEXT{ L"귀속 골드" };
        constexpr std::wstring_view ARMOR_ALL_TEXT{ L"방어구 전체" };
        constexpr std::wstring_view SLOT_JOIN_TEXT{ L", " };
        constexpr std::wstring_view REGULAR_ROUTE_FORMAT{ L"{} {} +{} → +{}" };
        constexpr std::wstring_view ADVANCED_ROUTE_FORMAT{ L"{} 상급 재련 {} → {}" };
        constexpr std::wstring_view INHERIT_ROUTE_FORMAT{ L"{} 계승 (에기르 +{} → 세르카 +{})" };
        constexpr std::wstring_view ORDER_RANGE_FORMAT{ L"{}~{}" };
        constexpr std::wstring_view TOTAL_FORMAT{ L"총 소요 재화  {} G   ({} → {})" };
        constexpr std::wstring_view TOTAL_EMPTY_TEXT{ L"총 소요 재화  -" };
        constexpr std::wstring_view SUMMARY_FORMAT{ L"평균 {} → {}" };
        constexpr std::wstring_view SUMMARY_TARGET_FORMAT{ L"평균 {} → {} (목표 {})" };

        constexpr std::wstring_view NO_PRICE_TEXT{ L"[시세 갱신]을 누르면 재료 시세를 받아 계산해요." };
        constexpr std::wstring_view REFRESHING_TEXT{ L"재료 시세 조회 중..." };
        constexpr std::wstring_view CALCULATING_TEXT{ L"계산 중..." };
        constexpr std::wstring_view STATUS_FORMAT{ L"마지막 시세 갱신 {} · 비용 최소 경로 (기대값)" };
        constexpr std::wstring_view STATUS_NO_UPDATE_TEXT{ L"직접 입력한 가격으로 계산했어요." };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"재련 시뮬레이터: {}분마다 재료 시세를 자동으로 갱신해요." };
        constexpr std::wstring_view AUTO_ERROR_FORMAT{ L"{}: {}" };

        constexpr std::wstring_view FAILURE_INVALID_TEXT{ L"시작 · 목표 단계가 재련표 범위를 벗어났어요. (에기르 +10~+25 · 상급 0~40, 세르카 +11~+25)" };
        constexpr std::wstring_view FAILURE_REGRESSION_TEXT{ L"목표가 시작보다 낮은 부위가 있어요. (세르카 → 에기르, 단계 낮추기는 안 돼요)" };
        constexpr std::wstring_view FAILURE_NOT_PREPARED_TEXT{ L"세르카 재련 · 계승은 모든 부위가 에기르 +20 · 상급 40 이상이어야 해요." };
        constexpr std::wstring_view FAILURE_UNREACHABLE_TEXT{ L"목표 아이템 레벨에 닿을 수 없어요." };
        constexpr std::wstring_view CALCULATION_ERROR_FORMAT{ L"계산하지 못했어요: {}" };

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TITLE{ L"재련 재료 시세 자동 갱신 실패" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 시세 갱신은 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"재련 시뮬레이터 설정을 저장하지 못했어요.\n" };
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

        // 입력 글자 → 숫자 (쉼표는 무시, 숫자가 아니면 nullopt)
        [[nodiscard]] std::optional<double> ParseNumber(const std::wstring& text)
        {
            std::string narrow;
            narrow.reserve(text.size());
            for (const wchar_t character : text)
            {
                if (character == L',') continue;
                if ((character < L'0' || character > L'9') && character != L'.') return std::nullopt;
                narrow.push_back(static_cast<char>(character));
            }
            if (narrow.empty()) return std::nullopt;

            double value{};
            const auto [end, error]{ std::from_chars(narrow.data(), narrow.data() + narrow.size(), value) };
            if (error != std::errc{} || end != narrow.data() + narrow.size()) return std::nullopt;
            return value;
        }

        [[nodiscard]] std::wstring FormatGold(double value)
        {
            return std::format(GOLD_FORMAT, Core::NumberFormat::FormatNumber(value, GOLD_DECIMALS));
        }

        // 부위별 비용: 만 골드 이상이면 "59.0만 G"
        [[nodiscard]] std::wstring FormatShortGold(double value)
        {
            if (value >= TEN_THOUSAND) return std::format(MAN_GOLD_FORMAT, Core::NumberFormat::FormatNumber(value / TEN_THOUSAND, MAN_DECIMALS));
            return FormatGold(value);
        }

        [[nodiscard]] std::wstring FormatLevel(double level)
        {
            return Core::NumberFormat::FormatNumber(level, LEVEL_DECIMALS);
        }

        [[nodiscard]] std::wstring FormatPrice(double price)
        {
            if (price <= 0.0) return std::wstring{ EMPTY_TEXT };
            if (price < SMALL_PRICE_LIMIT) return Core::NumberFormat::FormatNumber(price, SMALL_PRICE_DECIMALS);
            if (price < MIDDLE_PRICE_LIMIT) return Core::NumberFormat::FormatNumber(price, MIDDLE_PRICE_DECIMALS);
            return Core::NumberFormat::FormatNumber(price, GOLD_DECIMALS);
        }

        [[nodiscard]] std::wstring FormatAmount(double amount)
        {
            if (amount < MIN_SHOWN_AMOUNT) return std::wstring{ EMPTY_TEXT };
            return Core::NumberFormat::FormatNumber(amount, amount < SMALL_AMOUNT_LIMIT ? SMALL_AMOUNT_DECIMALS : GOLD_DECIMALS);
        }

        [[nodiscard]] std::wstring_view GetTierText(EquipmentTier tier) noexcept
        {
            return tier == EquipmentTier::TIER_SERCA ? TIER_SERCA_TEXT : TIER_AEGIR_TEXT;
        }

        [[nodiscard]] std::wstring_view GetSlotName(Models::RefineSlot slot)
        {
            return SLOT_NAMES.at(static_cast<std::size_t>(slot));
        }

        // 시작 → 목표 변화 글자 ("+5단계", "+2단계 · 상급 +20", "계승 → 세르카 +15", "변화 없음")
        [[nodiscard]] std::wstring MakeChangeText(const EquipmentState& start, const EquipmentState& terminal)
        {
            if (start == terminal) return std::wstring{ NO_CHANGE_TEXT };
            if (start.m_tier != terminal.m_tier) return std::format(INHERIT_CHANGE_FORMAT, terminal.m_enhancement);

            std::vector<std::wstring> parts;
            if (terminal.m_enhancement > start.m_enhancement) parts.push_back(std::format(STAGE_CHANGE_FORMAT, terminal.m_enhancement - start.m_enhancement));
            if (terminal.m_advancedEnhancement > start.m_advancedEnhancement) parts.push_back(std::format(ADVANCED_CHANGE_FORMAT, terminal.m_advancedEnhancement - start.m_advancedEnhancement));

            std::wstring text;
            for (const std::wstring& part : parts)
            {
                if (!text.empty()) text += CHANGE_SEPARATOR;
                text += part;
            }
            return text.empty() ? std::wstring{ NO_CHANGE_TEXT } : text;
        }

        [[nodiscard]] double GetAverageLevel(const Models::RefineStates& states) noexcept
        {
            int sum{};
            for (const EquipmentState& state : states) sum += Models::RefineData::CalculateItemLevel(state);
            return static_cast<double>(sum) / static_cast<double>(Models::REFINE_SLOT_COUNT);
        }

        // 지금 시작 상태가 프리셋과 같으면 그 번호 (직접 바꿨으면 nullopt)
        [[nodiscard]] std::optional<std::size_t> FindStartPreset(const Models::RefineStates& starts)
        {
            for (std::size_t index{}; index < START_PRESETS.size(); ++index)
            {
                const EquipmentState presetState{ Core::GetStartPresetState(START_PRESETS[index]) };
                if (std::ranges::all_of(starts, [&presetState](const EquipmentState& state) { return state == presetState; })) return index;
            }
            return std::nullopt;
        }

        // 경로 한 덩어리 (같은 부위의 이어지는 상급 재련은 하나로, 나머지는 걸음 하나)
        struct RouteRun
        {
            std::size_t m_first{};
            std::size_t m_last{};
            Models::RefineSlot m_slot{};
            Models::RefineAction m_action{};
            EquipmentTier m_tier{};
            int m_fromValue{};   // 일반 · 계승 = 강화 단계, 상급 = 상급 단계
            int m_toValue{};
        };

        [[nodiscard]] int GetRunValue(const Models::RefineStep& step, const EquipmentState& state) noexcept
        {
            return step.m_action == Models::RefineAction::ACTION_ADVANCED ? state.m_advancedEnhancement : state.m_enhancement;
        }

        // 걸음 → 덩어리 (같은 부위 상급 재련이 이어지면 합침)
        [[nodiscard]] std::vector<RouteRun> BuildRuns(const std::vector<Models::RefineStep>& steps)
        {
            std::vector<RouteRun> runs;
            for (std::size_t index{}; index < steps.size(); ++index)
            {
                const Models::RefineStep& step{ steps[index] };
                if (!runs.empty() && step.m_action == Models::RefineAction::ACTION_ADVANCED)
                {
                    RouteRun& last{ runs.back() };
                    if (last.m_action == step.m_action && last.m_slot == step.m_slot && last.m_toValue == step.m_from.m_advancedEnhancement)
                    {
                        last.m_last = index;
                        last.m_toValue = step.m_to.m_advancedEnhancement;
                        continue;
                    }
                }
                runs.push_back(RouteRun{ index, index, step.m_slot, step.m_action, step.m_from.m_tier, GetRunValue(step, step.m_from), GetRunValue(step, step.m_to) });
            }
            return runs;
        }

        // 화면 한 행으로 묶을 수 있는 덩어리인지 (방어구끼리 같은 동작 · 같은 범위, 부위는 서로 다름)
        [[nodiscard]] bool CanMerge(const RouteRun& head, const RouteRun& next, const std::vector<Models::RefineSlot>& slots) noexcept
        {
            const bool isArmor{ head.m_slot != Models::RefineSlot::SLOT_WEAPON && next.m_slot != Models::RefineSlot::SLOT_WEAPON };
            return isArmor && head.m_action == next.m_action && head.m_tier == next.m_tier && head.m_fromValue == next.m_fromValue
                && head.m_toValue == next.m_toValue && std::ranges::find(slots, next.m_slot) == slots.end();
        }

        // 묶음의 부위 이름 ("방어구 전체" 또는 "투구, 어깨")
        [[nodiscard]] std::wstring JoinSlotNames(const std::vector<Models::RefineSlot>& slots)
        {
            if (slots.size() == ARMOR_SLOT_COUNT && std::ranges::none_of(slots, [](Models::RefineSlot slot) { return slot == Models::RefineSlot::SLOT_WEAPON; }))
            {
                return std::wstring{ ARMOR_ALL_TEXT };
            }

            std::wstring text;
            for (const Models::RefineSlot slot : slots)
            {
                if (!text.empty()) text += SLOT_JOIN_TEXT;
                text += GetSlotName(slot);
            }
            return text;
        }
    }

    RefineSimulatorManager::RefineSimulatorManager() = default;

    RefineSimulatorManager::~RefineSimulatorManager() = default;

    std::wstring_view RefineSimulatorManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void RefineSimulatorManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 시작 · 목표 · 재료 가격 · 귀속 (RefineSimulatorData.json)
        m_service = std::make_unique<Services::MarketRefineSimulationService>();
        if (const std::optional<std::string>& warning{ m_service->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<RefineSimulatorView>(context.m_platform);
        m_view->SetOnRefresh([this] { RequestRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled)
        {
            m_isAutoRefreshWanted = isEnabled;
            SetAutoRefresh(isEnabled);
        });
        m_view->SetOnStartPresetSelected([this](std::size_t index) { OnStartPresetSelected(index); });
        m_view->SetOnTargetModeSelected([this](std::size_t index) { OnTargetModeSelected(index); });
        m_view->SetOnTargetItemLevelChanged([this](const std::wstring& text) { OnTargetItemLevelChanged(text); });
        m_view->SetOnSlotCellClicked([this](std::size_t row, RefineSlotColumn column) { OnSlotCellClicked(row, column); });
        m_view->SetOnSlotCellEdited([this](std::size_t row, RefineSlotColumn column, const std::wstring& text) { OnSlotCellEdited(row, column, text); });
        m_view->SetOnMaterialCellClicked([this](std::size_t row, RefineMaterialColumn column) { OnMaterialCellClicked(row, column); });
        m_view->SetOnMaterialCellEdited([this](std::size_t row, RefineMaterialColumn column, const std::wstring& text) { OnMaterialCellEdited(row, column, text); });

        std::vector<std::wstring> presetItems;
        for (const std::wstring_view item : START_PRESET_ITEMS) presetItems.emplace_back(item);
        m_view->SetStartPresetItems(std::move(presetItems));

        std::vector<std::wstring> modeItems;
        for (const std::wstring_view item : TARGET_MODE_ITEMS) modeItems.emplace_back(item);
        m_view->SetTargetModeItems(std::move(modeItems));

        m_resultRequest = m_service->GetRequest();
        RefreshAll();
    }

    void RefineSimulatorManager::Shutdown()
    {
        // 진행 중인 요청 · 계산이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에)
        m_autoTimer.Stop();
        m_refreshTask.Wait();
        m_calculationTask.Wait();

        m_view.reset();
        m_service.reset();
        m_catalog.reset();
        m_context = nullptr;
    }

    void RefineSimulatorManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void RefineSimulatorManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void RefineSimulatorManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);

        // 시세 조회 · 계산이 끝났으면 반영, 입력이 바뀌었으면 다시 계산, 자동 갱신 주기가 되면 조회
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_calculationTask.IsReady()) FinishCalculation();
        if (m_isCalculationDirty && !m_calculationTask.IsRunning()) StartCalculation();
        if (m_autoTimer.Poll()) RequestRefresh(true);
    }

    void RefineSimulatorManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool RefineSimulatorManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape RefineSimulatorManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> RefineSimulatorManager::GetBackgroundStatus() const
    {
        if (!m_view || !m_view->IsAutoRefreshChecked()) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES);
    }

    // 키를 바꿨고 사용자가 자동 갱신을 켜 두었다면 다시 시작 (키 오류로 멈췄던 경우)
    void RefineSimulatorManager::OnAPIKeyChanged()
    {
        if (!m_view || m_view->IsAutoRefreshChecked() || !m_isAutoRefreshWanted) return;
        SetAutoRefresh(true);
    }

    // ===================== 시세 조회 =====================

    void RefineSimulatorManager::RequestRefresh(bool isAuto)
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
        RefreshStatus();

        const Core::RefineSimulationService* service{ m_service.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([service, marketAPI]
        {
            return service->FetchPrices(*marketAPI);
        });
    }

    // 자동 갱신 켜면 바로 한 번 + 5분마다 (다른 검색기와 같은 방식)
    void RefineSimulatorManager::SetAutoRefresh(bool isEnabled)
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

    // 조회가 끝남 → 가격 반영 · 저장 · 다시 계산 (오류는 다른 검색기와 같은 방식으로 안내)
    void RefineSimulatorManager::FinishRefresh()
    {
        m_view->SetRefreshEnabled(true);
        const bool isAuto{ m_isRunningAuto };
        std::optional<std::string> keyError;

        try
        {
            m_service->ApplyPrices(m_refreshTask.TakeResult());   // 백그라운드에서 난 예외는 여기서 다시 던져짐
            m_lastAutoError.reset();
            MarkChanged(true);
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

        RefreshStatus();

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value()) HandleAPIKeyError(*keyError);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (다른 검색기와 같음)
    void RefineSimulatorManager::HandleAPIKeyError(const std::string& message)
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

    // 자동 갱신 오류는 내용이 바뀌었을 때만 알림 (같은 오류 반복 방지, 숨어 있으면 풍선 알림)
    void RefineSimulatorManager::ShowAutoError(std::wstring_view title, const std::wstring& message)
    {
        if (m_lastAutoError == message) return;
        m_lastAutoError = message;
        m_context->m_notifier.ShowToast(std::format(AUTO_ERROR_FORMAT, title, message), Core::NoticeKind::NOTICE_ERROR);
    }

    // ===================== 계산 =====================

    // 지금 입력으로 백그라운드 계산 시작 (가격이 그대로면 지난 비용표를 다시 씀)
    void RefineSimulatorManager::StartCalculation()
    {
        m_isCalculationDirty = false;
        if (!m_service->HasPrices())
        {
            RefreshStatus();
            return;   // 가격 없이는 계산하지 않음 (모든 재료가 0골드로 계산되는 것 방지)
        }

        const std::uint64_t revision{ m_service->GetPriceRevision() };
        std::shared_ptr<const Models::RefineCostCatalog> catalog{ m_catalog && m_catalogRevision == revision ? m_catalog : nullptr };
        m_calculationTask.Start([request = m_service->GetRequest(), prices = m_service->GetPrices(), catalog = std::move(catalog), revision]
        {
            std::shared_ptr<const Models::RefineCostCatalog> usedCatalog{ catalog
                ? catalog : std::make_shared<const Models::RefineCostCatalog>(Services::RefineCalculator::BuildCostCatalog(prices)) };
            Models::RefineRouteResult result{ Services::RefineCalculator::CalculateRoute(request, *usedCatalog) };
            return CalculationOutput{ std::move(usedCatalog), revision, request, std::move(result) };
        });
        RefreshStatus();
    }

    void RefineSimulatorManager::FinishCalculation()
    {
        try
        {
            CalculationOutput output{ m_calculationTask.TakeResult() };
            m_catalog = std::move(output.m_catalog);
            m_catalogRevision = output.m_priceRevision;
            m_resultRequest = output.m_request;
            m_result = std::move(output.m_result);
            m_calculationError.reset();
        }
        catch (const std::exception& error)
        {
            m_result.reset();
            m_calculationError = std::format(CALCULATION_ERROR_FORMAT, ToWideMessage(error.what()));
        }
        RefreshAll();
    }

    // 입력이 바뀜 → 저장 → 다시 계산 예약 → 설정 · 표 갱신
    void RefineSimulatorManager::MarkChanged(bool isPriceChanged)
    {
        SaveData();
        m_isCalculationDirty = true;
        if (isPriceChanged) RefreshMaterials();
        RefreshSetup();
        RefreshSlots();
        RefreshStatus();
    }

    void RefineSimulatorManager::SaveData()
    {
        try
        {
            m_service->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }

    // ===================== 사용자 입력 =====================

    void RefineSimulatorManager::OnStartPresetSelected(std::size_t presetIndex)
    {
        if (presetIndex >= START_PRESETS.size()) return;
        m_service->ApplyStartPreset(START_PRESETS[presetIndex]);
        MarkChanged(false);
    }

    // 부위별 단계로 바꾸면 목표 칸을 지난 계산의 목표(없으면 시작 상태)로 채워서 시작
    void RefineSimulatorManager::OnTargetModeSelected(std::size_t modeIndex)
    {
        const Models::RefineTargetMode mode{ modeIndex == TARGET_MODE_STAGE_INDEX ? Models::RefineTargetMode::TARGET_STAGE : Models::RefineTargetMode::TARGET_ITEM_LEVEL };
        if (mode == m_service->GetRequest().m_targetMode) return;

        if (mode == Models::RefineTargetMode::TARGET_STAGE)
        {
            for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
            {
                const bool hasResult{ m_result.has_value() && m_result->m_failure == Models::RefineFailure::FAILURE_NONE };
                m_service->SetTarget(static_cast<Models::RefineSlot>(slot), hasResult ? m_result->m_slots[slot].m_terminal : m_service->GetRequest().m_starts[slot]);
            }
        }
        m_service->SetTargetMode(mode);
        MarkChanged(false);
    }

    // 목표 아이템 레벨: 범위 안의 숫자일 때만 반영 (입력 중인 "17" 같은 값은 무시)
    void RefineSimulatorManager::OnTargetItemLevelChanged(const std::wstring& text)
    {
        const std::optional<double> value{ ParseNumber(text) };
        if (!value.has_value()) return;

        const int level{ static_cast<int>(*value) };
        if (level < MIN_TARGET_ITEM_LEVEL || level > MAX_TARGET_ITEM_LEVEL || level == m_service->GetRequest().m_targetItemLevel) return;
        m_service->SetTargetItemLevel(level);
        MarkChanged(false);
    }

    // 장비 칸을 누르면 에기르 ↔ 세르카 (에기르 → 세르카 +11, 세르카 → 에기르 +20 · 상급 40)
    void RefineSimulatorManager::OnSlotCellClicked(std::size_t row, RefineSlotColumn column)
    {
        if (row >= Models::REFINE_SLOT_COUNT) return;
        const Models::RefineSlot slot{ static_cast<Models::RefineSlot>(row) };
        const Models::RefineRequest& request{ m_service->GetRequest() };
        const auto toggle{ [](const EquipmentState& state)
        {
            return state.m_tier == EquipmentTier::TIER_AEGIR ? DEFAULT_SERCA_STATE : DEFAULT_AEGIR_STATE;
        } };

        if (column == RefineSlotColumn::COLUMN_START_TIER)
        {
            m_service->SetStart(slot, toggle(request.m_starts[row]));
            MarkChanged(false);
        }
        else if (column == RefineSlotColumn::COLUMN_TARGET_TIER && request.m_targetMode == Models::RefineTargetMode::TARGET_STAGE)
        {
            m_service->SetTarget(slot, toggle(request.m_targets[row]));
            MarkChanged(false);
        }
    }

    // 강화 · 상급 숫자 입력 → 표 범위 안으로 맞춰서 반영
    void RefineSimulatorManager::OnSlotCellEdited(std::size_t row, RefineSlotColumn column, const std::wstring& text)
    {
        if (row >= Models::REFINE_SLOT_COUNT) return;
        const std::optional<double> value{ ParseNumber(text) };
        if (!value.has_value())
        {
            RefreshSlots();   // 숫자가 아니면 원래 값으로 되돌림
            return;
        }

        const Models::RefineSlot slot{ static_cast<Models::RefineSlot>(row) };
        const Models::RefineRequest& request{ m_service->GetRequest() };
        const bool isStart{ column == RefineSlotColumn::COLUMN_START_ENHANCEMENT || column == RefineSlotColumn::COLUMN_START_ADVANCED };
        const bool isAdvanced{ column == RefineSlotColumn::COLUMN_START_ADVANCED || column == RefineSlotColumn::COLUMN_TARGET_ADVANCED };
        EquipmentState state{ isStart ? request.m_starts[row] : request.m_targets[row] };

        const int number{ static_cast<int>(*value) };
        if (isAdvanced)
        {
            if (state.m_tier == EquipmentTier::TIER_SERCA) return;
            state.m_advancedEnhancement = std::clamp(number, 0, Models::RefineData::MAX_ADVANCED_ENHANCEMENT);
        }
        else
        {
            const int minLevel{ state.m_tier == EquipmentTier::TIER_SERCA ? Models::RefineData::MIN_SERCA_ENHANCEMENT : Models::RefineData::MIN_AEGIR_ENHANCEMENT };
            state.m_enhancement = std::clamp(number, minLevel, Models::RefineData::MAX_ENHANCEMENT);
        }

        if (isStart) m_service->SetStart(slot, state);
        else m_service->SetTarget(slot, state);
        MarkChanged(false);
    }

    // 거래 칸을 누르면 거래 ↔ 귀속 (귀속 재료는 비용 0)
    void RefineSimulatorManager::OnMaterialCellClicked(std::size_t row, RefineMaterialColumn column)
    {
        if (row >= Models::RefineData::MATERIAL_COUNT || column != RefineMaterialColumn::COLUMN_TRADE) return;
        const RefineMaterial material{ static_cast<RefineMaterial>(row) };
        m_service->SetBound(material, !m_service->GetPrices().m_isBound[row]);
        MarkChanged(true);
    }

    // 가격 직접 입력 (다음 시세 갱신 때 거래소 시세로 덮어씀)
    void RefineSimulatorManager::OnMaterialCellEdited(std::size_t row, RefineMaterialColumn column, const std::wstring& text)
    {
        if (row >= Models::RefineData::MATERIAL_COUNT || column != RefineMaterialColumn::COLUMN_PRICE) return;
        const std::optional<double> value{ ParseNumber(text) };
        if (!value.has_value())
        {
            RefreshMaterials();
            return;
        }

        m_service->SetPrice(static_cast<RefineMaterial>(row), *value);
        MarkChanged(true);
    }

    // ===================== 화면 갱신 =====================

    void RefineSimulatorManager::RefreshAll()
    {
        if (!m_view) return;
        RefreshSetup();
        RefreshSlots();
        RefreshMaterials();
        RefreshRoute();
        RefreshStatus();
    }

    // 시작 프리셋 · 목표 기준 · 목표 레벨 · 요약
    void RefineSimulatorManager::RefreshSetup()
    {
        const Models::RefineRequest& request{ m_service->GetRequest() };
        const bool isStageMode{ request.m_targetMode == Models::RefineTargetMode::TARGET_STAGE };

        m_view->SetStartPresetIndex(FindStartPreset(request.m_starts));
        m_view->SetTargetModeIndex(isStageMode ? TARGET_MODE_STAGE_INDEX : TARGET_MODE_ITEM_LEVEL_INDEX);
        m_view->SetTargetItemLevelText(std::to_wstring(request.m_targetItemLevel));
        m_view->SetTargetItemLevelEnabled(!isStageMode);

        // 요약: 실패 이유 또는 "평균 시작 → 달성 (목표)"
        if (m_calculationError.has_value())
        {
            m_view->SetSummaryText(*m_calculationError, true);
            return;
        }
        if (!m_result.has_value())
        {
            m_view->SetSummaryText(std::format(SUMMARY_FORMAT, FormatLevel(GetAverageLevel(request.m_starts)), EMPTY_TEXT), false);
            return;
        }

        switch (m_result->m_failure)
        {
        case Models::RefineFailure::FAILURE_INVALID_STATE: m_view->SetSummaryText(std::wstring{ FAILURE_INVALID_TEXT }, true); return;
        case Models::RefineFailure::FAILURE_REGRESSION: m_view->SetSummaryText(std::wstring{ FAILURE_REGRESSION_TEXT }, true); return;
        case Models::RefineFailure::FAILURE_NOT_PREPARED: m_view->SetSummaryText(std::wstring{ FAILURE_NOT_PREPARED_TEXT }, true); return;
        case Models::RefineFailure::FAILURE_UNREACHABLE: m_view->SetSummaryText(std::wstring{ FAILURE_UNREACHABLE_TEXT }, true); return;
        case Models::RefineFailure::FAILURE_NONE: break;
        }

        const std::wstring start{ FormatLevel(m_result->m_startItemLevel) };
        const std::wstring achieved{ FormatLevel(m_result->m_achievedItemLevel) };
        if (m_resultRequest.m_targetMode == Models::RefineTargetMode::TARGET_ITEM_LEVEL)
        {
            m_view->SetSummaryText(std::format(SUMMARY_TARGET_FORMAT, start, achieved, m_resultRequest.m_targetItemLevel), false);
        }
        else
        {
            m_view->SetSummaryText(std::format(SUMMARY_FORMAT, start, achieved), false);
        }
    }

    // 부위 표: 시작 = 지금 입력, 목표 = 부위별 단계면 입력 / 아이템 레벨이면 계산 결과
    void RefineSimulatorManager::RefreshSlots()
    {
        const Models::RefineRequest& request{ m_service->GetRequest() };
        const bool isStageMode{ request.m_targetMode == Models::RefineTargetMode::TARGET_STAGE };
        const bool hasResult{ m_result.has_value() && m_result->m_failure == Models::RefineFailure::FAILURE_NONE && m_resultRequest.m_starts == request.m_starts };

        std::vector<RefineSlotRow> rows;
        rows.reserve(Models::REFINE_SLOT_COUNT);
        for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
        {
            const EquipmentState& start{ request.m_starts[slot] };
            const bool isStartSerca{ start.m_tier == EquipmentTier::TIER_SERCA };

            RefineSlotRow row;
            row.m_slotName = std::wstring{ SLOT_NAMES[slot] };
            row.m_startTier = std::wstring{ GetTierText(start.m_tier) };
            row.m_isStartSerca = isStartSerca;
            row.m_startEnhancement = std::to_wstring(start.m_enhancement);
            row.m_startAdvanced = isStartSerca ? std::wstring{ EMPTY_TEXT } : std::to_wstring(start.m_advancedEnhancement);
            row.m_isStartAdvancedEditable = !isStartSerca;
            row.m_isTargetEditable = isStageMode;

            // 목표 상태 (부위별 단계 = 입력값, 아이템 레벨 = 계산 결과가 있을 때만)
            std::optional<EquipmentState> terminal;
            if (isStageMode) terminal = request.m_targets[slot];
            else if (hasResult) terminal = m_result->m_slots[slot].m_terminal;

            if (terminal.has_value())
            {
                const bool isTargetSerca{ terminal->m_tier == EquipmentTier::TIER_SERCA };
                row.m_targetTier = std::wstring{ GetTierText(terminal->m_tier) };
                row.m_isTargetSerca = isTargetSerca;
                row.m_targetEnhancement = std::to_wstring(terminal->m_enhancement);
                row.m_targetAdvanced = isTargetSerca ? std::wstring{ EMPTY_TEXT } : std::to_wstring(terminal->m_advancedEnhancement);
                row.m_isTargetAdvancedEditable = !isTargetSerca;
                row.m_change = MakeChangeText(start, *terminal);
                row.m_hasChange = !(start == *terminal);
            }
            else
            {
                row.m_targetTier = std::wstring{ EMPTY_TEXT };
                row.m_targetEnhancement = std::wstring{ EMPTY_TEXT };
                row.m_targetAdvanced = std::wstring{ EMPTY_TEXT };
                row.m_change = std::wstring{ EMPTY_TEXT };
            }

            // 비용은 지금 입력과 같은 계산 결과가 있을 때만
            const bool isResultCurrent{ hasResult && (!isStageMode || m_resultRequest.m_targets[slot] == request.m_targets[slot]) };
            if (isResultCurrent && row.m_hasChange)
            {
                const Models::RefineSlotPlan& plan{ m_result->m_slots[slot] };
                row.m_cost = std::format(SLOT_COST_FORMAT, FormatShortGold(plan.m_expectedCost), plan.m_stepCount);
            }
            else
            {
                row.m_cost = std::wstring{ EMPTY_TEXT };
            }
            rows.push_back(std::move(row));
        }
        m_view->ShowSlots(rows);
    }

    // 재료 표: 가격 · 거래 여부 · 기대 소모 · 기대 골드
    void RefineSimulatorManager::RefreshMaterials()
    {
        const Models::RefinePrices& prices{ m_service->GetPrices() };
        const bool hasResult{ m_result.has_value() && m_result->m_failure == Models::RefineFailure::FAILURE_NONE };

        std::vector<RefineMaterialRow> rows;
        rows.reserve(Models::RefineData::MATERIAL_COUNT);
        for (std::size_t index{}; index < Models::RefineData::MATERIAL_COUNT; ++index)
        {
            const RefineMaterial material{ static_cast<RefineMaterial>(index) };
            const bool isGold{ material == RefineMaterial::MATERIAL_GOLD };
            const bool isBound{ prices.m_isBound[index] };
            const double amount{ hasResult ? m_result->m_materials[index] : 0.0 };

            RefineMaterialRow row;
            row.m_name = Core::TextEncoding::UTF8ToWide(Models::RefineData::GetMaterialInfo(material).m_name);
            row.m_price = FormatPrice(prices.m_prices[index]);
            row.m_isPriceEditable = !isGold;
            row.m_isBound = isBound;
            row.m_trade = std::wstring{ isGold ? (isBound ? GOLD_BOUND_TEXT : GOLD_TRADE_TEXT) : (isBound ? BOUND_TEXT : TRADE_TEXT) };
            row.m_amount = FormatAmount(amount);
            row.m_cost = (isBound || amount < MIN_SHOWN_AMOUNT) ? std::wstring{ EMPTY_TEXT } : FormatGold(amount * prices.m_prices[index]);
            rows.push_back(std::move(row));
        }
        m_view->ShowMaterials(rows);
    }

    // 경로 표: 같은 걸음은 한 행으로 묶고 달성 레벨 · 골드 · 누적 골드
    void RefineSimulatorManager::RefreshRoute()
    {
        if (!m_result.has_value() || m_result->m_failure != Models::RefineFailure::FAILURE_NONE)
        {
            m_view->ShowRoute(std::wstring{ TOTAL_EMPTY_TEXT }, {});
            return;
        }

        const std::vector<Models::RefineStep>& steps{ m_result->m_steps };
        const bool isItemLevelMode{ m_resultRequest.m_targetMode == Models::RefineTargetMode::TARGET_ITEM_LEVEL };
        Models::RefineStates states{ m_resultRequest.m_starts };
        std::vector<RefineRouteRow> rows;
        double cumulative{};
        bool hasReached{};

        const std::vector<RouteRun> runs{ BuildRuns(steps) };
        std::size_t rowNumber{};
        for (std::size_t firstRun{}; firstRun < runs.size();)
        {
            // 같은 행으로 묶을 덩어리 끝 찾기
            const RouteRun& head{ runs[firstRun] };
            std::vector<Models::RefineSlot> slots{ head.m_slot };
            std::size_t lastRun{ firstRun };
            while (lastRun + 1 < runs.size() && CanMerge(head, runs[lastRun + 1], slots))
            {
                ++lastRun;
                slots.push_back(runs[lastRun].m_slot);
            }

            double groupCost{};
            for (std::size_t index{ head.m_first }; index <= runs[lastRun].m_last; ++index)
            {
                const Models::RefineStep& step{ steps[index] };
                groupCost += step.m_expectedCost;
                states[static_cast<std::size_t>(step.m_slot)] = step.m_to;
            }
            cumulative += groupCost;

            // 행 번호: 묶인 덩어리 수 기준 ("1~5" = 5부위)
            const std::size_t firstNumber{ rowNumber + 1 };
            rowNumber += lastRun - firstRun + 1;
            RefineRouteRow row;
            row.m_order = firstNumber == rowNumber ? std::to_wstring(firstNumber) : std::format(ORDER_RANGE_FORMAT, firstNumber, rowNumber);
            switch (head.m_action)
            {
            case Models::RefineAction::ACTION_REGULAR:
                row.m_description = std::format(REGULAR_ROUTE_FORMAT, JoinSlotNames(slots), GetTierText(head.m_tier), head.m_fromValue, head.m_toValue);
                break;
            case Models::RefineAction::ACTION_ADVANCED:
                row.m_description = std::format(ADVANCED_ROUTE_FORMAT, JoinSlotNames(slots), head.m_fromValue, head.m_toValue);
                break;
            case Models::RefineAction::ACTION_INHERIT:
                row.m_description = std::format(INHERIT_ROUTE_FORMAT, JoinSlotNames(slots), head.m_fromValue, head.m_toValue);
                row.m_isInherit = true;
                break;
            }

            const double level{ GetAverageLevel(states) };
            row.m_itemLevel = FormatLevel(level);
            row.m_cost = FormatShortGold(groupCost);
            row.m_cumulative = FormatShortGold(cumulative);
            if (isItemLevelMode && !hasReached && level >= static_cast<double>(m_resultRequest.m_targetItemLevel))
            {
                row.m_isReached = true;
                hasReached = true;
            }
            rows.push_back(std::move(row));
            firstRun = lastRun + 1;
        }

        m_view->ShowRoute(std::format(TOTAL_FORMAT, Core::NumberFormat::FormatNumber(m_result->m_totalCost, GOLD_DECIMALS),
            FormatLevel(m_result->m_startItemLevel), FormatLevel(m_result->m_achievedItemLevel)), rows);
    }

    // 상단 상태: 조회 중 · 계산 중 · 가격 없음 · 마지막 갱신 시각
    void RefineSimulatorManager::RefreshStatus()
    {
        if (!m_view) return;
        if (m_refreshTask.IsRunning())
        {
            m_view->SetStatusText(std::wstring{ REFRESHING_TEXT });
            return;
        }
        if (!m_service->HasPrices())
        {
            m_view->SetStatusText(std::wstring{ NO_PRICE_TEXT });
            return;
        }
        if (m_calculationTask.IsRunning() || m_isCalculationDirty)
        {
            m_view->SetStatusText(std::wstring{ CALCULATING_TEXT });
            return;
        }

        const std::optional<std::int64_t>& lastUpdated{ m_service->GetLastUpdated() };
        m_view->SetStatusText(lastUpdated.has_value() ? std::format(STATUS_FORMAT, Core::LocalTime::FormatClock(*lastUpdated)) : std::wstring{ STATUS_NO_UPDATE_TEXT });
    }
}
