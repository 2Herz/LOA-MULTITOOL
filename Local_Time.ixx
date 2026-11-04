export module LostArkMultiTool.Core.LocalTime;

import std;

// Windows 로컬 시간 도우미 (원본 C# DateTime.Now와 같은 기준)
// 여러 서비스가 같은 날짜·시각 계산을 쓰도록 한 곳에 모음
export namespace LostArkMultiTool::Core::LocalTime
{
    // 오늘 날짜 (원정대 기록 키, 옛 "MM-dd" 기록 변환에 사용)
    [[nodiscard]] std::chrono::year_month_day GetLocalToday();

    // 로스트아크 기준 오늘 (일일 컨텐츠 초기화 오전 6시에 날짜가 바뀜 → 06시 전이면 전날)
    // 원정대 자산 그래프 · 보석 일별 최저가 기록의 날짜 키에 사용
    [[nodiscard]] std::chrono::year_month_day GetGameToday();

    // 파일 이름용 타임스탬프 "yyyyMMdd_HHmmss" (손상 파일 보관 이름에 사용)
    [[nodiscard]] std::string MakeTimestamp();

    // 지금 시각 (1970-01-01 UTC부터 지난 초, 호출 한도 · 일시정지 계산에 사용)
    [[nodiscard]] std::int64_t GetUnixSeconds();

    // UTC 초 → 화면 표시용 로컬 시각 "HH:mm:ss" (마지막 갱신 · 호출 한도 재개 시각)
    [[nodiscard]] std::wstring FormatClock(std::int64_t unixSeconds);

    // 날짜 ↔ 저장 키 "yyyy-MM-dd" (보석 시세 기록 키)
    [[nodiscard]] std::string MakeDateKey(const std::chrono::year_month_day& date);
    // "yyyy-MM-dd" → 날짜 (형식이 다르거나 없는 날짜면 nullopt, 시즌 기간 · 날짜 입력칸에 사용)
    [[nodiscard]] std::optional<std::chrono::year_month_day> ParseDateKey(std::string_view key);

    // 날짜에 일 수를 더함 (음수면 이전 날짜, 보관 기간 계산용)
    [[nodiscard]] std::chrono::year_month_day AddDays(const std::chrono::year_month_day& date, int days);
}