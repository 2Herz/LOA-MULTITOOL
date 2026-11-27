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
        constexpr std::wstring_view UNIT_FORMAT{ L"{}{}" };                              // "831억" (쉼표 없이 네 자리씩)
        constexpr std::wstring_view UNIT_SPACE{ L" " };                                  // 단위 사이
        constexpr float ANCHOR_PADDING_X{ 8.0f };            // 체력 숫자 자리 좌우 여백 (DIP)
        constexpr float ANCHOR_PADDING_Y{ 2.0f };            // 위아래 여백
        constexpr float ANCHOR_RADIUS{ 4.0f };               // 덮개 모서리
        constexpr float ANCHOR_BACK_ALPHA{ 0.92f };          // 덮개 바탕 불투명도 (게임 숫자를 가림)
        constexpr float ANCHOR_FONT_RATIO{ 0.95f };          // 글자 크기 = 게임 숫자 높이 × 이 값
        constexpr float MIN_ANCHOR_FONT{ 11.0f };            // 글자 크기 하한
        constexpr float DPS_FONT_RATIO{ 0.8f };              // 아래 DPS 글자 크기 비율
        constexpr float DPS_OFFSET_RATIO{ 1.9f };            // 아래 DPS 줄 위치 (숫자 높이 배수만큼 아래, 막대 아래)
        constexpr float MEASURE_WIDTH{ 2000.0f };            // 글자 폭 잴 때 최대 폭
        constexpr float ANCHOR_HALF{ 0.5f };                 // 절반
        constexpr float ANCHOR_SIDES{ 2.0f };                // 양쪽
        constexpr float ANCHOR_BORDER_WIDTH{ 1.0f };         // 조작 모드 테두리
        constexpr float FULL_ALPHA{ 1.0f };                  // 불투명
        constexpr std::wstring_view ANCHOR_FORMAT{ L"{} / {}  ({:.2f}%)" };   // 체력바 위 글자
        constexpr float SHADOW_OFFSET{ 1.0f };               // 글자 그림자 거리 (게임 글자처럼 검은 테두리 느낌)
        constexpr Core::Color GAME_TEXT_COLOR{ 1.0f, 1.0f, 1.0f, 1.0f };     // 게임 체력 글자 (흰색)
        constexpr Core::Color GAME_SHADOW_COLOR{ 0.0f, 0.0f, 0.0f, 0.85f };  // 게임 글자 그림자 (검정)
        constexpr float COLOR_SCALE{ 255.0f };               // 0 ~ 255 → 0 ~ 1

        // 흰 글자 + 검은 그림자 (게임 체력바 글자 모양)
        void DrawGameText(Core::Renderer& renderer, std::wstring_view text, const Core::RectF& rect, const Core::TextStyle& style)
        {
            const Core::RectF shadow{ rect.m_left + SHADOW_OFFSET, rect.m_top + SHADOW_OFFSET, rect.m_right + SHADOW_OFFSET, rect.m_bottom + SHADOW_OFFSET };   // 그림자 자리
            renderer.DrawString(text, shadow, GAME_SHADOW_COLOR, style);    // 그림자
            renderer.DrawString(text, rect, GAME_TEXT_COLOR, style);        // 글자
        }
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
        struct KoreanUnit
        {
            std::int64_t m_size;          // 단위 크기
            std::wstring_view m_name;     // 단위 글자
        };
        constexpr std::array<KoreanUnit, 3> UNITS{ { { TRILLION, L"조" }, { HUNDRED_MILLION, L"억" }, { TEN_THOUSAND, L"만" } } };   // 큰 단위부터
        if (value <= 0) return std::to_wstring(std::max<std::int64_t>(value, 0));                // 0
        std::wstring text;                                                                       // 결과
        std::int64_t rest = value;                                                               // 남은 값
        for (const KoreanUnit& unit : UNITS)                                                     // 단위마다
        {
            const std::int64_t part = rest / unit.m_size;                                        // 이 단위 몫 (조는 네 자리를 넘을 수 있음)
            rest %= unit.m_size;                                                                 // 나머지
            if (part == 0) continue;                                                             // 0인 단위는 뺌
            if (!text.empty()) text += UNIT_SPACE;                                               // 단위 사이 띄어쓰기
            text += std::format(UNIT_FORMAT, part, unit.m_name);                                 // "831억"
        }
        if (rest > 0)                                                                            // 1의 자리까지
        {
            if (!text.empty()) text += UNIT_SPACE;
            text += std::to_wstring(rest);                                                       // "458"
        }
        return text;                                                                             // "831억 1912만 458"
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

    void BossPanel::SetPixelScale(float dipPerPixel) noexcept
    {
        if (dipPerPixel > 0.0f) m_dipPerPixel = dipPerPixel;   // 배율
    }

    bool BossPanel::HandleInput(const Core::InputEvent&, bool)
    {
        return false;   // 게임 체력바에 고정 → 옮기거나 닫지 않음 (켜고 끄기는 도구 막대 · 설정 탭)
    }

    void BossPanel::Render(Core::Renderer& renderer, float backOpacity, bool isInteractive)
    {
        if (!m_sample || !m_sample->m_reading.m_textBounds)                                  // 체력바 위치를 모름
        {
            if (isInteractive) OverlayPanel::Render(renderer, backOpacity, isInteractive);   // 조작 모드에서는 안내 카드
            return;
        }
        const Vision::BossHealthReading& reading = m_sample->m_reading;                      // 체력
        const auto& text = *reading.m_textBounds;                                            // 게임 숫자 자리 (픽셀)
        const Core::RectF native{ text.m_x * m_dipPerPixel, text.m_y * m_dipPerPixel,
            (text.m_x + text.m_width) * m_dipPerPixel, (text.m_y + text.m_height) * m_dipPerPixel };   // DIP
        const float height = native.GetHeight();                                             // 숫자 높이
        const float centerX = (native.m_left + native.m_right) * ANCHOR_HALF;                       // 가운데
        const double ratio = reading.m_maximum > 0 ? static_cast<double>(reading.m_current) / static_cast<double>(reading.m_maximum) : 0.0;   // 남은 비율
        const std::wstring label = std::format(ANCHOR_FORMAT, FormatKoreanNumber(reading.m_current), FormatKoreanNumber(reading.m_maximum), ratio * PERCENT_SCALE);   // 글자
        const Core::TextStyle style{ std::max(MIN_ANCHOR_FONT, height * ANCHOR_FONT_RATIO), Core::FontWeight::BOLD, Core::TextAlign::CENTER };   // 글자 모양
        const float labelWidth = std::max(native.GetWidth(), renderer.MeasureString(label, style, MEASURE_WIDTH).m_width) + ANCHOR_PADDING_X * ANCHOR_SIDES;   // 덮개 폭 (게임 숫자보다 좁지 않게)
        const Core::RectF cover{ centerX - labelWidth * ANCHOR_HALF, native.m_top - ANCHOR_PADDING_Y, centerX + labelWidth * ANCHOR_HALF, native.m_bottom + ANCHOR_PADDING_Y };   // 덮개

        std::optional<Core::OpacityScope> fade;                                              // 요소 투명도
        if (GetAlpha() < FULL_ALPHA) fade.emplace(renderer, GetAlpha());
        const Core::Color coverColor = reading.m_barColor
            ? Core::Color{ (*reading.m_barColor)[0] / COLOR_SCALE, (*reading.m_barColor)[1] / COLOR_SCALE, (*reading.m_barColor)[2] / COLOR_SCALE, FULL_ALPHA }   // 게임 막대 색 그대로
            : Core::WithAlpha(Theme::BACKGROUND_COLOR, std::max(ANCHOR_BACK_ALPHA, backOpacity));   // 막대 색을 모르면 어두운 바탕
        renderer.FillRoundedRect(cover, ANCHOR_RADIUS, coverColor);                          // 게임 숫자 가리기 (막대와 같은 색)
        if (isInteractive) renderer.DrawRoundedRect(cover, ANCHOR_RADIUS, Theme::ACCENT_COLOR, ANCHOR_BORDER_WIDTH);   // 조작 모드 표시
        DrawGameText(renderer, label, cover, style);                                         // 조 · 억 · 만 · % (게임 글자 모양)

        if (const auto dps = GetDamagePerSecond())                                           // 공대 DPS
        {
            const int remaining = static_cast<int>(static_cast<double>(reading.m_current) / *dps);   // 남은 초
            const std::wstring dpsText = std::format(DPS_FORMAT, FormatKoreanNumber(static_cast<std::int64_t>(*dps)), remaining / SECONDS_PER_MINUTE, remaining % SECONDS_PER_MINUTE);
            const Core::TextStyle dpsStyle{ std::max(MIN_ANCHOR_FONT, height * DPS_FONT_RATIO), Core::FontWeight::BOLD, Core::TextAlign::CENTER };   // 작은 글자
            const float dpsWidth = renderer.MeasureString(dpsText, dpsStyle, MEASURE_WIDTH).m_width + ANCHOR_PADDING_X * ANCHOR_SIDES;   // 폭
            const float dpsTop = native.m_top + height * DPS_OFFSET_RATIO;                   // 막대 아래
            const Core::RectF dpsRect{ centerX - dpsWidth * ANCHOR_HALF, dpsTop, centerX + dpsWidth * ANCHOR_HALF, dpsTop + height + ANCHOR_PADDING_Y * ANCHOR_SIDES };   // 자리
            DrawGameText(renderer, dpsText, dpsRect, dpsStyle);                              // DPS · 남은 시간 (바탕 없이 게임 글자 모양)
        }
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
