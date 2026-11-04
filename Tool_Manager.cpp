module LostArkMultiTool.Core.ToolManager;

import std;

namespace LostArkMultiTool::Core
{
    ToolManager::~ToolManager() = default;

    std::optional<std::wstring> ToolManager::GetBackgroundStatus() const
    {
        return std::nullopt;   // 기본: 백그라운드 작업 없음
    }

    void ToolManager::OnAPIKeyChanged()
    {}
}