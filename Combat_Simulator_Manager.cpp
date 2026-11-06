module LostArkMultiTool.Views.CombatSimulatorManager;

import std;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.CombatFactorComponent;
import LostArkMultiTool.Core.CombatPowerCalculator;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Models.CombatState;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Services.CombatSimulatorStore;
import LostArkMultiTool.Views.CombatExtraEditor;
import LostArkMultiTool.Views.CombatSimulatorView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"전투력 시뮬레이터" };

        // ===== 입력 범위 =====
        constexpr int MIN_QUALITY{ 0 };
        constexpr int MAX_QUALITY{ 100 };
        constexpr std::size_t GEM_KEEP_INDEX{ 0 };   // 보석 목표 0번 = 현재 유지, 1번부터 = 6레벨 ..., 마지막 = 개별 설정
        constexpr std::size_t FIRST_GEM_LEVEL_INDEX{ 1 };   // 선택지 1번 = MIN_GEM_LEVEL
        constexpr int MIN_GEM_LEVEL{ 6 };            // 보석 목표 레벨 범위 (계수 표와 같은 6 ~ 10, 낮은 레벨은 쓰지 않음)
        constexpr int MAX_GEM_LEVEL{ 10 };
        constexpr std::string_view ESTHER_GRADE_NAME{ "에스더" };

        // ===== 숫자 표시 =====
        constexpr int POWER_DECIMALS{ 2 };
        constexpr int LEVEL_DECIMALS{ 2 };
        constexpr double PERCENT_SCALE{ 100.0 };     // 비율 → 퍼센트
        constexpr double NO_CHANGE_FACTOR{ 1.0 };
        constexpr double CHANGE_EPSILON{ 1e-9 };     // 이보다 작은 차이는 변화 없음
        constexpr double FACTOR_DISPLAY_EPSILON{ 5e-5 };   // 배율 내역에서 ×1.0000으로 보이는 요소는 숨김
        constexpr int MAX_VAMBRACE_TARGET{ Models::MAX_VAMBRACE_LEVEL };
        constexpr std::string_view VAMBRACE_GRADE_NAME{ "유물" };

        // ===== 문구 =====
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view ENHANCEMENT_FORMAT{ L"+{}" };
        constexpr std::wstring_view GEM_KEEP_TEXT{ L"현재 유지" };
        constexpr std::wstring_view GEM_INDIVIDUAL_TEXT{ L"개별 설정" };   // 보석 표에서 하나씩 바꾼 상태
        constexpr std::wstring_view GEM_SLOT_FORMAT{ L"{}" };
        constexpr std::wstring_view GEM_LEVEL_FORMAT{ L"{}레벨" };
        constexpr std::wstring_view GEM_SUMMARY_FORMAT{ L"보석 {}개" };
        constexpr std::wstring_view GEM_COUNT_FORMAT{ L" · {}레벨 {}개" };
        constexpr std::wstring_view NO_GEM_TEXT{ L"장착한 보석이 없어요." };
        constexpr std::wstring_view GEM_NOT_LOADED_TEXT{ L"보석: 조회 후 표시돼요." };
        constexpr std::wstring_view QUALITY_FORMAT{ L"무기 품질   현재 {}   →   목표" };
        constexpr std::wstring_view QUALITY_ESTHER_TEXT{ L"무기 품질   에스더 무기는 품질이 없어요" };
        constexpr std::wstring_view QUALITY_UNKNOWN_TEXT{ L"무기 품질   정보 없음" };

        constexpr std::wstring_view STATUS_FORMAT{ L"{} · {} · 아이템 레벨 {} · API 전투력 {}" };
        constexpr std::wstring_view NO_POWER_TEXT{ L"정보 없음" };
        constexpr std::wstring_view SEARCHING_FORMAT{ L"'{}' 무기고 조회 중..." };
        constexpr std::wstring_view NOT_SEARCHED_TEXT{ L"캐릭터 이름을 넣고 [조회]를 누르면 무기고를 불러와요." };
        constexpr std::wstring_view SEARCH_ERROR_FORMAT{ L"조회하지 못했어요: {}" };

        constexpr std::wstring_view CURRENT_POWER_FORMAT{ L"현재 전투력 (API)   {}" };
        constexpr std::wstring_view PREDICTED_POWER_FORMAT{ L"예상 전투력   {}" };
        constexpr std::wstring_view PREDICTED_FACTOR_FORMAT{ L"예상 배율   ×{:.4f}" };
        constexpr std::wstring_view CHANGE_FORMAT{ L"증감   {}{} ({}{:.2f}%)" };
        constexpr std::wstring_view NO_POWER_CHANGE_TEXT{ L"API 전투력 정보가 없어서 배율만 보여 줘요." };
        constexpr std::wstring_view ITEM_LEVEL_FORMAT{ L"예상 아이템 레벨   {} → {}" };
        constexpr std::wstring_view FACTOR_PREFIX{ L"배율  " };
        constexpr std::wstring_view FACTOR_ITEM_FORMAT{ L"{} ×{:.4f}" };
        constexpr std::wstring_view FACTOR_SEPARATOR{ L" · " };
        constexpr std::wstring_view PLUS_TEXT{ L"+" };
        constexpr std::wstring_view NO_SIGN_TEXT{ L"" };
        constexpr std::wstring_view FACTOR_GEAR_TEXT{ L"장비" };
        constexpr std::wstring_view FACTOR_QUALITY_TEXT{ L"무기 품질" };
        constexpr std::wstring_view FACTOR_GEM_TEXT{ L"보석" };
        constexpr std::wstring_view FACTOR_VAMBRACE_TEXT{ L"완갑" };
        constexpr std::wstring_view FACTOR_ACCESSORY_TEXT{ L"악세서리" };
        constexpr std::wstring_view FACTOR_BRACELET_TEXT{ L"팔찌" };
        constexpr std::wstring_view FACTOR_ENGRAVING_TEXT{ L"스톤 · 각인서" };
        constexpr std::wstring_view FACTOR_AVATAR_TEXT{ L"아바타" };
        constexpr std::wstring_view FACTOR_ARK_CORE_TEXT{ L"아크 코어" };
        constexpr std::wstring_view FACTOR_ARK_GEM_TEXT{ L"아크 젬" };
        constexpr std::wstring_view FACTOR_ARK_PASSIVE_TEXT{ L"아크 패시브" };
        constexpr std::wstring_view NO_FACTOR_CHANGE_TEXT{ L"변화 없음" };
        constexpr std::wstring_view VAMBRACE_PART_TEXT{ L"완갑" };
        constexpr std::wstring_view VAMBRACE_ITEM_TEXT{ L"완갑 (재련)" };

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 조회는 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };
        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"전투력 시뮬레이터 설정을 저장하지 못했어요.\n" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };   // 일반 공백 · 전각 공백

        // 예외 메시지(UTF-8) → 화면 문구 (변환할 수 없으면 기본 문구)
        [[nodiscard]] std::wstring ToWideMessage(std::string_view message)
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

        // 입력 글자 → 정수 (비었으면 nullopt, 쉼표 · 공백은 NumberFormat이 무시)
        [[nodiscard]] std::optional<int> ParseInteger(const std::wstring& text)
        {
            if (TrimSpaces(text).empty()) return std::nullopt;
            return static_cast<int>(Core::NumberFormat::ParseNumber(text));
        }

        // 선택지 번호 ↔ 보석 레벨 (1번 = 6레벨 ... 5번 = 10레벨)
        [[nodiscard]] int ToGemLevel(std::size_t index) noexcept
        {
            return MIN_GEM_LEVEL + static_cast<int>(index - FIRST_GEM_LEVEL_INDEX);
        }

        [[nodiscard]] std::size_t ToGemIndex(int level) noexcept
        {
            return FIRST_GEM_LEVEL_INDEX + static_cast<std::size_t>(level - MIN_GEM_LEVEL);
        }

        [[nodiscard]] bool IsGemLevelInRange(int level) noexcept
        {
            return level >= MIN_GEM_LEVEL && level <= MAX_GEM_LEVEL;
        }

        // 개별 설정 선택지 번호 (현재 유지 + 6 ~ 10레벨 다음)
        [[nodiscard]] std::size_t GetGemIndividualIndex() noexcept
        {
            return ToGemIndex(MAX_GEM_LEVEL) + 1;
        }

        [[nodiscard]] std::wstring_view GetFactorName(Core::CombatFactorKind kind) noexcept
        {
            switch (kind)
            {
                case Core::CombatFactorKind::FACTOR_GEAR:
                    return FACTOR_GEAR_TEXT;
                case Core::CombatFactorKind::FACTOR_WEAPON_QUALITY:
                    return FACTOR_QUALITY_TEXT;
                case Core::CombatFactorKind::FACTOR_GEM:
                    return FACTOR_GEM_TEXT;
                case Core::CombatFactorKind::FACTOR_VAMBRACE:
                    return FACTOR_VAMBRACE_TEXT;
                case Core::CombatFactorKind::FACTOR_ACCESSORY:
                    return FACTOR_ACCESSORY_TEXT;
                case Core::CombatFactorKind::FACTOR_BRACELET:
                    return FACTOR_BRACELET_TEXT;
                case Core::CombatFactorKind::FACTOR_ENGRAVING:
                    return FACTOR_ENGRAVING_TEXT;
                case Core::CombatFactorKind::FACTOR_AVATAR:
                    return FACTOR_AVATAR_TEXT;
                case Core::CombatFactorKind::FACTOR_ARK_CORE:
                    return FACTOR_ARK_CORE_TEXT;
                case Core::CombatFactorKind::FACTOR_ARK_GEM:
                    return FACTOR_ARK_GEM_TEXT;
                case Core::CombatFactorKind::FACTOR_ARK_PASSIVE:
                    return FACTOR_ARK_PASSIVE_TEXT;
            }
            return EMPTY_TEXT;
        }

        // 부호를 붙인 글자 앞부분 (양수 = "+", 그 밖 = 없음 → 음수는 숫자에 '-'가 붙어 있음)
        [[nodiscard]] std::wstring_view GetSignText(double value) noexcept
        {
            return value > CHANGE_EPSILON ? PLUS_TEXT : NO_SIGN_TEXT;
        }

        [[nodiscard]] std::wstring FormatEnhancement(int enhancement)
        {
            return std::format(ENHANCEMENT_FORMAT, enhancement);
        }

        // 상급 칸 글자 (상급 재련이 없는 분류 · 단계면 "-")
        [[nodiscard]] std::wstring FormatAdvanced(const Models::GearState& gear)
        {
            if (!gear.IsKnown() || (gear.m_advancedEnhancement <= 0 && !Models::CombatGear::CanAdvance(gear))) return std::wstring{ EMPTY_TEXT };
            return std::to_wstring(gear.m_advancedEnhancement);
        }

        // 무기고의 장비 하나 → 현재 상태 (계수 표에 없거나 지금 단계를 표에서 찾을 수 없으면 모르는 장비)
        [[nodiscard]] Models::GearState MakeGearState(Models::GearPart part, const Models::GearItem& item)
        {
            Models::GearState gear{ Models::CombatGear::DetectTableKey(part, item.m_name, item.m_grade), item.m_enhancement, item.m_advancedEnhancement };
            if (gear.IsKnown() && !Models::CombatGear::GetStat(gear).has_value()) gear.m_tableKey.clear();
            return gear;
        }
    }

    CombatSimulatorManager::CombatSimulatorManager() = default;

    CombatSimulatorManager::~CombatSimulatorManager() = default;

    std::wstring_view CombatSimulatorManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void CombatSimulatorManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 마지막 캐릭터 이름 · 목표 (CombatSimulatorData.json) + 배율 계산기
        m_store = std::make_unique<Services::CombatSimulatorStore>();
        if (const std::optional<std::string>& warning{ m_store->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(*warning), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }
        m_calculator = std::make_unique<Core::CombatPowerCalculator>();

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_editor = std::make_unique<CombatExtraEditor>();
        m_view = std::make_unique<CombatSimulatorView>(context.m_platform);
        m_view->SetOnSearch([this](const std::wstring& name) { RequestSearch(name); });
        m_view->SetOnGearCellEdited([this](std::size_t row, CombatGearColumn column, const std::wstring& text) { OnGearCellEdited(row, column, text); });
        m_view->SetOnQualityChanged([this](const std::wstring& text) { OnQualityChanged(text); });
        m_view->SetOnGemTargetSelected([this](std::size_t index) { OnGemTargetSelected(index); });
        m_view->SetOnGemCellEdited([this](std::size_t row, const std::wstring& text) { OnGemCellEdited(row, text); });
        m_view->SetOnCardEdited([this](CombatCard card, std::size_t row, const std::wstring& text) { OnCardEdited(card, row, text); });

        // 보석 목표 선택지: 현재 유지 + 6 ~ 10레벨 + 개별 설정 (보석 표에서 하나씩 바꾸면 이 항목이 표시됨)
        std::vector<std::wstring> gemItems;
        gemItems.emplace_back(GEM_KEEP_TEXT);
        for (int level{ MIN_GEM_LEVEL }; level <= MAX_GEM_LEVEL; ++level) gemItems.push_back(std::format(GEM_LEVEL_FORMAT, level));
        gemItems.emplace_back(GEM_INDIVIDUAL_TEXT);
        m_view->SetGemTargetItems(std::move(gemItems));

        // 마지막으로 조회한 이름을 넣어 둠 (조회는 사용자가 누를 때만 → 호출 한도 절약)
        m_view->SetCharacterName(ToWideMessage(m_store->GetSettings().m_characterName));
        RefreshAll();
    }

    void CombatSimulatorManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에)
        m_armoryTask.Wait();

        m_view.reset();
        m_editor.reset();
        m_calculator.reset();
        m_store.reset();
        m_context = nullptr;
    }

    void CombatSimulatorManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void CombatSimulatorManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();
    }

    void CombatSimulatorManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);
        if (m_armoryTask.IsReady()) FinishSearch();
    }

    void CombatSimulatorManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool CombatSimulatorManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape CombatSimulatorManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    // ===================== 조회 =====================

    void CombatSimulatorManager::RequestSearch(const std::wstring& characterName)
    {
        const std::wstring_view trimmedName{ TrimSpaces(characterName) };
        if (trimmedName.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        if (m_armoryTask.IsRunning()) return;   // 이미 조회 중이면 겹치지 않게 건너뜀

        // 남은 호출이 안전 여유분 이하면 건너뜀 (다른 툴과 키를 함께 쓰므로)
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        if (rate.IsExhausted(now))
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE },
                std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))), Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        m_searchError.reset();
        m_view->SetSearchBusy(true);
        m_view->SetStatusText(std::format(SEARCHING_FORMAT, trimmedName), false);

        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_armoryTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetArmory(name); });
    }

    // 조회가 끝남 → 무기고 반영 · 오류 처리
    void CombatSimulatorManager::FinishSearch()
    {
        m_view->SetSearchBusy(false);
        std::optional<std::string> keyError;

        try
        {
            ApplyArmory(m_armoryTask.TakeResult());   // 백그라운드에서 난 예외는 여기서 다시 던져짐
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            keyError = error.what();
            m_searchError = ToWideMessage(error.what());
        }
        catch (const Core::APIRateLimitException& error)
        {
            m_searchError = ToWideMessage(error.what());
            m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_WARNING, nullptr);
        }
        catch (const Core::APIMaintenanceException& error)
        {
            m_searchError = ToWideMessage(error.what());
            m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_INFO, nullptr);
        }
        catch (const std::exception& error)
        {
            m_searchError = ToWideMessage(error.what());   // 없는 캐릭터 · 통신 오류 등
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_ERROR, nullptr);
        }

        RefreshAll();

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value()) HandleAPIKeyError(*keyError);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (다른 탭과 같음)
    void CombatSimulatorManager::HandleAPIKeyError(const std::string& message)
    {
        const std::wstring reason{ ToWideMessage(message) };
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

    // 무기고 → 현재 상태, 목표 = 현재 (같은 캐릭터를 다시 조회했으면 저장된 목표를 범위 안으로 맞춰서 복원)
    void CombatSimulatorManager::ApplyArmory(Models::CharacterArmory armory)
    {
        Models::CombatState current;
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
        {
            if (const std::optional<Models::GearItem>& item{ armory.m_gears[index] }; item.has_value())
            {
                current.m_gears[index] = MakeGearState(Models::ToGearPart(index), *item);
            }
        }

        // 무기 품질: 에스더 무기는 품질이 없음
        const std::optional<Models::GearItem>& weapon{ armory.m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)] };
        const bool isEstherWeapon{ weapon.has_value() && (weapon->m_grade == ESTHER_GRADE_NAME
            || Models::CombatGear::IsEsther(current.m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)].m_tableKey)) };
        if (weapon.has_value() && !isEstherWeapon) current.m_weaponQuality = weapon->m_quality;

        current.m_gemLevels = armory.m_gemLevels;
        current.m_totalMainStat = armory.m_totalMainStat * Models::CombatCoefficientData::GetMainStatCalibration();   // 로아랩 실측 보정
        current.m_gemBaseAttackRate = armory.m_gemBaseAttackRate;
        current.m_weaponAttack = weapon.has_value() ? weapon->m_weaponAttack : 0.0;
        current.m_itemLevel = armory.m_itemAvgLevel;
        current.m_extra = armory.m_extra;
        current.m_extra.m_isEstherWeapon = isEstherWeapon;   // 완갑 표 · 무기 공격력 옵션 계수 선택

        Models::CombatState target{ current };
        Services::CombatSimulatorSettings& settings{ m_store->GetSettings() };
        if (settings.m_characterName == armory.m_characterName)
        {
            for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
            {
                Models::GearState& gear{ target.m_gears[index] };
                const std::optional<Services::CombatGearTarget>& saved{ settings.m_gearTargets[index] };
                if (!gear.IsKnown() || !saved.has_value()) continue;
                gear.m_enhancement = saved->m_enhancement;
                gear.m_advancedEnhancement = saved->m_advancedEnhancement;
                gear = Models::CombatGear::Clamp(gear);
            }
            if (target.m_weaponQuality.has_value() && settings.m_weaponQualityTarget.has_value())
            {
                target.m_weaponQuality = std::clamp(*settings.m_weaponQualityTarget, MIN_QUALITY, MAX_QUALITY);
            }

            // 보석: 저장된 개수가 지금 장착 개수와 같을 때만 복원 (보석을 바꿔 끼웠으면 현재 유지)
            if (settings.m_gemTargets.size() == target.m_gemLevels.size())
            {
                for (std::size_t index{}; index < target.m_gemLevels.size(); ++index)
                {
                    // 6 ~ 10만 목표로 복원 (그 밖의 값 = 현재 그대로 둔 보석 → 현재 레벨 유지)
                    if (IsGemLevelInRange(settings.m_gemTargets[index])) target.m_gemLevels[index] = settings.m_gemTargets[index];
                }
            }

            // 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 · 완갑: 저장된 목표 (완갑이 없어졌거나 새로 생겼으면 현재 기준)
            if (settings.m_extraTarget.has_value())
            {
                Models::ExtraState saved{ *settings.m_extraTarget };
                saved.m_isEstherWeapon = current.m_extra.m_isEstherWeapon;
                if (!current.m_extra.m_vambraceLevel.has_value()) saved.m_vambraceLevel.reset();
                else if (!saved.m_vambraceLevel.has_value()) saved.m_vambraceLevel = current.m_extra.m_vambraceLevel;
                target.m_extra = Models::CombatExtra::Clamp(saved);
            }
        }
        else
        {
            // 다른 캐릭터: 목표 초기화
            settings = Services::CombatSimulatorSettings{};
            settings.m_characterName = armory.m_characterName;
        }

        m_current = std::move(current);
        m_target = std::move(target);
        m_armory = std::move(armory);
        StoreTargets();
        SaveData();
    }

    // ===================== 사용자 입력 =====================

    // 목표 일반 · 상급 숫자 입력 → 표 범위 안으로 맞춰서 반영
    void CombatSimulatorManager::OnGearCellEdited(std::size_t row, CombatGearColumn column, const std::wstring& text)
    {
        if (row == Models::GEAR_PART_COUNT)
        {
            OnVambraceEdited(column, text);   // 마지막 행 = 완갑
            return;
        }
        if (row > Models::GEAR_PART_COUNT) return;
        Models::GearState& gear{ m_target.m_gears[row] };
        const std::optional<int> value{ ParseInteger(text) };
        if (!gear.IsKnown() || !value.has_value())
        {
            RefreshGears();   // 숫자가 아니면 원래 값으로 되돌림
            return;
        }

        Models::GearState edited{ gear };
        if (column == CombatGearColumn::COLUMN_TARGET_ENHANCEMENT) edited.m_enhancement = *value;
        else if (column == CombatGearColumn::COLUMN_TARGET_ADVANCED) edited.m_advancedEnhancement = *value;
        else return;

        gear = Models::CombatGear::Clamp(edited);   // 일반 단계가 내려가 상급을 못 넣게 되면 상급은 0
        StoreTargets();
        SaveData();
        RefreshGears();
        RefreshResult();
    }

    // 무기 품질 목표: 숫자일 때만 반영 (0 ~ 100으로 맞추고, 넘으면 칸도 고침)
    void CombatSimulatorManager::OnQualityChanged(const std::wstring& text)
    {
        if (!m_target.m_weaponQuality.has_value()) return;
        const std::optional<int> value{ ParseInteger(text) };
        if (!value.has_value()) return;

        const int quality{ std::clamp(*value, MIN_QUALITY, MAX_QUALITY) };
        if (quality != *value) m_view->SetQualityTarget(std::to_wstring(quality));
        if (quality == *m_target.m_weaponQuality) return;

        m_target.m_weaponQuality = quality;
        StoreTargets();
        SaveData();
        RefreshResult();
    }

    // 모든 보석 목표: 현재 유지 = 현재 레벨로 되돌림, N레벨 = 모두 N, 개별 설정 = 그대로 (표에서 고친 값 유지)
    void CombatSimulatorManager::OnGemTargetSelected(std::size_t index)
    {
        if (index >= GetGemIndividualIndex())
        {
            RefreshGems();
            return;
        }

        m_target.m_gemLevels = m_current.m_gemLevels;
        if (index != GEM_KEEP_INDEX) std::ranges::fill(m_target.m_gemLevels, ToGemLevel(index));   // 6레벨 미만 보석도 고른 레벨로 (그 보석 배율은 1)
        StoreTargets();
        SaveData();
        RefreshGems();
        RefreshResult();
    }

    // 보석 하나 목표 레벨 입력 → 1 ~ 10으로 맞춰서 그 보석만 덮어씀
    void CombatSimulatorManager::OnGemCellEdited(std::size_t row, const std::wstring& text)
    {
        const std::optional<int> value{ ParseInteger(text) };
        if (row >= m_target.m_gemLevels.size() || !value.has_value())
        {
            RefreshGems();   // 숫자가 아니면 원래 값으로 되돌림
            return;
        }

        m_target.m_gemLevels[row] = std::clamp(*value, MIN_GEM_LEVEL, MAX_GEM_LEVEL);   // 6 ~ 10 (현재가 6 미만인 보석은 배율 1)
        StoreTargets();
        SaveData();
        RefreshGems();
        RefreshResult();
    }

    // 완갑 목표 재련 입력 (0 ~ 25로 맞춤, 완갑이 없거나 숫자가 아니면 원래 값으로)
    void CombatSimulatorManager::OnVambraceEdited(CombatGearColumn column, const std::wstring& text)
    {
        const std::optional<int> value{ ParseInteger(text) };
        if (column != CombatGearColumn::COLUMN_TARGET_ENHANCEMENT || !m_target.m_extra.m_vambraceLevel.has_value() || !value.has_value())
        {
            RefreshGears();
            return;
        }

        m_target.m_extra.m_vambraceLevel = std::clamp(*value, Models::MIN_VALUE, MAX_VAMBRACE_TARGET);
        StoreTargets();
        SaveData();
        RefreshGears();
        RefreshResult();
    }

    // 목록형 카드 입력 (편집기가 목표 상태에 반영 → 범위 맞춤 → 저장 · 화면 갱신)
    void CombatSimulatorManager::OnCardEdited(CombatCard card, std::size_t row, const std::wstring& text)
    {
        if (!m_editor) return;
        Models::ExtraState edited{ m_target.m_extra };
        if (m_editor->Apply(card, row, text, edited)) m_target.m_extra = Models::CombatExtra::Clamp(edited);

        StoreTargets();
        SaveData();
        RefreshGears();   // 완갑 표시는 장비 표에 있어서 함께 갱신
        RefreshCards();
        RefreshResult();
    }

    // ===================== 화면 갱신 · 저장 =====================

    void CombatSimulatorManager::RefreshAll()
    {
        RefreshStatus();
        RefreshGears();
        RefreshOptions();
        RefreshCards();
        RefreshResult();
    }

    // 상태 줄: 조회 중이면 그대로, 오류 · 조회 전 · 캐릭터 요약
    void CombatSimulatorManager::RefreshStatus()
    {
        if (!m_view || m_armoryTask.IsRunning()) return;

        if (m_searchError.has_value())
        {
            m_view->SetStatusText(std::format(SEARCH_ERROR_FORMAT, *m_searchError), true);
            return;
        }
        if (!m_armory.has_value())
        {
            m_view->SetStatusText(std::wstring{ NOT_SEARCHED_TEXT }, false);
            return;
        }

        const std::wstring power{ m_armory->m_combatPower.has_value()
            ? Core::NumberFormat::FormatNumber(*m_armory->m_combatPower, POWER_DECIMALS) : std::wstring{ NO_POWER_TEXT } };
        m_view->SetStatusText(std::format(STATUS_FORMAT, ToWideMessage(m_armory->m_characterName), ToWideMessage(m_armory->m_className),
            Core::NumberFormat::FormatNumber(m_armory->m_itemAvgLevel, LEVEL_DECIMALS), power), false);
    }

    // 장비 표: 부위마다 한 행 (조회 전 · 안 낀 부위는 "-", 계수 표가 없는 장비는 목표 입력 잠금)
    void CombatSimulatorManager::RefreshGears()
    {
        if (!m_view) return;

        std::vector<CombatGearRow> rows;
        rows.reserve(Models::GEAR_PART_COUNT);
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
        {
            CombatGearRow& row{ rows.emplace_back() };
            row.m_partName = ToWideMessage(Models::GetGearPartType(Models::ToGearPart(index)));
            row.m_itemName = std::wstring{ EMPTY_TEXT };
            row.m_currentEnhancement = row.m_currentAdvanced = row.m_targetEnhancement = row.m_targetAdvanced = std::wstring{ EMPTY_TEXT };

            const std::optional<Models::GearItem>* item{ m_armory.has_value() ? &m_armory->m_gears[index] : nullptr };
            if (item == nullptr || !item->has_value()) continue;

            row.m_itemName = ToWideMessage((*item)->m_name);
            row.m_gradeName = (*item)->m_grade;
            row.m_currentEnhancement = FormatEnhancement((*item)->m_enhancement);
            row.m_currentAdvanced = (*item)->m_advancedEnhancement > 0 ? std::to_wstring((*item)->m_advancedEnhancement) : std::wstring{ EMPTY_TEXT };

            const Models::GearState& current{ m_current.m_gears[index] };
            const Models::GearState& target{ m_target.m_gears[index] };
            if (!target.IsKnown()) continue;   // 계수 표가 없는 장비: 목표 "-"

            row.m_currentAdvanced = FormatAdvanced(current);
            row.m_targetEnhancement = FormatEnhancement(target.m_enhancement);
            row.m_targetAdvanced = FormatAdvanced(target);
            row.m_isTargetEditable = true;
            row.m_isTargetAdvancedEditable = Models::CombatGear::CanAdvance(target);
            row.m_hasChange = target != current;
        }

        // 마지막 행: 완갑 재련 (주스탯이 장비 재련과 함께 계산됨, 상급 재련 없음)
        CombatGearRow& vambrace{ rows.emplace_back() };
        vambrace.m_partName = std::wstring{ VAMBRACE_PART_TEXT };
        vambrace.m_itemName = std::wstring{ EMPTY_TEXT };
        vambrace.m_currentEnhancement = vambrace.m_currentAdvanced = vambrace.m_targetEnhancement = vambrace.m_targetAdvanced = std::wstring{ EMPTY_TEXT };
        if (m_current.m_extra.m_vambraceLevel.has_value() && m_target.m_extra.m_vambraceLevel.has_value())
        {
            vambrace.m_itemName = std::wstring{ VAMBRACE_ITEM_TEXT };
            vambrace.m_gradeName = std::string{ VAMBRACE_GRADE_NAME };
            vambrace.m_currentEnhancement = FormatEnhancement(*m_current.m_extra.m_vambraceLevel);
            vambrace.m_targetEnhancement = FormatEnhancement(*m_target.m_extra.m_vambraceLevel);
            vambrace.m_isTargetEditable = true;
            vambrace.m_hasChange = *m_target.m_extra.m_vambraceLevel != *m_current.m_extra.m_vambraceLevel;
        }
        m_view->ShowGears(rows);
    }

    // 악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타 카드 (행 글자는 편집기가 만듦)
    void CombatSimulatorManager::RefreshCards()
    {
        if (!m_view || !m_editor) return;

        for (std::size_t index{}; index < static_cast<std::size_t>(CombatCard::CARD_COUNT); ++index)
        {
            const CombatCard card{ static_cast<CombatCard>(index) };
            m_view->ShowCard(card, m_editor->BuildRows(card, m_current.m_extra, m_target.m_extra));
        }
    }

    // 무기 품질 · 보석 상자
    void CombatSimulatorManager::RefreshOptions()
    {
        if (!m_view) return;

        // 품질: 에스더 · 모름 · 조회 전이면 입력 잠금
        const bool hasQuality{ m_current.m_weaponQuality.has_value() && m_target.m_weaponQuality.has_value() };
        const std::optional<Models::GearItem>* weapon{ m_armory.has_value() ? &m_armory->m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)] : nullptr };
        const bool isEsther{ weapon != nullptr && weapon->has_value() && (*weapon)->m_grade == ESTHER_GRADE_NAME };
        if (hasQuality) m_view->SetQualityText(std::format(QUALITY_FORMAT, *m_current.m_weaponQuality));
        else m_view->SetQualityText(std::wstring{ isEsther ? QUALITY_ESTHER_TEXT : QUALITY_UNKNOWN_TEXT });
        m_view->SetQualityTarget(hasQuality ? std::to_wstring(*m_target.m_weaponQuality) : std::wstring{});
        m_view->SetQualityEnabled(hasQuality);

        // 보석: "보석 11개 · 10레벨 5개 · 9레벨 6개" (높은 레벨부터)
        if (!m_armory.has_value()) m_view->SetGemText(std::wstring{ GEM_NOT_LOADED_TEXT });
        else if (m_current.m_gemLevels.empty()) m_view->SetGemText(std::wstring{ NO_GEM_TEXT });
        else
        {
            std::map<int, std::size_t, std::greater<>> counts;
            for (const int level : m_current.m_gemLevels) ++counts[level];

            std::wstring text{ std::format(GEM_SUMMARY_FORMAT, m_current.m_gemLevels.size()) };
            for (const auto& [level, count] : counts) text += std::format(GEM_COUNT_FORMAT, level, count);
            m_view->SetGemText(std::move(text));
        }
        m_view->SetGemTargetEnabled(m_armory.has_value() && !m_current.m_gemLevels.empty());
        RefreshGems();
    }

    // 보석 표: 장착 순서대로 슬롯 · 종류 · 현재 · 목표 (조회 전 · 보석 없음이면 빈 표)
    void CombatSimulatorManager::RefreshGems()
    {
        if (!m_view) return;

        std::vector<CombatGemRow> rows;
        const std::size_t count{ m_armory.has_value() ? std::min(m_current.m_gemLevels.size(), m_target.m_gemLevels.size()) : 0 };
        rows.reserve(count);
        for (std::size_t index{}; index < count; ++index)
        {
            const std::string_view kind{ index < m_armory->m_gemKinds.size() ? std::string_view{ m_armory->m_gemKinds[index] } : std::string_view{} };
            rows.push_back(CombatGemRow{ std::format(GEM_SLOT_FORMAT, index + 1), kind.empty() ? std::wstring{ EMPTY_TEXT } : ToWideMessage(kind),
                std::to_wstring(m_current.m_gemLevels[index]), std::to_wstring(m_target.m_gemLevels[index]),
                m_target.m_gemLevels[index] != m_current.m_gemLevels[index] });
        }
        m_view->ShowGems(rows);
        m_view->SetGemTargetIndex(GetGemTargetIndex());
    }

    // 목표가 현재와 같으면 현재 유지, 모두 같은 레벨이면 그 레벨, 그 밖은 개별 설정
    std::size_t CombatSimulatorManager::GetGemTargetIndex() const
    {
        const std::vector<int>& targets{ m_target.m_gemLevels };
        if (targets.empty() || targets == m_current.m_gemLevels) return GEM_KEEP_INDEX;
        if (IsGemLevelInRange(targets.front()) && std::ranges::all_of(targets, [first = targets.front()](int level) { return level == first; }))
        {
            return ToGemIndex(targets.front());
        }
        return GetGemIndividualIndex();
    }

    // 결과 상자: 현재 · 예상 전투력, 증감, 예상 아이템 레벨, 요소별 배율
    void CombatSimulatorManager::RefreshResult()
    {
        if (!m_view) return;

        CombatResultText text;
        if (!m_armory.has_value())
        {
            text.m_currentPower = std::format(CURRENT_POWER_FORMAT, EMPTY_TEXT);
            text.m_predictedPower = std::format(PREDICTED_POWER_FORMAT, EMPTY_TEXT);
            m_view->ShowResult(text);
            return;
        }

        const double currentPower{ m_armory->m_combatPower.value_or(0.0) };
        const Core::CombatPowerResult result{ m_calculator->Calculate(currentPower, m_current, m_target) };

        if (m_armory->m_combatPower.has_value())
        {
            text.m_currentPower = std::format(CURRENT_POWER_FORMAT, Core::NumberFormat::FormatNumber(currentPower, POWER_DECIMALS));
            text.m_predictedPower = std::format(PREDICTED_POWER_FORMAT, Core::NumberFormat::FormatNumber(result.m_predictedPower, POWER_DECIMALS));
            const double rate{ (result.m_totalFactor - NO_CHANGE_FACTOR) * PERCENT_SCALE };
            text.m_change = std::format(CHANGE_FORMAT, GetSignText(result.m_delta), Core::NumberFormat::FormatNumber(result.m_delta, POWER_DECIMALS), GetSignText(rate), rate);
        }
        else
        {
            text.m_currentPower = std::format(CURRENT_POWER_FORMAT, NO_POWER_TEXT);
            text.m_predictedPower = std::format(PREDICTED_FACTOR_FORMAT, result.m_totalFactor);
            text.m_change = std::wstring{ NO_POWER_CHANGE_TEXT };
        }
        text.m_isUp = result.m_totalFactor > NO_CHANGE_FACTOR + CHANGE_EPSILON;
        text.m_isDown = result.m_totalFactor < NO_CHANGE_FACTOR - CHANGE_EPSILON;
        text.m_itemLevel = std::format(ITEM_LEVEL_FORMAT, Core::NumberFormat::FormatNumber(result.m_currentItemLevel, LEVEL_DECIMALS),
            Core::NumberFormat::FormatNumber(result.m_predictedItemLevel, LEVEL_DECIMALS));

        // 배율 내역: 1이 아닌 요소만 (모두 1이면 변화 없음)
        text.m_factors = std::wstring{ FACTOR_PREFIX };
        bool hasFactor{};
        for (const Core::CombatFactorResult& factor : result.m_factors)
        {
            if (std::abs(factor.m_factor - NO_CHANGE_FACTOR) <= FACTOR_DISPLAY_EPSILON) continue;
            if (hasFactor) text.m_factors += FACTOR_SEPARATOR;
            text.m_factors += std::format(FACTOR_ITEM_FORMAT, GetFactorName(factor.m_kind), factor.m_factor);
            hasFactor = true;
        }
        if (!hasFactor) text.m_factors += NO_FACTOR_CHANGE_TEXT;
        m_view->ShowResult(text);
    }

    // 목표 → 저장 설정 (계수 표가 있는 부위만)
    void CombatSimulatorManager::StoreTargets()
    {
        Services::CombatSimulatorSettings& settings{ m_store->GetSettings() };
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
        {
            const Models::GearState& gear{ m_target.m_gears[index] };
            settings.m_gearTargets[index] = gear.IsKnown()
                ? std::optional<Services::CombatGearTarget>{ Services::CombatGearTarget{ gear.m_enhancement, gear.m_advancedEnhancement } } : std::nullopt;
        }
        settings.m_weaponQualityTarget = m_target.m_weaponQuality;
        settings.m_gemTargets = m_target.m_gemLevels != m_current.m_gemLevels ? m_target.m_gemLevels : std::vector<int>{};   // 현재와 같으면 저장 안 함
        settings.m_extraTarget = m_target.m_extra != m_current.m_extra ? std::optional<Models::ExtraState>{ m_target.m_extra } : std::nullopt;   // 현재와 같으면 저장 안 함
    }

    void CombatSimulatorManager::SaveData()
    {
        try
        {
            m_store->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }
}
