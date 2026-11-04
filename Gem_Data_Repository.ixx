export module LostArkMultiTool.Core.GemDataRepository;

import std;
import LostArkMultiTool.Models.GemData;

export namespace LostArkMultiTool::Core
{
    // 보석 검색기 저장 파일 읽기 · 쓰기 인터페이스 (실제 구현은 JSON 저장소, 원본 IGemDataRepository)
    // 서비스만 데이터를 고치고, 화면 쪽은 손상 복구 안내(GetLoadWarning)만 읽음
    class GemDataRepository
    {
    public:
        virtual ~GemDataRepository();

        [[nodiscard]] virtual Models::GemData& GetData() = 0;
        [[nodiscard]] virtual const Models::GemData& GetData() const = 0;

        // 손상 복구 안내 문구 (없으면 nullopt)
        [[nodiscard]] virtual const std::optional<std::string>& GetLoadWarning() const noexcept = 0;

        virtual void Save() = 0;
    };
}