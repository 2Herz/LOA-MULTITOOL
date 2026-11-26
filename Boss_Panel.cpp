module LostArkMultiTool.Views.BossPanel;  // 오버레이 보스 체력 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.NumberFormat;  // 쉼표 숫자 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 보스 체력 형식 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float DEFAULT_WIDTH{ 380.0f };            // 기본 가로
        constexpr float DEFAULT_HEIGHT{ 80.0f };            // 기본 세로
        constexpr float ROW_HEIGHT{ 22.0f };                // 줄 높이
        constexpr std::size_t ROW_COUNT{ 2 };               // 줄 수 (체력 · 비율/DPS)
        constexpr float BAR_HEIGHT{ 4.0f };                 // 남은 비율 막대 두께
        constexpr float BAR_GAP{ 3.0f };                    // 막대 위 여백
        constexpr float BAR_RADIUS{ 2.0f };                 // 막대 모서리
        constexpr float HEALTH_FONT_SIZE{ 15.0f };          // 체력 글자 크기
        constexpr float DETAIL_FONT_SIZE{ 12.0f };          // 둘째 줄 글자 크기
        constexpr double REFRESH_SECONDS{ 0.25 };           // 체력 다시 받는 간격
        constexpr double HISTORY_SECONDS{ 10.0 };           // 깎이는 속도를 볼 기간 (최근 10초)
        constexpr double MIN_RATE_SECONDS{ 2.0 };           // 이보다 짧은 기록으로는 속도를 내지 않음
        constexpr double PERCENT_SCALE{ 100.0 };            // 비율 → %
        constexpr std::int64_t TEN_THOUSAND{ 10'000 };      // 만
        constexpr std::int64_t HUNDRED_MILLION{ 100'000'000 };          // 억
        constexpr std::int64_t TRILLION{ 1'000'000'000'000 };           // 조
        constexpr int SECONDS_PER_MINUTE{ 60 };             // 초 → 분
        constexpr int WHOLE_DECIMALS{ 0 };                  // 정수
        constexpr std::wstring_view TITLE_TEXT{ L"보스 체력" };                          // 제목
        constexpr std::wstring_view LINES_TITLE_FORMAT{ L"보스 체력 · ×{}줄" };           // 줄 수가 있을 때 제목
        constexpr std::wstring_view HEALTH_FORMAT{ L"{} / {}" };                        // 현재 / 최대
        constexpr std::wstring_view PERCENT_FORMAT{ L"{:.2f}%" };                       // 남은 비율
        constexpr std::wstring_view DPS_FORMAT{ L"공대 DPS {}/초 · 남은 {}:{:02}" };     // 깎이는 속도 · 예상 남은 시간
        constexpr std::wstring_view DPS_WAITING_TEXT{ L"공대 DPS 계산 중..." };          // 기록이 모자람
        constexpr std::wstring_view EMPTY_TEXT{ L"보스 체력바가 보이면 여기에 표시돼요" };   // 조작 모드에서 보스가 없을 때
        constexpr std::wstring_view TRILLION_FORMAT{ L"{}조 {}억" };                     // 조 단위
        constexpr std::wstring_view TRILLION_ONLY_FORMAT{ L"{}조" };
        constexpr std::wstring_view HUNDRED_MILLION_FORMAT{ L"{}억 {}만" };              // 억 단위
        constexpr std::wstring_view HUNDRED_MILLION_ONLY_FORMAT{ L"{}억" };
        constexpr std::wstring_view TEN_THOUSAND_FORMAT{ L"{}만" };                      // 만 단위
        constexpr Core::TextStyle HEALTH_STYLE{ HEALTH_FONT_SIZE, Core::FontWeight::BOLD };                                  // 체력
        constexpr Core::TextStyle DETAIL_STYLE{ DETAIL_FONT_SIZE };                                                          // 둘째 줄 왼쪽
        constexpr Core::TextStyle DETAIL_RIGHT_STYLE{ DETAIL_FONT_SIZE, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };   // 둘째 줄 오른쪽
    }

    BossPanel::BossPanel(std::function<std::optional<Vision::BossHealthSample>()> provider)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{ TITLE_TEXT }, Core::SizeF{ DEFAULT_WIDTH, DEFAULT_HEIGHT }, false }   // 크기 고정 패널
        , m_provider{ std::move(provider) }   // 보스 체력 함수
    {
        SetHeight(ROW_HEIGHT * ROW_COUNT + BAR_GAP + BAR_HEIGHT);   // 내용 높이
    }

    std::wstring BossPanel::FormatKoreanNumber(std::int64_t value)
    {
        const auto number = [](std::int64_t part) { return Core::NumberFormat::FormatNumber(static_cast<double>(part), WHOLE_DECIMALS); };   // 쉼표 숫자
        if (value >= TRILLION)                                                               // 조
        {
            const std::int64_t rest = (value % TRILLION) / HUNDRED_MILLION;                  // 남은 억
            return rest > 0 ? std::format(TRILLION_FORMAT, number(value / TRILLION), number(rest)) : std::format(TRILLION_ONLY_FORMAT, number(value / TRILLION));
        }
        if (value >= HUNDRED_MILLION)                                                        // 억
        {
            const std::int64_t rest = (value % HUNDRED_MILLION) / TEN_THOUSAND;              // 남은 만
            return rest > 0 ? std::format(HUNDRED_MILLION_FORMAT, number(value / HUNDRED_MILLION), number(rest))
                : std::format(HUNDRED_MILLION_ONLY_FORMAT, number(value / HUNDRED_MILLION));
        }
        if (value >= TEN_THOUSAND) return std::format(TEN_THOUSAND_FORMAT, number(value / TEN_THOUSAND));   // 만
        return number(value);                                                                // 그대로
    }

    void BossPanel::Update(double deltaSeconds)
    {
        m_refreshElapsed += deltaSeconds;                                                    // 시간 누적
        if (m_hasSample && m_refreshElapsed < REFRESH_SECONDS) return;                       // 아직
        m_refreshElapsed = 0.0;                                                              // 초기화
        m_hasSample = true;                                                                  // 받음
        const std::optional<Vision::BossHealthSample> sample = m_provider ? m_provider() : std::nullopt;   // 체력
        const bool isNewBoss = !sample || !m_sample || sample->m_reading.m_maximum != m_sample->m_reading.m_maximum;   // 다른 보스 · 관문
        if (isNewBoss) m_history.clear();                                                    // 기록 새로
        m_sample = sample;                                                                   // 기록
        if (!sample) return;                                                                 // 보스 없음
        if (m_history.empty() || m_history.back().m_time != sample->m_capturedAt)            // 새로 읽은 값만
            m_history.push_back(HistoryPoint{ sample->m_capturedAt, sample->m_reading.m_current });
        while (!m_history.empty() && std::chrono::duration<double>(sample->m_capturedAt - m_history.front().m_time).count() > HISTORY_SECONDS)
            m_history.pop_front();                                                           // 오래된 기록 버림
    }

    bool BossPanel::IsContentAvailable() const noexcept
    {
        return m_sample.has_value();   // 보스 체력바가 보일 때만
    }

    std::optional<double> BossPanel::GetDamagePerSecond() const
    {
        if (m_history.size() < ROW_COUNT) return std::nullopt;                               // 기록 두 개 이상
        const double seconds = std::chrono::duration<double>(m_history.back().m_time - m_history.front().m_time).count();   // 기간
        if (seconds < MIN_RATE_SECONDS) return std::nullopt;                                 // 너무 짧음
        const double damage = static_cast<double>(m_history.front().m_current - m_history.back().m_current);   // 깎인 체력
        if (damage <= 0.0) return std::nullopt;                                              // 안 깎임 (무적 · 회복)
        return damage / seconds;                                                             // 초당
    }

    std::wstring BossPanel::GetTitleText() const
    {
        if (m_sample && m_sample->m_reading.m_lines) return std::format(LINES_TITLE_FORMAT, *m_sample->m_reading.m_lines);   // 줄 수 포함
        return std::wstring{ TITLE_TEXT };                                                   // 기본
    }

    void BossPanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        const Core::RectF first{ content.m_left, content.m_top, content.m_right, content.m_top + ROW_HEIGHT };   // 첫 줄
        if (!m_sample)                                                                       // 보스 없음 (조작 모드에서만 그려짐)
        {
            renderer.DrawString(EMPTY_TEXT, first, Theme::MUTED_TEXT_COLOR, DETAIL_STYLE);  // 안내
            return;
        }
        const Vision::BossHealthReading& reading = m_sample->m_reading;                      // 체력
        renderer.DrawString(std::format(HEALTH_FORMAT, FormatKoreanNumber(reading.m_current), FormatKoreanNumber(reading.m_maximum)),
            first, Theme::HEADER_TEXT_COLOR, HEALTH_STYLE);                                  // 현재 / 최대

        const double ratio = reading.m_maximum > 0 ? static_cast<double>(reading.m_current) / static_cast<double>(reading.m_maximum) : 0.0;   // 남은 비율
        const Core::RectF second{ content.m_left, first.m_bottom, content.m_right, first.m_bottom + ROW_HEIGHT };   // 둘째 줄
        renderer.DrawString(std::format(PERCENT_FORMAT, ratio * PERCENT_SCALE), second, Theme::HIGHLIGHT_TEXT_COLOR, DETAIL_STYLE);   // 남은 %
        std::wstring dpsText{ DPS_WAITING_TEXT };                                            // DPS 글자
        if (const auto dps = GetDamagePerSecond())                                           // 깎이는 속도
        {
            const int remaining = static_cast<int>(static_cast<double>(reading.m_current) / *dps);   // 예상 남은 초
            dpsText = std::format(DPS_FORMAT, FormatKoreanNumber(static_cast<std::int64_t>(*dps)), remaining / SECONDS_PER_MINUTE, remaining % SECONDS_PER_MINUTE);
        }
        renderer.DrawString(dpsText, second, Theme::TEXT_COLOR, DETAIL_RIGHT_STYLE);         // DPS · 남은 시간

        const float barTop = second.m_bottom + BAR_GAP;                                      // 막대 위치
        const Core::RectF track{ content.m_left, barTop, content.m_right, barTop + BAR_HEIGHT };   // 빈 막대
        renderer.FillRoundedRect(track, BAR_RADIUS, Theme::GRID_LINE_COLOR);                 // 빈 막대
        renderer.FillRoundedRect(Core::RectF{ track.m_left, track.m_top, track.m_left + track.GetWidth() * static_cast<float>(ratio), track.m_bottom },
            BAR_RADIUS, Theme::CAUTION_TEXT_COLOR);                                          // 남은 체력
    }
}
