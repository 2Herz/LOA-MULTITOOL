module LostArkMultiTool.Views.ParadisePanel;  // 오버레이 낙원력 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.NumberFormat;  // 숫자 글자 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기
import LostArkMultiTool.Views.TrackerManager;  // 트래커 요약 형식 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float DEFAULT_WIDTH{ 280.0f };          // 기본 가로
        constexpr float DEFAULT_HEIGHT{ 200.0f };         // 기본 세로 (내용에 맞춰 다시 정함)
        constexpr float ROW_HEIGHT{ 20.0f };              // 줄 높이
        constexpr float FONT_SIZE{ 12.0f };               // 글자 크기
        constexpr double REFRESH_SECONDS{ 1.0 };          // 요약 다시 받는 간격
        constexpr int WHOLE_DECIMALS{ 0 };                // 정수
        constexpr std::size_t MIN_ROW_COUNT{ 1 };         // 최소 줄 수 (안내 한 줄)
        constexpr std::wstring_view TITLE_TEXT{ L"낙원력 주간 현황" };                    // 기본 제목
        constexpr std::wstring_view TITLE_FORMAT{ L"낙원력 · {} (기록 {}/{})" };          // 주차 · 기록 수
        constexpr std::wstring_view NO_WEEK_TEXT{ L"낙원력 트래커에 이번 주차가 없어요" };
        constexpr std::wstring_view UNRECORDED_TEXT{ L"미기록" };                         // 아직 안 적은 캐릭터
        constexpr Core::TextStyle LEFT_STYLE{ FONT_SIZE };                                // 이름
        constexpr Core::TextStyle RIGHT_STYLE{ FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };   // 점수
    }

    ParadisePanel::ParadisePanel(std::function<TrackerOverlaySummary()> provider)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{ TITLE_TEXT }, Core::SizeF{ DEFAULT_WIDTH, DEFAULT_HEIGHT }, false }   // 크기 고정 패널
        , m_provider{ std::move(provider) }   // 요약 함수
    {
    }

    void ParadisePanel::Update(double deltaSeconds)
    {
        m_refreshElapsed += deltaSeconds;                                      // 시간 누적
        if (m_hasSummary && m_refreshElapsed < REFRESH_SECONDS) return;        // 아직 이름
        m_refreshElapsed = 0.0;                                                // 초기화
        if (m_provider) m_summary = m_provider();                              // 요약 받기
        m_hasSummary = true;                                                   // 받음
        SetHeight(static_cast<float>(std::max(m_summary.m_rows.size(), MIN_ROW_COUNT)) * ROW_HEIGHT);   // 내용에 맞춘 높이
    }

    std::wstring ParadisePanel::GetTitleText() const
    {
        if (m_summary.m_weekLabel.empty()) return std::wstring{ TITLE_TEXT };   // 주차 없음
        return std::format(TITLE_FORMAT, m_summary.m_weekLabel, m_summary.m_recordedCount, m_summary.m_rows.size());   // "낙원력 · 3주차 (기록 5/13)"
    }

    void ParadisePanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        float y = content.m_top;                                               // 지금 줄 위쪽
        if (m_summary.m_weekLabel.empty())                                     // 주차 없음
        {
            renderer.DrawString(NO_WEEK_TEXT, Core::RectF{ content.m_left, y, content.m_right, y + ROW_HEIGHT }, Theme::CAUTION_TEXT_COLOR, LEFT_STYLE);
            return;                                                            // 끝
        }
        for (const TrackerOverlayRow& row : m_summary.m_rows)                  // 캐릭터마다
        {
            const Core::RectF line{ content.m_left, y, content.m_right, y + ROW_HEIGHT };   // 줄 영역
            const bool isRecorded = row.m_score.has_value();                   // 기록했는지
            renderer.DrawString(row.m_name, line, isRecorded ? Theme::TEXT_COLOR : Theme::CAUTION_TEXT_COLOR, LEFT_STYLE);   // 이름 (미기록은 강조)
            renderer.DrawString(isRecorded ? Core::NumberFormat::FormatNumber(*row.m_score, WHOLE_DECIMALS) : std::wstring{ UNRECORDED_TEXT },
                line, isRecorded ? Theme::HIGHLIGHT_TEXT_COLOR : Theme::CAUTION_TEXT_COLOR, RIGHT_STYLE);   // 점수
            y += ROW_HEIGHT;                                                   // 다음 줄
        }
    }
}
