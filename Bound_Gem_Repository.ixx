export module LostArkMultiTool.Core.BoundGemRepository;

import std;
import LostArkMultiTool.Models.BoundGemRosterData;

export namespace LostArkMultiTool::Core
{
    // 귀속 젬 보유 수량(캐릭터별) 저장 파일 읽기 · 쓰기 인터페이스 (실제 구현은 JSON 저장소)
    class BoundGemRepository
    {
    public:
        virtual ~BoundGemRepository();

        [[nodiscard]] virtual Models::BoundGemRosterData& GetData() = 0;
        [[nodiscard]] virtual const Models::BoundGemRosterData& GetData() const = 0;

        // 손상 복구 안내 문구 (없으면 nullopt)
        [[nodiscard]] virtual const std::optional<std::string>& GetLoadWarning() const noexcept = 0;

        virtual void Save() = 0;   // 실패하면 예외 (화면에서 안내)
    };
}
