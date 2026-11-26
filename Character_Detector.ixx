export module LostArkMultiTool.Vision.CharacterDetector;  // 현재 접속 캐릭터 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.NameplateMemory;  // 이름표 모양 기억 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

// 게임 화면에서 지금 접속한 캐릭터 알아내기 (원정대 캐릭터 이름 목록과만 비교)
// 1) 로딩 화면: 아래쪽이 까맣고 진행 막대만 있음 (픽셀로 판단, OCR 없음) → 캐릭터가 바뀌었을 수 있으니 다시 확인하라는 신호
// 2) 머리 위 이름표: 내 이름만 연한 노란색 → 색으로 잘라 기억한 모양과 비교 (OCR 없음, 가장 빠름)
// 3) 처음 보는 이름표: 이름표만 3배로 OCR → 원정대 이름과 맞으면 그 모양을 기억
// 4) 캐릭터 정보 창(P): "Lv." 줄 바로 위가 이름 (화면 전체 OCR, 이름표가 안 보일 때)
// - 이름 비교는 한글 자모(초성 · 중성 · 종성) 단위 → "숡"을 "숙"으로 읽는 OCR 실수는 한 글자 차이로 봄
// - 직접 고른 캐릭터(별칭): OCR이 늘 다르게 읽는 이름은 "읽은 글자 → 캐릭터" 짝으로 기억해 정확히 같으면 바로 알아봄
// 커서 위치와 상관없이 감시 스레드가 몇 초마다 화면 전체를 한 번 OCR해서 부름
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 화면에서 찾은 신호 종류
    enum class CharacterSignal
    {
        SIGNAL_NONE,       // 아무것도 못 찾음 (전투 중 · 이름표가 가려짐 등 → 지금 캐릭터 유지)
        SIGNAL_NAME,       // 원정대 캐릭터 이름을 찾음
        SIGNAL_LOADING     // 로딩 화면 (캐릭터가 바뀌었을 수 있음)
    };

    // 신호를 찾은 곳
    enum class CharacterSource
    {
        SOURCE_NONE,           // 없음
        SOURCE_INFO_WINDOW,    // 캐릭터 정보 창
        SOURCE_NAMEPLATE       // 머리 위 이름표
    };

    // 한 번 읽은 결과
    struct CharacterReading
    {
        CharacterSignal m_signal{ CharacterSignal::SIGNAL_NONE };   // 신호
        CharacterSource m_source{ CharacterSource::SOURCE_NONE };   // 찾은 곳
        std::wstring m_name;                                        // 찾은 캐릭터 이름 (원정대 목록의 이름 그대로)
        std::optional<double> m_itemLevel;                          // 캐릭터 정보 창의 장착 아이템 레벨 (창이 없거나 못 읽으면 nullopt)
        std::wstring m_unknownName;                                 // 이름 자리에서 읽었지만 원정대와 안 맞은 글자 (직접 고를 때 별칭으로 기억)
        bool m_hasUnknownPlate{};                                   // 내 이름표는 보이지만 누군지 모름 (모양은 기억 대기 → 직접 고르면 기억)
        std::optional<PixelRect> m_plateBounds;                     // 찾은 내 이름표 위치 (설정 탭 미리 보기용)
    };

    // 별칭 하나 (OCR이 읽은 글자 → 원정대 캐릭터 이름)
    struct CharacterAlias
    {
        std::wstring m_readText;   // OCR이 읽은 글자 (띄어쓰기 없이)
        std::wstring m_name;       // 원정대 캐릭터 이름
    };

    // 현재 캐릭터 인식기
    class CharacterDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "character" };                  // 감시 결과 화면 이름: 캐릭터 확정
        static constexpr std::string_view LOADING_SCREEN_NAME{ "character_loading" };  // 감시 결과 화면 이름: 로딩 화면
        static constexpr std::string_view UNKNOWN_SCREEN_NAME{ "character_unknown" };  // 감시 결과 화면 이름: 정보 창 이름을 못 맞춤
        static constexpr std::string_view SOURCE_VALUE_KEY{ "source" };                // 결과 값: 찾은 곳 (CharacterSource 번호)
        static constexpr std::string_view ITEM_LEVEL_VALUE_KEY{ "item_level" };        // 결과 값: 아이템 레벨 × ITEM_LEVEL_SCALE
        static constexpr int ITEM_LEVEL_SCALE{ 100 };                                  // 아이템 레벨 소수 둘째 자리까지 정수로

        // 원정대 이름 · 별칭 · 이름표 모양 기억 (기억이 없으면 모양 비교 · 학습을 하지 않음)
        CharacterDetector(std::vector<std::wstring> names, std::vector<CharacterAlias> aliases = {}, std::shared_ptr<NameplateMemory> memory = nullptr);

        [[nodiscard]] bool HasNames() const noexcept;                  // 비교할 이름이 있는지

        // 프레임 한 장 읽기 (감시 스레드에서만), isFullScanAllowed가 false면 화면 전체 OCR(정보 창 찾기)을 건너뜀 (무거워서 가끔만)
        [[nodiscard]] CharacterReading Detect(const FrameImage& frame, const TextRecognizer& text, bool isFullScanAllowed = true) const;

        // OCR 결과 해석 (위치는 프레임 기준, frame이 있으면 이름표 글자색도 봄, 자체 검사용으로도 공개)
        [[nodiscard]] CharacterReading Parse(const TextResult& text, int frameWidth, int frameHeight, const FrameImage* frame = nullptr) const;

        // 단어 하나가 원정대 이름 중 하나인지 (별칭 · 자모 거리, 애매하면 nullopt) → (이름, 자모 거리)
        [[nodiscard]] std::optional<std::pair<std::wstring, std::size_t>> MatchName(std::wstring_view word) const;

        [[nodiscard]] static std::size_t FindJamoDistance(std::wstring_view name, std::wstring_view text);   // text 안 어딘가와 name의 자모 편집 거리
        [[nodiscard]] static std::optional<double> ParseItemLevel(std::wstring_view text);                  // "1775.00" · "1,775.00" · "I775.0O" → 1775.0
        [[nodiscard]] static bool IsOwnNameColor(const FrameImage& frame, const PixelRect& bounds) noexcept; // 글자가 연한 노란색(내 이름표)인지
        [[nodiscard]] static bool IsLoadingScreen(const FrameImage& frame) noexcept;                        // 로딩 화면인지 (아래쪽 검은 띠 + 진행 막대)

    private:
        std::vector<std::wstring> m_names;          // 원정대 이름 (원래 글자)
        std::vector<std::wstring> m_compactNames;   // 띄어쓰기를 뺀 비교용 이름 (같은 순서)
        std::vector<CharacterAlias> m_aliases;      // 별칭 (읽은 글자는 띄어쓰기 없이)
        std::shared_ptr<NameplateMemory> m_memory;  // 이름표 모양 기억 (화면 스레드와 함께 씀, 잠금은 기억이 함)
    };
}
