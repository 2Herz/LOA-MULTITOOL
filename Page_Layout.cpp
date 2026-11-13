module LostArkMultiTool.Views.PageLayout;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;

namespace LostArkMultiTool::Views::PageLayout
{
    namespace
    {
        constexpr float HALF{ 0.5f };                          // 가운데 계산
        constexpr float EYEBROW_TOP{ 10.0f };                  // 머리말 영문 소제목 위치
        constexpr float EYEBROW_HEIGHT{ 14.0f };               // 영문 소제목 줄 높이
        constexpr float TITLE_TOP{ 26.0f };                    // 머리말 제목 위치
        constexpr float TITLE_HEIGHT{ 20.0f };                 // 제목 줄 높이
        constexpr float RIGHT_TEXT_WIDTH{ 220.0f };            // 머리말 오른쪽 글자 칸 너비
        constexpr float LIVE_DOT_RADIUS{ 3.0f };               // LIVE 점 반지름
        constexpr float LIVE_DOT_GAP{ 8.0f };                  // 점과 글자 사이
        constexpr float LIVE_TEXT_WIDTH{ 34.0f };              // "LIVE" 글자 너비
        constexpr float ICON_BOX_SIZE{ 36.0f };                // 통계 카드 아이콘 상자 한 변
        constexpr float ICON_BOX_RADIUS{ 8.0f };               // 아이콘 상자 모서리
        constexpr float ICON_BACK_ALPHA{ 0.14f };              // 아이콘 상자 바탕 투명도
        constexpr float STAT_TEXT_GAP{ 14.0f };                // 아이콘과 글자 사이
        constexpr float STAT_LABEL_HEIGHT{ 16.0f };            // 통계 이름 줄 높이
        constexpr float STAT_VALUE_HEIGHT{ 26.0f };            // 통계 값 줄 높이
        constexpr float STAT_SUB_GAP{ 8.0f };                  // 값과 작은 글자 사이
        constexpr float EMPTY_TITLE_HEIGHT{ 22.0f };           // 빈 상태 제목 줄
        constexpr float EMPTY_HINT_HEIGHT{ 18.0f };            // 빈 상태 안내 줄
        constexpr float FONT_SIZE_STAT_VALUE{ 20.0f };         // 통계 큰 값 글자 크기
        constexpr float FONT_SIZE_CARD_TITLE{ 14.0f };         // 카드 제목 글자 크기
        constexpr float FONT_SIZE_ICON{ 16.0f };               // 아이콘 글자 크기
        constexpr std::wstring_view LIVE_TEXT{ L"LIVE" };      // 실시간 표시

        constexpr Core::TextStyle EYEBROW_STYLE{ Theme::FONT_SIZE_EYEBROW, Core::FontWeight::BOLD, Core::TextAlign::LEADING };        // 청록 영문
        constexpr Core::TextStyle TITLE_STYLE{ FONT_SIZE_CARD_TITLE, Core::FontWeight::BOLD, Core::TextAlign::LEADING };              // 카드 제목
        constexpr Core::TextStyle RIGHT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };         // 오른쪽 작은 글자
        constexpr Core::TextStyle LIVE_STYLE{ Theme::FONT_SIZE_EYEBROW, Core::FontWeight::BOLD, Core::TextAlign::LEADING };           // LIVE
        constexpr Core::TextStyle ICON_STYLE{ FONT_SIZE_ICON, Core::FontWeight::BOLD, Core::TextAlign::CENTER };                      // 아이콘
        constexpr Core::TextStyle LABEL_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };          // 통계 이름
        constexpr Core::TextStyle VALUE_STYLE{ FONT_SIZE_STAT_VALUE, Core::FontWeight::BOLD, Core::TextAlign::LEADING };               // 통계 값
        constexpr Core::TextStyle SUB_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };              // 값 옆 작은 글자
        constexpr Core::TextStyle FOOTER_LEFT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };    // 표 아래 왼쪽
        constexpr Core::TextStyle FOOTER_RIGHT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };  // 표 아래 오른쪽
        constexpr Core::TextStyle EMPTY_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::CENTER };      // 빈 상태 제목
        constexpr Core::TextStyle EMPTY_HINT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };      // 빈 상태 안내
    }

    Core::RectF GetPageColumn(const Core::RectF& area) noexcept
    {
        const float width{ area.m_right - area.m_left };                               // 쓸 수 있는 너비
        if (width <= PAGE_MAX_WIDTH) return area;                                       // 좁으면 그대로
        const float left{ area.m_left + (width - PAGE_MAX_WIDTH) * HALF };              // 가운데 정렬
        return Core::RectF{ left, area.m_top, left + PAGE_MAX_WIDTH, area.m_bottom };    // 최대 너비 열
    }

    std::vector<Core::RectF> SplitColumns(const Core::RectF& area, std::size_t count, float gap, std::span<const float> weights)
    {
        std::vector<Core::RectF> columns;                                               // 결과
        if (count == 0) return columns;                                                 // 나눌 칸 없음
        columns.reserve(count);                                                         // 칸 수만큼

        const float usable{ std::max(0.0f, area.m_right - area.m_left - gap * static_cast<float>(count - 1)) };   // 간격을 뺀 너비
        const bool hasWeights{ weights.size() == count };                               // 비율을 줬는지
        const float totalWeight{ hasWeights ? std::accumulate(weights.begin(), weights.end(), 0.0f) : static_cast<float>(count) };   // 비율 합

        float left{ area.m_left };                                                      // 다음 칸 왼쪽
        for (std::size_t index{}; index < count; ++index)
        {
            const float weight{ hasWeights ? weights[index] : 1.0f };                   // 이 칸 비율
            const float width{ totalWeight > 0.0f ? usable * weight / totalWeight : 0.0f };   // 이 칸 너비
            columns.push_back(Core::RectF{ left, area.m_top, left + width, area.m_bottom });   // 칸 추가
            left += width + gap;                                                        // 다음 칸으로
        }
        return columns;
    }

    Core::RectF TakeTop(Core::RectF& area, float height, float gap) noexcept
    {
        const float bottom{ std::min(area.m_bottom, area.m_top + height) };             // 떼어 낼 아래 경계
        const Core::RectF taken{ area.m_left, area.m_top, area.m_right, bottom };        // 떼어 낸 영역
        area.m_top = std::min(area.m_bottom, bottom + gap);                             // 남은 영역은 간격 아래부터
        return taken;
    }

    void DrawCard(Core::Renderer& renderer, const Core::RectF& rect)
    {
        renderer.FillRoundedRect(rect, CARD_RADIUS, Theme::CARD_BACK_COLOR);                         // 카드 바탕
        renderer.DrawRoundedRect(rect, CARD_RADIUS, Theme::CARD_BORDER_COLOR, Theme::BORDER_WIDTH);  // 얇은 테두리
    }

    Core::RectF DrawCardWithHeader(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view eyebrow, std::wstring_view title,
        std::wstring_view rightText, bool isLive)
    {
        DrawCard(renderer, rect);                                                                    // 카드
        const float left{ rect.m_left + CARD_PADDING };                                              // 글자 왼쪽
        const float right{ rect.m_right - CARD_PADDING };                                            // 글자 오른쪽
        renderer.DrawString(eyebrow, Core::RectF{ left, rect.m_top + EYEBROW_TOP, right, rect.m_top + EYEBROW_TOP + EYEBROW_HEIGHT },
            Theme::ACCENT_COLOR, EYEBROW_STYLE);                                                     // 청록 영문 소제목
        renderer.DrawString(title, Core::RectF{ left, rect.m_top + TITLE_TOP, right, rect.m_top + TITLE_TOP + TITLE_HEIGHT },
            Theme::HEADER_TEXT_COLOR, TITLE_STYLE);                                                  // 제목

        const float headerBottom{ rect.m_top + CARD_HEADER_HEIGHT };                                 // 머리말 아래 경계
        const float middleY{ (rect.m_top + headerBottom) * HALF };                                   // 머리말 세로 가운데
        if (isLive)                                                                                  // 오른쪽 "● LIVE"
        {
            const float textLeft{ right - LIVE_TEXT_WIDTH };                                         // LIVE 글자 왼쪽
            renderer.FillEllipse(Core::PointF{ textLeft - LIVE_DOT_GAP, middleY }, LIVE_DOT_RADIUS, LIVE_DOT_RADIUS, Theme::ACCENT_COLOR);   // 점
            renderer.DrawString(LIVE_TEXT, Core::RectF{ textLeft, rect.m_top, right, headerBottom }, Theme::ACCENT_COLOR, LIVE_STYLE);       // LIVE
        }
        else if (!rightText.empty())                                                                 // 오른쪽 회색 글자
        {
            renderer.DrawString(rightText, Core::RectF{ right - RIGHT_TEXT_WIDTH, rect.m_top, right, headerBottom }, Theme::MUTED_TEXT_COLOR, RIGHT_STYLE);
        }
        renderer.DrawLine(rect.m_left, headerBottom, rect.m_right, headerBottom, Theme::CARD_BORDER_COLOR, Theme::BORDER_WIDTH);   // 구분선
        return Core::RectF{ rect.m_left, headerBottom, rect.m_right, rect.m_bottom };                 // 본문 영역
    }

    void DrawStatCard(Core::Renderer& renderer, const Core::RectF& rect, const StatCard& card)
    {
        DrawCard(renderer, rect);                                                                    // 카드
        const float middleY{ (rect.m_top + rect.m_bottom) * HALF };                                  // 세로 가운데
        const Core::RectF iconRect{ rect.m_left + CARD_PADDING, middleY - ICON_BOX_SIZE * HALF,
            rect.m_left + CARD_PADDING + ICON_BOX_SIZE, middleY + ICON_BOX_SIZE * HALF };             // 아이콘 상자
        renderer.FillRoundedRect(iconRect, ICON_BOX_RADIUS, Core::WithAlpha(card.m_iconColor, ICON_BACK_ALPHA));   // 연한 바탕
        renderer.DrawString(card.m_icon, iconRect, card.m_iconColor, ICON_STYLE);                    // 아이콘

        const float textLeft{ iconRect.m_right + STAT_TEXT_GAP };                                    // 글자 왼쪽
        const float textRight{ rect.m_right - CARD_PADDING };                                        // 글자 오른쪽
        const float labelTop{ middleY - (STAT_LABEL_HEIGHT + STAT_VALUE_HEIGHT) * HALF };            // 두 줄을 세로 가운데에
        renderer.DrawString(card.m_label, Core::RectF{ textLeft, labelTop, textRight, labelTop + STAT_LABEL_HEIGHT }, Theme::MUTED_TEXT_COLOR, LABEL_STYLE);   // 이름

        const Core::RectF valueRect{ textLeft, labelTop + STAT_LABEL_HEIGHT, textRight, labelTop + STAT_LABEL_HEIGHT + STAT_VALUE_HEIGHT };   // 값 줄
        renderer.DrawString(card.m_value, valueRect, Theme::HEADER_TEXT_COLOR, VALUE_STYLE);         // 큰 값
        if (card.m_sub.empty()) return;                                                              // 작은 글자 없음
        const float valueWidth{ renderer.MeasureString(card.m_value, VALUE_STYLE, textRight - textLeft).m_width };   // 값 너비
        renderer.DrawString(card.m_sub, Core::RectF{ std::min(textRight, textLeft + valueWidth + STAT_SUB_GAP), valueRect.m_top, textRight, valueRect.m_bottom },
            card.m_subColor, SUB_STYLE);                                                             // 값 오른쪽 작은 글자
    }

    void DrawTableFooter(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view leftText, std::wstring_view rightText)
    {
        const Core::RectF textRect{ rect.m_left + CARD_PADDING, rect.m_top, rect.m_right - CARD_PADDING, rect.m_bottom };   // 여백 안쪽
        renderer.DrawString(leftText, textRect, Theme::MUTED_TEXT_COLOR, FOOTER_LEFT_STYLE);        // "총 N개의 검색 결과"
        renderer.DrawString(rightText, textRect, Theme::MUTED_TEXT_COLOR, FOOTER_RIGHT_STYLE);      // "거래소 최근 데이터 기준"
    }

    void DrawEmptyState(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view title, std::wstring_view hint)
    {
        const float middleY{ (rect.m_top + rect.m_bottom) * HALF };                                  // 세로 가운데
        renderer.DrawString(title, Core::RectF{ rect.m_left, middleY - EMPTY_TITLE_HEIGHT, rect.m_right, middleY }, Theme::TEXT_COLOR, EMPTY_TITLE_STYLE);   // 제목
        renderer.DrawString(hint, Core::RectF{ rect.m_left, middleY, rect.m_right, middleY + EMPTY_HINT_HEIGHT }, Theme::MUTED_TEXT_COLOR, EMPTY_HINT_STYLE); // 해결 방법
    }
}
