export module LostArkMultiTool.Core.IntervalTimer;

import std;

export namespace LostArkMultiTool::Core
{
    // 일정 간격마다 한 번씩 "때가 됐다"고 알려 주는 타이머 (원본 Windows.Forms.Timer 대응: 자동 갱신 5분 · 점검 확인 30분)
    // 프레임 간격(deltaSeconds)을 더하는 대신 실제 시계로 재므로, 프레임 간격 제한이나 트레이에 숨은 동안에도 간격이 정확함
    class IntervalTimer
    {
    public:
        void Start(int intervalMilliseconds);   // 지금부터 간격을 새로 셈
        void Stop() noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;

        // 간격이 지났으면 true를 한 번 돌려주고 다음 간격을 셈 (매 프레임 호출)
        [[nodiscard]] bool Poll();

    private:
        std::int64_t m_intervalTicks{};   // 간격 (시계 눈금 수)
        std::int64_t m_nextTicks{};       // 다음에 알려 줄 시각 (시계 눈금 수)
        bool m_isRunning{};
    };
}