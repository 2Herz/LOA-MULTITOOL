module LostArkMultiTool.Services.RosterChangeService;

import std;
import LostArkMultiTool.Core.RosterService;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.GrowthData;
import LostArkMultiTool.Models.Season;
import LostArkMultiTool.Models.WeeklyRecord;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr double UNRECORDED_SCORE{ 0.0 };
        constexpr std::size_t SINGLE_MATCH{ 1 };   // 정확히 한 명일 때만 같은 캐릭터로 추정

        // 같은 서버 · 같은 직업인지
        [[nodiscard]] bool IsSameType(const Models::CharacterInfo& left, const Models::CharacterInfo& right)
        {
            return left.GetServerName() == right.GetServerName() && left.GetCharacterClassName() == right.GetCharacterClassName();
        }

        // 어느 시즌이든 0보다 큰 기록이 있는지
        [[nodiscard]] bool HasAnyScore(const Models::GrowthData& data, const std::string& characterName)
        {
            for (const Models::Season& season : data.GetSeasons())
            {
                for (const Models::WeeklyRecord& record : season.GetRecords())
                {
                    if (record.GetScore(characterName) > UNRECORDED_SCORE) return true;
                }
            }
            return false;
        }

        [[nodiscard]] std::set<std::string> CollectNames(const std::vector<Models::CharacterInfo>& roster)
        {
            std::set<std::string> names;
            for (const Models::CharacterInfo& character : roster) names.insert(character.GetCharacterName());
            return names;
        }
    }

    // 동기화 전후 비교: 사라진 캐릭터(기록 있음)와 새 캐릭터(기록 없음)가 같은 서버 · 직업으로 정확히 1:1일 때만 후보
    std::vector<Core::RenameCandidate> RosterChangeService::FindRenameCandidates(const std::vector<Models::CharacterInfo>& oldRoster,
        const std::vector<Models::CharacterInfo>& newRoster, const Models::GrowthData& data) const
    {
        const std::set<std::string> oldNames{ CollectNames(oldRoster) };
        const std::set<std::string> newNames{ CollectNames(newRoster) };

        // 포인터는 호출하는 쪽의 원정대 목록을 가리키기만 함 (이 함수 안에서만 사용)
        std::vector<const Models::CharacterInfo*> removed;
        for (const Models::CharacterInfo& character : oldRoster)
        {
            if (!newNames.contains(character.GetCharacterName()) && HasAnyScore(data, character.GetCharacterName())) removed.push_back(&character);
        }

        std::vector<const Models::CharacterInfo*> added;
        for (const Models::CharacterInfo& character : newRoster)
        {
            if (!oldNames.contains(character.GetCharacterName()) && !HasAnyScore(data, character.GetCharacterName())) added.push_back(&character);
        }

        std::vector<Core::RenameCandidate> candidates;
        for (const Models::CharacterInfo* oldCharacter : removed)
        {
            // 새 캐릭터가 정확히 1명일 때만 (없거나 여러 명 → 추정 불가)
            const Models::CharacterInfo* match{};
            std::size_t matchCount{};
            for (const Models::CharacterInfo* newCharacter : added)
            {
                if (!IsSameType(*oldCharacter, *newCharacter)) continue;
                match = newCharacter;
                ++matchCount;
            }
            if (matchCount != SINGLE_MATCH) continue;

            // 같은 새 캐릭터를 두 명 이상이 가리키면 애매하므로 제외
            const auto pointingCount{ std::ranges::count_if(removed, [match](const Models::CharacterInfo* other) { return IsSameType(*other, *match); }) };
            if (static_cast<std::size_t>(pointingCount) != SINGLE_MATCH) continue;

            candidates.push_back(Core::RenameCandidate{ oldCharacter->GetCharacterName(), match->GetCharacterName(), oldCharacter->GetCharacterClassName() });
        }
        return candidates;
    }

    // 모든 시즌 · 주차의 기록 이름 변경 (새 이름에 이미 값이 있는 주차는 덮어쓰지 않고 둘 다 보존)
    int RosterChangeService::RenameCharacter(Models::GrowthData& data, const std::string& oldName, const std::string& newName) const
    {
        int movedCount{};
        for (Models::Season& season : data.GetSeasons())
        {
            for (Models::WeeklyRecord& record : season.GetRecords())
            {
                std::map<std::string, double>& scores{ record.GetScores() };
                const auto found{ scores.find(oldName) };
                if (found == scores.end()) continue;
                if (!scores.try_emplace(newName, found->second).second) continue;   // 이미 있으면 그대로 둠 (원본 TryAdd)

                scores.erase(found);   // try_emplace는 다른 원소의 반복자를 무효화하지 않음
                ++movedCount;
            }
        }
        return movedCount;
    }

    // 시즌 기록(0보다 큰 값)은 있지만 현재 원정대에 없는 캐릭터 (주차 순서대로 처음 기록된 순서)
    std::vector<std::string> RosterChangeService::GetPastCharacters(const Models::Season& season, const std::vector<Models::CharacterInfo>& roster) const
    {
        const std::set<std::string> rosterNames{ CollectNames(roster) };
        std::vector<std::string> pastNames;
        std::set<std::string> seenNames;

        for (const Models::WeeklyRecord& record : season.GetRecords())
        {
            for (const auto& [name, score] : record.GetScores())
            {
                if (score <= UNRECORDED_SCORE || rosterNames.contains(name) || !seenNames.insert(name).second) continue;
                pastNames.push_back(name);
            }
        }
        return pastNames;
    }
}
