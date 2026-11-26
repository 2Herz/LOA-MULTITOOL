export module LostArkMultiTool.Vision.ScreenLayout;  // 화면 배치(레이아웃) 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기

// 게임 화면별 "어디를 읽을지" 정의 (창고 · 상단바 · 낙원 UI · 툴팁)
// 위치는 게임 화면 크기에 대한 비율(0 ~ 1)로 저장 → 창 크기가 바뀌어도 같은 비율이면 그대로 맞음
// 해상도 · UI 배율이 달라 비율이 어긋나면 교정 화면에서 다시 잡아 저장 (사용자 데이터 폴더 ScreenLayouts.json)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 비율 사각형 (게임 화면 가로 · 세로 대비 0 ~ 1)
    struct NormalizedRect
    {
        double m_left{};     // 왼쪽 비율
        double m_top{};      // 위쪽 비율
        double m_width{};    // 가로 비율
        double m_height{};   // 세로 비율

        [[nodiscard]] PixelRect ToPixels(int width, int height) const noexcept;                         // 실제 픽셀 사각형으로
        [[nodiscard]] static NormalizedRect FromPixels(const PixelRect& rect, int width, int height) noexcept;   // 픽셀 → 비율
        [[nodiscard]] bool IsValid() const noexcept;                                                    // 0 ~ 1 안의 넓이 있는 사각형인지
    };

    // 읽을 영역 하나 (예: "dest_top" = 상급 파괴석 수량 칸)
    struct ScreenRegion
    {
        std::string m_name;      // 영역 이름 (인식기가 찾는 키)
        NormalizedRect m_rect;   // 위치
    };

    // 화면 하나의 배치
    struct ScreenLayout
    {
        std::string m_screenName;            // 화면 이름 ("storage" · "shard_bar" · "paradise" · "tooltip")
        std::string m_anchorFile;            // 이 화면이 떠 있는지 확인할 앵커 템플릿 파일 이름 (템플릿 폴더 기준)
        NormalizedRect m_anchorArea;         // 앵커를 찾을 범위 (툴팁처럼 위치가 바뀌면 화면 전체)
        double m_anchorMinScore{ 0.8 };      // 앵커로 인정할 최소 일치 점수
        std::vector<ScreenRegion> m_regions; // 읽을 영역들

        [[nodiscard]] const ScreenRegion* FindRegion(std::string_view name) const noexcept;   // 이름으로 영역 찾기 (없으면 nullptr)
    };

    // 화면 배치 저장소 (사용자 데이터 폴더 ScreenLayouts.json)
    class ScreenLayoutStore
    {
    public:
        ScreenLayoutStore();   // 생성자 (저장 파일을 읽음, 없으면 빈 목록)

        [[nodiscard]] const ScreenLayout* Find(std::string_view screenName) const noexcept;   // 화면 이름으로 찾기 (없으면 nullptr)
        [[nodiscard]] std::span<const ScreenLayout> GetLayouts() const noexcept;               // 전체 목록
        void Set(ScreenLayout layout);                                                           // 추가 또는 교체 (화면 이름 기준)
        void Save();                                                                             // 파일로 저장 (실패하면 예외)
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept;         // 읽을 때 생긴 안내 (손상 복구 등)

        [[nodiscard]] static std::filesystem::path GetTemplateFolder();                          // 템플릿 폴더 (사용자 데이터 폴더\Vision)

        // JSON 텍스트 ↔ 목록 (자체 검사용으로 공개)
        [[nodiscard]] static std::string WriteText(std::span<const ScreenLayout> layouts);
        [[nodiscard]] static std::vector<ScreenLayout> ReadText(std::string_view text);

    private:
        Services::JSONFileStore m_fileStore;      // 안전한 파일 읽기 · 쓰기
        std::vector<ScreenLayout> m_layouts;      // 화면 배치 목록
        std::optional<std::string> m_loadWarning; // 읽기 안내
    };
}
