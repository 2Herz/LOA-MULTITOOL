module LostArkMultiTool.Models.GrowthData;

import std;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.Season;
import LostArkMultiTool.Models.WeeklyRecord;

namespace LostArkMultiTool::Models
{
    namespace
    {
        constexpr int NEXT_NUMBER_STEP{ 1 };
        constexpr std::string_view NO_SEASON_MESSAGE{ "시즌 기록이 없어요! (EnsureValid 호출 전)" };
    }

    const std::vector<CharacterInfo>& GrowthData::GetCharacters() const noexcept
    {
        return m_characters;
    }

    void GrowthData::SetCharacters(std::vector<CharacterInfo> characters)
    {
        m_characters = std::move(characters);
    }

    const std::vector<Season>& GrowthData::GetSeasons() const noexcept
    {
        return m_seasons;
    }

    std::vector<Season>& GrowthData::GetSeasons() noexcept
    {
        return m_seasons;
    }

    void GrowthData::SetSeasons(std::vector<Season> seasons)
    {
        m_seasons = std::move(seasons);
    }

    int GrowthData::GetSelectedSeasonNumber() const noexcept
    {
        return m_selectedSeasonNumber;
    }

    void GrowthData::SetSelectedSeasonNumber(int seasonNumber) noexcept
    {
        m_selectedSeasonNumber = seasonNumber;
    }

    Season& GrowthData::GetCurrentSeason()
    {
        // const 버전과 같은 규칙 → 한 곳에서만 찾음
        return const_cast<Season&>(std::as_const(*this).GetCurrentSeason());
    }

    const Season& GrowthData::GetCurrentSeason() const
    {
        if (m_seasons.empty()) throw std::logic_error{ std::string{ NO_SEASON_MESSAGE } };

        const auto found{ std::ranges::find(m_seasons, m_selectedSeasonNumber, &Season::GetSeasonNumber) };
        return found != m_seasons.end() ? *found : m_seasons.back();   // 원본 FirstOrDefault(...) ?? Seasons[^1]
    }

    bool GrowthData::HasSeason(int seasonNumber) const noexcept
    {
        return std::ranges::any_of(m_seasons, [seasonNumber](const Season& season) { return season.GetSeasonNumber() == seasonNumber; });
    }

    int GrowthData::GetNextSeasonNumber() const noexcept
    {
        int maxNumber{};
        for (const Season& season : m_seasons) maxNumber = std::max(maxNumber, season.GetSeasonNumber());
        return maxNumber + NEXT_NUMBER_STEP;
    }

    void GrowthData::EnsureValid(std::vector<WeeklyRecord> legacyRecords)
    {
        // 이름이 빈 캐릭터는 표 열 · 기록의 열쇠가 될 수 없음
        std::erase_if(m_characters, [](const CharacterInfo& character) { return character.GetCharacterName().empty(); });

        // v1 기록은 그대로 첫 시즌(시즌 4)으로 옮김 (원본과 같음: 시즌이 이미 있으면 옮기지 않음)
        if (!legacyRecords.empty() && m_seasons.empty())
        {
            Season firstSeason{ FIRST_SEASON_NUMBER };
            firstSeason.SetRecords(std::move(legacyRecords));
            m_seasons.push_back(std::move(firstSeason));
        }

        if (m_seasons.empty()) m_seasons.emplace_back(FIRST_SEASON_NUMBER);

        // 시즌 시작점은 시즌 4로 고정 → 원본 C# 기록처럼 "시즌 1"부터 시작하는 기록은 차례를 유지한 채 4부터로 밀어 줌
        const int lowestNumber{ std::ranges::min(m_seasons, {}, &Season::GetSeasonNumber).GetSeasonNumber() };
        if (lowestNumber < FIRST_SEASON_NUMBER)
        {
            const int shift{ FIRST_SEASON_NUMBER - lowestNumber };
            for (Season& season : m_seasons) season.SetSeasonNumber(season.GetSeasonNumber() + shift);
            m_selectedSeasonNumber += shift;
        }
        if (!HasSeason(m_selectedSeasonNumber)) m_selectedSeasonNumber = m_seasons.back().GetSeasonNumber();
    }
}
