module LostArkMultiTool.Services.RefineCalculator;

import std;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.RefinePlan;

namespace LostArkMultiTool::Services::RefineCalculator
{
    namespace
    {
        using Models::RefineData::AdvancedBlock;
        using Models::RefineData::AdvancedTryRow;
        using Models::RefineData::EquipmentKind;
        using Models::RefineData::EquipmentState;
        using Models::RefineData::EquipmentTier;
        using Models::RefineData::MaterialAmount;
        using Models::RefineData::NormalStep;
        using Models::RefineData::RefineMaterial;

        // ===== 일반 재련 규칙 =====
        constexpr double PPM_SCALE{ 1'000'000.0 };            // 확률을 정수(백만분율)로 바꿔 장인의 기운을 정확히 나눔
        constexpr double ARTISAN_ENERGY_FULL{ 2.15 };         // 실패한 시도의 확률 합이 2.15가 되면 장인의 기운 100% (실패마다 확률 × 0.465)
        constexpr double FAILURE_BONUS_RATE{ 0.1 };           // 실패 1회마다 기본 확률의 10%씩 오름
        constexpr int MAX_FAILURE_BONUS_COUNT{ 10 };          // 최대 10회 (기본 확률의 2배까지)
        constexpr std::size_t ATTEMPT_STAGE_COUNT{ MAX_FAILURE_BONUS_COUNT + 1 };   // 1~10번째 시도 + 11번째 이후(같은 확률)
        constexpr double MAX_PROBABILITY{ 1.0 };
        constexpr double MIN_REACH_PROBABILITY{ 1e-15 };      // 이보다 작은 도달 확률은 무시 (기대값에 영향 없음)
        constexpr double COST_EPSILON{ 1e-9 };                // 같은 비용 비교 허용 오차
        constexpr int BOOK_USE_OPTIONS{ 2 };                  // 책: 안 씀 / 씀

        // ===== 상급 재련 =====
        constexpr int BOOK_K_WEIGHT{ 2 };                     // 표의 K = 숨결 사용(0 · 1) + 2 × 책 사용(0 · 1)
        constexpr int SUPPORT_USE_OPTIONS{ 2 };               // 턴마다 숨결 · 책을 넣거나 안 넣거나
        constexpr double ADVANCED_STEPS_PER_BLOCK{ static_cast<double>(Models::RefineData::ADVANCED_BLOCK_SIZE) };

        // ===== 경로 =====
        constexpr int INHERIT_REQUIRED_ITEM_LEVEL{ 1730 };   // 계승은 평균 아이템 레벨 1730 이상에서
        constexpr std::size_t TIER_AEGIR_INDEX{ 0 };
        constexpr std::size_t TIER_SERCA_INDEX{ 1 };
        constexpr std::size_t KIND_ARMOR_INDEX{ 0 };
        constexpr std::size_t KIND_WEAPON_INDEX{ 1 };

        // 같은 효율일 때 진행 순서 (무기 → 장갑 → 어깨 → 투구 → 하의 → 상의, 로아업 순서)
        constexpr std::array<Models::RefineSlot, Models::REFINE_SLOT_COUNT> STEP_SLOT_ORDER{
            Models::RefineSlot::SLOT_WEAPON, Models::RefineSlot::SLOT_GLOVES, Models::RefineSlot::SLOT_SHOULDER,
            Models::RefineSlot::SLOT_HELMET, Models::RefineSlot::SLOT_PANTS, Models::RefineSlot::SLOT_CHEST
        };

        [[nodiscard]] std::int64_t ToPPM(double probability) noexcept
        {
            return std::llround(probability * PPM_SCALE);
        }

        [[nodiscard]] constexpr std::size_t ToSlotIndex(Models::RefineSlot slot) noexcept
        {
            return static_cast<std::size_t>(slot);
        }

        [[nodiscard]] constexpr std::size_t ToTierIndex(EquipmentTier tier) noexcept
        {
            return tier == EquipmentTier::TIER_AEGIR ? TIER_AEGIR_INDEX : TIER_SERCA_INDEX;
        }

        [[nodiscard]] constexpr std::size_t ToKindIndex(EquipmentKind kind) noexcept
        {
            return kind == EquipmentKind::KIND_ARMOR ? KIND_ARMOR_INDEX : KIND_WEAPON_INDEX;
        }

        // 비용 계산에 쓰는 가격 (귀속 재료 = 0)
        [[nodiscard]] double GetCostPrice(const Models::RefinePrices& prices, RefineMaterial material) noexcept
        {
            const std::size_t index{ Models::RefineData::ToIndex(material) };
            return prices.m_isBound[index] ? 0.0 : prices.m_prices[index];
        }

        [[nodiscard]] double SumCost(const std::vector<MaterialAmount>& materials, const Models::RefinePrices& prices) noexcept
        {
            double cost{};
            for (const MaterialAmount& material : materials) cost += material.m_amount * GetCostPrice(prices, material.m_material);
            return cost;
        }

        void AddMaterials(Models::MaterialAmounts& target, const std::vector<MaterialAmount>& materials, double multiplier) noexcept
        {
            for (const MaterialAmount& material : materials) target[Models::RefineData::ToIndex(material.m_material)] += material.m_amount * multiplier;
        }

        void AddMaterial(Models::MaterialAmounts& target, RefineMaterial material, double amount) noexcept
        {
            target[Models::RefineData::ToIndex(material)] += amount;
        }

        void AddAmounts(Models::MaterialAmounts& target, const Models::MaterialAmounts& source, double multiplier) noexcept
        {
            for (std::size_t index{}; index < target.size(); ++index) target[index] += source[index] * multiplier;
        }

        // ===================== 일반 재련 1단계 =====================

        // 한 번의 시도에서 고를 수 있는 보조 재료 조합 (숨결 개수 · 책)
        struct SupportAction
        {
            int m_breathCount{};
            bool m_isBookUsed{};
            std::int64_t m_units{};     // 추가 확률 (단위 수)
            double m_supportCost{};     // 보조 재료 비용
        };

        // 장인의 기운 · 확률을 같은 정수 단위로 나눌 최대 공약수 (모든 확률이 이 단위의 배수가 되게)
        [[nodiscard]] std::int64_t FindProbabilityUnit(const NormalStep& step)
        {
            std::vector<std::int64_t> values{ ToPPM(ARTISAN_ENERGY_FULL), ToPPM(step.m_baseProbability),
                ToPPM(step.m_baseProbability * FAILURE_BONUS_RATE) };
            if (step.m_breath.has_value()) values.push_back(ToPPM(step.m_breath->m_probability));
            if (step.m_book.has_value()) values.push_back(ToPPM(step.m_book->m_probability));

            std::int64_t unit{};
            for (const std::int64_t value : values)
            {
                if (value > 0) unit = std::gcd(unit, value);
            }
            return std::max<std::int64_t>(unit, 1);
        }

        // 보조 재료 조합 전체 (추가 확률이 같으면 싼 것만 남김)
        [[nodiscard]] std::vector<SupportAction> BuildSupportActions(const NormalStep& step, const Models::RefinePrices& prices, std::int64_t unit)
        {
            const int maxBreath{ step.m_breath.has_value() ? step.m_breath->m_maxCount : 0 };
            const int bookOptions{ step.m_book.has_value() ? BOOK_USE_OPTIONS : 1 };

            std::map<std::int64_t, SupportAction> bestByUnits;
            for (int breathCount{}; breathCount <= maxBreath; ++breathCount)
            {
                for (int bookUse{}; bookUse < bookOptions; ++bookUse)
                {
                    const bool isBookUsed{ bookUse > 0 };
                    std::int64_t ppm{};
                    double supportCost{};
                    if (breathCount > 0)
                    {
                        ppm += ToPPM(step.m_breath->m_probability) * breathCount;
                        supportCost += GetCostPrice(prices, step.m_breath->m_material) * breathCount;
                    }
                    if (isBookUsed)
                    {
                        ppm += ToPPM(step.m_book->m_probability);
                        supportCost += GetCostPrice(prices, step.m_book->m_material);
                    }

                    const SupportAction action{ breathCount, isBookUsed, ppm / unit, supportCost };
                    const auto found{ bestByUnits.find(action.m_units) };
                    if (found == bestByUnits.end() || action.m_supportCost < found->second.m_supportCost) bestByUnits.insert_or_assign(action.m_units, action);
                }
            }

            std::vector<SupportAction> actions;
            actions.reserve(bestByUnits.size());
            for (const auto& [units, action] : bestByUnits) actions.push_back(action);
            return actions;
        }

        // 일반 재련 1단계의 기대 비용 · 재료
        // 상태 = (몇 번째 시도인지, 쌓인 장인의 기운) → 시도마다 기대 비용이 가장 작은 보조 재료를 고름 (뒤에서부터 동적 계획법)
        [[nodiscard]] Models::RefineStepCost CalculateNormalStep(const NormalStep& step, const Models::RefinePrices& prices)
        {
            const std::int64_t unit{ FindProbabilityUnit(step) };
            const std::int64_t threshold{ ToPPM(ARTISAN_ENERGY_FULL) / unit };
            const std::int64_t maxUnits{ static_cast<std::int64_t>(PPM_SCALE) / unit };
            const double baseCost{ SumCost(step.m_materials, prices) };
            const std::vector<SupportAction> actions{ BuildSupportActions(step, prices, unit) };

            // 시도 순서별 기본 확률 (실패마다 +10%, 10회까지)
            std::array<std::int64_t, ATTEMPT_STAGE_COUNT> baseUnits{};
            for (std::size_t stage{}; stage < ATTEMPT_STAGE_COUNT; ++stage)
            {
                const double bonus{ 1.0 + FAILURE_BONUS_RATE * static_cast<double>(std::min<std::size_t>(stage, MAX_FAILURE_BONUS_COUNT)) };
                baseUnits[stage] = ToPPM(step.m_baseProbability * bonus) / unit;
            }

            const std::size_t energyCount{ static_cast<std::size_t>(threshold) };
            std::vector<double> values(ATTEMPT_STAGE_COUNT * energyCount);
            std::vector<std::size_t> choices(ATTEMPT_STAGE_COUNT * energyCount);
            const auto at{ [energyCount](std::size_t stage, std::int64_t energy) noexcept
            {
                return stage * energyCount + static_cast<std::size_t>(energy);
            } };

            // 마지막 단계(11번째 이후)는 자기 자신을 다음 단계로 씀 → 기운이 많은 쪽부터 채우면 항상 계산된 값을 참조
            for (std::size_t stage{ ATTEMPT_STAGE_COUNT }; stage-- > 0;)
            {
                const std::size_t nextStage{ std::min(stage + 1, ATTEMPT_STAGE_COUNT - 1) };
                for (std::int64_t energy{ threshold - 1 }; energy >= 0; --energy)
                {
                    double bestValue{ std::numeric_limits<double>::infinity() };
                    std::size_t bestAction{};
                    for (std::size_t actionIndex{}; actionIndex < actions.size(); ++actionIndex)
                    {
                        const std::int64_t successUnits{ baseUnits[stage] + actions[actionIndex].m_units };
                        if (successUnits > maxUnits) continue;   // 100%를 넘는 조합은 낭비

                        const double probability{ static_cast<double>(successUnits * unit) / PPM_SCALE };
                        const std::int64_t nextEnergy{ energy + successUnits };
                        const double failValue{ nextEnergy >= threshold ? baseCost : values[at(nextStage, nextEnergy)] };
                        const double value{ baseCost + actions[actionIndex].m_supportCost + (MAX_PROBABILITY - probability) * failValue };
                        if (value < bestValue - COST_EPSILON)
                        {
                            bestValue = value;
                            bestAction = actionIndex;
                        }
                    }
                    values[at(stage, energy)] = bestValue;
                    choices[at(stage, energy)] = bestAction;
                }
            }

            // 고른 전략대로 앞에서부터 따라가며 기대 재료 · 시도 횟수 (실패하면 상태가 하나로 정해지므로 한 줄로 이어짐)
            Models::RefineStepCost result{ values[at(0, 0)], 0.0, {} };
            double reach{ 1.0 };
            std::size_t stage{};
            std::int64_t energy{};
            while (energy < threshold && reach > MIN_REACH_PROBABILITY)
            {
                const SupportAction& action{ actions[choices[at(stage, energy)]] };
                const std::int64_t successUnits{ baseUnits[stage] + action.m_units };
                const double probability{ std::min(MAX_PROBABILITY, static_cast<double>(successUnits * unit) / PPM_SCALE) };

                result.m_expectedTries += reach;
                AddMaterials(result.m_materials, step.m_materials, reach);
                if (action.m_breathCount > 0) AddMaterial(result.m_materials, step.m_breath->m_material, reach * action.m_breathCount);
                if (action.m_isBookUsed) AddMaterial(result.m_materials, step.m_book->m_material, reach);

                reach *= MAX_PROBABILITY - probability;
                energy += successUnits;
                stage = std::min(stage + 1, ATTEMPT_STAGE_COUNT - 1);
            }

            // 장인의 기운 100% → 다음 시도는 반드시 성공 (보조 재료 없이)
            if (energy >= threshold && reach > MIN_REACH_PROBABILITY)
            {
                result.m_expectedTries += reach;
                AddMaterials(result.m_materials, step.m_materials, reach);
            }
            return result;
        }

        // ===================== 상급 재련 10단계 묶음 =====================

        constexpr std::size_t ADVANCED_TURN_KIND_COUNT{ 3 };   // 일반 · 선조 · 강화 선조 턴

        // 표에서 (일반 K, 선조 K, 강화 선조 K) 줄 찾기 (없는 조합이면 nullptr)
        [[nodiscard]] const AdvancedTryRow* FindTryRow(std::span<const AdvancedTryRow> rows, int normalK, int bonusK, int enhancedK) noexcept
        {
            const auto found{ std::ranges::find_if(rows, [normalK, bonusK, enhancedK](const AdvancedTryRow& row)
            {
                return row.m_normalK == normalK && row.m_bonusK == bonusK && row.m_enhancedBonusK == enhancedK;
            }) };
            return found == rows.end() ? nullptr : &*found;
        }

        // 상급 재련 묶음의 기대 비용 · 재료 (일반 · 선조 · 강화 선조 턴마다 숨결 · 책을 넣을지 모든 조합 중 최소)
        [[nodiscard]] Models::RefineStepCost CalculateAdvancedBlock(const AdvancedBlock& block, const Models::RefinePrices& prices)
        {
            const double baseCost{ SumCost(block.m_materials, prices) };
            const double breathCost{ block.m_breath.m_amount * GetCostPrice(prices, block.m_breath.m_material) };
            const double bookCost{ GetCostPrice(prices, block.m_book) };
            const int enhancedOptions{ block.m_hasEnhancedBonus ? SUPPORT_USE_OPTIONS : 1 };

            Models::RefineStepCost best{ std::numeric_limits<double>::infinity(), 0.0, {} };
            for (int normalBreath{}; normalBreath < SUPPORT_USE_OPTIONS; ++normalBreath)
            for (int normalBook{}; normalBook < SUPPORT_USE_OPTIONS; ++normalBook)
            for (int bonusBreath{}; bonusBreath < SUPPORT_USE_OPTIONS; ++bonusBreath)
            for (int bonusBook{}; bonusBook < SUPPORT_USE_OPTIONS; ++bonusBook)
            for (int enhancedBreath{}; enhancedBreath < enhancedOptions; ++enhancedBreath)
            for (int enhancedBook{}; enhancedBook < enhancedOptions; ++enhancedBook)
            {
                const AdvancedTryRow* row{ FindTryRow(block.m_tryRows, normalBreath + BOOK_K_WEIGHT * normalBook,
                    bonusBreath + BOOK_K_WEIGHT * bonusBook, enhancedBreath + BOOK_K_WEIGHT * enhancedBook) };
                if (row == nullptr) continue;

                const double normalSupport{ normalBreath * breathCost + normalBook * bookCost };
                const double bonusSupport{ bonusBreath * breathCost + bonusBook * bookCost };
                const double enhancedSupport{ enhancedBreath * breathCost + enhancedBook * bookCost };

                // 무료 일반 턴은 기본 재료 없이 보조 재료만 소모
                const double cost{ (baseCost + normalSupport) * row->m_paidNormalTry + normalSupport * row->m_freeNormalTry
                    + (baseCost + bonusSupport) * row->m_bonusTry + (baseCost + enhancedSupport) * row->m_enhancedBonusTry };
                if (cost >= best.m_expectedCost - COST_EPSILON) continue;

                const double normalTries{ row->m_freeNormalTry + row->m_paidNormalTry };
                const std::array<double, ADVANCED_TURN_KIND_COUNT> supportTries{ normalTries, row->m_bonusTry, row->m_enhancedBonusTry };
                const std::array<int, ADVANCED_TURN_KIND_COUNT> breathUses{ normalBreath, bonusBreath, enhancedBreath };
                const std::array<int, ADVANCED_TURN_KIND_COUNT> bookUses{ normalBook, bonusBook, enhancedBook };

                // 기본 재료는 유료 턴(일반 · 선조 · 강화 선조)만, 보조 재료는 넣은 턴 전부
                Models::RefineStepCost candidate{ cost, normalTries + row->m_bonusTry + row->m_enhancedBonusTry, {} };
                AddMaterials(candidate.m_materials, block.m_materials, row->m_paidNormalTry + row->m_bonusTry + row->m_enhancedBonusTry);
                for (std::size_t turn{}; turn < ADVANCED_TURN_KIND_COUNT; ++turn)
                {
                    AddMaterial(candidate.m_materials, block.m_breath.m_material, block.m_breath.m_amount * breathUses[turn] * supportTries[turn]);
                    AddMaterial(candidate.m_materials, block.m_book, bookUses[turn] * supportTries[turn]);
                }
                best = candidate;
            }
            return best;
        }

        // ===================== 경로 =====================

        // 부위 하나를 어떤 목표 상태까지 올리는 방법 하나
        struct SlotOption
        {
            EquipmentState m_terminal{};
            EquipmentState m_preState{};               // 계승 직전 상태 (계승이 없으면 목표 상태)
            std::vector<Models::RefineStep> m_regularPre;
            std::vector<Models::RefineStep> m_advancedPre;
            std::optional<Models::RefineStep> m_inherit;
            std::vector<Models::RefineStep> m_post;    // 계승 뒤 세르카 재련
            double m_cost{};
            int m_actionCount{};
            int m_finalLevel{};
            bool m_hasSercaRegular{};                  // 세르카 재련이 들어 있음 (모든 부위 계승 준비 필요)
            bool m_isPrepared{};                       // 계승 직전(또는 목표) 상태가 계승 준비 완료
        };

        // 일반 재련 from.강화 → targetEnhancement (비용표에 없는 단계면 false)
        [[nodiscard]] bool AppendRegular(Models::RefineSlot slot, EquipmentState& state, int targetEnhancement,
            const Models::RefineCostCatalog& catalog, std::vector<Models::RefineStep>& steps, double& cost)
        {
            const std::size_t tier{ ToTierIndex(state.m_tier) };
            const std::size_t kind{ ToKindIndex(Models::GetSlotKind(slot)) };
            while (state.m_enhancement < targetEnhancement)
            {
                if (state.m_enhancement < 0 || static_cast<std::size_t>(state.m_enhancement) >= Models::RefineCostCatalog::LEVEL_COUNT) return false;
                const std::optional<Models::RefineStepCost>& stepCost{ catalog.m_regular[tier][kind][static_cast<std::size_t>(state.m_enhancement)] };
                if (!stepCost.has_value()) return false;

                EquipmentState next{ state };
                ++next.m_enhancement;
                steps.push_back(Models::RefineStep{ slot, Models::RefineAction::ACTION_REGULAR, state, next, stepCost->m_expectedCost });
                cost += stepCost->m_expectedCost;
                state = next;
            }
            return true;
        }

        // 상급 재련 from.상급 → targetAdvanced (1단계 = 묶음 비용의 1/10)
        void AppendAdvanced(Models::RefineSlot slot, EquipmentState& state, int targetAdvanced,
            const Models::RefineCostCatalog& catalog, std::vector<Models::RefineStep>& steps, double& cost)
        {
            const std::size_t kind{ ToKindIndex(Models::GetSlotKind(slot)) };
            while (state.m_advancedEnhancement < targetAdvanced)
            {
                const std::size_t block{ static_cast<std::size_t>(state.m_advancedEnhancement / Models::RefineData::ADVANCED_BLOCK_SIZE) };
                const double stepCost{ catalog.m_advanced[kind][block].m_expectedCost / ADVANCED_STEPS_PER_BLOCK };

                EquipmentState next{ state };
                ++next.m_advancedEnhancement;
                steps.push_back(Models::RefineStep{ slot, Models::RefineAction::ACTION_ADVANCED, state, next, stepCost });
                cost += stepCost;
                state = next;
            }
        }

        // 걸음 목록으로 방법 정보 채우기
        void FinishOption(SlotOption& option)
        {
            option.m_actionCount = static_cast<int>(option.m_regularPre.size() + option.m_advancedPre.size() + option.m_post.size())
                + (option.m_inherit.has_value() ? 1 : 0);
            option.m_finalLevel = Models::RefineData::CalculateItemLevel(option.m_terminal);
            option.m_isPrepared = Models::RefineData::IsInheritPrepared(option.m_preState);

            const auto isSercaRegular{ [](const Models::RefineStep& step)
            {
                return step.m_action == Models::RefineAction::ACTION_REGULAR && step.m_from.m_tier == EquipmentTier::TIER_SERCA;
            } };
            option.m_hasSercaRegular = std::ranges::any_of(option.m_regularPre, isSercaRegular) || std::ranges::any_of(option.m_post, isSercaRegular);
        }

        // 같은 장비 단계 안에서 start → terminal (일반 재련 다음 상급 재련)
        [[nodiscard]] std::optional<SlotOption> MakeSameTierOption(Models::RefineSlot slot, const EquipmentState& start,
            const EquipmentState& terminal, const Models::RefineCostCatalog& catalog)
        {
            if (start.m_tier != terminal.m_tier || terminal.m_enhancement < start.m_enhancement
                || terminal.m_advancedEnhancement < start.m_advancedEnhancement) return std::nullopt;

            SlotOption option;
            EquipmentState state{ start };
            if (!AppendRegular(slot, state, terminal.m_enhancement, catalog, option.m_regularPre, option.m_cost)) return std::nullopt;
            AppendAdvanced(slot, state, terminal.m_advancedEnhancement, catalog, option.m_advancedPre, option.m_cost);

            option.m_terminal = terminal;
            option.m_preState = terminal;
            FinishOption(option);
            return option;
        }

        // 에기르 start → (+20 이상 · 상급 40) → 계승 → 세르카 terminal
        [[nodiscard]] std::optional<SlotOption> MakeInheritOption(Models::RefineSlot slot, const EquipmentState& start,
            const EquipmentState& terminal, const Models::RefineCostCatalog& catalog)
        {
            if (start.m_tier != EquipmentTier::TIER_AEGIR || terminal.m_tier != EquipmentTier::TIER_SERCA) return std::nullopt;

            const int inheritEnhancement{ std::max(start.m_enhancement, Models::RefineData::INHERIT_ENHANCEMENT) };
            const int sercaStart{ Models::RefineData::GetInheritStartLevel(inheritEnhancement) };
            if (terminal.m_enhancement < sercaStart) return std::nullopt;

            SlotOption option;
            EquipmentState state{ start };
            if (!AppendRegular(slot, state, inheritEnhancement, catalog, option.m_regularPre, option.m_cost)) return std::nullopt;
            AppendAdvanced(slot, state, Models::RefineData::INHERIT_ADVANCED_ENHANCEMENT, catalog, option.m_advancedPre, option.m_cost);

            const EquipmentState sercaState{ EquipmentTier::TIER_SERCA, sercaStart, 0 };
            option.m_preState = state;
            option.m_inherit = Models::RefineStep{ slot, Models::RefineAction::ACTION_INHERIT, state, sercaState, 0.0 };

            EquipmentState postState{ sercaState };
            if (!AppendRegular(slot, postState, terminal.m_enhancement, catalog, option.m_post, option.m_cost)) return std::nullopt;

            option.m_terminal = terminal;
            FinishOption(option);
            return option;
        }

        // start → terminal (같은 단계 또는 계승)
        [[nodiscard]] std::optional<SlotOption> MakeOption(Models::RefineSlot slot, const EquipmentState& start,
            const EquipmentState& terminal, const Models::RefineCostCatalog& catalog)
        {
            if (std::optional<SlotOption> option{ MakeSameTierOption(slot, start, terminal, catalog) }) return option;
            return MakeInheritOption(slot, start, terminal, catalog);
        }

        // 상급 재련 목표 후보 (지금 단계 + 다음 10단위들 → 상급 재련은 10단계씩 끊어서 진행)
        [[nodiscard]] std::vector<int> GetAdvancedTargets(int current)
        {
            std::vector<int> targets{ current };
            for (int next{ (current / Models::RefineData::ADVANCED_BLOCK_SIZE + 1) * Models::RefineData::ADVANCED_BLOCK_SIZE };
                next <= Models::RefineData::MAX_ADVANCED_ENHANCEMENT; next += Models::RefineData::ADVANCED_BLOCK_SIZE)
            {
                targets.push_back(next);
            }
            return targets;
        }

        // 아이템 레벨 기준: 부위 하나의 모든 목표 후보 (같은 결과면 싼 것만)
        [[nodiscard]] std::vector<SlotOption> EnumerateOptions(Models::RefineSlot slot, const EquipmentState& start, const Models::RefineCostCatalog& catalog)
        {
            std::vector<SlotOption> candidates;
            if (start.m_tier == EquipmentTier::TIER_AEGIR)
            {
                // 에기르는 계승 단계(+20)까지만 올리고, 더 올릴 때는 계승해서 세르카로 (로아업과 같은 규칙)
                const int maxEnhancement{ std::max(start.m_enhancement, Models::RefineData::INHERIT_ENHANCEMENT) };
                for (int enhancement{ start.m_enhancement }; enhancement <= maxEnhancement; ++enhancement)
                {
                    for (const int advanced : GetAdvancedTargets(start.m_advancedEnhancement))
                    {
                        if (std::optional<SlotOption> option{ MakeSameTierOption(slot, start, EquipmentState{ EquipmentTier::TIER_AEGIR, enhancement, advanced }, catalog) })
                        {
                            candidates.push_back(std::move(*option));
                        }
                    }
                }
                for (int enhancement{ Models::RefineData::MIN_SERCA_ENHANCEMENT }; enhancement <= Models::RefineData::MAX_ENHANCEMENT; ++enhancement)
                {
                    if (std::optional<SlotOption> option{ MakeInheritOption(slot, start, EquipmentState{ EquipmentTier::TIER_SERCA, enhancement, 0 }, catalog) })
                    {
                        candidates.push_back(std::move(*option));
                    }
                }
            }
            else
            {
                for (int enhancement{ start.m_enhancement }; enhancement <= Models::RefineData::MAX_ENHANCEMENT; ++enhancement)
                {
                    if (std::optional<SlotOption> option{ MakeSameTierOption(slot, start, EquipmentState{ EquipmentTier::TIER_SERCA, enhancement, 0 }, catalog) })
                    {
                        candidates.push_back(std::move(*option));
                    }
                }
            }

            // 결과(아이템 레벨 · 계승 여부 · 세르카 재련 여부 · 준비 여부)가 같으면 비용 → 걸음 수가 작은 것만
            std::map<std::tuple<int, bool, bool, bool>, std::size_t> bestByKey;
            for (std::size_t index{}; index < candidates.size(); ++index)
            {
                const SlotOption& option{ candidates[index] };
                const std::tuple<int, bool, bool, bool> key{ option.m_finalLevel, option.m_inherit.has_value(), option.m_hasSercaRegular, option.m_isPrepared };
                const auto found{ bestByKey.find(key) };
                if (found == bestByKey.end())
                {
                    bestByKey.emplace(key, index);
                    continue;
                }
                const SlotOption& current{ candidates[found->second] };
                if (option.m_cost < current.m_cost - COST_EPSILON
                    || (std::abs(option.m_cost - current.m_cost) <= COST_EPSILON && option.m_actionCount < current.m_actionCount))
                {
                    found->second = index;
                }
            }

            std::vector<SlotOption> options;
            options.reserve(bestByKey.size());
            for (const auto& [key, index] : bestByKey) options.push_back(std::move(candidates[index]));
            return options;
        }

        // 부위 조합 탐색 상태 (아이템 레벨 합 · 계승 · 세르카 재련 · 준비된 부위 수가 같으면 싼 것만)
        struct CombineState
        {
            double m_cost{};
            int m_actionCount{};
            int m_levelSum{};
            bool m_hasNextTier{};          // 계승 · 세르카 재련이 들어 있음
            int m_preparedCount{};
            std::array<std::size_t, Models::REFINE_SLOT_COUNT> m_choices{};   // 부위별 고른 방법 번호
        };

        [[nodiscard]] bool IsBetter(const CombineState& candidate, const CombineState& current) noexcept
        {
            if (std::abs(candidate.m_cost - current.m_cost) > COST_EPSILON) return candidate.m_cost < current.m_cost;
            return candidate.m_actionCount < current.m_actionCount;
        }

        // 부위별 방법을 하나씩 골라 모든 조합을 상태로 줄여 가며 탐색 (부위 6개 × 방법 수)
        [[nodiscard]] std::vector<CombineState> CombineOptions(const std::array<std::vector<SlotOption>, Models::REFINE_SLOT_COUNT>& options)
        {
            std::vector<CombineState> states{ CombineState{} };
            for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
            {
                std::map<std::tuple<int, bool, int>, CombineState> nextStates;
                for (const CombineState& state : states)
                {
                    for (std::size_t optionIndex{}; optionIndex < options[slot].size(); ++optionIndex)
                    {
                        const SlotOption& option{ options[slot][optionIndex] };
                        CombineState next{ state };
                        next.m_cost += option.m_cost;
                        next.m_actionCount += option.m_actionCount;
                        next.m_levelSum += option.m_finalLevel;
                        next.m_hasNextTier = next.m_hasNextTier || option.m_inherit.has_value() || option.m_hasSercaRegular;
                        next.m_preparedCount += option.m_isPrepared ? 1 : 0;
                        next.m_choices[slot] = optionIndex;

                        const std::tuple<int, bool, int> key{ next.m_levelSum, next.m_hasNextTier, next.m_preparedCount };
                        const auto found{ nextStates.find(key) };
                        if (found == nextStates.end() || IsBetter(next, found->second)) nextStates.insert_or_assign(key, next);
                    }
                }

                states.clear();
                states.reserve(nextStates.size());
                for (const auto& [key, state] : nextStates) states.push_back(state);
            }
            return states;
        }

        // 계승 · 세르카 재련은 모든 부위가 계승 준비를 마친 조합만 가능
        [[nodiscard]] bool IsLegal(const CombineState& state) noexcept
        {
            return !state.m_hasNextTier || state.m_preparedCount == static_cast<int>(Models::REFINE_SLOT_COUNT);
        }

        // ===== 진행 순서 =====
        // 부위별 남은 걸음 (일반 · 상급은 계승 전, 세르카 재련은 계승 뒤)
        struct SlotLane
        {
            const SlotOption* m_option{};
            std::size_t m_regularIndex{};
            std::size_t m_advancedIndex{};
            std::size_t m_postIndex{};
            bool m_isInherited{};
        };

        // 다음에 할 걸음 후보
        struct StepCandidate
        {
            Models::RefineStep m_step;
            std::size_t m_slotIndex{};
            int m_laneKind{};             // 0 = 일반(계승 전), 1 = 상급, 2 = 계승, 3 = 계승 뒤
            int m_levelIncrease{};
        };

        constexpr int LANE_REGULAR{ 0 };
        constexpr int LANE_ADVANCED{ 1 };
        constexpr int LANE_INHERIT{ 2 };
        constexpr int LANE_POST{ 3 };

        [[nodiscard]] int GetActionPriority(Models::RefineAction action) noexcept
        {
            constexpr int INHERIT_PRIORITY{ 0 };
            constexpr int ADVANCED_PRIORITY{ 1 };
            constexpr int REGULAR_PRIORITY{ 2 };
            if (action == Models::RefineAction::ACTION_INHERIT) return INHERIT_PRIORITY;
            return action == Models::RefineAction::ACTION_ADVANCED ? ADVANCED_PRIORITY : REGULAR_PRIORITY;
        }

        [[nodiscard]] std::size_t GetOrderIndex(Models::RefineSlot slot) noexcept
        {
            return static_cast<std::size_t>(std::distance(STEP_SLOT_ORDER.begin(), std::ranges::find(STEP_SLOT_ORDER, slot)));
        }

        // 계승이 먼저, 그다음은 아이템 레벨 1당 비용이 싼 순 (로아업과 같은 정렬)
        [[nodiscard]] bool ComesBefore(const StepCandidate& left, const StepCandidate& right) noexcept
        {
            const bool hasInherit{ left.m_step.m_action == Models::RefineAction::ACTION_INHERIT || right.m_step.m_action == Models::RefineAction::ACTION_INHERIT };
            if (hasInherit)
            {
                const int priority{ GetActionPriority(left.m_step.m_action) - GetActionPriority(right.m_step.m_action) };
                if (priority != 0) return priority < 0;
                return GetOrderIndex(left.m_step.m_slot) < GetOrderIndex(right.m_step.m_slot);
            }

            const double infinity{ std::numeric_limits<double>::infinity() };
            const double leftRatio{ left.m_levelIncrease > 0 ? left.m_step.m_expectedCost / left.m_levelIncrease : infinity };
            const double rightRatio{ right.m_levelIncrease > 0 ? right.m_step.m_expectedCost / right.m_levelIncrease : infinity };
            if (leftRatio != rightRatio) return leftRatio < rightRatio;

            const std::size_t leftOrder{ GetOrderIndex(left.m_step.m_slot) };
            const std::size_t rightOrder{ GetOrderIndex(right.m_step.m_slot) };
            if (leftOrder != rightOrder) return leftOrder < rightOrder;
            if (left.m_levelIncrease != right.m_levelIncrease) return left.m_levelIncrease > right.m_levelIncrease;
            if (left.m_step.m_expectedCost != right.m_step.m_expectedCost) return left.m_step.m_expectedCost < right.m_step.m_expectedCost;
            return GetActionPriority(left.m_step.m_action) < GetActionPriority(right.m_step.m_action);
        }

        // 미리 만든 걸음을 지금 상태 기준으로 다시 씀 (일반 · 상급이 섞여 진행되므로 바뀌는 칸만 반영)
        [[nodiscard]] StepCandidate MakeCandidate(const Models::RefineStep& planned, const EquipmentState& current, std::size_t slotIndex, int laneKind)
        {
            Models::RefineStep step{ planned };
            step.m_from = current;
            if (planned.m_action == Models::RefineAction::ACTION_REGULAR)
            {
                step.m_to = current;
                step.m_to.m_enhancement = planned.m_to.m_enhancement;
            }
            else if (planned.m_action == Models::RefineAction::ACTION_ADVANCED)
            {
                step.m_to = current;
                step.m_to.m_advancedEnhancement = planned.m_to.m_advancedEnhancement;
            }

            return StepCandidate{ step, slotIndex, laneKind,
                Models::RefineData::CalculateItemLevel(step.m_to) - Models::RefineData::CalculateItemLevel(step.m_from) };
        }

        [[nodiscard]] double GetAverageLevel(const Models::RefineStates& states) noexcept
        {
            int sum{};
            for (const EquipmentState& state : states) sum += Models::RefineData::CalculateItemLevel(state);
            return static_cast<double>(sum) / static_cast<double>(Models::REFINE_SLOT_COUNT);
        }

        // 고른 방법들의 걸음을 실제 진행 순서로 늘어놓음
        // 세르카 재련은 모든 부위가 계승 준비를 마친 뒤, 계승은 평균 1730 이상일 때
        [[nodiscard]] std::vector<Models::RefineStep> OrderSteps(const Models::RefineStates& starts, const std::array<const SlotOption*, Models::REFINE_SLOT_COUNT>& chosen)
        {
            std::array<SlotLane, Models::REFINE_SLOT_COUNT> lanes{};
            std::size_t totalSteps{};
            bool hasNextTier{};
            for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
            {
                lanes[slot].m_option = chosen[slot];
                totalSteps += static_cast<std::size_t>(chosen[slot]->m_actionCount);
                hasNextTier = hasNextTier || chosen[slot]->m_inherit.has_value() || chosen[slot]->m_hasSercaRegular;
            }

            Models::RefineStates current{ starts };
            std::vector<Models::RefineStep> ordered;
            ordered.reserve(totalSteps);
            while (ordered.size() < totalSteps)
            {
                const double averageLevel{ GetAverageLevel(current) };
                const bool isAllPrepared{ !hasNextTier || std::ranges::all_of(current, Models::RefineData::IsInheritPrepared) };

                std::vector<StepCandidate> candidates;
                for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
                {
                    const SlotLane& lane{ lanes[slot] };
                    const SlotOption& option{ *lane.m_option };
                    const bool hasRegular{ lane.m_regularIndex < option.m_regularPre.size() };
                    const bool hasAdvanced{ lane.m_advancedIndex < option.m_advancedPre.size() };
                    if (hasRegular || hasAdvanced)
                    {
                        if (hasRegular && (!hasNextTier || current[slot].m_tier != EquipmentTier::TIER_SERCA || isAllPrepared))
                        {
                            candidates.push_back(MakeCandidate(option.m_regularPre[lane.m_regularIndex], current[slot], slot, LANE_REGULAR));
                        }
                        if (hasAdvanced) candidates.push_back(MakeCandidate(option.m_advancedPre[lane.m_advancedIndex], current[slot], slot, LANE_ADVANCED));
                        continue;
                    }

                    if (option.m_inherit.has_value() && !lane.m_isInherited)
                    {
                        if (isAllPrepared && averageLevel >= INHERIT_REQUIRED_ITEM_LEVEL) candidates.push_back(MakeCandidate(*option.m_inherit, current[slot], slot, LANE_INHERIT));
                        continue;
                    }

                    if (lane.m_postIndex < option.m_post.size()) candidates.push_back(MakeCandidate(option.m_post[lane.m_postIndex], current[slot], slot, LANE_POST));
                }

                if (candidates.empty()) break;   // 진행할 수 없는 경로 (조합 검사에서 걸러지므로 원래는 생기지 않음)

                const StepCandidate& next{ *std::ranges::min_element(candidates, [](const StepCandidate& left, const StepCandidate& right)
                {
                    return ComesBefore(left, right);
                }) };

                SlotLane& lane{ lanes[next.m_slotIndex] };
                if (next.m_laneKind == LANE_REGULAR) ++lane.m_regularIndex;
                else if (next.m_laneKind == LANE_ADVANCED) ++lane.m_advancedIndex;
                else if (next.m_laneKind == LANE_INHERIT) lane.m_isInherited = true;
                else ++lane.m_postIndex;

                current[next.m_slotIndex] = next.m_step.m_to;
                ordered.push_back(next.m_step);
            }
            return ordered;
        }

        // 걸음 하나의 기대 재료 (일반 = 비용표 그대로, 상급 = 묶음의 1/10, 계승 = 없음)
        void AddStepMaterials(Models::MaterialAmounts& target, const Models::RefineStep& step, const Models::RefineCostCatalog& catalog)
        {
            const std::size_t kind{ ToKindIndex(Models::GetSlotKind(step.m_slot)) };
            if (step.m_action == Models::RefineAction::ACTION_REGULAR)
            {
                const std::optional<Models::RefineStepCost>& stepCost{
                    catalog.m_regular[ToTierIndex(step.m_from.m_tier)][kind][static_cast<std::size_t>(step.m_from.m_enhancement)] };
                if (stepCost.has_value()) AddAmounts(target, stepCost->m_materials, 1.0);
            }
            else if (step.m_action == Models::RefineAction::ACTION_ADVANCED)
            {
                const std::size_t block{ static_cast<std::size_t>(step.m_from.m_advancedEnhancement / Models::RefineData::ADVANCED_BLOCK_SIZE) };
                AddAmounts(target, catalog.m_advanced[kind][block].m_materials, 1.0 / ADVANCED_STEPS_PER_BLOCK);
            }
        }

        [[nodiscard]] Models::RefineRouteResult MakeFailure(Models::RefineFailure failure, const Models::RefineStates& starts)
        {
            Models::RefineRouteResult result;
            result.m_failure = failure;
            result.m_startItemLevel = GetAverageLevel(starts);
            result.m_achievedItemLevel = result.m_startItemLevel;
            for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot) result.m_slots[slot].m_terminal = starts[slot];
            return result;
        }
    }

    Models::RefineCostCatalog BuildCostCatalog(const Models::RefinePrices& prices)
    {
        Models::RefineCostCatalog catalog;
        for (const EquipmentTier tier : { EquipmentTier::TIER_AEGIR, EquipmentTier::TIER_SERCA })
        {
            for (const EquipmentKind kind : { EquipmentKind::KIND_ARMOR, EquipmentKind::KIND_WEAPON })
            {
                for (std::size_t level{}; level < Models::RefineCostCatalog::LEVEL_COUNT; ++level)
                {
                    const std::optional<NormalStep> step{ Models::RefineData::FindNormalStep(tier, kind, static_cast<int>(level)) };
                    if (step.has_value()) catalog.m_regular[ToTierIndex(tier)][ToKindIndex(kind)][level] = CalculateNormalStep(*step, prices);
                }
            }
        }

        for (const EquipmentKind kind : { EquipmentKind::KIND_ARMOR, EquipmentKind::KIND_WEAPON })
        {
            for (int block{}; block < Models::RefineData::ADVANCED_BLOCK_COUNT; ++block)
            {
                const std::optional<AdvancedBlock> data{ Models::RefineData::FindAdvancedBlock(kind, block) };
                if (data.has_value()) catalog.m_advanced[ToKindIndex(kind)][static_cast<std::size_t>(block)] = CalculateAdvancedBlock(*data, prices);
            }
        }
        return catalog;
    }

    Models::RefineRouteResult CalculateRoute(const Models::RefineRequest& request, const Models::RefineCostCatalog& catalog)
    {
        if (!std::ranges::all_of(request.m_starts, Models::RefineData::IsValidState)) return MakeFailure(Models::RefineFailure::FAILURE_INVALID_STATE, request.m_starts);

        // 부위별 방법 후보
        std::array<std::vector<SlotOption>, Models::REFINE_SLOT_COUNT> options;
        for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
        {
            const Models::RefineSlot refineSlot{ static_cast<Models::RefineSlot>(slot) };
            if (request.m_targetMode == Models::RefineTargetMode::TARGET_ITEM_LEVEL)
            {
                options[slot] = EnumerateOptions(refineSlot, request.m_starts[slot], catalog);
                continue;
            }

            // 부위별 단계: 정한 목표 상태 하나만
            const EquipmentState& target{ request.m_targets[slot] };
            if (!Models::RefineData::IsValidState(target)) return MakeFailure(Models::RefineFailure::FAILURE_INVALID_STATE, request.m_starts);

            std::optional<SlotOption> option{ MakeOption(refineSlot, request.m_starts[slot], target, catalog) };
            if (!option.has_value()) return MakeFailure(Models::RefineFailure::FAILURE_REGRESSION, request.m_starts);
            options[slot].push_back(std::move(*option));
        }

        // 조합 탐색 → 조건을 만족하는 가장 싼 조합
        const int targetSum{ request.m_targetItemLevel * static_cast<int>(Models::REFINE_SLOT_COUNT) };
        const bool isStageMode{ request.m_targetMode == Models::RefineTargetMode::TARGET_STAGE };
        const std::vector<CombineState> states{ CombineOptions(options) };

        const CombineState* best{};
        bool hasIllegalOnly{};
        for (const CombineState& state : states)
        {
            if (!isStageMode && state.m_levelSum < targetSum) continue;
            if (!IsLegal(state))
            {
                hasIllegalOnly = true;
                continue;
            }

            if (best == nullptr) { best = &state; continue; }
            if (std::abs(state.m_cost - best->m_cost) > COST_EPSILON)
            {
                if (state.m_cost < best->m_cost) best = &state;
                continue;
            }
            // 같은 비용이면 목표를 덜 넘는 쪽 → 걸음 수가 적은 쪽
            if (state.m_levelSum != best->m_levelSum)
            {
                if (state.m_levelSum < best->m_levelSum) best = &state;
                continue;
            }
            if (state.m_actionCount < best->m_actionCount) best = &state;
        }

        if (best == nullptr)
        {
            return MakeFailure(hasIllegalOnly ? Models::RefineFailure::FAILURE_NOT_PREPARED : Models::RefineFailure::FAILURE_UNREACHABLE, request.m_starts);
        }

        // 결과 정리
        Models::RefineRouteResult result;
        std::array<const SlotOption*, Models::REFINE_SLOT_COUNT> chosen{};
        int finalSum{};
        for (std::size_t slot{}; slot < Models::REFINE_SLOT_COUNT; ++slot)
        {
            const SlotOption& option{ options[slot][best->m_choices[slot]] };
            chosen[slot] = &option;
            result.m_slots[slot] = Models::RefineSlotPlan{ option.m_terminal, option.m_cost, option.m_actionCount };
            finalSum += option.m_finalLevel;
        }

        result.m_steps = OrderSteps(request.m_starts, chosen);
        for (const Models::RefineStep& step : result.m_steps)
        {
            result.m_totalCost += step.m_expectedCost;
            AddStepMaterials(result.m_materials, step, catalog);
        }
        result.m_startItemLevel = GetAverageLevel(request.m_starts);
        result.m_achievedItemLevel = static_cast<double>(finalSum) / static_cast<double>(Models::REFINE_SLOT_COUNT);
        return result;
    }
}
