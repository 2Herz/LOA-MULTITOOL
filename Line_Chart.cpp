module LostArkMultiTool.Views.UI.LineChart;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        // ===== 배치 (DIP) =====
        constexpr float CHART_PADDING{ 12.0f };          // 그래프 테두리 안쪽 여백
        constexpr float TITLE_HEIGHT{ 40.0f };
        constexpr float PLOT_TOP_GAP{ 24.0f };           // 제목과 곡선 영역 사이 (마지막 값 글자 자리)
        constexpr float PLOT_LEFT_GAP{ 10.0f };          // y축 글자와 곡선 영역 사이
        constexpr float PLOT_RIGHT_MARGIN{ 28.0f };
        constexpr float PLOT_BOTTOM_MARGIN{ 34.0f };     // x축 글자 자리
        constexpr float Y_LABEL_MIN_WIDTH{ 36.0f };
        constexpr float Y_LABEL_HALF_HEIGHT{ 10.0f };
        constexpr float X_LABEL_TOP_GAP{ 6.0f };
        constexpr float X_LABEL_HEIGHT{ 20.0f };
        constexpr float X_LABEL_GAP{ 14.0f };            // x축 글자 사이 최소 간격
        constexpr std::wstring_view X_LABEL_SAMPLE{ L"00/00" };   // x축 글자 너비를 잴 때 쓰는 대표 글자 (날짜 MM/dd)
        constexpr float HALF{ 0.5f };
        constexpr float BOTH_SIDES{ 2.0f };

        // ===== y축 눈금 =====
        constexpr double TARGET_TICK_COUNT{ 5.0 };       // 대략 이 개수만큼 눈금
        constexpr std::size_t MAX_TICK_COUNT{ 20 };
        constexpr double Y_PADDING_RATIO{ 0.1 };         // 위아래 여유 (값 범위 대비)
        constexpr double FLAT_RANGE_RATIO{ 0.1 };        // 모든 값이 같을 때 만들 범위 (값 대비)
        constexpr double MIN_FLAT_RANGE{ 1.0 };
        constexpr double DECIMAL_BASE{ 10.0 };
        constexpr double NICE_STEP_ONE{ 1.0 };           // 눈금 간격은 1 · 2 · 5 × 10ⁿ 중 하나 (읽기 쉬운 숫자)
        constexpr double NICE_STEP_TWO{ 2.0 };
        constexpr double NICE_STEP_FIVE{ 5.0 };
        constexpr double NICE_STEP_TEN{ 10.0 };
        constexpr int INTEGER_DECIMALS{ 0 };
        constexpr int FRACTION_DECIMALS{ 2 };            // 눈금 간격이 1보다 작을 때

        // ===== 곡선 · 점 =====
        constexpr float SPLINE_TENSION{ 0.5f };          // 곡선이 휘는 정도 (원본 Spline 기본 장력)
        constexpr float TENSION_DIVISOR{ 3.0f };         // 장력 → 베지에 조절점 거리 변환
        constexpr std::size_t TWO_POINTS{ 2 };           // 점이 2개뿐이면 곡선 계산이 직선이 되므로 따로 처리
        constexpr float TWO_POINT_CONTROL_RATIO{ 0.5f }; // 2개일 때 조절점 = 가로 거리의 절반 (양 끝을 수평으로 → 부드러운 S자)
        constexpr float LINE_WIDTH{ 3.0f };
        constexpr float MARKER_RADIUS{ 3.5f };
        constexpr float HOVER_MARKER_RADIUS{ 5.0f };
        constexpr float LAST_MARKER_RADIUS{ 5.5f };      // 가장 최근 점 (원본 MarkerSize 11)
        constexpr float MARKER_BORDER_WIDTH{ 2.0f };
        constexpr float VALUE_LABEL_OFFSET{ 20.0f };     // 마지막 값 글자를 점 위로
        constexpr float VALUE_LABEL_HALF_WIDTH{ 90.0f };
        constexpr float VALUE_LABEL_HALF_HEIGHT{ 10.0f };

        // ===== 말풍선 =====
        constexpr float TOOLTIP_PADDING{ 8.0f };
        constexpr float TOOLTIP_OFFSET{ 12.0f };         // 점에서 떨어진 거리
        constexpr float TOOLTIP_LINE_GAP{ 2.0f };

        // ===== 직전 기록 대비 · 최고 기록선 (원본 낙원력 그래프) =====
        constexpr std::size_t PEAK_MIN_POINTS{ 2 };      // 기록이 2개 이상일 때만 최고 기록선
        constexpr std::size_t FIRST_POINT{ 0 };
        constexpr float PEAK_LABEL_HEIGHT{ 18.0f };
        constexpr float PEAK_LABEL_GAP{ 2.0f };          // 점선과 글자 사이
        constexpr float PEAK_LABEL_LEFT{ 6.0f };
        constexpr std::wstring_view PEAK_FORMAT{ L"최고 {}" };
        constexpr std::wstring_view CHANGE_FORMAT{ L"직전 기록 대비 {} {}" };   // ▲/▼, 차이
        constexpr std::wstring_view CHANGE_SAME_TEXT{ L"직전 기록과 같음" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };

        constexpr Core::TextStyle TITLE_STYLE{ Theme::FONT_SIZE_CHART_TITLE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };
        constexpr Core::TextStyle Y_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };
        constexpr Core::TextStyle X_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };
        constexpr Core::TextStyle VALUE_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::CENTER };
        constexpr Core::TextStyle TOOLTIP_LABEL_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP };
        constexpr Core::TextStyle TOOLTIP_VALUE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP };
        constexpr Core::TextStyle PEAK_LABEL_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_BOTTOM };

        // 읽기 쉬운 눈금 간격 (1 · 2 · 5 × 10ⁿ) — 예: 37,000 → 50,000
        [[nodiscard]] double MakeNiceStep(double rawStep) noexcept
        {
            if (!(rawStep > 0.0) || !std::isfinite(rawStep)) return NICE_STEP_ONE;

            const double magnitude{ std::pow(DECIMAL_BASE, std::floor(std::log10(rawStep))) };
            const double fraction{ rawStep / magnitude };
            if (fraction <= NICE_STEP_ONE) return NICE_STEP_ONE * magnitude;
            if (fraction <= NICE_STEP_TWO) return NICE_STEP_TWO * magnitude;
            if (fraction <= NICE_STEP_FIVE) return NICE_STEP_FIVE * magnitude;
            return NICE_STEP_TEN * magnitude;
        }

        [[nodiscard]] Core::PointF Add(const Core::PointF& point, const Core::PointF& offset, float scale) noexcept
        {
            return Core::PointF{ point.m_x + offset.m_x * scale, point.m_y + offset.m_y * scale };
        }

        [[nodiscard]] Core::PointF Subtract(const Core::PointF& end, const Core::PointF& start) noexcept
        {
            return Core::PointF{ end.m_x - start.m_x, end.m_y - start.m_y };
        }
    }

    LineChart::LineChart()
        : m_lineColor{ Theme::CHART_LINE_COLOR }
        , m_areaTopColor{ Theme::CHART_AREA_TOP_COLOR }
        , m_areaBottomColor{ Theme::CHART_AREA_BOTTOM_COLOR }
    {}

    void LineChart::SetTitle(std::wstring title)
    {
        m_title = std::move(title);
    }

    void LineChart::SetPoints(std::vector<ChartPoint> points)
    {
        m_points = std::move(points);
        m_isLayoutDirty = true;
        if (m_hoverIndex.has_value() && *m_hoverIndex >= m_points.size()) m_hoverIndex.reset();
    }

    void LineChart::SetValueSuffix(std::wstring valueSuffix)
    {
        m_valueSuffix = std::move(valueSuffix);
    }

    void LineChart::SetValuePrefix(std::wstring valuePrefix)
    {
        m_valuePrefix = std::move(valuePrefix);
    }

    void LineChart::SetChangeShown(bool isShown) noexcept
    {
        m_isChangeShown = isShown;
    }

    void LineChart::SetPeakLineShown(bool isShown) noexcept
    {
        m_isPeakLineShown = isShown;
    }

    void LineChart::SetSeriesColor(const Core::Color& lineColor) noexcept
    {
        m_lineColor = lineColor;
        m_areaTopColor = Core::WithAlpha(lineColor, Theme::CHART_AREA_ALPHA);
        m_areaBottomColor = Core::WithAlpha(lineColor, Theme::TRANSPARENT_ALPHA);
    }

    Core::CursorShape LineChart::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_ARROW;
    }

    void LineChart::Render(Core::Renderer& renderer)
    {
        const Core::RectF& bounds{ GetBounds() };
        renderer.FillRect(bounds, Theme::BACKGROUND_COLOR);
        renderer.DrawRect(bounds, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

        const Core::RectF titleRect{ bounds.m_left + CHART_PADDING, bounds.m_top, bounds.m_right - CHART_PADDING, bounds.m_top + TITLE_HEIGHT };
        renderer.DrawString(m_title, titleRect, Theme::TEXT_COLOR, TITLE_STYLE);

        UpdateLayout(renderer);
        if (m_layout.m_positions.empty()) return;   // 기록이 없거나 그래프가 너무 작음 → 제목(안내 문구)만

        RenderAxes(renderer);
        if (m_isPeakLineShown) RenderPeakLine(renderer);

        // 마우스가 가리키는 날짜에 세로 점선
        const bool hasHover{ IsHovered() && m_hoverIndex.has_value() && *m_hoverIndex < m_layout.m_positions.size() };
        if (hasHover)
        {
            const float hoverX{ m_layout.m_positions[*m_hoverIndex].m_x };
            renderer.DrawDashedLine(hoverX, m_layout.m_plotArea.m_top, hoverX, m_layout.m_plotArea.m_bottom, Theme::MUTED_TEXT_COLOR, Theme::BORDER_WIDTH);
        }

        RenderSeries(renderer);
        if (hasHover) RenderTooltip(renderer, *m_hoverIndex);
    }

    void LineChart::OnMouseMove(const Core::MouseMoveEvent& event)
    {
        const Core::RectF& plot{ m_layout.m_plotArea };
        if (m_layout.m_positions.empty() || event.m_x < plot.m_left || event.m_x > plot.m_right)
        {
            m_hoverIndex.reset();
            return;
        }

        // x가 가장 가까운 점 (같은 간격으로 놓여 있으므로 칸 번호로 바로 계산)
        const float slotWidth{ plot.GetWidth() / static_cast<float>(m_layout.m_positions.size()) };
        const auto slot{ static_cast<std::size_t>(std::max(0.0f, (event.m_x - plot.m_left) / slotWidth)) };
        m_hoverIndex = std::min(slot, m_layout.m_positions.size() - 1);
    }

    // 점 · 부품 크기가 바뀌었을 때만: y축 범위 · 눈금, 곡선 영역, 점 위치, 곡선 다시 계산
    void LineChart::UpdateLayout(Core::Renderer& renderer)
    {
        const Core::RectF& bounds{ GetBounds() };
        if (!m_isLayoutDirty && m_layoutBounds == bounds) return;

        m_isLayoutDirty = false;
        m_layoutBounds = bounds;
        m_layout = Layout{};
        if (m_points.empty()) return;

        // 1) y축 범위 (0부터 시작하지 않음 → 작은 변화도 보이게, 원본 IsStartedFromZero = false)
        const auto [minPoint, maxPoint] { std::ranges::minmax_element(m_points, {}, &ChartPoint::m_value) };
        double lowValue{ minPoint->m_value };
        double highValue{ maxPoint->m_value };
        if (highValue - lowValue <= 0.0)
        {
            const double flatRange{ std::max(std::abs(highValue) * FLAT_RANGE_RATIO, MIN_FLAT_RANGE) };
            lowValue -= flatRange * HALF;
            highValue += flatRange * HALF;
        }

        const double padding{ (highValue - lowValue) * Y_PADDING_RATIO };
        const double step{ MakeNiceStep((highValue - lowValue + padding * BOTH_SIDES) / TARGET_TICK_COUNT) };
        m_layout.m_minValue = std::floor((lowValue - padding) / step) * step;
        m_layout.m_maxValue = std::ceil((highValue + padding) / step) * step;

        const auto tickCount{ std::min(static_cast<std::size_t>(std::lround((m_layout.m_maxValue - m_layout.m_minValue) / step)) + 1, MAX_TICK_COUNT) };
        const int decimals{ step >= NICE_STEP_ONE ? INTEGER_DECIMALS : FRACTION_DECIMALS };
        float labelWidth{ Y_LABEL_MIN_WIDTH };
        for (std::size_t index{}; index < tickCount; ++index)
        {
            const double tickValue{ m_layout.m_minValue + step * static_cast<double>(index) };
            m_layout.m_tickValues.push_back(tickValue);
            labelWidth = std::max(labelWidth, renderer.MeasureString(Core::NumberFormat::FormatNumber(tickValue, decimals), Y_LABEL_STYLE, bounds.GetWidth()).m_width);
        }

        // 2) 곡선 영역 (왼쪽은 y축 글자 너비만큼 비움)
        const Core::RectF plot{ bounds.m_left + CHART_PADDING + labelWidth + PLOT_LEFT_GAP, bounds.m_top + TITLE_HEIGHT + PLOT_TOP_GAP,
            bounds.m_right - PLOT_RIGHT_MARGIN, bounds.m_bottom - PLOT_BOTTOM_MARGIN };
        if (plot.GetWidth() <= 0.0f || plot.GetHeight() <= 0.0f) return;
        m_layout.m_plotArea = plot;

        // 3) 점 위치: 칸마다 가운데 (원본 IsXValueIndexed — 날짜 간격과 상관없이 같은 간격)
        const float slotWidth{ plot.GetWidth() / static_cast<float>(m_points.size()) };
        for (std::size_t index{}; index < m_points.size(); ++index)
        {
            m_layout.m_positions.push_back(Core::PointF{ plot.m_left + slotWidth * (static_cast<float>(index) + HALF), ToY(m_points[index].m_value) });
        }

        // 4) x축 글자 간격 (글자가 겹치면 몇 칸씩 건너뜀)
        const float labelSlot{ renderer.MeasureString(X_LABEL_SAMPLE, X_LABEL_STYLE, bounds.GetWidth()).m_width + X_LABEL_GAP };
        m_layout.m_labelStep = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(labelSlot / slotWidth)));

        BuildSegments();
    }

    float LineChart::ToY(double value) const noexcept
    {
        const Core::RectF& plot{ m_layout.m_plotArea };
        const double range{ m_layout.m_maxValue - m_layout.m_minValue };
        if (range <= 0.0) return plot.m_bottom;

        const double ratio{ (value - m_layout.m_minValue) / range };
        return plot.m_bottom - static_cast<float>(ratio) * plot.GetHeight();
    }

    // 점들을 지나는 부드러운 곡선 (Catmull-Rom 스플라인 → 베지에 구간)
    void LineChart::BuildSegments()
    {
        const std::vector<Core::PointF>& positions{ m_layout.m_positions };
        if (positions.size() < TWO_POINTS) return;

        // 점 2개: 이웃 점이 없어 Catmull-Rom이 직선이 됨 → 양 끝 접선을 수평으로 둔 S자 곡선
        if (positions.size() == TWO_POINTS)
        {
            const Core::PointF& start{ positions.front() };
            const Core::PointF& end{ positions.back() };
            const float controlDistance{ (end.m_x - start.m_x) * TWO_POINT_CONTROL_RATIO };
            m_layout.m_segments.push_back(Core::BezierSegment{
                Core::PointF{ start.m_x + controlDistance, start.m_y },
                Core::PointF{ end.m_x - controlDistance, end.m_y },
                end });
            return;
        }

        const float controlScale{ SPLINE_TENSION / TENSION_DIVISOR };
        const std::size_t lastIndex{ positions.size() - 1 };
        for (std::size_t index{}; index < lastIndex; ++index)
        {
            const Core::PointF& previous{ positions[index > 0 ? index - 1 : index] };
            const Core::PointF& start{ positions[index] };
            const Core::PointF& end{ positions[index + 1] };
            const Core::PointF& next{ positions[std::min(index + 2, lastIndex)] };

            m_layout.m_segments.push_back(Core::BezierSegment{
                Add(start, Subtract(end, previous), controlScale),
                Add(end, Subtract(next, start), -controlScale),
                end });
        }
    }

    void LineChart::RenderAxes(Core::Renderer& renderer) const
    {
        const Core::RectF& bounds{ GetBounds() };
        const Core::RectF& plot{ m_layout.m_plotArea };
        const double step{ m_layout.m_tickValues.size() > 1 ? m_layout.m_tickValues[1] - m_layout.m_tickValues[0] : NICE_STEP_ONE };
        const int decimals{ step >= NICE_STEP_ONE ? INTEGER_DECIMALS : FRACTION_DECIMALS };

        // y축: 점선 눈금 + 값 (원본 MajorGrid Dot)
        for (const double tickValue : m_layout.m_tickValues)
        {
            const float y{ ToY(tickValue) };
            renderer.DrawDashedLine(plot.m_left, y, plot.m_right, y, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

            const Core::RectF labelRect{ bounds.m_left + CHART_PADDING, y - Y_LABEL_HALF_HEIGHT, plot.m_left - PLOT_LEFT_GAP, y + Y_LABEL_HALF_HEIGHT };
            renderer.DrawString(Core::NumberFormat::FormatNumber(tickValue, decimals), labelRect, Theme::MUTED_TEXT_COLOR, Y_LABEL_STYLE);
        }

        // x축: 바닥선 + 날짜
        renderer.DrawLine(plot.m_left, plot.m_bottom, plot.m_right, plot.m_bottom, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

        const float labelHalfWidth{ plot.GetWidth() / static_cast<float>(m_layout.m_positions.size()) * static_cast<float>(m_layout.m_labelStep) * HALF };
        for (std::size_t index{}; index < m_layout.m_positions.size(); index += m_layout.m_labelStep)
        {
            const float x{ m_layout.m_positions[index].m_x };
            const Core::RectF labelRect{ x - labelHalfWidth, plot.m_bottom + X_LABEL_TOP_GAP, x + labelHalfWidth, plot.m_bottom + X_LABEL_TOP_GAP + X_LABEL_HEIGHT };
            renderer.DrawString(m_points[index].m_label, labelRect, Theme::MUTED_TEXT_COLOR, X_LABEL_STYLE);
        }
    }

    void LineChart::RenderSeries(Core::Renderer& renderer) const
    {
        const std::vector<Core::PointF>& positions{ m_layout.m_positions };
        const Core::RectF& plot{ m_layout.m_plotArea };

        // 곡선 아래 영역 (위는 진한 계열 색 → 바닥은 투명) + 곡선
        if (!m_layout.m_segments.empty())
        {
            {
                const Core::ClipScope clip{ renderer, plot };   // 곡선이 살짝 튀어나와도 영역은 곡선 영역 안에만
                renderer.FillCurveArea(positions.front(), m_layout.m_segments, plot.m_bottom, plot.m_top,
                    m_areaTopColor, m_areaBottomColor);
            }
            renderer.DrawCurve(positions.front(), m_layout.m_segments, m_lineColor, LINE_WIDTH);
        }

        // 점 (마지막 점은 크게, 마우스가 가리키는 점은 조금 크게)
        const std::size_t lastIndex{ positions.size() - 1 };
        for (std::size_t index{}; index < positions.size(); ++index)
        {
            float radius{ MARKER_RADIUS };
            if (index == lastIndex) radius = LAST_MARKER_RADIUS;
            else if (IsHovered() && m_hoverIndex == index) radius = HOVER_MARKER_RADIUS;

            renderer.FillEllipse(positions[index], radius, radius, Theme::CHART_MARKER_COLOR);
            renderer.DrawEllipse(positions[index], radius, radius, m_lineColor, MARKER_BORDER_WIDTH);
        }

        // 가장 최근 값은 점 위에 숫자로 (원본 마지막 점 IsValueShownAsLabel)
        const Core::PointF& lastPosition{ positions.back() };
        const Core::RectF& bounds{ GetBounds() };
        const float labelLeft{ std::clamp(lastPosition.m_x - VALUE_LABEL_HALF_WIDTH, bounds.m_left, bounds.m_right - VALUE_LABEL_HALF_WIDTH * BOTH_SIDES) };
        const float labelCenterY{ lastPosition.m_y - VALUE_LABEL_OFFSET };
        const Core::RectF valueRect{ labelLeft, labelCenterY - VALUE_LABEL_HALF_HEIGHT, labelLeft + VALUE_LABEL_HALF_WIDTH * BOTH_SIDES,
            labelCenterY + VALUE_LABEL_HALF_HEIGHT };
        renderer.DrawString(Core::NumberFormat::FormatNumber(m_points.back().m_value, INTEGER_DECIMALS), valueRect, Theme::HIGHLIGHT_TEXT_COLOR, VALUE_LABEL_STYLE);
    }

    // 날짜 + 값 (+ 직전 기록 대비) 말풍선 (점 오른쪽 위, 그래프 밖으로 나가면 반대쪽으로)
    void LineChart::RenderTooltip(Core::Renderer& renderer, std::size_t index) const
    {
        const Core::RectF& bounds{ GetBounds() };
        const Core::PointF& position{ m_layout.m_positions[index] };
        const std::wstring& labelText{ m_points[index].m_label };
        const std::wstring valueText{ m_valuePrefix + Core::NumberFormat::FormatNumber(m_points[index].m_value, INTEGER_DECIMALS) + m_valueSuffix };
        const std::wstring changeText{ BuildChangeText(index) };

        const Core::SizeF labelSize{ renderer.MeasureString(labelText, TOOLTIP_LABEL_STYLE, bounds.GetWidth()) };
        const Core::SizeF valueSize{ renderer.MeasureString(valueText, TOOLTIP_VALUE_STYLE, bounds.GetWidth()) };
        const Core::SizeF changeSize{ changeText.empty() ? Core::SizeF{} : renderer.MeasureString(changeText, TOOLTIP_LABEL_STYLE, bounds.GetWidth()) };
        const float changeHeight{ changeText.empty() ? 0.0f : TOOLTIP_LINE_GAP + changeSize.m_height };
        const float width{ std::max({ labelSize.m_width, valueSize.m_width, changeSize.m_width }) + TOOLTIP_PADDING * BOTH_SIDES };
        const float height{ labelSize.m_height + TOOLTIP_LINE_GAP + valueSize.m_height + changeHeight + TOOLTIP_PADDING * BOTH_SIDES };

        float left{ position.m_x + TOOLTIP_OFFSET };
        if (left + width > bounds.m_right) left = position.m_x - TOOLTIP_OFFSET - width;
        float top{ position.m_y - TOOLTIP_OFFSET - height };
        if (top < bounds.m_top) top = position.m_y + TOOLTIP_OFFSET;
        left = std::clamp(left, bounds.m_left, std::max(bounds.m_left, bounds.m_right - width));
        top = std::clamp(top, bounds.m_top, std::max(bounds.m_top, bounds.m_bottom - height));

        const Core::RectF box{ left, top, left + width, top + height };
        renderer.FillRoundedRect(box, Theme::CORNER_RADIUS, Theme::PANEL_BACK_COLOR);
        renderer.DrawRoundedRect(box, Theme::CORNER_RADIUS, Theme::PANEL_BORDER_COLOR, Theme::BORDER_WIDTH);

        const float textLeft{ box.m_left + TOOLTIP_PADDING };
        const float labelTop{ box.m_top + TOOLTIP_PADDING };
        renderer.DrawString(labelText, Core::RectF{ textLeft, labelTop, box.m_right, labelTop + labelSize.m_height }, Theme::MUTED_TEXT_COLOR, TOOLTIP_LABEL_STYLE);

        const float valueTop{ labelTop + labelSize.m_height + TOOLTIP_LINE_GAP };
        renderer.DrawString(valueText, Core::RectF{ textLeft, valueTop, box.m_right, valueTop + valueSize.m_height }, Theme::HEADER_TEXT_COLOR, TOOLTIP_VALUE_STYLE);

        if (changeText.empty()) return;
        const float changeTop{ valueTop + valueSize.m_height + TOOLTIP_LINE_GAP };
        renderer.DrawString(changeText, Core::RectF{ textLeft, changeTop, box.m_right, changeTop + changeSize.m_height }, GetChangeColor(index), TOOLTIP_LABEL_STYLE);
    }

    // 시즌 최고값에 가로 점선 + 왼쪽 위에 "최고 N" (최신 점의 골드 강조와 구분되는 차분한 색)
    void LineChart::RenderPeakLine(Core::Renderer& renderer) const
    {
        if (m_points.size() < PEAK_MIN_POINTS) return;

        const Core::RectF& plot{ m_layout.m_plotArea };
        const double peakValue{ std::ranges::max(m_points, {}, &ChartPoint::m_value).m_value };
        const float y{ ToY(peakValue) };
        renderer.DrawDashedLine(plot.m_left, y, plot.m_right, y, Theme::MUTED_TEXT_COLOR, Theme::BORDER_WIDTH);

        const Core::RectF labelRect{ plot.m_left + PEAK_LABEL_LEFT, y - PEAK_LABEL_GAP - PEAK_LABEL_HEIGHT, plot.m_right, y - PEAK_LABEL_GAP };
        renderer.DrawString(std::format(PEAK_FORMAT, Core::NumberFormat::FormatNumber(peakValue, INTEGER_DECIMALS)), labelRect,
            Theme::MUTED_TEXT_COLOR, PEAK_LABEL_STYLE);
    }

    // "직전 기록 대비 ▲ 56" / "직전 기록과 같음" (표시 자릿수 기준으로 같은지 판단, 첫 점은 비교 대상 없음)
    std::wstring LineChart::BuildChangeText(std::size_t index) const
    {
        if (!m_isChangeShown || index == FIRST_POINT || index >= m_points.size()) return {};

        const double change{ std::round(m_points[index].m_value) - std::round(m_points[index - 1].m_value) };
        if (change == 0.0) return std::wstring{ CHANGE_SAME_TEXT };

        return std::format(CHANGE_FORMAT, change > 0.0 ? UP_MARK : DOWN_MARK, Core::NumberFormat::FormatNumber(std::abs(change), INTEGER_DECIMALS));
    }

    const Core::Color& LineChart::GetChangeColor(std::size_t index) const noexcept
    {
        if (index == FIRST_POINT || index >= m_points.size()) return Theme::MUTED_TEXT_COLOR;

        const double change{ std::round(m_points[index].m_value) - std::round(m_points[index - 1].m_value) };
        if (change > 0.0) return Theme::UP_TEXT_COLOR;
        if (change < 0.0) return Theme::DOWN_TEXT_COLOR;
        return Theme::MUTED_TEXT_COLOR;
    }
}