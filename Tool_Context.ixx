export module LostArkMultiTool.Core.ToolContext;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;

// 최상위 매니저가 Initialize에서 받는 공용 서비스 묶음 (의존성 주입)
// 매니저는 구현 클래스(WinHTTP · DPAPI · DX11)를 모르고 인터페이스만 사용
// 소유자는 Application → 매니저의 Shutdown이 끝날 때까지 유효
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Core
{
    struct ToolContext
    {
        Platform& m_platform;
        Notifier& m_notifier;
        MarketAPI& m_marketAPI;                    // 세 툴이 공유하는 API (키 하나)
        std::function<void()> m_openAPIKeyDialog;  // API 키 입력 창 열기 (401 오류 → "다시 입력할까요?"에서 사용)
    };
}