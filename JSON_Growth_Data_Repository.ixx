export module LostArkMultiTool.Services.JSONGrowthDataRepository;

import std;
import LostArkMultiTool.Core.GrowthDataRepository;
import LostArkMultiTool.Models.GrowthData;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 낙원력 트래커 JSON 저장소 (ParadiseTrackerData.json 전용, 원본 DataManager)
    // 저장 파일이 없으면 원본 낙원력 트래커의 기록(%LOCALAPPDATA%\낙원력 트래커)을 한 번만 읽어 와서 이 파일로 저장 (원본은 그대로)
    //  - 가져오는 순서: RosterGrowthData_v2.json → 그 .bak → 이전 버전 RosterGrowthData.json → 그 .bak
    // 파일을 안전하게 읽고 쓰는 일은 JSONFileStore, JSON 해석 · 작성은 JSONValue가 맡고 여기서는 모델 변환만 함
    class JSONGrowthDataRepository final : public Core::GrowthDataRepository
    {
    public:
        JSONGrowthDataRepository();

        [[nodiscard]] Models::GrowthData& GetData() override;
        [[nodiscard]] const Models::GrowthData& GetData() const override;
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept override;
        void Save() override;

    private:
        JSONFileStore m_fileStore;
        Models::GrowthData m_data;
        std::optional<std::string> m_loadWarning;
    };
}
