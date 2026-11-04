export module LostArkMultiTool.Models.GemCatalog;

import std;
import LostArkMultiTool.Core.APPConstants;

export namespace LostArkMultiTool::Models
{
    // 보석 종류 (ShortName = 화면 표시용 짧은 이름, ItemSuffix = 경매장 아이템 이름 뒷부분, Tier = 검색용 티어)
    struct GemType
    {
        std::string_view m_shortName;    // 겁화
        std::string_view m_itemSuffix;   // 겁화의 보석
        int m_tier{};
    };

    // 보석 하나 (종류 + 레벨) — 조회 대상 · 콤보 상자 항목 · 알림 대상을 같은 형태로 다룸
    struct GemSelection
    {
        std::string m_typeName;   // 겁화
        int m_level{};            // 10

        [[nodiscard]] bool operator==(const GemSelection& other) const = default;
    };
}

// 조회 대상 보석 목록 (보석 종류는 여기만 고치면 표 · 알림 · 그래프에 모두 반영됨, 원본 GemCatalog)
// 상수 표는 constexpr라 선언과 함께 여기에 둠 (함수 정의는 .cpp)
export namespace LostArkMultiTool::Models::GemCatalog
{
    inline constexpr std::size_t GEM_TYPE_COUNT{ 2 };
    inline constexpr std::array<GemType, GEM_TYPE_COUNT> GEM_TYPES{
        GemType{ "겁화", "겁화의 보석", Core::APPConstants::GEM_TIER_4 },
        GemType{ "작열", "작열의 보석", Core::APPConstants::GEM_TIER_4 }
    };

    // 조회하는 레벨 수 (7 ~ 10레벨 = 4개) → 전체 조회 대상 수 = 종류 수 × 레벨 수
    inline constexpr std::size_t LEVEL_COUNT{ static_cast<std::size_t>(Core::APPConstants::MAX_GEM_LEVEL - Core::APPConstants::MIN_GEM_LEVEL + 1) };
    inline constexpr std::size_t TRACKED_GEM_COUNT{ GEM_TYPE_COUNT * LEVEL_COUNT };

    // 이름으로 종류 찾기 (없으면 nullptr, 가리키는 대상은 GEM_TYPES라 항상 유효)
    [[nodiscard]] const GemType* FindType(std::string_view shortName) noexcept;

    // 조회 범위(7 ~ 10레벨) 안의 레벨인지
    [[nodiscard]] bool IsLevelInRange(int level) noexcept;

    // 보석 키 "겁화_10" (시세 · 기록 · 알림을 구분하는 문자열)
    [[nodiscard]] std::string MakeKey(std::string_view typeName, int level);
    [[nodiscard]] std::string MakeKey(const GemSelection& gem);

    // 경매장 아이템 이름 "10레벨 겁화의 보석"
    [[nodiscard]] std::string MakeItemName(const GemType& type, int level);

    // 화면 표시 이름 "10레벨 겁화"
    [[nodiscard]] std::string MakeDisplayName(const GemSelection& gem);

    // 지금 조회 대상인 보석 키인지 (종류가 있고 레벨이 범위 안)
    [[nodiscard]] bool IsTrackedKey(std::string_view gemKey);

    // 전체 조회 대상 (종류 순서대로, 각 종류 안에서는 높은 레벨부터 → 표 · 콤보 상자 순서와 같음)
    [[nodiscard]] std::vector<GemSelection> GetAllGems();
}