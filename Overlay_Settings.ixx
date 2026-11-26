export module LostArkMultiTool.Views.OverlaySettings;  // 인게임 오버레이 설정 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기

// 인게임 오버레이 설정 (사용자 데이터 폴더 OverlaySettings.json)
// 켜기 · 패널별 표시 · 위치 · 크기 · 미니 모드 탭 · 패널 바탕 불투명도
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 패널 하나의 저장 상태 (위치 · 크기는 오버레이 왼쪽 위 기준 DIP)
    struct OverlayPanelState
    {
        std::string m_id;          // 패널 이름 ("material" · "paradise" · "mini")
        bool m_isVisible{};        // 보이는지
        float m_x{};               // 왼쪽
        float m_y{};               // 위쪽
        float m_width{};           // 가로 (크기 조절 패널만 의미 있음)
        float m_height{};          // 세로
        float m_alpha{ 1.0f };     // 요소 투명도 (0.1 ~ 1, 패널 전체 · 글자까지)
    };

    // 오버레이 설정
    class OverlaySettings
    {
    public:
        static constexpr float DEFAULT_OPACITY{ 0.85f };   // 기본 패널 바탕 불투명도
        static constexpr float MIN_OPACITY{ 0.3f };        // 최소 불투명도
        static constexpr float MAX_OPACITY{ 1.0f };        // 최대 불투명도
        static constexpr float MIN_ELEMENT_ALPHA{ 0.1f };  // 요소 투명도 최소 (10%)
        static constexpr float MAX_ELEMENT_ALPHA{ 1.0f };  // 요소 투명도 최대 (100%)

        OverlaySettings();   // 생성자 (저장 파일을 읽음, 없으면 기본값)

        [[nodiscard]] bool IsEnabled() const noexcept;                          // 오버레이 켜짐
        void SetEnabled(bool isEnabled) noexcept;                               // 켜기 · 끄기
        [[nodiscard]] bool IsCaptureHidden() const noexcept;                    // 캡처 · 방송에서 숨기기
        void SetCaptureHidden(bool isHidden) noexcept;                          // 숨기기 바꾸기
        [[nodiscard]] float GetOpacity() const noexcept;                        // 패널 바탕 불투명도
        void SetOpacity(float opacity) noexcept;                                // 불투명도 (범위 제한)
        [[nodiscard]] std::size_t GetMiniToolIndex() const noexcept;            // 미니 모드 탭 번호
        void SetMiniToolIndex(std::size_t index) noexcept;                      // 미니 모드 탭 바꾸기
        [[nodiscard]] std::optional<OverlayPanelState> FindPanel(std::string_view id) const;   // 패널 상태 (없으면 nullopt)
        void SetPanel(OverlayPanelState state);                                 // 패널 상태 저장 (이름 기준 교체)
        void RemovePanel(std::string_view id);                                  // 패널 상태 지우기 (기본 위치로 되돌릴 때)
        void Save();                                                            // 파일로 저장 (실패하면 예외)

    private:
        Services::JSONFileStore m_fileStore;          // 안전한 파일 읽기 · 쓰기
        bool m_isEnabled{};                           // 켜짐
        bool m_isCaptureHidden{};                     // 캡처 · 방송에서 숨기기
        float m_opacity{ DEFAULT_OPACITY };           // 불투명도
        std::size_t m_miniToolIndex{};                // 미니 모드 탭
        std::vector<OverlayPanelState> m_panels;      // 패널 상태들
    };
}
