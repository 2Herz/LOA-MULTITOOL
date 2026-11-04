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
}
