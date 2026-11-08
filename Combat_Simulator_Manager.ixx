export module LostArkMultiTool.Views.CombatSimulatorManager;  // 전투력 시뮬레이터 매니저 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.BackgroundTask;  // 백그라운드 작업 모듈 가져오기
import LostArkMultiTool.Core.CombatPowerCalculator;  // 전투력 계산기 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Core.Singleton;  // 싱글톤 모듈 가져오기
import LostArkMultiTool.Core.ToolContext;  // 도구 문맥 모듈 가져오기
import LostArkMultiTool.Core.ToolManager;  // 도구 매니저 기반 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모델 가져오기
import LostArkMultiTool.Services.CombatSimulatorStore;  // 전투력 시뮬레이터 저장소 가져오기
import LostArkMultiTool.Views.CombatExtraEditor;  // 추가 장착 편집기 가져오기
import LostArkMultiTool.Views.CombatSimulatorView;  // 전투력 시뮬레이터 화면 가져오기

export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 전투력 시뮬레이터 최상위 매니저 (캐릭터 무기고 조회 → 목표 재련 · 품질 · 보석 · 악세서리 · 팔찌 · 스톤/각인서 · 아바타 · 아크 그리드/패시브 · 완갑 → 예상 전투력)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = CombatSimulatorView, 계산 = CombatPowerCalculator(배율 요소 전략들),
    // 저장 = CombatSimulatorStore (마지막 캐릭터 이름 + 목표), 조회 = MarketAPI::GetArmory (백그라운드)
    class CombatSimulatorManager final : public Core::ToolManager, public Core::Singleton<CombatSimulatorManager>
    {
    public:  // 공개 멤버 시작
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;  // 탭 제목 조회

        void Initialize(Core::ToolContext& context) override;  // 초기화: 저장소 · 계산기 · 편집기 · 화면 생성
        void Shutdown() override;  // 종료: 백그라운드 작업 대기 후 정리

        void OnActivated() override;  // 탭이 선택되었을 때
        void OnDeactivated() override;  // 탭이 선택 해제되었을 때

        void Update(double deltaSeconds) override;  // 프레임 갱신
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;  // 화면 그리기

        [[nodiscard]] bool HandleInput(const Core::InputEvent& event) override;  // 입력 처리
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;  // 마우스 커서 모양 조회

    private:  // 비공개 멤버 시작
        friend class Core::Singleton<CombatSimulatorManager>;   // GetInstance만 생성 · 소멸할 수 있게

        CombatSimulatorManager();  // 생성자 (싱글톤 전용)
        ~CombatSimulatorManager() override;  // 소멸자

        // ===== 조회 =====
        void RequestSearch(const std::wstring& characterName);  // 무기고 조회 요청
        void FinishSearch();   // 백그라운드 조회가 끝났을 때 (화면 스레드에서)
        void HandleAPIKeyError(const std::string& message);  // API 키 오류 처리
        void ApplyArmory(Models::CharacterArmory armory);   // 무기고 → 현재 상태 + 목표 (같은 캐릭터면 저장된 목표 복원)

        // ===== 사용자 입력 =====
        void OnGearCellEdited(std::size_t row, CombatGearColumn column, const std::wstring& text);  // 장비 칸 편집 처리
        void OnQualityChanged(const std::wstring& text);  // 무기 품질 편집 처리
        void OnGemTargetSelected(std::size_t index);                       // 모든 보석 목표 (한꺼번에)
        void OnGemCellEdited(std::size_t row, const std::wstring& text);   // 보석 하나 목표 (개별로 덮어씀)
        void OnCardEdited(CombatCard card, std::size_t row, const std::wstring& text);   // 목록형 카드의 숫자 입력 · 선택
        void OnVambraceEdited(CombatGearColumn column, const std::wstring& text);        // 장비 표 마지막 행(완갑) 입력

        // ===== 화면 갱신 · 저장 =====
        void RefreshAll();  // 전체 화면 갱신
        void RefreshGears();  // 장비 표 갱신
        void RefreshCards();     // 악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타 카드
        void RefreshOptions();  // 보석 · 품질 옵션 갱신
        void RefreshResult();  // 결과 상자 갱신
        void RefreshStatus();  // 상태 줄 갱신
        void RefreshGems();      // 보석 표 + 모든 보석 목표 선택 표시
        [[nodiscard]] std::size_t GetGemTargetIndex() const;   // 목표 보석 목록 → 선택지 번호 (현재 유지 · N레벨 · 개별 설정)
        void StoreTargets();     // 목표 → 저장 설정
        void SaveData();  // 저장 설정을 파일에 저장

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 모델 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Services::CombatSimulatorStore> m_store;  // 무기고 저장소 (마지막 캐릭터 이름 · 목표)
        std::unique_ptr<Core::CombatPowerCalculator> m_calculator;  // 전투력 계산기
        std::unique_ptr<CombatExtraEditor> m_editor;  // 추가 장착 편집기
        std::unique_ptr<CombatSimulatorView> m_view;  // 전투력 시뮬레이터 화면

        Core::BackgroundTask<Models::CharacterArmory> m_armoryTask;   // 무기고 조회
        std::optional<Models::CharacterArmory> m_armory;              // 마지막으로 받은 무기고 (조회 전 = nullopt)
        Models::CombatState m_current;                                // API 기준 현재 상태
        Models::CombatState m_target;                                 // 사용자가 고친 목표 상태
        std::optional<std::wstring> m_searchError;                    // 마지막 조회 오류 (상태 줄에 표시)
    };
}
