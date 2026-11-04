export module LostArkMultiTool.Models.PriceData;

export namespace LostArkMultiTool::Models
{
    // 재료 단가 (현재 / 직전 갱신)
    class PriceData
    {
    public:
        PriceData() noexcept;

        [[nodiscard]] double GetCurrent() const noexcept;
        void SetCurrent(double current) noexcept;

        [[nodiscard]] double GetPrevious() const noexcept;
        void SetPrevious(double previous) noexcept;

        [[nodiscard]] double GetDiff() const noexcept;   // 현재 - 직전 (원본 Diff 속성)

    private:
        double m_current{};
        double m_previous{};
    };
}