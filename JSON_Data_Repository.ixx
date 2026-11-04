export module LostArkMultiTool.Services.JSONDataRepository;

import std;
import LostArkMultiTool.Core.DataRepository;
import LostArkMultiTool.Models.UserData;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 귀속재료 계산기 JSON 저장소 (원정대 파일 전용)
    // 파일을 안전하게 읽고 쓰는 일은 JSONFileStore, JSON 해석은 JSONValue가 맡고 여기서는 모델 변환만 함
    class JSONDataRepository final : public Core::DataRepository
    {
    public:
        JSONDataRepository();

        [[nodiscard]] Models::UserData& GetData() override;
        [[nodiscard]] const Models::UserData& GetData() const override;
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept override;
        void Save() override;

    private:
        JSONFileStore m_fileStore;
        Models::UserData m_data;
        std::optional<std::string> m_loadWarning;
    };
}