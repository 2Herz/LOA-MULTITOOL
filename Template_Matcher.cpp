module LostArkMultiTool.Vision.TemplateMatcher;  // 템플릿 매칭 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr double NO_MATCH_SCORE{ -1.0 };          // 비교할 수 없을 때 점수
        constexpr double FLAT_EPSILON{ 1e-6 };            // 이보다 분산이 작으면 단색으로 봄
        constexpr int PYRAMID_MIN_SIZE{ 24 };             // 템플릿 가로 · 세로가 이 이상이면 절반 크기로 먼저 훑음
        constexpr int PYRAMID_SCALE{ 2 };                 // 축소 배율
        constexpr std::size_t COARSE_CANDIDATES{ 8 };     // 축소 단계에서 다시 볼 후보 수
        constexpr int REFINE_RADIUS{ 3 };                 // 후보 주변을 다시 볼 반경 (원래 크기 픽셀)
        constexpr double COARSE_SCORE_MARGIN{ 0.3 };      // 모두 찾기의 축소 단계는 흐려져서 기준을 이만큼 낮춰 후보로 받음

        // 템플릿을 미리 계산해 둔 것 (평균을 뺀 값 · 크기)
        struct PreparedPattern
        {
            int m_width{};                    // 가로 크기
            int m_height{};                   // 세로 크기
            std::vector<double> m_centered;   // 픽셀 - 평균
            double m_norm{};                  // 평균을 뺀 값의 제곱합 제곱근
        };

        // 템플릿 준비 (평균 · 크기 계산)
        [[nodiscard]] PreparedPattern Prepare(const GrayImage& pattern)
        {
            PreparedPattern prepared{ pattern.GetWidth(), pattern.GetHeight() };   // 크기 기록
            const auto pixels = pattern.GetPixels();                                 // 템플릿 픽셀
            if (pixels.empty()) return prepared;                                     // 빈 템플릿
            const double mean = std::accumulate(pixels.begin(), pixels.end(), 0.0) / static_cast<double>(pixels.size());   // 평균 밝기
            prepared.m_centered.reserve(pixels.size());                              // 할당 한 번
            double squareSum = 0.0;                                                  // 제곱합
            for (std::uint8_t value : pixels)                                        // 픽셀마다
            {
                const double centered = value - mean;                                // 평균을 뺀 값
                prepared.m_centered.push_back(centered);                             // 저장
                squareSum += centered * centered;                                    // 제곱 누적
            }
            prepared.m_norm = std::sqrt(squareSum);                                  // 크기
            return prepared;                                                         // 준비된 템플릿
        }

        // 준비된 템플릿으로 (x, y) 위치 점수 계산
        [[nodiscard]] double ScorePrepared(const GrayImage& image, const PreparedPattern& pattern, int x, int y) noexcept
        {
            if (pattern.m_centered.empty()) return NO_MATCH_SCORE;                                    // 빈 템플릿
            if (x < 0 || y < 0 || x + pattern.m_width > image.GetWidth()
                || y + pattern.m_height > image.GetHeight()) return NO_MATCH_SCORE;                   // 이미지 밖으로 나감 (경계 검사)

            const auto pixels = image.GetPixels();                       // 이미지 픽셀
            const int imageWidth = image.GetWidth();                     // 이미지 줄 길이
            const double count = static_cast<double>(pattern.m_centered.size());   // 픽셀 수
            double sum = 0.0;                                            // 창 밝기 합
            double squareSum = 0.0;                                      // 창 밝기 제곱합
            double cross = 0.0;                                          // 창 * 템플릿(평균 뺀 값) 합
            std::size_t index = 0;                                       // 템플릿 순번
            for (int row = 0; row < pattern.m_height; ++row)             // 템플릿 줄마다
            {
                const std::size_t start = static_cast<std::size_t>(y + row) * imageWidth + static_cast<std::size_t>(x);   // 이미지 줄 시작
                for (int column = 0; column < pattern.m_width; ++column, ++index)   // 템플릿 칸마다
                {
                    const double value = pixels[start + column];         // 이미지 밝기
                    sum += value;                                        // 합
                    squareSum += value * value;                          // 제곱합
                    cross += value * pattern.m_centered[index];          // 상관 (템플릿 평균이 0이라 창 평균은 빼지 않아도 같음)
                }
            }
            const double windowNorm = std::sqrt(std::max(squareSum - sum * sum / count, 0.0));   // 창의 평균 뺀 크기
            if (windowNorm < FLAT_EPSILON || pattern.m_norm < FLAT_EPSILON)                     // 어느 쪽이든 단색이면
                return (windowNorm < FLAT_EPSILON && pattern.m_norm < FLAT_EPSILON) ? 1.0 : 0.0; // 둘 다 단색이면 같다고 봄
            return cross / (windowNorm * pattern.m_norm);                                        // 정규화 상관계수
        }

        // 영역 안 모든 위치를 훑어 점수를 모음 (step 간격)
        void ScanArea(const GrayImage& image, const PreparedPattern& pattern, const PixelRect& area,
            double minScore, std::vector<MatchResult>& results)
        {
            const int lastX = std::min(area.GetRight(), image.GetWidth()) - pattern.m_width;     // 템플릿 왼쪽이 갈 수 있는 끝
            const int lastY = std::min(area.GetBottom(), image.GetHeight()) - pattern.m_height;  // 템플릿 위쪽이 갈 수 있는 끝
            for (int y = std::max(area.m_y, 0); y <= lastY; ++y)                                // 줄마다
            {
                for (int x = std::max(area.m_x, 0); x <= lastX; ++x)                            // 칸마다
                {
                    const double score = ScorePrepared(image, pattern, x, y);                   // 점수
                    if (score >= minScore) results.push_back(MatchResult{ x, y, score });       // 기준 이상만 모음
                }
            }
        }

        // 점수 높은 순으로 정렬 후 서로 가까운 결과를 걸러냄 (비최대 억제)
        [[nodiscard]] std::vector<MatchResult> SuppressNearby(std::vector<MatchResult> results, int minDistance)
        {
            std::ranges::sort(results, std::ranges::greater{}, &MatchResult::m_score);   // 점수 높은 순
            std::vector<MatchResult> kept;                                               // 남길 결과
            for (const MatchResult& candidate : results)                                  // 후보마다
            {
                const bool isNear = std::ranges::any_of(kept, [&](const MatchResult& other)   // 이미 남긴 결과와 가까운지
                    {
                        return std::abs(other.m_x - candidate.m_x) < minDistance
                            && std::abs(other.m_y - candidate.m_y) < minDistance;           // 가로 · 세로 모두 가까움
                    });
                if (!isNear) kept.push_back(candidate);                                    // 멀면 남김
            }
            return kept;   // 걸러낸 결과
        }

        // 축소 이미지에서 후보를 찾고 원래 크기로 다듬기
        [[nodiscard]] std::vector<MatchResult> FindCandidates(const GrayImage& image, const GrayImage& pattern,
            const PixelRect& searchArea, double minScore, std::size_t maxCandidates, int minDistance)
        {
            const PixelRect area = ClampRect(searchArea, image.GetWidth(), image.GetHeight());   // 이미지 안 검색 영역
            const PreparedPattern full = Prepare(pattern);                                         // 원래 크기 템플릿
            std::vector<MatchResult> results;                                                      // 결과

            const bool usePyramid = pattern.GetWidth() >= PYRAMID_MIN_SIZE && pattern.GetHeight() >= PYRAMID_MIN_SIZE;   // 축소 단계를 쓸지
            if (!usePyramid)                                                                       // 작은 템플릿은 바로 훑음
            {
                ScanArea(image, full, area, minScore, results);                                    // 전체 훑기
                return SuppressNearby(std::move(results), minDistance);                            // 가까운 결과 거르기
            }

            const GrayImage smallImage = image.Resize(image.GetWidth() / PYRAMID_SCALE, image.GetHeight() / PYRAMID_SCALE);        // 절반 이미지
            const PreparedPattern smallPattern = Prepare(pattern.Resize(pattern.GetWidth() / PYRAMID_SCALE, pattern.GetHeight() / PYRAMID_SCALE));   // 절반 템플릿
            const PixelRect smallArea{ area.m_x / PYRAMID_SCALE, area.m_y / PYRAMID_SCALE,
                area.m_width / PYRAMID_SCALE + 1, area.m_height / PYRAMID_SCALE + 1 };           // 절반 검색 영역
            std::vector<MatchResult> coarse;                                                      // 축소 단계 후보
            const bool isLimited = maxCandidates != std::numeric_limits<std::size_t>::max();      // 후보 수가 정해졌는지 (가장 좋은 하나 찾기)
            const double coarseScore = isLimited ? NO_MATCH_SCORE : minScore - COARSE_SCORE_MARGIN;   // 하나 찾기는 기준 없이 상위 후보만, 모두 찾기는 낮춘 기준
            ScanArea(smallImage, smallPattern, smallArea, coarseScore, coarse);                    // 축소 이미지 훑기 (흐려진 점수는 순위로만 씀)
            coarse = SuppressNearby(std::move(coarse), std::max(minDistance / PYRAMID_SCALE, 1)); // 가까운 후보 거르기
            if (coarse.size() > maxCandidates) coarse.resize(maxCandidates);                       // 상위 후보만

            for (const MatchResult& candidate : coarse)                                            // 후보마다 원래 크기로 다듬기
            {
                const PixelRect refineArea{ candidate.m_x * PYRAMID_SCALE - REFINE_RADIUS, candidate.m_y * PYRAMID_SCALE - REFINE_RADIUS,
                    pattern.GetWidth() + REFINE_RADIUS * 2 + 1, pattern.GetHeight() + REFINE_RADIUS * 2 + 1 };   // 후보 주변
                std::vector<MatchResult> local;                                                    // 주변 결과
                const int left = std::max(refineArea.m_x, area.m_x);                               // 검색 영역과 겹치는 왼쪽
                const int top = std::max(refineArea.m_y, area.m_y);                                // 겹치는 위쪽
                const int right = std::min(refineArea.GetRight(), area.GetRight());                // 겹치는 오른쪽
                const int bottom = std::min(refineArea.GetBottom(), area.GetBottom());             // 겹치는 아래쪽
                if (right <= left || bottom <= top) continue;                                      // 겹치지 않으면 버림
                ScanArea(image, full, PixelRect{ left, top, right - left, bottom - top }, minScore, local);   // 주변 훑기 (원래 검색 영역 안)
                if (local.empty()) continue;                                                       // 기준 이상이 없으면 버림
                results.push_back(*std::ranges::max_element(local, {}, &MatchResult::m_score));    // 주변 최고점만 남김
            }
            return SuppressNearby(std::move(results), minDistance);                                // 가까운 결과 거르기
        }
    }

    double TemplateMatcher::Score(const GrayImage& image, const GrayImage& pattern, int x, int y) noexcept
    {
        try
        {
            return ScorePrepared(image, Prepare(pattern), x, y);   // 한 위치 점수
        }
        catch (...)
        {
            return NO_MATCH_SCORE;   // 메모리 부족 등 → 비교 불가
        }
    }

    std::optional<MatchResult> TemplateMatcher::FindBest(const GrayImage& image, const GrayImage& pattern,
        const PixelRect& searchArea, double minScore)
    {
        if (image.IsEmpty() || pattern.IsEmpty()) return std::nullopt;                         // 비교할 수 없음
        const int minDistance = std::max(std::min(pattern.GetWidth(), pattern.GetHeight()), 1);// 같은 자리로 볼 거리
        auto results = FindCandidates(image, pattern, searchArea, minScore, COARSE_CANDIDATES, minDistance);   // 후보 찾기
        if (results.empty()) return std::nullopt;                                              // 기준 이상이 없음
        return results.front();                                                                // 점수 최고 (정렬돼 있음)
    }

    std::vector<MatchResult> TemplateMatcher::FindAll(const GrayImage& image, const GrayImage& pattern,
        const PixelRect& searchArea, double minScore, int minDistance)
    {
        if (image.IsEmpty() || pattern.IsEmpty()) return {};                                    // 비교할 수 없음
        constexpr std::size_t ALL_CANDIDATES{ std::numeric_limits<std::size_t>::max() };         // 후보 수 제한 없음
        return FindCandidates(image, pattern, searchArea, minScore, ALL_CANDIDATES, std::max(minDistance, 1));   // 모든 후보
    }
}
