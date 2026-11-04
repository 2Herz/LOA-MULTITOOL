export module LostArkMultiTool.Services.RosterChangeService;

import std;
import LostArkMultiTool.Core.RosterService;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.GrowthData;
import LostArkMultiTool.Models.Season;

export namespace LostArkMultiTool::Services
{
    // 원정대 변화 처리 (원본 RosterService)
    // 로스트아크 API에는 캐릭터 고유 번호가 없어서, 닉네임 변경은 "같은 서버 · 같은 직업 1:1"로만 추정하고 사용자에게 확인받음
    class RosterChangeService final : public Core::RosterService
    {
    public:
        [[nodiscard]] std::vector<Core::RenameCandidate> FindRenameCandidates(const std::vector<Models::CharacterInfo>& oldRoster,
            const std::vector<Models::CharacterInfo>& newRoster, const Models::GrowthData& data) const override;

        int RenameCharacter(Models::GrowthData& data, const std::string& oldName, const std::string& newName) const override;

        [[nodiscard]] std::vector<std::string> GetPastCharacters(const Models::Season& season,
            const std::vector<Models::CharacterInfo>& roster) const override;
    };
}
