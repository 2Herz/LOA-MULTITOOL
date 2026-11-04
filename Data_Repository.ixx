export module LostArkMultiTool.Core.DataRepository;

import std;
import LostArkMultiTool.Models.UserData;

export namespace LostArkMultiTool::Core
{
    // 저장 파일 읽기·쓰기 인터페이스 (실제 구현은 JSON 저장소가 맡음)
    class DataRepository
    {
    public:
        virtual ~DataRepository();

        [[nodiscard]] virtual Models::UserData& GetData() = 0;
        [[nodiscard]] virtual const Models::UserData& GetData() const = 0;

        // 손상 복구 안내 문구 (없으면 nullopt)
        [[nodiscard]] virtual const std::optional<std::string>& GetLoadWarning() const noexcept = 0;

        virtual void Save() = 0;
    };
}