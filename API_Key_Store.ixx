export module LostArkMultiTool.Core.APIKeyStore;

import std;

export namespace LostArkMultiTool::Core
{
    // API 키 저장소 인터페이스 (세 툴이 키 하나를 공유 → 한 번만 입력)
    class APIKeyStore
    {
    public:
        virtual ~APIKeyStore();

        // 저장된 키 (파일이 없거나, 다른 PC·계정에서 만든 파일이라 풀 수 없으면 nullopt → 다시 입력받음)
        [[nodiscard]] virtual std::optional<std::string> Load() const = 0;

        // 키를 암호화해서 저장 (실패하면 예외 → 입력 창에서 안내)
        virtual void Save(std::string_view apiKey) = 0;
    };
}