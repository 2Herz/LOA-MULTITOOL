export module LostArkMultiTool.Vision.NameplateMemory;  // 머리 위 이름표 모양 기억 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기

// 내 캐릭터 머리 위 이름표(연한 노란 글자)의 "모양"을 캐릭터별로 기억 (사용자 데이터 폴더 NameplateShapes.json)
// - 게임 글꼴의 겹받침 이름(핢 · 숡 · 읾 등)은 OCR이 자주 틀리지만, 같은 이름은 화면에 늘 같은 모양으로 그려짐
// - 처음 한 번(OCR로 맞추거나 [변경]으로 직접 고를 때) 모양을 기억해 두면, 다음부터는 OCR 없이 모양만 비교해 바로 알아봄
// - 감시 스레드(비교 · 기억)와 화면 스레드([변경] · 초기화)가 함께 쓰므로 잠금으로 보호 (모니터 패턴)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 이름표 모양 하나 (노란 글자 픽셀 = 1, 나머지 = 0)
    struct NameplateShape
    {
        int m_width{};                     // 가로 (픽셀)
        int m_height{};                    // 세로 (픽셀)
        std::vector<std::uint8_t> m_bits;  // 픽셀마다 0 · 1 (줄 순서)

        [[nodiscard]] bool IsEmpty() const noexcept;          // 비었는지
        [[nodiscard]] int GetPixelCount() const noexcept;     // 글자 픽셀 수
    };

    // 프레임에서 찾은 내 이름표
    struct NameplateBlob
    {
        PixelRect m_bounds;        // 이름 글자 위치 (프레임 기준)
        NameplateShape m_shape;    // 모양
    };

    // 기억을 파일에 둘지
    enum class MemoryStorage
    {
        STORAGE_FILE,    // 사용자 데이터 폴더 파일 (프로그램)
        STORAGE_NONE     // 메모리에만 (자체 검사 · 파일을 건드리지 않음)
    };

    // 이름표 모양 기억 (모니터)
    class NameplateMemory
    {
    public:
        static constexpr std::size_t MAX_SHAPES_PER_CHARACTER{ 4 };   // 캐릭터마다 기억할 모양 수 (배경에 따라 조금씩 다름)
        static constexpr double MATCH_SCORE{ 0.80 };                  // 이 이상 닮으면 같은 이름표
        static constexpr double SAME_SHAPE_SCORE{ 0.95 };             // 이 이상 닮으면 이미 기억한 모양 (새로 넣지 않음)

        explicit NameplateMemory(MemoryStorage storage = MemoryStorage::STORAGE_FILE);   // 생성자 (파일이면 읽음, 없으면 빈 기억)

        // 모양으로 캐릭터 찾기 (가장 닮은 캐릭터, MATCH_SCORE 미만이거나 두 캐릭터가 비슷하게 닮으면 nullopt) → UTF-8 이름
        [[nodiscard]] std::optional<std::string> Match(const NameplateShape& shape) const;
        // Match와 같지만 점수도 함께 (SAME_SHAPE_SCORE 미만이면 다른 캐릭터와 헷갈릴 수 있어 글자로 다시 확인)
        [[nodiscard]] std::optional<std::pair<std::string, double>> MatchWithScore(const NameplateShape& shape) const;
        void Learn(const std::string& name, const NameplateShape& shape);   // 이 캐릭터의 모양으로 기억 (저장까지)
        void SetPending(const NameplateShape& shape);                       // 못 알아본 모양 (직접 고를 때 쓰려고 잠깐 기억)
        [[nodiscard]] bool LearnPending(const std::string& name, std::chrono::seconds maxAge);   // 잠깐 기억한 모양을 이 캐릭터로 (오래됐으면 false)
        [[nodiscard]] std::size_t GetCharacterCount() const;                // 모양을 기억한 캐릭터 수
        void Clear();                                                        // 모두 지우기 (저장까지)
        void RemoveMissing(const std::vector<std::string>& names);           // 원정대에 없는 캐릭터 모양 지우기

        // 두 모양이 얼마나 닮았는지 (0 ~ 1, 크기가 많이 다르면 0, 1픽셀 어긋남까지 맞춰 봄)
        [[nodiscard]] static double Compare(const NameplateShape& a, const NameplateShape& b) noexcept;

        // 프레임 가운데 근처에서 내 이름표(연한 노란 글자) 찾기 (없으면 nullopt)
        [[nodiscard]] static std::optional<NameplateBlob> FindOwnNameplate(const FrameImage& frame);

    private:
        void SaveLocked();   // 파일로 저장 (잠금 안에서, 실패는 조용히 넘김)

        // 캐릭터 하나의 모양들
        struct Entry
        {
            std::string m_name;                    // 캐릭터 이름 (UTF-8)
            std::vector<NameplateShape> m_shapes;  // 모양들 (오래된 것이 앞)
        };

        mutable std::mutex m_mutex;                                   // 두 스레드 보호
        MemoryStorage m_storage;                                      // 파일에 둘지
        Services::JSONFileStore m_fileStore;                          // 안전한 파일 읽기 · 쓰기
        std::vector<Entry> m_entries;                                 // 캐릭터별 모양
        std::optional<NameplateShape> m_pending;                      // 못 알아본 모양
        std::chrono::steady_clock::time_point m_pendingAt;            // 그 시각
    };
}
