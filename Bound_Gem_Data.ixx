export module LostArkMultiTool.Models.BoundGemData;

import std;

export namespace LostArkMultiTool::Models
{
    // 화면 인식으로 읽은 젬 수량의 종류 (계산기 칸 = 전체 보유 수량(귀속 · 거래 가능 구분 없음) + 가공 젬 개수)
    enum class GemStockKind
    {
        STOCK_BOUND,       // 캐릭터 귀속 젬 묶음 ("전체 보유 수량")
        STOCK_TRADABLE,    // 거래 가능 젬 묶음 ("전체 보유 수량")
        STOCK_PROCESSED    // 가공 중인 젬 (한 개씩, 툴팁 숫자로 서로 구분)
    };

    // 한 칸(젬 · 등급)의 화면 인식 내역
    struct GemScanSlot
    {
        std::optional<int> m_bound;               // 묶음 전체 보유 수량 (귀속 · 거래 가능 합계, 아직 못 읽었으면 nullopt)
        std::optional<int> m_tradable;            // 예전(v1.10.2) 거래 가능 묶음 수량 (새로 읽으면 비움)
        std::vector<std::string> m_processed;     // 읽은 가공 젬 구분 글자 (한 개 = 한 줄)

        [[nodiscard]] bool IsEmpty() const noexcept;    // 읽은 내역이 없음
        [[nodiscard]] int GetTotal() const noexcept;    // 귀속 + 거래 가능 + 가공 젬 개수
        bool operator==(const GemScanSlot&) const = default;   // 비교 (되돌리기 · 검사용)
    };

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

        // 화면 인식 내역 (손으로 수량을 바꾸면 ClearScan으로 비움)
        [[nodiscard]] const GemScanSlot& GetScanSlot(std::size_t gemIndex, std::size_t gradeIndex) const;
        void SetScanSlot(std::size_t gemIndex, std::size_t gradeIndex, GemScanSlot slot);       // 저장 파일 읽기 · 되돌리기
        void ClearScan(std::size_t gemIndex, std::size_t gradeIndex);                          // 내역 비우기
        // 읽은 값 하나 반영 → 칸 수량 = 내역 합계 (maxCount로 자름), 새 수량을 돌려줌
        int ApplyScan(std::size_t gemIndex, std::size_t gradeIndex, GemStockKind kind, int quantity, std::string_view processedKey, int maxCount);

    private:
        std::array<std::array<int, GRADE_COUNT>, GEM_COUNT> m_quantities{};
        std::array<std::array<GemScanSlot, GRADE_COUNT>, GEM_COUNT> m_scanSlots{};   // 화면 인식 내역
    };
}
