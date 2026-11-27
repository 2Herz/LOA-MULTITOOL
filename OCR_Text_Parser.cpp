module LostArkMultiTool.Vision.OCRTextParser;  // OCR 글자 해석 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // OCR 결과 가져오기

namespace LostArkMultiTool::Vision::OCRTextParser  // OCR 해석 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr std::size_t GROUP_DIGITS{ 3 };          // 천 단위 묶음 자릿수
        constexpr std::size_t MAX_FIRST_GROUP_DIGITS{ 3 };// 첫 묶음 최대 자릿수
        constexpr std::size_t MAX_TOTAL_DIGITS{ 15 };     // 이보다 긴 숫자는 오인식으로 봄 (int64 넘침 방지)
        constexpr int DECIMAL_BASE{ 10 };                 // 십진수
        constexpr std::size_t LOOSE_DISTANCE{ 1 };        // 이 거리 이하는 같은 수준으로 보고 긴 이름을 우선
        constexpr std::size_t NAME_LENGTH_PER_ERROR{ 4 }; // 이름 길이 4글자마다 오인식 1글자 허용
        constexpr int EDGE_MARGIN{ 4 };                   // OCR 영역 오른쪽 끝에서 이만큼 안쪽까지를 "닿음"으로 봄
        constexpr std::string_view TIER_TOP_SUFFIX{ "/top" };        // 상위 값 이름 꼬리
        constexpr std::string_view TIER_BOTTOM_SUFFIX{ "/bottom" };  // 하위 값 이름 꼬리
        constexpr std::string_view TIER_LEVEL_SUFFIX{ "/level" };    // 레벨로 정하는 값 이름 꼬리

        // 알아볼 재료 이름 (계산기 Asset_Calculator_Service의 상위 · 하위 재료 이름과 같음)
        constexpr std::array MATERIAL_NAMES{
            MaterialName{ L"운명의 파괴석 결정", "Dest", MaterialTierHint::TIER_TOP },
            MaterialName{ L"운명의 수호석 결정", "Guard", MaterialTierHint::TIER_TOP },
            MaterialName{ L"위대한 운명의 돌파석", "Leap", MaterialTierHint::TIER_TOP },
            MaterialName{ L"상급 아비도스 융화 재료", "Fusion", MaterialTierHint::TIER_TOP },
            MaterialName{ L"운명의 파괴석", "Dest", MaterialTierHint::TIER_BOTTOM },
            MaterialName{ L"운명의 수호석", "Guard", MaterialTierHint::TIER_BOTTOM },
            MaterialName{ L"운명의 돌파석", "Leap", MaterialTierHint::TIER_BOTTOM },
            MaterialName{ L"아비도스 융화 재료", "Fusion", MaterialTierHint::TIER_BOTTOM },
            MaterialName{ L"용암의 숨결", "Yong", MaterialTierHint::TIER_BY_LEVEL },
            MaterialName{ L"빙하의 숨결", "Bing", MaterialTierHint::TIER_BY_LEVEL } };

        // 숫자 글자인지
        [[nodiscard]] bool IsDigit(wchar_t value) noexcept
        {
            return value >= L'0' && value <= L'9';   // 0 ~ 9
        }

        // 숫자 묶음 구분자로 쓰일 수 있는 글자인지 (OCR이 쉼표를 마침표 · 띄어쓰기로 읽음)
        [[nodiscard]] bool IsSeparator(wchar_t value) noexcept
        {
            return value == L',' || value == L'.' || value == L' ';   // 쉼표 · 마침표 · 띄어쓰기
        }

        // 띄어쓰기 · 탭인지
        [[nodiscard]] bool IsBlank(wchar_t value) noexcept
        {
            return value == L' ' || value == L'\t';   // 지울 글자
        }

        // pattern과 가장 비슷한 text 안 구간: (거리, 구간 끝 위치) — 앞뒤 글자는 공짜로 건너뜀 (반전역 정렬)
        [[nodiscard]] std::pair<std::size_t, std::size_t> FindBestMatch(std::wstring_view pattern, std::wstring_view text)
        {
            const std::size_t columns = text.size() + 1;                    // 열 수 (text 위치 0 ~ n)
            std::vector<std::size_t> previous(columns, 0);                  // pattern 0글자: 어디서 시작해도 거리 0
            std::vector<std::size_t> current(columns, 0);                   // 이번 줄
            for (std::size_t row = 1; row <= pattern.size(); ++row)         // pattern 글자마다
            {
                current[0] = row;                                           // text를 하나도 안 쓰면 pattern 글자 수만큼 지움
                for (std::size_t column = 1; column < columns; ++column)    // text 글자마다
                {
                    const std::size_t substitute = previous[column - 1] + (pattern[row - 1] == text[column - 1] ? 0 : 1);   // 바꾸기
                    current[column] = std::min({ substitute, previous[column] + 1, current[column - 1] + 1 });             // 바꾸기 · 지우기 · 넣기 중 최소
                }
                std::swap(previous, current);                               // 다음 줄로
            }
            const auto best = std::ranges::min_element(previous);           // 마지막 줄의 최소 (끝 위치 자유)
            return { *best, static_cast<std::size_t>(best - previous.begin()) };   // 거리 · 끝 위치
        }

        // 띄어쓰기를 지운 글자 + 지운 글자의 각 위치가 원래 몇 번째였는지
        [[nodiscard]] std::pair<std::wstring, std::vector<std::size_t>> CompactWithMap(std::wstring_view text)
        {
            std::wstring compact;                    // 지운 글자
            std::vector<std::size_t> positions;      // 원래 위치
            for (std::size_t index = 0; index < text.size(); ++index)   // 글자마다
            {
                if (IsBlank(text[index])) continue;  // 띄어쓰기는 건너뜀
                compact += text[index];              // 글자 추가
                positions.push_back(index);          // 원래 위치 기록
            }
            return { std::move(compact), std::move(positions) };   // 결과
        }

        // 이름 길이에 따른 허용 거리
        [[nodiscard]] std::size_t GetAllowedDistance(std::size_t nameLength) noexcept
        {
            return std::max(LOOSE_DISTANCE, nameLength / NAME_LENGTH_PER_ERROR);   // 최소 1
        }
    }

    std::wstring Compact(std::wstring_view text)
    {
        return CompactWithMap(text).first;   // 띄어쓰기 지운 글자
    }

    std::size_t FindDistance(std::wstring_view pattern, std::wstring_view text)
    {
        return FindBestMatch(pattern, text).first;   // 가장 작은 거리
    }

    std::optional<std::int64_t> ParseGroupedNumber(std::wstring_view text)
    {
        const auto start = std::ranges::find_if(text, IsDigit);          // 첫 숫자
        if (start == text.end()) return std::nullopt;                     // 숫자가 없음

        std::vector<std::wstring> groups(1);                              // 구분자로 나눈 묶음들
        for (auto position = start; position != text.end(); ++position)   // 숫자부터 끝까지
        {
            if (IsDigit(*position)) { groups.back() += *position; continue; }                  // 숫자는 지금 묶음에
            if (!IsSeparator(*position)) break;                                                 // 숫자가 끝남
            const auto afterSeparators = std::find_if_not(position, text.end(), IsSeparator);   // 이어진 구분자 끝 (", " · "  " 처럼 여러 글자)
            if (afterSeparators == text.end() || !IsDigit(*afterSeparators)) break;             // 구분자 뒤가 숫자가 아니면 끝
            groups.emplace_back();                                                              // 다음 묶음 시작
            position = std::prev(afterSeparators);                                              // 구분자 건너뜀 (반복문이 한 칸 더 감)
        }

        if (groups.size() > 1)                                            // 묶음이 여러 개면 천 단위 규칙 검사
        {
            if (groups.front().size() > MAX_FIRST_GROUP_DIGITS) return std::nullopt;   // 첫 묶음이 너무 김
            const bool isGrouped = std::ranges::all_of(groups | std::views::drop(1),
                [](const std::wstring& group) { return group.size() == GROUP_DIGITS; });   // 나머지는 3자리
            if (!isGrouped) return std::nullopt;                          // 다른 숫자가 붙어 읽힘 → 믿을 수 없음
        }

        std::int64_t value = 0;                                           // 결과
        std::size_t digitCount = 0;                                       // 자릿수
        for (const std::wstring& group : groups)                          // 묶음마다
        {
            for (wchar_t digit : group)                                   // 숫자마다
            {
                if (++digitCount > MAX_TOTAL_DIGITS) return std::nullopt; // 너무 긺
                value = value * DECIMAL_BASE + (digit - L'0');            // 자릿수 추가
            }
        }
        return value;   // 숫자
    }

    std::optional<std::int64_t> FindNumberAfter(const TextResult& text, std::wstring_view keyword, std::size_t maxDistance,
        std::optional<int> areaWidth)
    {
        const std::wstring compactKeyword = Compact(keyword);                      // 띄어쓰기 없는 keyword
        std::optional<std::int64_t> bestValue;                                     // 가장 잘 맞는 줄의 숫자
        std::size_t bestDistance = std::numeric_limits<std::size_t>::max();        // 그 줄의 거리
        for (const TextLine& line : text.m_lines)                                  // 줄마다
        {
            const auto [compact, positions] = CompactWithMap(line.m_text);         // 띄어쓰기 없는 줄
            if (compact.empty()) continue;                                         // 빈 줄
            const auto [distance, end] = FindBestMatch(compactKeyword, compact);   // keyword 위치
            if (distance > maxDistance || distance >= bestDistance) continue;      // 안 맞거나 더 나쁜 줄
            if (areaWidth && line.m_bounds.GetRight() >= *areaWidth - EDGE_MARGIN) continue;   // 영역 끝에 닿음 → 숫자가 잘렸을 수 있음
            const std::size_t originalEnd = end == 0 ? 0 : positions[end - 1] + 1; // 원래 글자에서 keyword 끝 다음 위치
            const auto value = ParseGroupedNumber(std::wstring_view{ line.m_text }.substr(originalEnd));   // 뒤의 숫자
            if (!value) continue;                                                  // 숫자가 없음
            bestValue = value;                                                     // 결과 갱신
            bestDistance = distance;                                               // 거리 갱신
        }
        return bestValue;   // 숫자 (없으면 nullopt)
    }

    std::optional<std::int64_t> ParseQuantity(std::wstring_view text)
    {
        constexpr std::wstring_view COUNT_UNIT{ L"개" };                                       // 수량 단위
        constexpr std::array<std::wstring_view, 3> UNIT_MISREADS{ L"7h", L"7H", L"74" };      // OCR이 "개"를 이렇게 읽음 (17개 → 1774 · 1 77h)
        constexpr std::size_t MAX_PLAIN_DIGITS{ 4 };                                           // "개"가 없을 때 쉼표 없이 이어 붙여도 되는 자릿수 (1,000 이상은 쉼표가 있어야 함)
        constexpr std::size_t MAX_UNIT_DIGITS{ 9 };                                            // "개"가 보이면 (잘못 읽은 74가 끼지 않음) 쉼표를 놓쳐도 이어 붙임
        constexpr int DECIMAL_BASE{ 10 };                                                      // 십진수
        std::wstring_view rest = text;                                                         // 읽을 글자
        std::size_t maxDigits = MAX_PLAIN_DIGITS;                                              // 이어 붙일 자릿수 상한
        if (const std::size_t unit = rest.find(COUNT_UNIT); unit != std::wstring_view::npos) { rest = rest.substr(0, unit); maxDigits = MAX_UNIT_DIGITS; }   // "개" 앞까지
        else                                                                                   // "개"가 없으면 잘못 읽은 모양을 끝에서 떼어 냄
        {
            while (!rest.empty() && IsBlank(rest.back())) rest.remove_suffix(1);              // 끝 띄어쓰기
            for (std::wstring_view misread : UNIT_MISREADS)                                    // 모양마다
            {
                if (rest.size() > misread.size() && rest.ends_with(misread)) { rest.remove_suffix(misread.size()); break; }   // 떼어 냄 (앞에 숫자가 남아야 함)
            }
        }
        if (const auto grouped = ParseGroupedNumber(rest)) return grouped;                    // "1,234" · "417"
        std::int64_t value = 0;                                                                // 묶음 검사 실패 → 숫자만 (짧은 수량)
        std::size_t digits = 0;                                                                // 자릿수
        for (wchar_t character : rest)                                                         // 글자마다
        {
            if (!IsDigit(character)) continue;                                                 // 숫자만
            if (++digits > maxDigits) return std::nullopt;                                     // 길면 쉼표 없이는 믿을 수 없음
            value = value * DECIMAL_BASE + (character - L'0');                                 // 자릿수 추가
        }
        if (digits == 0) return std::nullopt;                                                  // 숫자 없음
        return value;                                                                          // "1 7" → 17
    }

    std::optional<std::int64_t> FindQuantityAfter(const TextResult& text, std::wstring_view keyword, std::size_t maxDistance, std::optional<int> areaWidth)
    {
        const std::wstring compactKeyword = Compact(keyword);                      // 띄어쓰기 없는 keyword
        std::optional<std::int64_t> bestValue;                                     // 가장 잘 맞는 줄의 수량
        std::size_t bestDistance = std::numeric_limits<std::size_t>::max();        // 그 줄의 거리
        for (const TextLine& line : text.m_lines)                                  // 줄마다
        {
            const auto [compact, positions] = CompactWithMap(line.m_text);         // 띄어쓰기 없는 줄
            if (compact.empty()) continue;                                         // 빈 줄
            const auto [distance, end] = FindBestMatch(compactKeyword, compact);   // keyword 위치
            if (distance > maxDistance || distance >= bestDistance) continue;      // 안 맞거나 더 나쁜 줄
            if (areaWidth && line.m_bounds.GetRight() >= *areaWidth - EDGE_MARGIN) continue;   // 영역 끝에 닿음 → 잘렸을 수 있음
            const std::size_t originalEnd = end == 0 ? 0 : positions[end - 1] + 1; // keyword 끝 다음 위치
            const auto value = ParseQuantity(std::wstring_view{ line.m_text }.substr(originalEnd));   // 뒤의 수량
            if (!value) continue;                                                  // 수량 없음
            bestValue = value;                                                     // 결과
            bestDistance = distance;                                               // 거리
        }
        return bestValue;   // 수량 (없으면 nullopt)
    }

    std::span<const MaterialName> GetMaterialNames() noexcept
    {
        return MATERIAL_NAMES;   // 재료 이름 표
    }

    std::optional<MaterialMatch> MatchMaterial(std::wstring_view line)
    {
        const std::wstring compactLine = Compact(line);                                   // 띄어쓰기 없는 줄
        std::optional<MaterialMatch> best;                                                // 가장 잘 맞는 재료
        std::size_t bestLength = 0;                                                       // 그 이름 길이
        for (const MaterialName& material : MATERIAL_NAMES)                               // 재료마다
        {
            const std::wstring name = Compact(material.m_name);                           // 띄어쓰기 없는 이름
            const std::size_t distance = FindDistance(name, compactLine);                 // 거리
            if (distance > GetAllowedDistance(name.size())) continue;                    // 너무 다름
            const std::size_t bucket = distance <= LOOSE_DISTANCE ? 0 : distance;         // 1글자 차이까지는 같은 수준
            const std::size_t bestBucket = !best ? std::numeric_limits<std::size_t>::max()
                : (best->m_distance <= LOOSE_DISTANCE ? 0 : best->m_distance);            // 지금 최고의 수준
            const bool isBetter = bucket < bestBucket                                     // 더 잘 맞거나
                || (bucket == bestBucket && name.size() > bestLength)                     // 같은 수준이면 긴 이름 ("결정" · "위대한" 포함)
                || (bucket == bestBucket && name.size() == bestLength && distance < best->m_distance);   // 길이도 같으면 거리 작은 것
            if (!isBetter) continue;                                                      // 그대로
            best = MaterialMatch{ &material, distance };                                  // 갱신
            bestLength = name.size();                                                     // 길이 기록
        }
        return best;   // 결과 (없으면 nullopt)
    }

    std::string MakeMaterialValueKey(std::string_view key, MaterialTierHint tier)
    {
        const std::string_view suffix = tier == MaterialTierHint::TIER_TOP ? TIER_TOP_SUFFIX
            : tier == MaterialTierHint::TIER_BOTTOM ? TIER_BOTTOM_SUFFIX : TIER_LEVEL_SUFFIX;   // 꼬리
        return std::string{ key } + std::string{ suffix };                                    // "Dest/top"
    }

    std::optional<std::pair<std::string, MaterialTierHint>> SplitMaterialValueKey(std::string_view valueKey)
    {
        const std::array suffixes{
            std::pair{ TIER_TOP_SUFFIX, MaterialTierHint::TIER_TOP },
            std::pair{ TIER_BOTTOM_SUFFIX, MaterialTierHint::TIER_BOTTOM },
            std::pair{ TIER_LEVEL_SUFFIX, MaterialTierHint::TIER_BY_LEVEL } };                // 꼬리 표
        for (const auto& [suffix, tier] : suffixes)                                            // 꼬리마다
        {
            if (!valueKey.ends_with(suffix)) continue;                                         // 다른 꼬리
            return std::pair{ std::string{ valueKey.substr(0, valueKey.size() - suffix.size()) }, tier };   // 키 · 구분
        }
        return std::nullopt;   // 재료 값 이름이 아님
    }
}
