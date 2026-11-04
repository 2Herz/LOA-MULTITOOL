export module LostArkMultiTool.Models.PriceAlarm;

import std;
import LostArkMultiTool.Models.GemCatalog;

export namespace LostArkMultiTool::Models
{
    // 목표가 알림 (원본 PriceAlarm, 보석 하나당 알림 하나)
    class PriceAlarm
    {
    public:
        PriceAlarm(GemSelection gem, std::int64_t targetPrice);

        [[nodiscard]] const GemSelection& GetGem() const noexcept;
        [[nodiscard]] std::string GetGemKey() const;        // "겁화_10" (저장하지 않는 계산 값)
        [[nodiscard]] std::string GetDisplayName() const;   // "10레벨 겁화"

        [[nodiscard]] std::int64_t GetTargetPrice() const noexcept;
        void SetTargetPrice(std::int64_t targetPrice) noexcept;

        // 현재 목표가 이하인지 (화면 표시용, 저장됨)
        [[nodiscard]] bool IsTriggered() const noexcept;
        void SetTriggered(bool isTriggered) noexcept;

        // 이번 실행에서 도달 알림을 이미 보냈는지 (저장하지 않음)
        // → 재시작하면 false로 시작해서, 여전히 목표가 이하라면 첫 갱신 때 한 번 알려 줌
        [[nodiscard]] bool IsNotified() const noexcept;
        void SetNotified(bool isNotified) noexcept;

    private:
        GemSelection m_gem;
        std::int64_t m_targetPrice{};
        bool m_isTriggered{};
        bool m_isNotified{};
    };
}