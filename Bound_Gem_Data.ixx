export module LostArkMultiTool.Models.BoundGemData;

import std;

export namespace LostArkMultiTool::Models
{
    // 귀속 젬 보유 수량 (젬 6종 × 등급 3단계, 귀속 젬 계산기의 저장 데이터)
    // 젬 · 등급 번호는 아래 이름 목록의 순서 (표의 행 · 열 순서와 같음)
    class BoundGemData
    {
    public:
        static constexpr std::size_t GEM_COUNT{ 6 };
        static constexpr std::size_t GRADE_COUNT{ 3 };

        // 거래소 아이템 이름 (UTF-8, 질서 = 안정 · 불변 · 견고, 혼돈 = 침식 · 왜곡 · 붕괴)
        static constexpr std::array<std::string_view, GEM_COUNT> GEM_NAMES
        {
            "질서의 젬 : 안정", "질서의 젬 : 불변", "질서의 젬 : 견고",
            "혼돈의 젬 : 침식", "혼돈의 젬 : 왜곡", "혼돈의 젬 : 붕괴"
        };

        // 등급 이름 (UTF-8, 낮은 등급부터)
        static constexpr std::array<std::string_view, GRADE_COUNT> GRADE_NAMES{ "고급", "희귀", "영웅" };

        BoundGemData() noexcept;

        // 범위를 벗어난 번호면 std::out_of_range (경계 검사)
        [[nodiscard]] int GetQuantity(std::size_t gemIndex, std::size_t gradeIndex) const;
        void SetQuantity(std::size_t gemIndex, std::size_t gradeIndex, int quantity);   // 음수는 0으로

    private:
        std::array<std::array<int, GRADE_COUNT>, GEM_COUNT> m_quantities{};
    };
}
