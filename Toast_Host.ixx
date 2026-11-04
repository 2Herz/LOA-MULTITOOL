export module LostArkMultiTool.Views.UI.ToastHost;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Views::UI
{
    // 오른쪽 아래에 잠깐 떴다가 사라지는 알림 (원본 트레이 풍선 알림 · "완료!" 메시지 대응)
    // 입력을 막지 않고, 여러 개면 위로 쌓임 (가장 최근 것이 맨 아래)
    class ToastHost
    {
    public:
        ToastHost();

        void Add(std::wstring message, Core::NoticeKind kind);
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& screenArea);

    private:
        struct Toast
        {
            std::wstring m_message;
            Core::NoticeKind m_kind{ Core::NoticeKind::NOTICE_INFO };
            double m_ageSeconds{};
            double m_lifeSeconds{};
        };

        [[nodiscard]] static float GetOpacity(const Toast& toast) noexcept;

        std::deque<Toast> m_toasts;   // 앞 = 오래된 알림
    };
}