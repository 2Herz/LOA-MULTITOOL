export module LostArkMultiTool.Views.UI.LineChart;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 그래프 점 하나 (x축 글자 + 값)
    struct ChartPoint
    {
        std::wstring m_label;   // x축 글자 (예: "11/05")
        double m_value{};
    };

    // 날짜별 값 변화를 부드러운 곡선으로 그리는 그래프 (원본 Chart: Spline + 아래 영역 그라데이션 + 점 + 마지막 값 표시)
    // 마우스를 올리면 가장 가까운 점의 날짜 · 값을 말풍선으로 보여 줌 (원본 ToolTip)
    class LineChart final : public Control
    {
    public:
        LineChart();

        void SetTitle(std::wstring title);
        void SetPoints(std::vector<ChartPoint> points);
        void SetValueSuffix(std::wstring valueSuffix);   // 말풍선 값 뒤에 붙일 단위 (예: " 골드")
        void SetValuePrefix(std::wstring valuePrefix);   // 말풍선 값 앞에 붙일 글자 (예: "평균 ")
        void SetChangeShown(bool isShown) noexcept;      // 말풍선에 "직전 기록 대비 ▲ 56" 줄 표시 (원본 낙원력 그래프 툴팁)
        void SetPeakLineShown(bool isShown) noexcept;    // 가장 높은 값에 가로 점선 + "최고 N" (점이 2개 이상일 때, 원본 StripLine)
        void SetSeriesColor(const Core::Color& lineColor) noexcept;   // 곡선 · 점 테두리 색 (아래 영역은 같은 색의 반투명 그라데이션, 기본 골드)

        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;
        void Render(Core::Renderer& renderer) override;
        void OnMouseMove(const Core::MouseMoveEvent& event) override;

    private:
        // 그리기에 필요한 계산 결과 (점 · 크기가 바뀔 때만 다시 계산 → 매 프레임 글자 측정 · 곡선 계산을 하지 않음)
        struct Layout
        {
            Core::RectF m_plotArea;                       // 곡선이 그려지는 영역
            std::vector<Core::PointF> m_positions;        // 점 위치
            std::vector<Core::BezierSegment> m_segments;  // 점 사이 곡선
            std::vector<double> m_tickValues;             // y축 눈금 값
            double m_minValue{};                          // y축 아래 · 위 끝 값
            double m_maxValue{};
            std::size_t m_labelStep{ 1 };                 // x축 글자를 몇 점마다 쓸지 (겹치지 않게)
        };

        void UpdateLayout(Core::Renderer& renderer);
        [[nodiscard]] float ToY(double value) const noexcept;
        void BuildSegments();

        void RenderAxes(Core::Renderer& renderer) const;
        void RenderSeries(Core::Renderer& renderer) const;
        void RenderTooltip(Core::Renderer& renderer, std::size_t index) const;
        void RenderPeakLine(Core::Renderer& renderer) const;

        // 말풍선의 직전 기록 대비 줄 (첫 점이면 빈 글자) + 글자색
        [[nodiscard]] std::wstring BuildChangeText(std::size_t index) const;
        [[nodiscard]] const Core::Color& GetChangeColor(std::size_t index) const noexcept;

        std::wstring m_title;
        std::vector<ChartPoint> m_points;
        std::wstring m_valueSuffix;
        std::wstring m_valuePrefix;
        bool m_isChangeShown{};
        bool m_isPeakLineShown{};
        Core::Color m_lineColor;
        Core::Color m_areaTopColor;
        Core::Color m_areaBottomColor;

        Layout m_layout;
        bool m_isLayoutDirty{ true };
        Core::RectF m_layoutBounds;                       // 마지막으로 계산한 부품 크기
        std::optional<std::size_t> m_hoverIndex;          // 마우스가 가리키는 점
    };
}