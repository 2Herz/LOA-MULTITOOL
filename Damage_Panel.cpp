module LostArkMultiTool.Views.DamagePanel;  // 오버레이 딜 지분 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.BossPanel;  // 조 · 억 · 만 단위 글자 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float DEFAULT_WIDTH{ 300.0f };          // 기본 가로
        constexpr float DEFAULT_HEIGHT{ 66.0f };          // 기본 세로 (세 줄)
        constexpr float ROW_HEIGHT{ 22.0f };              // 줄 높이
        constexpr std::size_t ROW_COUNT{ 3 };             // 줄 수
        constexpr float THIRD_ROW{ 2.0f };                // 셋째 줄 번호 (0부터)
        constexpr float BUTTON_WIDTH{ 54.0f };            // 단추 가로
        constexpr float BUTTON_HEIGHT{ 20.0f };           // 단추 세로
        constexpr float BUTTON_RADIUS{ 5.0f };            // 단추 모서리
        constexpr float BUTTON_GAP{ 6.0f };               // 글자와 단추 사이
        constexpr float BORDER_WIDTH{ 1.0f };             // 테두리 두께
        constexpr float HOVER_ALPHA{ 0.30f };             // 마우스 올림 바탕 투명도
        constexpr float BUTTON_BACK_ALPHA{ 0.12f };       // 단추 바탕 투명도
        constexpr float VALUE_FONT_SIZE{ 13.0f };         // 값 글자 크기
        constexpr float NOTE_FONT_SIZE{ 11.0f };          // 안내 글자 크기
        constexpr float STAGE_FONT_SIZE{ 13.0f };         // 단계 글자 크기
        constexpr float BUTTON_FONT_SIZE{ 11.0f };        // 단추 글자 크기
        constexpr float HALF{ 0.5f };                     // 절반
        constexpr double REFRESH_SECONDS{ 0.3 };          // 값 다시 받는 간격
        constexpr double PERCENT{ 100.0 };                // 백분율
        constexpr double MAX_SHARE{ 1.0 };                // 지분 상한 (읽은 시각 차이로 100%를 넘지 않게)
        constexpr int DEALERS_EIGHT{ 6 };                 // 8인 딜러 수
        constexpr int DEALERS_FOUR{ 3 };                  // 4인 딜러 수
        constexpr std::array<double, 2> STAGE_EIGHT{ 0.15, 0.20 };   // 8인 단계 기준 (강투 · 잔혈)
        constexpr std::array<double, 2> STAGE_FOUR{ 0.30, 0.40 };    // 4인 단계 기준
        constexpr std::array<std::wstring_view, 3> STAGE_NAMES{ L"투사", L"강투", L"잔혈" };   // 단계 이름
        constexpr std::wstring_view TITLE_TEXT{ L"딜 지분" };                                    // 제목
        constexpr std::wstring_view TITLE_FORMAT{ L"딜 지분 · {}인" };                           // 제목 + 인원
        constexpr std::wstring_view DAMAGE_FORMAT{ L"내 피해량  {}" };                           // 첫 줄 왼쪽
        constexpr std::wstring_view DPS_FORMAT{ L"DPS  {}" };                                    // 첫 줄 오른쪽
        constexpr std::wstring_view SHARE_FORMAT{ L"깎인 체력의 {:.2f}%  ·  {:.2f}인분" };          // 둘째 줄
        constexpr std::wstring_view MAXIMUM_FORMAT{ L"최대 체력의 {:.2f}%" };                     // 셋째 줄
        constexpr std::wstring_view NO_BOSS_TEXT{ L"보스 체력을 읽으면 지분을 계산해요" };          // 보스 없음
        constexpr std::wstring_view NO_DAMAGE_TEXT{ L"아직 피해량이 없어요" };                     // 피해량 없음
        constexpr std::wstring_view EMPTY_VALUE_TEXT{ L"-" };                                    // 값 없음
        constexpr std::wstring_view SIZE_BUTTON_FORMAT{ L"{}인 ⇄" };                             // 인원 단추
        constexpr Core::TextStyle VALUE_STYLE{ VALUE_FONT_SIZE, Core::FontWeight::BOLD };                                   // 값
        constexpr Core::TextStyle RIGHT_VALUE_STYLE{ VALUE_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };  // 오른쪽 값
        constexpr Core::TextStyle NOTE_STYLE{ NOTE_FONT_SIZE };                                                             // 안내
        constexpr Core::TextStyle STAGE_STYLE{ STAGE_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };        // 단계
        constexpr Core::TextStyle BUTTON_STYLE{ BUTTON_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };        // 단추

        // 값 → 한국어 단위 글자 (없으면 "-")
        [[nodiscard]] std::wstring FormatAmount(const std::optional<std::int64_t>& value)
        {
            return value ? BossPanel::FormatKoreanNumber(*value) : std::wstring{ EMPTY_VALUE_TEXT };
        }

        // 단계 색 (투사 흐리게 · 강투 강조 · 잔혈 금색)
        [[nodiscard]] Core::Color GetStageColor(int stage) noexcept
        {
            constexpr int STAGE_STRONG{ 1 };   // 강투
            if (stage <= 0) return Theme::MUTED_TEXT_COLOR;               // 투사
            return stage == STAGE_STRONG ? Theme::ACCENT_COLOR : Theme::HIGHLIGHT_TEXT_COLOR;   // 강투 · 잔혈
        }
    }

    DamagePanel::DamagePanel(std::function<DamageShareSource()> provider, std::function<int()> getRaidSize, std::function<void(int)> setRaidSize)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{ TITLE_TEXT }, Core::SizeF{ DEFAULT_WIDTH, DEFAULT_HEIGHT }, false }   // 크기 고정 패널
        , m_provider{ std::move(provider) }          // 값 함수
        , m_getRaidSize{ std::move(getRaidSize) }    // 인원 읽기
        , m_setRaidSize{ std::move(setRaidSize) }    // 인원 바꾸기
    {
        SetHeight(ROW_HEIGHT * static_cast<float>(ROW_COUNT));   // 세 줄
    }

    std::optional<DamageShare> DamagePanel::Calculate(std::int64_t damage, std::int64_t current, std::int64_t maximum, int raidSize) noexcept
    {
        const std::int64_t depleted = maximum - current;                                       // 깎인 체력
        if (maximum <= 0 || depleted <= 0 || damage < 0) return std::nullopt;                  // 계산할 수 없음
        const bool isFour = raidSize == RAID_SIZE_FOUR;                                        // 4인
        DamageShare share;                                                                     // 결과
        share.m_depletedShare = std::min(static_cast<double>(damage) / static_cast<double>(depleted), MAX_SHARE);   // 깎인 체력 대비
        share.m_maximumShare = std::min(static_cast<double>(damage) / static_cast<double>(maximum), MAX_SHARE);     // 최대 체력 대비
        share.m_portions = share.m_depletedShare * (isFour ? DEALERS_FOUR : DEALERS_EIGHT);   // 몇 인분
        const auto& stages = isFour ? STAGE_FOUR : STAGE_EIGHT;                                // 단계 기준
        share.m_stage = static_cast<int>(std::ranges::count_if(stages, [&share](double limit) { return share.m_depletedShare >= limit; }));   // 넘은 기준 수
        return share;
    }

    void DamagePanel::Update(double deltaSeconds)
    {
        SetHeight(ROW_HEIGHT * static_cast<float>(ROW_COUNT));                 // 세 줄 (저장된 · 기본 위치 크기에 머리말이 빠져 잘리지 않게 매번 맞춤)
        m_refreshElapsed += deltaSeconds;                                      // 시간 누적
        if (m_hasSource && m_refreshElapsed < REFRESH_SECONDS) return;         // 아직
        m_refreshElapsed = 0.0;                                                // 초기화
        if (m_provider) m_source = m_provider();                               // 값 받기
        m_hasSource = true;                                                    // 받음
    }

    bool DamagePanel::IsContentAvailable() const noexcept
    {
        return m_source.m_combat.has_value();                                  // 전투 분석기가 보일 때만
    }

    std::wstring DamagePanel::GetTitleText() const
    {
        return std::format(TITLE_FORMAT, m_getRaidSize ? m_getRaidSize() : RAID_SIZE_EIGHT);   // "딜 지분 · 8인"
    }

    Core::RectF DamagePanel::GetSizeButtonRect(const Core::RectF& content) const noexcept
    {
        const float top = content.m_top + ROW_HEIGHT * THIRD_ROW + (ROW_HEIGHT - BUTTON_HEIGHT) * HALF;   // 셋째 줄 세로 가운데
        return Core::RectF{ content.m_left + content.GetWidth() * HALF - BUTTON_WIDTH * HALF, top,
            content.m_left + content.GetWidth() * HALF + BUTTON_WIDTH * HALF, top + BUTTON_HEIGHT };   // 셋째 줄 가운데
    }

    void DamagePanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        const auto rowRect = [&content](std::size_t row)                       // 줄 영역
            {
                const float top = content.m_top + ROW_HEIGHT * static_cast<float>(row);
                return Core::RectF{ content.m_left, top, content.m_right, top + ROW_HEIGHT };
            };
        const int raidSize = m_getRaidSize ? m_getRaidSize() : RAID_SIZE_EIGHT;   // 인원
        const std::optional<std::int64_t> damage = m_source.m_combat ? m_source.m_combat->m_reading.m_damage : std::nullopt;   // 내 피해량
        const std::optional<std::int64_t> dps = m_source.m_combat ? m_source.m_combat->m_reading.m_dps : std::nullopt;         // DPS

        renderer.DrawString(std::format(DAMAGE_FORMAT, FormatAmount(damage)), rowRect(0), Theme::HEADER_TEXT_COLOR, VALUE_STYLE);   // 피해량
        renderer.DrawString(std::format(DPS_FORMAT, FormatAmount(dps)), rowRect(0), Theme::TEXT_COLOR, RIGHT_VALUE_STYLE);          // DPS

        const bool isInteractive = IsInteractive();                                                  // 조작 모드 (단추)
        if (isInteractive)                                                                           // 8인 · 4인 단추
        {
            const Core::RectF button = GetSizeButtonRect(content);
            renderer.FillRoundedRect(button, BUTTON_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, m_isHoverButton ? HOVER_ALPHA : BUTTON_BACK_ALPHA));
            renderer.DrawRoundedRect(button, BUTTON_RADIUS, Theme::ACCENT_COLOR, BORDER_WIDTH);
            renderer.DrawString(std::format(SIZE_BUTTON_FORMAT, raidSize), button, Theme::TEXT_COLOR, BUTTON_STYLE);
        }

        if (!damage) { renderer.DrawString(NO_DAMAGE_TEXT, rowRect(1), Theme::MUTED_TEXT_COLOR, NOTE_STYLE); return; }   // 피해량 없음
        if (!m_source.m_boss) { renderer.DrawString(NO_BOSS_TEXT, rowRect(1), Theme::MUTED_TEXT_COLOR, NOTE_STYLE); return; }   // 보스 없음
        const auto share = Calculate(*damage, m_source.m_boss->m_reading.m_current, m_source.m_boss->m_reading.m_maximum, raidSize);   // 지분
        if (!share) { renderer.DrawString(NO_BOSS_TEXT, rowRect(1), Theme::MUTED_TEXT_COLOR, NOTE_STYLE); return; }      // 아직 안 깎임

        renderer.DrawString(std::format(SHARE_FORMAT, share->m_depletedShare * PERCENT, share->m_portions), rowRect(1), Theme::TEXT_COLOR, VALUE_STYLE);   // 지분 · 인분
        renderer.DrawString(std::format(MAXIMUM_FORMAT, share->m_maximumShare * PERCENT), rowRect(2), Theme::MUTED_TEXT_COLOR, NOTE_STYLE);              // 최대 체력 대비
        renderer.DrawString(STAGE_NAMES[static_cast<std::size_t>(std::clamp(share->m_stage, 0, static_cast<int>(STAGE_NAMES.size()) - 1))],
            rowRect(2), GetStageColor(share->m_stage), STAGE_STYLE);                                 // 단계
    }

    bool DamagePanel::HandleContentInput(const Core::InputEvent& event, const Core::RectF& content)
    {
        const Core::RectF button = GetSizeButtonRect(content);                                       // 단추 영역
        if (const auto* move = std::get_if<Core::MouseMoveEvent>(&event))                            // 마우스 이동
        {
            m_isHoverButton = button.Contains(move->m_x, move->m_y);                                 // 단추 위
            return false;                                                                            // 이동은 삼키지 않음
        }
        if (std::holds_alternative<Core::MouseLeaveEvent>(event)) { m_isHoverButton = false; return false; }   // 밖으로
        if (const auto* click = std::get_if<Core::MouseButtonEvent>(&event))                         // 마우스 버튼
        {
            if (!content.Contains(click->m_x, click->m_y)) return false;                             // 내용 밖
            if (click->m_isPressed && click->m_button == Core::MouseButton::MOUSE_BUTTON_LEFT && button.Contains(click->m_x, click->m_y))   // 단추
            {
                const int raidSize = m_getRaidSize ? m_getRaidSize() : RAID_SIZE_EIGHT;               // 지금 인원
                if (m_setRaidSize) m_setRaidSize(raidSize == RAID_SIZE_FOUR ? RAID_SIZE_EIGHT : RAID_SIZE_FOUR);   // 8인 ⇄ 4인
            }
            return true;                                                                             // 내용 위 클릭은 삼킴
        }
        return false;                                                                                // 처리 안 함
    }
}
