export module LostArkMultiTool.Models.ItemData;

export namespace LostArkMultiTool::Models
{
    // 재료 하나의 보유 수량 + 계산된 가치
    class ItemData
    {
    public:
        ItemData() noexcept;

        [[nodiscard]] double GetTopQty() const noexcept;
        void SetTopQty(double topQty) noexcept;

        [[nodiscard]] double GetBottomQty() const noexcept;
        void SetBottomQty(double bottomQty) noexcept;

        [[nodiscard]] double GetCalculatedTopValue() const noexcept;
        void SetCalculatedTopValue(double calculatedTopValue) noexcept;

        [[nodiscard]] double GetCalculatedBotValue() const noexcept;
        void SetCalculatedBotValue(double calculatedBotValue) noexcept;

    private:
        double m_topQty{};
        double m_bottomQty{};
        double m_calculatedTopValue{};   // 저장 대상 아님 (원본 [JsonIgnore])
        double m_calculatedBotValue{};   // 저장 대상 아님 (원본 [JsonIgnore])
    };
}