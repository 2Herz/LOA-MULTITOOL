export module LostArkMultiTool.Core.PriceHistoryRepository;

import std;
import LostArkMultiTool.Models.PriceHistory;

export namespace LostArkMultiTool::Core
{
    // 일별 최저가 기록 저장소 인터페이스 (실제 구현은 JSON 저장소, 보석 검색기의 GemDataRepository와 같은 구조)
    // 매니저가 기록을 고치고 저장하며, 손상 복구 안내(GetLoadWarning)는 화면 쪽이 읽음
    class PriceHistoryRepository
    {
    public:
        virtual ~PriceHistoryRepository();

        [[nodiscard]] virtual Models::PriceHistory& GetHistory() = 0;                 // 기록 (수정 가능)
        [[nodiscard]] virtual const Models::PriceHistory& GetHistory() const = 0;     // 기록 (읽기 전용)

        // 손상 복구 안내 문구 (없으면 nullopt)
        [[nodiscard]] virtual const std::optional<std::string>& GetLoadWarning() const noexcept = 0;

        virtual void Save() = 0;   // 파일에 저장 (실패하면 예외)
    };
}
