export module LostArkMultiTool.Core.RosterService;

import std;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.GrowthData;
import LostArkMultiTool.Models.Season;

export namespace LostArkMultiTool::Core
{
    // 닉네임 변경 후보 (동기화 전후 비교 결과, 저장하지 않는 읽기 전용 값, 원본 RenameCandidate)
    struct RenameCandidate
    {
        std::string m_oldName;
        std::string m_newName;
        std::optional<std::string> m_className;
    };

    // 원정대 변화 처리 인터페이스 (닉네임 변경 감지 · 기록 이전, 원정대에서 빠진 캐릭터 찾기, 원본 IRosterService)
    class RosterService
    {
    public:
        virtual ~RosterService();

        // 동기화 전후 원정대를 비교해서 "닉네임만 바뀐 것 같은" 캐릭터 쌍을 찾음
        [[nodiscard]] virtual std::vector<RenameCandidate> FindRenameCandidates(const std::vector<Models::CharacterInfo>& oldRoster,
            const std::vector<Models::CharacterInfo>& newRoster, const Models::GrowthData& data) const = 0;

        // 모든 시즌의 기록을 예전 이름 → 새 이름으로 옮김 (옮긴 기록 수 반환)
        virtual int RenameCharacter(Models::GrowthData& data, const std::string& oldName, const std::string& newName) const = 0;

        // 시즌 기록은 있지만 현재 원정대에는 없는 캐릭터 이름 (처음 기록된 순서)
        [[nodiscard]] virtual std::vector<std::string> GetPastCharacters(const Models::Season& season,
            const std::vector<Models::CharacterInfo>& roster) const = 0;
    };
}
