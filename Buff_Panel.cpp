module LostArkMultiTool.Views.BuffPanel;  // 오버레이 버프 타이머 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기
import LostArkMultiTool.Vision.BuffTemplates;  // 버프 종류 · 아이콘 템플릿 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 버프 읽기 형식 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float CELL_WIDTH{ 52.0f };              // 칸 가로
        constexpr float ICON_SIZE{ 30.0f };               // 아이콘 크기
        constexpr float ICON_TOP{ 2.0f };                 // 아이콘 위 여백
        constexpr float TIME_HEIGHT{ 18.0f };             // 남은 시간 줄 높이
        constexpr float LABEL_HEIGHT{ 14.0f };            // 이름 줄 높이
        constexpr float CONTENT_HEIGHT{ ICON_TOP + ICON_SIZE + TIME_HEIGHT + LABEL_HEIGHT };   // 내용 높이
        constexpr float SIDE_PADDING{ 20.0f };            // 패널 좌우 안쪽 여백 합 (OverlayPanel 여백 10 × 2)
        constexpr float DEFAULT_WIDTH{ CELL_WIDTH * static_cast<float>(Vision::BUFF_KIND_COUNT) + SIDE_PADDING };   // 기본 가로 (칸 6개 + 여백)
        constexpr float HALF{ 0.5f };                     // 절반
        constexpr float ACTIVE_OPACITY{ 1.0f };           // 걸려 있을 때 아이콘
        constexpr float INACTIVE_OPACITY{ 0.25f };        // 꺼졌을 때 아이콘 (흐리게)
        constexpr float ICON_BORDER{ 1.0f };              // 아이콘 테두리 두께
        constexpr float TIME_FONT_SIZE{ 15.0f };          // 남은 시간 글자 크기
        constexpr float LABEL_FONT_SIZE{ 10.0f };         // 이름 글자 크기
        constexpr double REFRESH_SECONDS{ 0.1 };          // 값 다시 받는 간격
        constexpr double MISSING_GRACE_SECONDS{ 0.6 };    // 아이콘이 이만큼 안 보이면 끝난 것으로 봄
        constexpr double WARNING_SECONDS{ 3.0 };          // 이 이하로 남으면 빨간 글자
        constexpr double TENTHS_LIMIT{ 10.0 };            // 10초 미만은 소수 한 자리
        constexpr int SECONDS_PER_MINUTE{ 60 };           // 분 → 초
        constexpr std::size_t TEMPLATE_CHANNELS{ 3 };     // 템플릿 RGB
        constexpr std::uint8_t OPAQUE_ALPHA{ 255 };       // 불투명
        constexpr std::size_t BGRA_BYTES{ 4 };            // 그림 픽셀 바이트 (B · G · R · A)
        constexpr std::size_t SOURCE_RED{ 0 };            // 템플릿 R 위치
        constexpr std::size_t SOURCE_GREEN{ 1 };          // 템플릿 G 위치
        constexpr std::size_t SOURCE_BLUE{ 2 };           // 템플릿 B 위치
        constexpr std::size_t TARGET_BLUE{ 0 };           // 그림 B 위치
        constexpr std::size_t TARGET_GREEN{ 1 };          // 그림 G 위치
        constexpr std::size_t TARGET_RED{ 2 };            // 그림 R 위치
        constexpr std::size_t TARGET_ALPHA{ 3 };          // 그림 A 위치
        constexpr std::uint64_t ICON_VERSION{ 1 };        // 아이콘 그림 버전 (바뀌지 않음)
        constexpr std::wstring_view TITLE_TEXT{ L"버프 타이머" };   // 제목
        constexpr std::wstring_view INACTIVE_TEXT{ L"-" };          // 꺼짐
        constexpr std::wstring_view ACTIVE_UNKNOWN_TEXT{ L"●" };    // 걸려 있는데 남은 시간을 못 읽음
        constexpr std::array<std::wstring_view, Vision::BUFF_KIND_COUNT> LABELS{ L"암수", L"아드로핀", L"초각성기", L"초각 스킬", L"공증", L"아덴" };   // 칸 이름 (BuffKind 순서)
        constexpr Core::TextStyle TIME_STYLE{ TIME_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };   // 남은 시간
        constexpr Core::TextStyle LABEL_STYLE{ LABEL_FONT_SIZE, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };   // 이름

        // 남은 시간 글자 ("4:32" · "17" · "2.4")
        [[nodiscard]] std::wstring FormatRemaining(double seconds)
        {
            if (seconds >= SECONDS_PER_MINUTE)                                        // 1분 이상
            {
                const int total = static_cast<int>(seconds);                          // 정수 초
                return std::format(L"{}:{:02}", total / SECONDS_PER_MINUTE, total % SECONDS_PER_MINUTE);
            }
            if (seconds >= TENTHS_LIMIT) return std::format(L"{}", static_cast<int>(seconds));   // 10초 이상
            return std::format(L"{:.1f}", seconds);                                   // 10초 미만
        }

        // 종류 → 칸 번호
        [[nodiscard]] std::size_t ToIndex(Vision::BuffKind kind) noexcept
        {
            return static_cast<std::size_t>(kind);                                    // 열거 순서 = 칸 순서
        }
    }

    BuffPanel::BuffPanel(std::function<std::optional<Vision::BuffSample>()> provider)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{ TITLE_TEXT }, Core::SizeF{ DEFAULT_WIDTH, CONTENT_HEIGHT }, false }   // 크기 고정 패널
        , m_provider{ std::move(provider) }                                           // 값 함수
    {
        SetHeight(CONTENT_HEIGHT);                                                    // 내용 높이
        std::array<bool, Vision::BUFF_KIND_COUNT> isMade{};                           // 그림을 만든 종류
        for (const Vision::BuffIconTemplate& pattern : Vision::GetBuffIcons())        // 템플릿마다 (종류마다 첫 템플릿)
        {
            const std::size_t index = ToIndex(pattern.m_kind);                        // 칸
            if (index >= m_icons.size() || isMade[index]) continue;                   // 이미 만듦
            isMade[index] = true;
            Core::ImageData& image = m_icons[index];                                  // 그림
            image.m_width = pattern.m_width;
            image.m_height = pattern.m_height;
            image.m_version = ICON_VERSION;
            image.m_pixels.resize(static_cast<std::size_t>(pattern.m_width) * pattern.m_height * BGRA_BYTES);
            for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(pattern.m_width) * pattern.m_height; ++pixel)
            {
                const std::size_t source = pixel * TEMPLATE_CHANNELS;                // RGB 위치
                const std::size_t target = pixel * BGRA_BYTES;       // BGRA 위치
                image.m_pixels[target + TARGET_BLUE] = pattern.m_rgb[source + SOURCE_BLUE];     // B
                image.m_pixels[target + TARGET_GREEN] = pattern.m_rgb[source + SOURCE_GREEN];   // G
                image.m_pixels[target + TARGET_RED] = pattern.m_rgb[source + SOURCE_RED];       // R
                image.m_pixels[target + TARGET_ALPHA] = OPAQUE_ALPHA;                           // A
            }
        }
    }

    void BuffPanel::Update(double deltaSeconds)
    {
        SetHeight(CONTENT_HEIGHT);                                                    // 내용 높이 (저장된 · 기본 위치 크기에 머리말이 빠져 잘리지 않게 매번 맞춤)
        if (const Core::RectF bounds = GetBounds(); bounds.GetWidth() != DEFAULT_WIDTH)   // 가로도 칸 6개에 맞춤 (예전에 저장한 좁은 크기)
            SetBounds(Core::RectF{ bounds.m_left, bounds.m_top, bounds.m_left + DEFAULT_WIDTH, bounds.m_bottom });
        m_refreshElapsed += deltaSeconds;                                             // 시간 누적
        if (m_refreshElapsed < REFRESH_SECONDS) return;                               // 아직
        m_refreshElapsed = 0.0;                                                       // 초기화
        const auto now = std::chrono::steady_clock::now();                            // 지금
        const auto sample = m_provider ? m_provider() : std::nullopt;                 // 마지막 읽기
        if (sample && sample->m_capturedAt != m_lastSampleAt)                         // 새로 읽은 값
        {
            m_lastSampleAt = sample->m_capturedAt;                                    // 반영 기록
            std::array<std::optional<double>, Vision::BUFF_KIND_COUNT> seconds{};     // 이번 읽기의 종류별 남은 시간 (같은 종류가 둘이면 긴 쪽)
            std::array<bool, Vision::BUFF_KIND_COUNT> isSeen{};                       // 이번에 본 종류
            for (const Vision::BuffIcon& icon : sample->m_reading.m_icons)
            {
                const std::size_t index = ToIndex(icon.m_kind);                       // 칸
                if (index >= m_tracks.size()) continue;
                isSeen[index] = true;                                                 // 봄
                if (icon.m_seconds && (!seconds[index] || *icon.m_seconds > *seconds[index])) seconds[index] = icon.m_seconds;   // 긴 쪽
            }
            for (std::size_t index = 0; index < m_tracks.size(); ++index)
            {
                BuffTrack& track = m_tracks[index];                                   // 상태
                if (!isSeen[index]) continue;                                         // 안 보임 (아래에서 끝 처리)
                if (!track.m_isActive) track.m_seconds.reset();                       // 새로 걸림 → 지난 시간 버림
                track.m_isActive = true;                                              // 걸려 있음
                track.m_seenAt = sample->m_capturedAt;                                // 본 시각
                if (seconds[index]) { track.m_seconds = seconds[index]; track.m_readAt = sample->m_capturedAt; }   // 읽은 시간 (못 읽었으면 지난 값으로 계속 줄임)
            }
        }
        if (!sample)                                                                  // 감시가 멈춤 · 오래됨 → 모두 끝
            for (BuffTrack& track : m_tracks) track.m_isActive = false;
        for (BuffTrack& track : m_tracks)                                             // 잠깐 안 보이면 끝
            if (track.m_isActive && std::chrono::duration<double>(now - track.m_seenAt).count() > MISSING_GRACE_SECONDS) track.m_isActive = false;
    }

    bool BuffPanel::IsContentAvailable() const noexcept
    {
        return std::ranges::any_of(m_tracks, [](const BuffTrack& track) { return track.m_isActive; });   // 하나라도 걸려 있음
    }

    std::optional<double> BuffPanel::GetRemaining(const BuffTrack& track) const
    {
        if (!track.m_seconds) return std::nullopt;                                    // 못 읽음
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - track.m_readAt).count();   // 읽은 뒤 지난 시간
        return std::max(0.0, *track.m_seconds - elapsed);                             // 지금 남은 시간
    }

    void BuffPanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        for (std::size_t index = 0; index < m_tracks.size(); ++index)                 // 칸마다
        {
            const BuffTrack& track = m_tracks[index];                                 // 상태
            const float left = content.m_left + CELL_WIDTH * static_cast<float>(index);   // 칸 왼쪽
            const float iconLeft = left + (CELL_WIDTH - ICON_SIZE) * HALF;            // 아이콘 왼쪽 (가운데)
            const float iconTop = content.m_top + ICON_TOP;                           // 아이콘 위
            const Core::RectF iconRect{ iconLeft, iconTop, iconLeft + ICON_SIZE, iconTop + ICON_SIZE };   // 아이콘 칸
            if (!m_icons[index].m_pixels.empty())                                     // 그림
                renderer.DrawImage(m_icons[index], iconRect, track.m_isActive ? ACTIVE_OPACITY : INACTIVE_OPACITY);
            const auto remaining = track.m_isActive ? GetRemaining(track) : std::nullopt;   // 남은 시간
            const bool isWarning = remaining && *remaining <= WARNING_SECONDS;        // 곧 끝남
            renderer.DrawRect(iconRect, track.m_isActive ? (isWarning ? Theme::ALERT_TEXT_COLOR : Theme::TOTAL_TEXT_COLOR) : Theme::GRID_LINE_COLOR, ICON_BORDER);   // 테두리

            const float timeTop = iconTop + ICON_SIZE;                                // 남은 시간 줄
            const Core::RectF timeRect{ left, timeTop, left + CELL_WIDTH, timeTop + TIME_HEIGHT };
            std::wstring timeText{ INACTIVE_TEXT };                                   // 꺼짐
            if (track.m_isActive) timeText = remaining ? FormatRemaining(*remaining) : std::wstring{ ACTIVE_UNKNOWN_TEXT };   // 남은 시간 · 걸려 있음
            const Core::Color timeColor = !track.m_isActive ? Theme::MUTED_TEXT_COLOR : (isWarning ? Theme::ALERT_TEXT_COLOR : Theme::HEADER_TEXT_COLOR);
            renderer.DrawString(timeText, timeRect, timeColor, TIME_STYLE);           // 남은 시간

            const Core::RectF labelRect{ left, timeRect.m_bottom, left + CELL_WIDTH, timeRect.m_bottom + LABEL_HEIGHT };   // 이름 줄
            renderer.DrawString(LABELS[index], labelRect, track.m_isActive ? Theme::TEXT_COLOR : Theme::MUTED_TEXT_COLOR, LABEL_STYLE);   // 이름
        }
    }
}
