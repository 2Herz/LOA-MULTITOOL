export module LostArkMultiTool.Services.DPAPIKeyStore;

import std;
import LostArkMultiTool.Core.APIKeyStore;

export namespace LostArkMultiTool::Services
{
    // Windows DPAPI로 암호화해서 저장 → 같은 Windows 사용자 계정에서만 복호화 가능
    // 소스 코드에 키를 넣지 않으니 코드를 공유·백업해도 키가 같이 나가지 않음 (원본 보석·낙원력 APIKeyStore와 같은 방식)
    class DPAPIKeyStore final : public Core::APIKeyStore
    {
    public:
        DPAPIKeyStore();

        [[nodiscard]] std::optional<std::string> Load() const override;
        void Save(std::string_view apiKey) override;

    private:
        std::filesystem::path m_keyPath;   // %LOCALAPPDATA%\LostArkMultiTool\APIKey.dat (생성할 때 한 번만 계산)
    };
}