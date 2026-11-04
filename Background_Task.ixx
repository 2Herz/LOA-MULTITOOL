export module LostArkMultiTool.Core.BackgroundTask;

import std;

// 오래 걸리는 일(HTTP 요청 등)을 백그라운드 스레드에서 한 번 실행하고, 화면 스레드가 매 프레임 끝났는지 확인해 결과를 가져가는 도구
// 원본 C# async/await 대응: 화면이 멈추지 않고, 결과 · 예외는 TakeResult에서 화면 스레드로 그대로 전달됨
// 템플릿은 쓰는 곳에서 정의가 보여야 하므로 .cpp 없이 이 파일에 정의까지 둠
export namespace LostArkMultiTool::Core
{
    template <typename ResultType>
    class BackgroundTask
    {
    public:
        BackgroundTask() = default;

        // 실행 중인 일이 끝날 때까지 기다림 (스레드가 이미 사라진 객체를 건드리지 않게 → RAII)
        ~BackgroundTask()
        {
            Wait();
        }

        BackgroundTask(const BackgroundTask&) = delete;
        BackgroundTask& operator=(const BackgroundTask&) = delete;

        // 실행 중이 아닐 때만 시작 (이미 실행 중이면 false → 같은 요청이 겹치지 않음)
        template <typename Work>
        bool Start(Work&& work)
        {
            if (IsRunning()) return false;

            m_future = std::async(std::launch::async, std::forward<Work>(work));
            return true;
        }

        // 시작했고 아직 결과를 가져가지 않음
        [[nodiscard]] bool IsRunning() const noexcept
        {
            return m_future.valid();
        }

        // 일이 끝나서 결과를 가져갈 수 있음 (기다리지 않고 바로 확인)
        [[nodiscard]] bool IsReady() const
        {
            return m_future.valid() && m_future.wait_for(NO_WAIT) == std::future_status::ready;
        }

        // 결과 가져가기 (IsReady일 때 호출, 일에서 던진 예외는 여기서 다시 던져짐) → 가져간 뒤에는 IsRunning() == false
        ResultType TakeResult()
        {
            return m_future.get();
        }

        // 끝날 때까지 기다림 (프로그램 종료 전, 공용 서비스가 사라지기 전에 호출)
        void Wait() const
        {
            if (m_future.valid()) m_future.wait();
        }

    private:
        static constexpr std::chrono::seconds NO_WAIT{ 0 };

        std::future<ResultType> m_future;
    };
}