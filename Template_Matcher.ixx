export module LostArkMultiTool.Vision.TemplateMatcher;  // 템플릿 매칭 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

// 흑백 이미지 안에서 작은 그림(템플릿: 창 제목 · 재료 아이콘 · 숫자 글자)이 어디 있는지 찾는 도구
// 정규화 상관계수(NCC)를 써서 밝기 · 대비가 조금 달라도 같은 모양이면 높은 점수 (1 = 완전히 같음)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 매칭 결과 한 건
    struct MatchResult
    {
        int m_x{};          // 찾은 위치 왼쪽 X (검색 이미지 기준)
        int m_y{};          // 찾은 위치 위쪽 Y
        double m_score{};   // 일치 점수 (-1 ~ 1, 높을수록 비슷)
    };

    // 템플릿 매칭 (상태 없음)
    class TemplateMatcher
    {
    public:
        // (x, y) 위치에 pattern을 겹쳤을 때 일치 점수 (pattern이 이미지 밖으로 나가면 -1)
        [[nodiscard]] static double Score(const GrayImage& image, const GrayImage& pattern, int x, int y) noexcept;

        // searchArea 안에서 가장 잘 맞는 위치 하나 (minScore 미만이면 nullopt)
        // 큰 템플릿은 절반 크기로 먼저 훑고 후보 근처만 원래 크기로 다시 봐서 빠르게 찾음
        [[nodiscard]] static std::optional<MatchResult> FindBest(const GrayImage& image, const GrayImage& pattern,
            const PixelRect& searchArea, double minScore);

        // searchArea 안에서 minScore 이상인 위치 모두 (서로 minDistance보다 가까운 결과는 점수 높은 것만 남김)
        [[nodiscard]] static std::vector<MatchResult> FindAll(const GrayImage& image, const GrayImage& pattern,
            const PixelRect& searchArea, double minScore, int minDistance);
    };
}
