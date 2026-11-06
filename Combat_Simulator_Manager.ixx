export module LostArkMultiTool.Views.CombatSimulatorManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.CombatPowerCalculator;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Models.CombatState;
import LostArkMultiTool.Services.CombatSimulatorStore;
import LostArkMultiTool.Views.CombatExtraEditor;
import LostArkMultiTool.Views.CombatSimulatorView;

export namespace LostArkMultiTool::Views
{
    // 전투력 시뮬레이터 최상위 매니저 (캐릭터 무기고 조회 → 목표 재련 · 품질 · 보석 · 악세서리 · 팔찌 · 스톤/각인서 · 아바타 · 아크 그리드/패시브 · 완갑 → 예상 전투력)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = CombatSimulatorView, 계산 = CombatPowerCalculator(배율 요소 전략들),
    // 저장 = CombatSimulatorStore (마지막 캐릭터 이름 + 목표), 조회 = MarketAPI::GetArmory (백그라운드)
    class CombatSimulatorManager final : public Core::ToolManager, public Core::Singleton<CombatSimulatorManager>
    {
    public:
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;

        void Initialize(Core::ToolContext& context) override;
        void Shutdown() override;

        void OnActivated() override;
        void OnDeactivated() override;

        void Update(double deltaSeconds) override;
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;

        [[nodiscard]] bool HandleInput(const Core::InputEvent& event) override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;

    private:
        friend class Core::Singleton<CombatSimulatorManager>;   // GetInstance만 생성 · 소멸할 수 있게

        CombatSimulatorManager();
        ~CombatSimulatorManager() override;

        // ===== 조회 =====
        void RequestSearch(const std::wstring& characterName);
        void FinishSearch();   // 백그라운드 조회가 끝났을 때 (화면 스레드에서)
        void HandleAPIKeyError(const std::string& message);
        void ApplyArmory(Models::CharacterArmory armory);   // 무기고 → 현재 상태 + 목표 (같은 캐릭터면 저장된 목표 복원)

        // ===== 사용자 입력 =====
        void OnGearCellEdited(std::size_t row, CombatGearColumn column, const std::wstring& text);
        void OnQualityChanged(const std::wstring& text);
        void OnGemTargetSelected(std::size_t index);                       // 모든 보석 목표 (한꺼번에)
        void OnGemCellEdited(std::size_t row, const std::wstring& text);   // 보석 하나 목표 (개별로 덮어씀)
        void OnCardEdited(CombatCard card, std::size_t row, const std::wstring& text);   // 목록형 카드의 숫자 입력 · 선택
        void OnVambraceEdited(CombatGearColumn column, const std::wstring& text);        // 장비 표 마지막 행(완갑) 입력

        // ===== 화면 갱신 · 저장 =====
        void RefreshAll();
        void RefreshGears();
        void RefreshCards();     // 악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타 카드
        void RefreshOptions();
        void RefreshResult();
        void RefreshStatus();
        void RefreshGems();      // 보석 표 + 모든 보석 목표 선택 표시
        [[nodiscard]] std::size_t GetGemTargetIndex() const;   // 목표 보석 목록 → 선택지 번호 (현재 유지 · N레벨 · 개별 설정)
        void StoreTargets();     // 목표 → 저장 설정
        void SaveData();

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 모델 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Services::CombatSimulatorStore> m_store;
        std::unique_ptr<Core::CombatPowerCalculator> m_calculator;
        std::unique_ptr<CombatExtraEditor> m_editor;
        std::unique_ptr<CombatSimulatorView> m_view;

        Core::BackgroundTask<Models::CharacterArmory> m_armoryTask;   // 무기고 조회
        std::optional<Models::CharacterArmory> m_armory;              // 마지막으로 받은 무기고 (조회 전 = nullopt)
        Models::CombatState m_current;                                // API 기준 현재 상태
        Models::CombatState m_target;                                 // 사용자가 고친 목표 상태
        std::optional<std::wstring> m_searchError;                    // 마지막 조회 오류 (상태 줄에 표시)
    };
}
