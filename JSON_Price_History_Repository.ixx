export module LostArkMultiTool.Services.JSONPriceHistoryRepository;

import std;
import LostArkMultiTool.Core.PriceHistoryRepository;
import LostArkMultiTool.Models.PriceHistory;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 일별 최저가 기록 저장 파일 이름 (사용자 데이터 폴더 기준, 멀티툴 전용 새 파일)
    namespace PriceHistoryFiles
    {
        inline constexpr std::string_view ENGRAVING_FILE_NAME{ "EngravingFinderData.json" };   // 유각 시세 검색기
        inline constexpr std::string_view ARK_GEM_FILE_NAME{ "ArkGemFinderData.json" };        // 젬 시세 검색기
    }

    // 일별 최저가 기록 JSON 저장소 (파일 이름만 바꿔 유각 · 젬 검색기가 각각 하나씩 사용)
    // 파일을 안전하게 읽고 쓰는 일은 JSONFileStore(손상 시 백업 복구 · 원자적 저장), JSON 해석 · 작성은 JSONValue가 맡고 여기서는 모델 변환만 함
    class JSONPriceHistoryRepository final : public Core::PriceHistoryRepository
    {
    public:
        explicit JSONPriceHistoryRepository(std::string_view fileName);

        [[nodiscard]] Models::PriceHistory& GetHistory() override;
        [[nodiscard]] const Models::PriceHistory& GetHistory() const override;
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept override;
        void Save() override;

    private:
        JSONFileStore m_fileStore;                      // 저장 파일 도우미
        Models::PriceHistory m_history;                 // 메모리의 기록
        std::optional<std::string> m_loadWarning;       // 손상 복구 안내 문구
    };
}
