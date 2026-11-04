export module LostArkMultiTool.Core.LocalTime;

import std;

// Windows 로컬 시간 도우미 (원본 C# DateTime.Now와 같은 기준)
// 여러 서비스가 같은 날짜·시각 계산을 쓰도록 한 곳에 모음
export namespace LostArkMultiTool::Core::LocalTime
{
    // 오늘 날짜 (원정대 기록 키, 옛 "MM-dd" 기록 변환에 사용)
    [[nodiscard]] std::chrono::year_month_day GetLocalToday();

    // 파일 이름용 타임스탬프 "yyyyMMdd_HHmmss" (손상 파일 보관 이름에 사용)
    [[nodiscard]] std::string MakeTimestamp();
}