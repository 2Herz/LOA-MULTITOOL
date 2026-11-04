export module LostArkMultiTool.Services.JSONBoundGemRepository;

import std;
import LostArkMultiTool.Core.BoundGemRepository;
import LostArkMultiTool.Models.BoundGemRosterData;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 귀속 젬 보유 수량 JSON 저장소 (BoundGemData.json, 사용자 데이터 폴더)
    // 형식: { "Characters": [ { "Name": "캐릭터", "ClassName": "직업", "ItemLevel": 1740.0,
    //                          "Quantities": [ { "Name": "질서의 젬 : 안정", "Grade": "영웅", "Count": 3 }, ... ] }, ... ],
    //         "Quantities": [ ... ],    ← 캐릭터별 저장 이전 파일의 공용 수량 (원정대 동기화 전까지만 남음)
    //         "History": { "2026-11-06": 1234567.0, ... } }   ← 날짜별 원정대 총 가치 (그래프)
    // 0개는 저장하지 않음
    class JSONBoundGemRepository final : public Core::BoundGemRepository
    {
    public:
        JSONBoundGemRepository();

        [[nodiscard]] Models::BoundGemRosterData& GetData() override;
        [[nodiscard]] const Models::BoundGemRosterData& GetData() const override;
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept override;
        void Save() override;

    private:
        JSONFileStore m_fileStore;
        Models::BoundGemRosterData m_data;
        std::optional<std::string> m_loadWarning;
    };
}
