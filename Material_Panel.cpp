module LostArkMultiTool.Views.MaterialPanel;  // 오버레이 재료 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.NumberFormat;  // 숫자 글자 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 계산기 요약 형식 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float DEFAULT_WIDTH{ 360.0f };          // 기본 가로
        constexpr float DEFAULT_HEIGHT{ 280.0f };         // 기본 세로 (내용에 맞춰 다시 정함)
        constexpr float ROW_HEIGHT{ 20.0f };              // 줄 높이
        constexpr float SECTION_GAP{ 6.0f };              // 표와 합계 사이
        constexpr float FONT_SIZE{ 12.0f };               // 글자 크기
        constexpr float QUANTITY_RIGHT_RATIO{ 0.48f };    // 수량 열 오른쪽 끝 (내용 가로 비율)
        constexpr float PRICE_RIGHT_RATIO{ 0.73f };       // 단가 열 오른쪽 끝
        constexpr double REFRESH_SECONDS{ 0.5 };          // 요약 다시 받는 간격
        constexpr double SMALL_PRICE{ 1.0 };              // 이보다 싼 단가는 소수 4자리 (운명의 파편)
        constexpr double MIDDLE_PRICE{ 100.0 };           // 이보다 싼 단가는 소수 1자리
        constexpr int SMALL_PRICE_DECIMALS{ 4 };          // 싼 단가 소수 자리
        constexpr int MIDDLE_PRICE_DECIMALS{ 1 };         // 중간 단가 소수 자리
        constexpr int WHOLE_DECIMALS{ 0 };                // 정수
        constexpr std::size_t TOTAL_ROW_COUNT{ 2 };       // 합계 줄 수 (캐릭터 · 원정대)
        constexpr std::size_t HEADER_ROW_COUNT{ 1 };      // 표 제목 줄 수
        constexpr float HALF{ 0.5f };                     // 절반
        constexpr float DIVIDER_WIDTH{ 1.0f };            // 구분선 두께
        constexpr std::wstring_view NO_VALUE_TEXT{ L"-" };   // 시세 없음 표시
        constexpr std::wstring_view TITLE_TEXT{ L"재료 시세 · 자산" };                 // 기본 제목
        constexpr std::wstring_view TITLE_FORMAT{ L"재료 시세 · 자산 · {}" };          // 캐릭터가 있을 때 제목
        constexpr std::wstring_view VISION_SUFFIX{ L"  (화면 인식 켜짐)" };            // 화면 인식 표시
        constexpr std::wstring_view NO_CHARACTER_TEXT{ L"귀속재료 계산기에서 캐릭터를 골라 주세요" };
        constexpr std::wstring_view NO_PRICE_TEXT{ L"시세 갱신 전이에요 · 계산기에서 [시세 갱신]" };
        constexpr std::wstring_view HEADER_MATERIAL{ L"재료" };     // 표 제목
        constexpr std::wstring_view HEADER_QUANTITY{ L"수량" };
        constexpr std::wstring_view HEADER_PRICE{ L"단가" };
        constexpr std::wstring_view HEADER_VALUE{ L"가치(G)" };
        constexpr std::wstring_view CHARACTER_TOTAL_TEXT{ L"캐릭터 자산" };
        constexpr std::wstring_view ROSTER_TOTAL_TEXT{ L"원정대 자산" };
        constexpr std::wstring_view GOLD_FORMAT{ L"{} G" };          // 골드 글자
        constexpr Core::TextStyle LEFT_STYLE{ FONT_SIZE };                                           // 왼쪽 정렬
        constexpr Core::TextStyle RIGHT_STYLE{ FONT_SIZE, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };   // 오른쪽 정렬
        constexpr Core::TextStyle TOTAL_LEFT_STYLE{ FONT_SIZE, Core::FontWeight::BOLD };             // 합계 왼쪽
        constexpr Core::TextStyle TOTAL_RIGHT_STYLE{ FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };   // 합계 오른쪽

        // 단가 글자 (싼 재료는 소수 자리를 늘림)
        [[nodiscard]] std::wstring FormatPrice(double price)
        {
            const int decimals = price < SMALL_PRICE ? SMALL_PRICE_DECIMALS : (price < MIDDLE_PRICE ? MIDDLE_PRICE_DECIMALS : WHOLE_DECIMALS);   // 자릿수
            return Core::NumberFormat::FormatNumber(price, decimals);   // 글자
        }

        // 단가 변동 색 (오름 빨강 · 내림 파랑 · 그대로 기본)
        [[nodiscard]] Core::Color GetDiffColor(double diff) noexcept
        {
            if (diff > 0.0) return Theme::UP_TEXT_COLOR;     // 오름
            if (diff < 0.0) return Theme::DOWN_TEXT_COLOR;   // 내림
            return Theme::TEXT_COLOR;                        // 그대로
        }
    }

    MaterialPanel::MaterialPanel(std::function<CalculatorOverlaySummary()> provider)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{ TITLE_TEXT }, Core::SizeF{ DEFAULT_WIDTH, DEFAULT_HEIGHT }, false }   // 크기 고정 패널
        , m_provider{ std::move(provider) }   // 요약 함수
    {
    }

    void MaterialPanel::Update(double deltaSeconds)
    {
        m_refreshElapsed += deltaSeconds;                                      // 시간 누적
        if (m_hasSummary && m_refreshElapsed < REFRESH_SECONDS) return;        // 아직 이름
        m_refreshElapsed = 0.0;                                                // 초기화
        if (m_provider) m_summary = m_provider();                              // 요약 받기
        m_hasSummary = true;                                                   // 받음
        const std::size_t rowCount = HEADER_ROW_COUNT + std::max<std::size_t>(m_summary.m_rows.size(), 1) + TOTAL_ROW_COUNT;   // 줄 수
        SetHeight(static_cast<float>(rowCount) * ROW_HEIGHT + SECTION_GAP);    // 내용에 맞춘 높이
    }

    std::wstring MaterialPanel::GetTitleText() const
    {
        std::wstring title = m_summary.m_characterName.empty() ? std::wstring{ TITLE_TEXT }
            : std::format(TITLE_FORMAT, m_summary.m_characterName);           // 캐릭터 이름 붙이기
        if (m_summary.m_isVisionOn) title += VISION_SUFFIX;                    // 화면 인식 표시
        return title;                                                          // 제목
    }

    void MaterialPanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        const float width = content.GetWidth();                                // 내용 가로
        const float quantityRight = content.m_left + width * QUANTITY_RIGHT_RATIO;   // 수량 열 끝
        const float priceRight = content.m_left + width * PRICE_RIGHT_RATIO;         // 단가 열 끝
        float y = content.m_top;                                               // 지금 줄 위쪽

        // 한 줄 그리기 (재료 · 수량 · 단가 · 가치)
        const auto drawRow = [&](std::wstring_view label, std::wstring_view quantity, std::wstring_view price, std::wstring_view value,
            const Core::Color& labelColor, const Core::Color& priceColor)
            {
                const float bottom = y + ROW_HEIGHT;                                                                     // 줄 아래
                renderer.DrawString(label, Core::RectF{ content.m_left, y, quantityRight, bottom }, labelColor, LEFT_STYLE);            // 재료
                renderer.DrawString(quantity, Core::RectF{ content.m_left, y, quantityRight, bottom }, Theme::TEXT_COLOR, RIGHT_STYLE); // 수량
                renderer.DrawString(price, Core::RectF{ quantityRight, y, priceRight, bottom }, priceColor, RIGHT_STYLE);              // 단가
                renderer.DrawString(value, Core::RectF{ priceRight, y, content.m_right, bottom }, Theme::TEXT_COLOR, RIGHT_STYLE);     // 가치
                y = bottom;                                                                                              // 다음 줄
            };

        drawRow(HEADER_MATERIAL, HEADER_QUANTITY, HEADER_PRICE, HEADER_VALUE, Theme::MUTED_TEXT_COLOR, Theme::MUTED_TEXT_COLOR);   // 표 제목
        if (m_summary.m_rows.empty())                                                                                    // 캐릭터 없음
        {
            renderer.DrawString(NO_CHARACTER_TEXT, Core::RectF{ content.m_left, y, content.m_right, y + ROW_HEIGHT }, Theme::CAUTION_TEXT_COLOR, LEFT_STYLE);
            y += ROW_HEIGHT;                                                                                             // 다음 줄
        }
        for (const CalculatorOverlayRow& row : m_summary.m_rows)                                                         // 재료마다
        {
            drawRow(row.m_label, Core::NumberFormat::FormatNumber(row.m_quantity, WHOLE_DECIMALS),
                m_summary.m_hasPrices ? FormatPrice(row.m_unitPrice) : std::wstring{ NO_VALUE_TEXT },
                Core::NumberFormat::FormatNumber(row.m_value, WHOLE_DECIMALS), Theme::TEXT_COLOR, GetDiffColor(row.m_priceDiff));   // 한 줄
        }

        y += SECTION_GAP;                                                                                                // 합계 앞 여백
        renderer.DrawLine(content.m_left, y - SECTION_GAP * HALF, content.m_right, y - SECTION_GAP * HALF, Theme::PANEL_BORDER_COLOR, DIVIDER_WIDTH);   // 구분선
        const auto drawTotal = [&](std::wstring_view label, double value)                                                // 합계 줄
            {
                renderer.DrawString(label, Core::RectF{ content.m_left, y, content.m_right, y + ROW_HEIGHT }, Theme::ACCENT_COLOR, TOTAL_LEFT_STYLE);
                renderer.DrawString(m_summary.m_hasPrices ? std::format(GOLD_FORMAT, Core::NumberFormat::FormatNumber(value, WHOLE_DECIMALS)) : std::wstring{ NO_PRICE_TEXT },
                    Core::RectF{ content.m_left, y, content.m_right, y + ROW_HEIGHT },
                    m_summary.m_hasPrices ? Theme::HIGHLIGHT_TEXT_COLOR : Theme::MUTED_TEXT_COLOR, TOTAL_RIGHT_STYLE);
                y += ROW_HEIGHT;                                                                                         // 다음 줄
            };
        drawTotal(CHARACTER_TOTAL_TEXT, m_summary.m_characterTotal);                                                     // 캐릭터 자산
        drawTotal(ROSTER_TOTAL_TEXT, m_summary.m_rosterTotal);                                                           // 원정대 자산
    }
}
