module LostArkMultiTool.Models.BoundGemRosterData;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Models.BoundGemData;
import LostArkMultiTool.Models.CharacterInfo;

namespace LostArkMultiTool::Models
{
    namespace
    {
        // 수량 하나라도 있는지 (0이 아닌 칸이 있으면 true)
        [[nodiscard]] bool HasAnyQuantity(const BoundGemData& data)
        {
            for (std::size_t gemIndex{}; gemIndex < BoundGemData::GEM_COUNT; ++gemIndex)
            {
                for (std::size_t gradeIndex{}; gradeIndex < BoundGemData::GRADE_COUNT; ++gradeIndex)
                {
                    if (data.GetQuantity(gemIndex, gradeIndex) > 0) return true;
                }
            }
            return false;
        }

        // target에 source 수량을 더함 (int 범위를 넘지 않게 최대값에서 멈춤)
        void AddQuantities(BoundGemData& target, const BoundGemData& source)
        {
            constexpr int MAX_QUANTITY{ std::numeric_limits<int>::max() };
            for (std::size_t gemIndex{}; gemIndex < BoundGemData::GEM_COUNT; ++gemIndex)
            {
                for (std::size_t gradeIndex{}; gradeIndex < BoundGemData::GRADE_COUNT; ++gradeIndex)
                {
                    const int current{ target.GetQuantity(gemIndex, gradeIndex) };
                    const int added{ source.GetQuantity(gemIndex, gradeIndex) };
                    target.SetQuantity(gemIndex, gradeIndex, added > MAX_QUANTITY - current ? MAX_QUANTITY : current + added);
                }
            }
        }
    }

    BoundGemRosterData::BoundGemRosterData() noexcept = default;

    const std::vector<BoundGemCharacter>& BoundGemRosterData::GetCharacters() const noexcept
    {
        return m_characters;
    }

    BoundGemCharacter* BoundGemRosterData::FindCharacter(std::string_view name) noexcept
    {
        const auto found{ std::ranges::find(m_characters, name, &BoundGemCharacter::m_name) };
        return found != m_characters.end() ? &*found : nullptr;
    }

    const BoundGemCharacter* BoundGemRosterData::FindCharacter(std::string_view name) const noexcept
    {
        const auto found{ std::ranges::find(m_characters, name, &BoundGemCharacter::m_name) };
        return found != m_characters.end() ? &*found : nullptr;
    }

    void BoundGemRosterData::AddCharacter(BoundGemCharacter character)
    {
        if (BoundGemCharacter* existing{ FindCharacter(character.m_name) }; existing != nullptr) *existing = std::move(character);
        else m_characters.push_back(std::move(character));
        SortCharacters();
    }

    bool BoundGemRosterData::SetRoster(const std::vector<CharacterInfo>& roster)
    {
        std::vector<BoundGemCharacter> characters;
        characters.reserve(roster.size());
        for (const CharacterInfo& info : roster)
        {
            // 기존 캐릭터면 수량은 그대로 두고 직업 · 레벨만 갱신
            const BoundGemCharacter* existing{ FindCharacter(info.GetCharacterName()) };
            BoundGemCharacter character{ existing != nullptr ? *existing : BoundGemCharacter{ info.GetCharacterName() } };
            character.m_className = info.GetCharacterClassName();
            character.m_itemLevel = info.GetNumericItemLevel();
            characters.push_back(std::move(character));
        }
        m_characters = std::move(characters);
        SortCharacters();

        // 공용 수량 → 레벨이 가장 높은 캐릭터 (예전 파일의 수량을 잃지 않게)
        if (m_characters.empty() || !HasUnassigned()) return false;
        AddQuantities(m_characters.front().m_quantities, m_unassigned);
        m_unassigned = BoundGemData{};
        return true;
    }

    BoundGemData& BoundGemRosterData::GetUnassigned() noexcept
    {
        return m_unassigned;
    }

    const BoundGemData& BoundGemRosterData::GetUnassigned() const noexcept
    {
        return m_unassigned;
    }

    bool BoundGemRosterData::HasUnassigned() const
    {
        return HasAnyQuantity(m_unassigned);
    }

    BoundGemData BoundGemRosterData::GetTotal() const
    {
        BoundGemData total{ m_unassigned };
        for (const BoundGemCharacter& character : m_characters) AddQuantities(total, character.m_quantities);
        return total;
    }

    const std::map<std::string, double>& BoundGemRosterData::GetHistory() const noexcept
    {
        return m_history;
    }

    void BoundGemRosterData::RecordHistory(const std::string& dateKey, double totalValue)
    {
        m_history.insert_or_assign(dateKey, std::max(totalValue, 0.0));

        // "yyyy-MM-dd"는 글자순 = 날짜순 → 앞(오래된 것)부터 지움
        while (m_history.size() > static_cast<std::size_t>(Core::APPConstants::HISTORY_KEEP_DAYS)) m_history.erase(m_history.begin());
    }

    // 레벨 높은 순, 같으면 이름순 (실행할 때마다 순서가 바뀌지 않게)
    void BoundGemRosterData::SortCharacters()
    {
        std::ranges::sort(m_characters, [](const BoundGemCharacter& left, const BoundGemCharacter& right)
        {
            if (left.m_itemLevel != right.m_itemLevel) return left.m_itemLevel > right.m_itemLevel;
            return left.m_name < right.m_name;
        });
    }
}
