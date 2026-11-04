export module LostArkMultiTool.Core.Singleton;

// CRTP 싱글톤 베이스 (최상위 매니저 3종이 함께 상속)
// 템플릿은 쓰는 곳에서 정의가 보여야 하므로 .cpp 없이 이 파일에 정의까지 둠
//
// 사용법: class XxxManager final : public ToolManager, public Singleton<XxxManager>
//         {
//         private:
//             friend class Singleton<XxxManager>;   // GetInstance만 생성할 수 있게
//             XxxManager();                         // 생성자·소멸자는 private
//             ~XxxManager() override;
//         };
export namespace LostArkMultiTool::Core
{
    template <typename T>
    class Singleton
    {
    public:
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        Singleton(Singleton&&) = delete;
        Singleton& operator=(Singleton&&) = delete;

        // 처음 호출할 때 한 번만 생성 (함수 안 static 초기화는 스레드 안전, 프로그램 종료 때 자동 소멸 → 해제 누락 없음)
        [[nodiscard]] static T& GetInstance()
        {
            static T instance;
            return instance;
        }

    protected:
        Singleton() = default;
        ~Singleton() = default;
    };
}