export module LostArkMultiTool.Core.GrowthDataRepository;

import std;
import LostArkMultiTool.Models.GrowthData;

export namespace LostArkMultiTool::Core
{
    // 낙원력 트래커 저장 파일 읽기 · 쓰기 인터페이스 (실제 구현은 JSON 저장소, 원본 IDataManager)
    class GrowthDataRepository
    {
    public:
        virtual ~GrowthDataRepository();

        [[nodiscard]] virtual Models::GrowthData& GetData() = 0;
        [[nodiscard]] virtual const Models::GrowthData& GetData() const = 0;

        // 손상 복구 · 원본 낙원력 트래커 기록 가져오기 안내 문구 (없으면 nullopt)
        [[nodiscard]] virtual const std::optional<std::string>& GetLoadWarning() const noexcept = 0;

        virtual void Save() = 0;   // 실패하면 예외 (화면에서 안내)
    };
}
