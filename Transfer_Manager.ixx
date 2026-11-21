export module LostArkMultiTool.Views.TransferManager;  // 2부 이관 매니저 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.BackgroundTask;  // 백그라운드 작업 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Core.Singleton;  // 싱글톤 모듈 가져오기
import LostArkMultiTool.Core.ToolContext;  // 도구 문맥 모듈 가져오기
import LostArkMultiTool.Core.ToolManager;  // 도구 매니저 기반 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Services.TransferCalculator;  // 2부 이관 계산기 가져오기
import LostArkMultiTool.Views.TransferView;  // 2부 이관 화면 가져오기

export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 2부 이관 최상위 매니저 (캐릭터 무기고 조회 → 2부에서 받을 장비 · 슬롯 재련 · 장신구/인장 주문서 · 광휘/성휘 · 각인 업그레이드)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면 = TransferView, 계산 = TransferCalculator (표는 Models::TransferData), 조회 = MarketAPI::GetArmory (백그라운드)
    class TransferManager final : public Core::ToolManager, public Core::Singleton<TransferManager>
    {
    public:
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;  // 탭 제목

        void Initialize(Core::ToolContext& context) override;  // 초기화: 계산기 · 화면 생성
        void Shutdown() override;  // 종료: 백그라운드 작업 대기 후 정리

        void OnActivated() override;    // 탭이 선택됨
        void OnDeactivated() override;  // 탭 선택이 풀림

        void Update(double deltaSeconds) override;  // 프레임 갱신
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;  // 화면 그리기

        [[nodiscard]] bool HandleInput(const Core::InputEvent& event) override;  // 입력 처리
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;     // 커서 모양

    private:
        friend class Core::Singleton<TransferManager>;   // GetInstance만 생성 · 소멸

        TransferManager();             // 생성자 (싱글톤 전용)
        ~TransferManager() override;   // 소멸자

        // ===== 조회 =====
        void RequestSearch(const std::wstring& characterName);   // 무기고 조회 요청
        void FinishSearch();                                     // 조회가 끝났을 때 (화면 스레드)
        void HandleAPIKeyError(const std::string& message);      // API 키 오류 처리

        // ===== 화면 갱신 =====
        void RefreshAll();        // 상태 줄 · 요약 · 모든 카드
        void RefreshStatus();     // 상태 줄
        void RefreshSummary();    // 요약 큰 글자
        void RefreshArmor();      // 장비 카드
        void RefreshAccessory();  // 장신구 슬롯 카드
        void RefreshScrolls();    // 장신구 주문서 카드
        void RefreshBracelet();   // 팔찌 카드
        void RefreshGemStone();   // 보석 · 스톤 카드
        void RefreshOther();      // 에스더 · 그 밖 카드

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        std::unique_ptr<Services::TransferCalculator> m_calculator;   // 이관 계산기
        std::unique_ptr<TransferView> m_view;                         // 이관 화면

        Core::BackgroundTask<Models::CharacterArmory> m_armoryTask;   // 무기고 조회
        std::optional<Models::CharacterArmory> m_armory;              // 마지막으로 받은 무기고 (조회 전 = nullopt)
        std::optional<Services::TransferResult> m_result;             // 마지막 이관 결과
        std::optional<std::wstring> m_searchError;                    // 마지막 조회 오류 (상태 줄에 표시)
    };
}
