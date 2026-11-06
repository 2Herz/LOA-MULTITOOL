module LostArkMultiTool.Views.CombatExtraEditor;

import std;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Views.CombatSimulatorView;

namespace LostArkMultiTool::Views
{
    namespace
    {
        namespace Data = Models::CombatCoefficientData;

        // ===== 글자 =====
        constexpr std::wstring_view NONE_LABEL{ L"없음" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };
        constexpr std::wstring_view NUMBER_FORMAT{ L"{}" };
        constexpr std::wstring_view MAIN_STAT_FORMAT{ L"{:.0f}" };
        constexpr std::wstring_view BOOK_FORMAT{ L"{} X{}" };               // "유물 X2"
        constexpr std::wstring_view RANK_FORMAT{ L"{}랭크" };
        constexpr std::wstring_view LEGEND_NAME{ L"전설" };
        constexpr std::wstring_view RELIC_NAME{ L"유물" };
        constexpr std::wstring_view EPIC_NAME{ L"영웅" };
        constexpr std::wstring_view ANCIENT_NAME{ L"고대" };
        constexpr std::size_t BOOK_LEVEL_COUNT{ static_cast<std::size_t>(Data::ENGRAVING_BOOK_MAX_LEVEL) + 1 };   // 각인서 레벨 0 ~ 4
        constexpr std::wstring_view ACCESSORY_STAT_FORMAT{ L"{} 주스탯" };
        constexpr std::wstring_view ACCESSORY_OPTION_FORMAT{ L"{} 연마 {}" };
        constexpr std::wstring_view BRACELET_STAT_HEADER{ L"팔찌 스탯" };
        constexpr std::wstring_view BRACELET_OPTION_HEADER{ L"팔찌 옵션 (공백 없는 문구)" };
        constexpr std::wstring_view BRACELET_STAT_TYPE_FORMAT{ L"스탯 {} 종류" };
        constexpr std::wstring_view BRACELET_STAT_VALUE_FORMAT{ L"스탯 {} 수치" };
        constexpr std::wstring_view BRACELET_OPTION_FORMAT{ L"옵션 {}" };
        constexpr std::wstring_view STONE_HEADER{ L"어빌리티 스톤 (각인 두 줄)" };
        constexpr std::wstring_view STONE_NAME_FORMAT{ L"스톤 각인 {}" };
        constexpr std::wstring_view STONE_LEVEL_FORMAT{ L"스톤 각인 {} 레벨" };
        constexpr std::wstring_view BOOK_HEADER{ L"각인서 (유물 각인 5개)" };
        constexpr std::wstring_view BOOK_NAME_FORMAT{ L"각인서 {}" };
        constexpr std::wstring_view BOOK_LEVEL_FORMAT{ L"각인서 {} 등급 · 레벨" };
        constexpr std::wstring_view CORE_HEADER{ L"코어 (등급 · 의지력 포인트 합)" };
        constexpr std::wstring_view CORE_GRADE_FORMAT{ L"{} 등급" };
        constexpr std::wstring_view CORE_POINT_FORMAT{ L"{} 포인트" };
        constexpr std::wstring_view GEM_EFFECT_HEADER{ L"젬 효과 레벨 합 (장착한 젬 전체)" };
        constexpr std::wstring_view PASSIVE_HEADER{ L"아크 패시브" };
        constexpr std::wstring_view RANK_LABEL{ L"랭크" };
        constexpr std::wstring_view ENLIGHTENMENT_LABEL{ L"깨달음 포인트 구성" };
        constexpr std::wstring_view AVATAR_HEADER{ L"아바타 (장착 중인 것)" };

        // 악세서리 슬롯 이름 · 종류 (목걸이, 귀걸이 1 · 2, 반지 1 · 2)
        constexpr std::array<std::wstring_view, Models::ACCESSORY_SLOT_COUNT> ACCESSORY_NAMES{ L"목걸이", L"귀걸이 1", L"귀걸이 2", L"반지 1", L"반지 2" };
        constexpr std::array<Data::AccessoryKind, Models::ACCESSORY_SLOT_COUNT> ACCESSORY_KINDS{
            Data::AccessoryKind::KIND_NECKLACE, Data::AccessoryKind::KIND_EARRING, Data::AccessoryKind::KIND_EARRING,
            Data::AccessoryKind::KIND_RING, Data::AccessoryKind::KIND_RING };

        constexpr std::array<std::wstring_view, Models::ARK_CORE_COUNT> CORE_NAMES{
            L"질서의 해 코어", L"질서의 달 코어", L"질서의 별 코어", L"혼돈의 해 코어", L"혼돈의 달 코어", L"혼돈의 별 코어" };
        constexpr std::array<std::wstring_view, Models::ARK_GEM_EFFECT_COUNT> GEM_EFFECT_NAMES{ L"공격력 레벨 합", L"추가 피해 레벨 합", L"보스 피해 레벨 합" };
        constexpr std::array<std::wstring_view, Models::PASSIVE_TREE_COUNT> PASSIVE_LEVEL_NAMES{ L"진화 레벨 (효과 없음)", L"깨달음 레벨", L"도약 레벨" };
        constexpr std::array<std::wstring_view, Models::AVATAR_SLOT_COUNT> AVATAR_NAMES{ L"무기 아바타", L"머리 아바타", L"상의 아바타", L"하의 아바타" };

        // 아바타 · 코어 등급 선택지 (열거형 순서와 같음)
        constexpr std::array<std::wstring_view, 3> AVATAR_CHOICES{ NONE_LABEL, EPIC_NAME, LEGEND_NAME };
        constexpr std::array<std::wstring_view, 4> CORE_GRADE_CHOICES{ EPIC_NAME, LEGEND_NAME, RELIC_NAME, ANCIENT_NAME };

        // UTF-8 → 화면 글자 (변환할 수 없으면 빈 글자)
        [[nodiscard]] std::wstring Wide(std::string_view text)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(text);
            }
            catch (const std::exception&)
            {
                return std::wstring{};
            }
        }

        [[nodiscard]] std::string Narrow(const std::wstring& text)
        {
            try
            {
                return Core::TextEncoding::WideToUTF8(text);
            }
            catch (const std::exception&)
            {
                return std::string{};
            }
        }

        // 옵션 글자를 화면용으로 (비었으면 "없음")
        [[nodiscard]] std::wstring Display(std::string_view text)
        {
            return Models::IsNone(text) ? std::wstring{ NONE_LABEL } : Wide(text);
        }

        [[nodiscard]] bool IsBlank(const std::wstring& text) noexcept
        {
            return text.find_first_not_of(TRIM_CHARACTERS) == std::wstring::npos;
        }

        [[nodiscard]] std::vector<std::wstring> ToWideList(std::span<const std::string_view> list)
        {
            std::vector<std::wstring> items;
            items.reserve(list.size());
            for (const std::string_view item : list) items.push_back(Display(item));
            return items;
        }

        template <std::size_t Count>
        [[nodiscard]] std::vector<std::wstring> ToWideList(const std::array<std::wstring_view, Count>& list)
        {
            return std::vector<std::wstring>(list.begin(), list.end());
        }

        // 각인서 이름이 두 칸 이상 같은지 ("없음"은 제외)
        [[nodiscard]] bool HasDuplicateBook(const Models::ExtraState& state)
        {
            for (std::size_t first{}; first < Models::BOOK_COUNT; ++first)
            {
                if (Models::IsNone(state.m_books[first].m_name)) continue;
                for (std::size_t second{ first + 1 }; second < Models::BOOK_COUNT; ++second)
                {
                    if (state.m_books[first].m_name == state.m_books[second].m_name) return true;
                }
            }
            return false;
        }

        // ===== 행 명세 만들기 =====
        // 접근 함수는 generic lambda: 상태 참조를 받아 해당 칸 참조를 돌려줌 (const 상태면 const 참조)
        // 구분 제목 행
        template <typename Spec>
        void AddHeader(std::vector<Spec>& specs, std::wstring_view label)
        {
            Spec spec;
            spec.m_label = std::wstring{ label };
            spec.m_kind = CombatRowKind::ROW_HEADER;
            specs.push_back(std::move(spec));
        }

        // 문구 선택 행 (선택지는 표의 목록, 현재 값이 목록에 없으면 "없음" 다음에 끼움)
        template <typename Spec, typename Accessor>
        void AddTextChoice(std::vector<Spec>& specs, std::wstring_view label, Accessor accessor, std::span<const std::string_view> candidates)
        {
            Spec spec;
            spec.m_label = std::wstring{ label };
            spec.m_kind = CombatRowKind::ROW_CHOICE;
            spec.m_read = [accessor](const Models::ExtraState& state) { return Display(accessor(state)); };
            spec.m_choices = [accessor, candidates](const Models::ExtraState& state)
            {
                std::vector<std::wstring> items{ ToWideList(candidates) };
                const std::wstring currentText{ Display(accessor(state)) };
                if (std::ranges::find(items, currentText) == items.end()) items.insert(items.empty() ? items.begin() : items.begin() + 1, currentText);
                return items;
            };
            spec.m_write = [accessor](Models::ExtraState& state, const std::wstring& text) { accessor(state) = Narrow(text); };
            specs.push_back(std::move(spec));
        }

        // 숫자 행 (정수 또는 실수 칸, 범위 맞춤은 CombatExtra::Clamp가 함)
        template <typename Spec, typename Accessor, typename Number>
        void AddNumber(std::vector<Spec>& specs, std::wstring_view label, Accessor accessor, std::type_identity<Number>)
        {
            Spec spec;
            spec.m_label = std::wstring{ label };
            spec.m_kind = CombatRowKind::ROW_NUMBER;
            spec.m_read = [accessor](const Models::ExtraState& state)
            {
                if constexpr (std::is_floating_point_v<Number>) return std::format(MAIN_STAT_FORMAT, static_cast<double>(accessor(state)));
                else return std::format(NUMBER_FORMAT, accessor(state));
            };
            spec.m_write = [accessor](Models::ExtraState& state, const std::wstring& text)
            {
                if (IsBlank(text)) return;   // 비우면 그대로
                accessor(state) = static_cast<Number>(Core::NumberFormat::ParseNumber(text));
            };
            specs.push_back(std::move(spec));
        }

        // 번호로 고르는 행 (선택지 글자 목록 + 현재 번호 읽기 / 쓰기)
        template <typename Spec, typename Getter, typename Setter>
        void AddIndexChoice(std::vector<Spec>& specs, std::wstring_view label, std::vector<std::wstring> items, Getter getter, Setter setter)
        {
            Spec spec;
            spec.m_label = std::wstring{ label };
            spec.m_kind = CombatRowKind::ROW_CHOICE;
            spec.m_read = [items, getter](const Models::ExtraState& state)
            {
                const std::size_t index{ getter(state) };
                return index < items.size() ? items[index] : std::wstring{};
            };
            spec.m_choices = [items](const Models::ExtraState&) { return items; };
            spec.m_write = [items, setter](Models::ExtraState& state, const std::wstring& text)
            {
                const auto found{ std::ranges::find(items, text) };
                if (found != items.end()) setter(state, static_cast<std::size_t>(std::distance(items.begin(), found)));
            };
            specs.push_back(std::move(spec));
        }
    }

    CombatExtraEditor::CombatExtraEditor()
    {
        BuildAccessoryRows();
        BuildBraceletRows();
        BuildArkGridRows();
        BuildPassiveRows();
    }

    CombatExtraEditor::~CombatExtraEditor() = default;

    // 악세서리 5종: 슬롯마다 주스탯(숫자) + 연마 옵션 3줄(선택)
    void CombatExtraEditor::BuildAccessoryRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_ACCESSORY)] };
        for (std::size_t slot{}; slot < Models::ACCESSORY_SLOT_COUNT; ++slot)
        {
            AddNumber(specs, std::format(ACCESSORY_STAT_FORMAT, ACCESSORY_NAMES[slot]),
                [slot](auto& state) -> auto& { return state.m_accessories[slot].m_mainStat; }, std::type_identity<double>{});
            for (std::size_t option{}; option < Models::ACCESSORY_OPTION_COUNT; ++option)
            {
                AddTextChoice(specs, std::format(ACCESSORY_OPTION_FORMAT, ACCESSORY_NAMES[slot], option + 1),
                    [slot, option](auto& state) -> auto& { return state.m_accessories[slot].m_options[option]; }, Data::GetAccessoryCandidates(ACCESSORY_KINDS[slot]));
            }
        }
    }

    // 팔찌 스탯 · 옵션, 어빌리티 스톤 두 줄, 각인서 5개
    void CombatExtraEditor::BuildBraceletRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_BRACELET)] };

        AddHeader(specs, BRACELET_STAT_HEADER);
        for (std::size_t line{}; line < Models::BRACELET_STAT_COUNT; ++line)
        {
            AddTextChoice(specs, std::format(BRACELET_STAT_TYPE_FORMAT, line + 1),
                [line](auto& state) -> auto& { return state.m_braceletStats[line].m_type; }, Data::GetBraceletStatTypes());
            AddNumber(specs, std::format(BRACELET_STAT_VALUE_FORMAT, line + 1),
                [line](auto& state) -> auto& { return state.m_braceletStats[line].m_value; }, std::type_identity<int>{});
        }

        AddHeader(specs, BRACELET_OPTION_HEADER);
        for (std::size_t line{}; line < Models::BRACELET_OPTION_COUNT; ++line)
        {
            AddTextChoice(specs, std::format(BRACELET_OPTION_FORMAT, line + 1),
                [line](auto& state) -> auto& { return state.m_braceletOptions[line]; }, Data::GetBraceletOptionCandidates());
        }

        AddHeader(specs, STONE_HEADER);
        for (std::size_t line{}; line < Models::STONE_LINE_COUNT; ++line)
        {
            AddTextChoice(specs, std::format(STONE_NAME_FORMAT, line + 1),
                [line](auto& state) -> auto& { return state.m_stone[line].m_name; }, Data::GetStoneCandidates());
            AddNumber(specs, std::format(STONE_LEVEL_FORMAT, line + 1),
                [line](auto& state) -> auto& { return state.m_stone[line].m_level; }, std::type_identity<int>{});
        }

        AddHeader(specs, BOOK_HEADER);
        std::vector<std::wstring> gradeLevels;   // "유물 X0" ... "유물 X4", "전설 X0" ... "전설 X4"
        for (const std::wstring_view grade : { RELIC_NAME, LEGEND_NAME })
        {
            for (std::size_t level{}; level < BOOK_LEVEL_COUNT; ++level) gradeLevels.push_back(std::format(BOOK_FORMAT, grade, level));
        }
        for (std::size_t book{}; book < Models::BOOK_COUNT; ++book)
        {
            AddTextChoice(specs, std::format(BOOK_NAME_FORMAT, book + 1),
                [book](auto& state) -> auto& { return state.m_books[book].m_name; }, Data::GetBookCandidates());
            AddIndexChoice(specs, std::format(BOOK_LEVEL_FORMAT, book + 1), gradeLevels,
                [book](const Models::ExtraState& state)
                {
                    const Models::BookState& item{ state.m_books[book] };
                    return (item.m_isLegendary ? BOOK_LEVEL_COUNT : 0) + static_cast<std::size_t>(std::clamp(item.m_level, Models::MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL));
                },
                [book](Models::ExtraState& state, std::size_t index)
                {
                    state.m_books[book].m_isLegendary = index >= BOOK_LEVEL_COUNT;
                    state.m_books[book].m_level = static_cast<int>(index % BOOK_LEVEL_COUNT);
                });
        }
    }

    // 아크 그리드: 코어 6개 (등급 선택 + 포인트 숫자) + 젬 효과 레벨 합 3개
    void CombatExtraEditor::BuildArkGridRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_ARK_GRID)] };

        AddHeader(specs, CORE_HEADER);
        for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core)
        {
            AddIndexChoice(specs, std::format(CORE_GRADE_FORMAT, CORE_NAMES[core]), ToWideList(CORE_GRADE_CHOICES),
                [core](const Models::ExtraState& state) { return static_cast<std::size_t>(state.m_cores[core].m_grade); },
                [core](Models::ExtraState& state, std::size_t index) { state.m_cores[core].m_grade = static_cast<Models::ArkCoreGrade>(index); });
            AddNumber(specs, std::format(CORE_POINT_FORMAT, CORE_NAMES[core]),
                [core](auto& state) -> auto& { return state.m_cores[core].m_points; }, std::type_identity<int>{});
        }

        AddHeader(specs, GEM_EFFECT_HEADER);
        for (std::size_t effect{}; effect < Models::ARK_GEM_EFFECT_COUNT; ++effect)
        {
            AddNumber(specs, GEM_EFFECT_NAMES[effect], [effect](auto& state) -> auto& { return state.m_arkGemLevels[effect]; }, std::type_identity<int>{});
        }
    }

    // 아크 패시브 (랭크 · 깨달음 포인트 구성 · 트리 레벨) + 아바타 4부위
    void CombatExtraEditor::BuildPassiveRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_PASSIVE)] };

        AddHeader(specs, PASSIVE_HEADER);
        std::vector<std::wstring> ranks;
        for (int rank{ Models::MIN_VALUE }; rank <= Models::MAX_PASSIVE_RANK; ++rank) ranks.push_back(std::format(RANK_FORMAT, rank));
        AddIndexChoice(specs, RANK_LABEL, std::move(ranks),
            [](const Models::ExtraState& state) { return static_cast<std::size_t>(std::clamp(state.m_passive.m_rank, Models::MIN_VALUE, Models::MAX_PASSIVE_RANK)); },
            [](Models::ExtraState& state, std::size_t index) { state.m_passive.m_rank = static_cast<int>(index); });

        // 깨달음 포인트 구성: 배율이 큰 구성부터
        std::vector<Data::KeyedValue> enlightenments{ Data::GetEnlightenmentRatios().begin(), Data::GetEnlightenmentRatios().end() };
        std::ranges::sort(enlightenments, std::greater<>{}, &Data::KeyedValue::m_value);
        std::vector<std::wstring> enlightenmentItems;
        for (const Data::KeyedValue& item : enlightenments) enlightenmentItems.push_back(Wide(item.m_key));
        AddIndexChoice(specs, ENLIGHTENMENT_LABEL, enlightenmentItems,
            [enlightenmentItems](const Models::ExtraState& state)
            {
                const auto found{ std::ranges::find(enlightenmentItems, Wide(state.m_passive.m_enlightenment)) };
                return static_cast<std::size_t>(std::distance(enlightenmentItems.begin(), found));   // 목록에 없으면 끝(빈 글자)
            },
            [enlightenmentItems](Models::ExtraState& state, std::size_t index)
            {
                if (index < enlightenmentItems.size()) state.m_passive.m_enlightenment = Narrow(enlightenmentItems[index]);
            });

        for (std::size_t tree{}; tree < Models::PASSIVE_TREE_COUNT; ++tree)
        {
            AddNumber(specs, PASSIVE_LEVEL_NAMES[tree], [tree](auto& state) -> auto& { return state.m_passive.m_levels[tree]; }, std::type_identity<int>{});
        }

        AddHeader(specs, AVATAR_HEADER);
        for (std::size_t slot{}; slot < Models::AVATAR_SLOT_COUNT; ++slot)
        {
            AddIndexChoice(specs, AVATAR_NAMES[slot], ToWideList(AVATAR_CHOICES),
                [slot](const Models::ExtraState& state) { return static_cast<std::size_t>(state.m_avatars[slot]); },
                [slot](Models::ExtraState& state, std::size_t index) { state.m_avatars[slot] = static_cast<Models::AvatarGrade>(index); });
        }
    }

    std::vector<CombatListRow> CombatExtraEditor::BuildRows(CombatCard card, const Models::ExtraState& current, const Models::ExtraState& target) const
    {
        std::vector<CombatListRow> rows;
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };
        if (cardIndex >= m_specs.size()) return rows;

        rows.reserve(m_specs[cardIndex].size());
        for (const RowSpec& spec : m_specs[cardIndex])
        {
            CombatListRow& row{ rows.emplace_back() };
            row.m_label = spec.m_label;
            row.m_kind = spec.m_kind;
            if (spec.m_kind == CombatRowKind::ROW_HEADER) continue;

            row.m_current = spec.m_read(current);
            row.m_target = spec.m_read(target);
            row.m_hasChange = row.m_current != row.m_target;
            if (spec.m_kind != CombatRowKind::ROW_CHOICE) continue;

            row.m_choices = spec.m_choices(current);
            auto found{ std::ranges::find(row.m_choices, row.m_target) };
            if (found == row.m_choices.end() && !row.m_target.empty())
            {
                // 목표가 선택지에 없으면(저장 파일 등) 끼워서 고를 수 있게
                found = row.m_choices.insert(row.m_choices.empty() ? row.m_choices.begin() : row.m_choices.begin() + 1, row.m_target);
            }
            if (found != row.m_choices.end()) row.m_selectedChoice = static_cast<std::size_t>(std::distance(row.m_choices.begin(), found));
        }
        return rows;
    }

    bool CombatExtraEditor::Apply(CombatCard card, std::size_t row, const std::wstring& text, Models::ExtraState& target) const
    {
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };
        if (cardIndex >= m_specs.size() || row >= m_specs[cardIndex].size()) return false;

        const RowSpec& spec{ m_specs[cardIndex][row] };
        if (spec.m_kind == CombatRowKind::ROW_HEADER || !spec.m_write) return false;

        // 각인서는 같은 이름을 두 칸에 쓸 수 없음 (게임 · lopec 모두 무시함) → 중복이 새로 생기면 반영하지 않음
        Models::ExtraState edited{ target };
        spec.m_write(edited, text);
        if (HasDuplicateBook(edited) && !HasDuplicateBook(target)) return false;
        target = std::move(edited);
        return true;
    }
}
