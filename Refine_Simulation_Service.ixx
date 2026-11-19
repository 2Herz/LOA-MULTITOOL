export module LostArkMultiTool.Core.RefineSimulationService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.RefinePlan;

export namespace LostArkMultiTool::Core
{
    // 시작 상태 프리셋 (로아업 통합 재련과 같은 세 가지)
    enum class RefineStartPreset
    {
        PRESET_1700,   // 에기르 +18 · 상급 20
        PRESET_1730,   // 세르카 +11
        PRESET_1750    // 세르카 +15
    };

    // 프리셋의 부위 상태 (여섯 부위 모두 같은 상태)
    [[nodiscard]] Models::RefineData::EquipmentState GetStartPresetState(RefineStartPreset preset) noexcept;

    // 재료 시세 조회 결과 하나 (조회에 성공한 재료만)
    struct RefinePriceQuote
    {
        Models::RefineData::RefineMaterial m_material{};
        double m_price{};   // 1개당 골드
    };

    // 재련 시뮬레이터의 모델 (시작 · 목표 · 재료 가격 · 귀속 설정, 저장 파일 포함)
    // 스레드 규칙: FetchPrices만 백그라운드 스레드에서 호출 가능, 나머지는 화면 스레드에서만 호출
    class RefineSimulationService
    {
    public:
        virtual ~RefineSimulationService();

        // ===== 계산 입력 =====
        [[nodiscard]] virtual const Models::RefineRequest& GetRequest() const noexcept = 0;
        virtual void SetStart(Models::RefineSlot slot, const Models::RefineData::EquipmentState& state) = 0;
        virtual void SetTarget(Models::RefineSlot slot, const Models::RefineData::EquipmentState& state) = 0;
        virtual void SetTargetMode(Models::RefineTargetMode mode) = 0;
        virtual void SetTargetItemLevel(int itemLevel) = 0;
        virtual void ApplyStartPreset(RefineStartPreset preset) = 0;   // 여섯 부위 시작 상태를 한 번에

        // ===== 캐릭터 조회 =====
        [[nodiscard]] virtual const std::wstring& GetCharacterName() const noexcept = 0;   // 마지막으로 조회한 캐릭터 이름 (없으면 빈 글자)
        virtual void SetCharacterName(std::wstring name) = 0;                              // 조회한 캐릭터 이름 기억 (저장은 Save)

        // ===== 재료 가격 · 귀속 =====
        [[nodiscard]] virtual const Models::RefinePrices& GetPrices() const noexcept = 0;
        virtual void SetPrice(Models::RefineData::RefineMaterial material, double price) = 0;   // 직접 입력 (다음 시세 갱신 때 덮어씀)
        virtual void SetBound(Models::RefineData::RefineMaterial material, bool isBound) = 0;
        [[nodiscard]] virtual bool HasPrices() const noexcept = 0;                             // 한 번이라도 시세를 받았는지 (저장 파일 포함)
        [[nodiscard]] virtual std::uint64_t GetPriceRevision() const noexcept = 0;             // 가격 · 귀속이 바뀔 때마다 1씩 (비용표 캐시 확인용)
        [[nodiscard]] virtual const std::optional<std::int64_t>& GetLastUpdated() const noexcept = 0;   // 마지막 시세 갱신 시각 (UTC 초)

        // ===== 보유 재료 =====
        [[nodiscard]] virtual const Models::MaterialAmounts& GetHoldings() const noexcept = 0;              // 재료별 보유 수량 (RefineMaterial 순서)
        virtual void SetHolding(Models::RefineData::RefineMaterial material, double amount) = 0;            // 보유 수량 직접 입력 · 불러오기 (음수는 0)

        // 거래소에서 재료 시세 조회 (데이터를 건드리지 않아 백그라운드 스레드에서 호출해도 안전, 매물이 없는 재료는 빠짐)
        [[nodiscard]] virtual std::vector<RefinePriceQuote> FetchPrices(MarketAPI& api) const = 0;
        virtual void ApplyPrices(const std::vector<RefinePriceQuote>& quotes) = 0;

        // ===== 저장 =====
        [[nodiscard]] virtual const std::optional<std::string>& GetLoadWarning() const noexcept = 0;   // 손상 복구 안내 (없으면 nullopt)
        virtual void Save() = 0;   // 실패하면 예외 (화면에서 안내)
    };
}
