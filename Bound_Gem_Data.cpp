module LostArkMultiTool.Models.BoundGemData;

import std;

namespace LostArkMultiTool::Models
{
    BoundGemData::BoundGemData() noexcept = default;

    int BoundGemData::GetQuantity(std::size_t gemIndex, std::size_t gradeIndex) const
    {
        return m_quantities.at(gemIndex).at(gradeIndex);
    }

    void BoundGemData::SetQuantity(std::size_t gemIndex, std::size_t gradeIndex, int quantity)
    {
        m_quantities.at(gemIndex).at(gradeIndex) = std::max(quantity, 0);
    }

    bool GemScanSlot::IsEmpty() const noexcept
    {
        return !m_bound && !m_tradable && m_processed.empty();   // 아무것도 안 읽음
    }

    int GemScanSlot::GetTotal() const noexcept
    {
        return m_bound.value_or(0) + m_tradable.value_or(0) + static_cast<int>(m_processed.size());   // 귀속 + 거래 가능 + 가공 젬
    }

    const GemScanSlot& BoundGemData::GetScanSlot(std::size_t gemIndex, std::size_t gradeIndex) const
    {
        return m_scanSlots.at(gemIndex).at(gradeIndex);   // 경계 검사
    }

    void BoundGemData::SetScanSlot(std::size_t gemIndex, std::size_t gradeIndex, GemScanSlot slot)
    {
        m_scanSlots.at(gemIndex).at(gradeIndex) = std::move(slot);   // 통째로 바꿈
    }

    void BoundGemData::ClearScan(std::size_t gemIndex, std::size_t gradeIndex)
    {
        m_scanSlots.at(gemIndex).at(gradeIndex) = GemScanSlot{};   // 비움
    }

    int BoundGemData::ApplyScan(std::size_t gemIndex, std::size_t gradeIndex, GemStockKind kind, int quantity, std::string_view processedKey, int maxCount)
    {
        GemScanSlot& slot = m_scanSlots.at(gemIndex).at(gradeIndex);                 // 이 칸 내역
        const int count = std::max(quantity, 0);                                     // 음수 없음
        switch (kind)
        {
        case GemStockKind::STOCK_BOUND: slot.m_bound = count; break;                 // 귀속 묶음
        case GemStockKind::STOCK_TRADABLE: slot.m_tradable = count; break;           // 거래 가능 묶음
        case GemStockKind::STOCK_PROCESSED:                                          // 가공 젬 한 개
            if (!processedKey.empty() && std::ranges::find(slot.m_processed, processedKey) == slot.m_processed.end()
                && std::cmp_less(slot.m_processed.size(), maxCount)) slot.m_processed.emplace_back(processedKey);   // 처음 본 젬만 더함
            break;
        }
        const int total = std::min(slot.GetTotal(), maxCount);                       // 합계 (상한)
        SetQuantity(gemIndex, gradeIndex, total);                                    // 칸 수량
        return total;                                                                // 새 수량
    }
}
