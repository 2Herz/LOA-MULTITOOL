export module LostArkMultiTool.Services.JSONGemDataRepository;

import std;
import LostArkMultiTool.Core.GemDataRepository;
import LostArkMultiTool.Models.GemData;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 보석 검색기 JSON 저장소 (GemFinderData.json 전용, 원본 GemDataRepository)
    // 파일을 안전하게 읽고 쓰는 일은 JSONFileStore, JSON 해석 · 작성은 JSONValue가 맡고 여기서는 모델 변환만 함
    class JSONGemDataRepository final : public Core::GemDataRepository
    {
    public:
        JSONGemDataRepository();

        [[nodiscard]] Models::GemData& GetData() override;
        [[nodiscard]] const Models::GemData& GetData() const override;
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept override;
        void Save() override;

    private:
        JSONFileStore m_fileStore;
        Models::GemData m_data;
        std::optional<std::string> m_loadWarning;
    };
}