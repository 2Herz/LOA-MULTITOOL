export module LostArkMultiTool.Vision.VisionSettings;  // 화면 인식 설정 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기

// 화면 인식 설정 (사용자 데이터 폴더 VisionSettings.json)
// - 화면 인식 켜기 · 현재 캐릭터 자동 인식 켜기
// - 직접 고른 캐릭터(별칭): OCR이 늘 다르게 읽는 이름을 "읽은 글자 → 캐릭터"로 기억
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 저장된 별칭 하나 (UTF-8)
    struct SavedCharacterAlias
    {
        std::string m_readText;   // OCR이 읽은 글자 (띄어쓰기 없이)
        std::string m_name;       // 원정대 캐릭터 이름
    };

    // 화면 인식 설정
    class VisionSettings
    {
    public:
        static constexpr std::size_t MAX_CHARACTER_ALIASES{ 64 };   // 별칭 최대 개수 (넘으면 오래된 것부터 지움)
        static constexpr int DEFAULT_CHARACTER_INTERVAL_SECONDS{ 2 };   // 현재 캐릭터 확인 간격 기본 (초)
        static constexpr int MIN_CHARACTER_INTERVAL_SECONDS{ 1 };       // 간격 하한
        static constexpr int MAX_CHARACTER_INTERVAL_SECONDS{ 10 };      // 간격 상한

        VisionSettings();   // 생성자 (저장 파일을 읽음, 없으면 꺼짐)

        [[nodiscard]] bool IsEnabled() const noexcept;   // 화면 인식을 켰는지
        void SetEnabled(bool isEnabled) noexcept;        // 켜기 · 끄기 (저장은 Save)
        [[nodiscard]] bool IsCharacterAutoEnabled() const noexcept;   // 현재 캐릭터 자동 인식을 켰는지 (기본 켜짐)
        void SetCharacterAutoEnabled(bool isEnabled) noexcept;        // 자동 인식 켜기 · 끄기 (끄면 직접 고른 캐릭터만)
        [[nodiscard]] const std::vector<SavedCharacterAlias>& GetCharacterAliases() const noexcept;   // 별칭들
        void AddCharacterAlias(std::string readText, std::string name);   // 별칭 추가 (같은 글자는 바꿈)
        void ClearCharacterAliases() noexcept;                            // 별칭 모두 지우기
        [[nodiscard]] int GetCharacterIntervalSeconds() const noexcept;   // 현재 캐릭터 확인 간격 (초)
        void SetCharacterIntervalSeconds(int seconds) noexcept;           // 간격 바꾸기 (범위 제한)
        void Save();                                     // 파일로 저장 (실패하면 예외)

    private:
        Services::JSONFileStore m_fileStore;                  // 안전한 파일 읽기 · 쓰기
        bool m_isEnabled{};                                   // 화면 인식 켜짐
        bool m_isCharacterAutoEnabled{ true };                // 현재 캐릭터 자동 인식
        std::vector<SavedCharacterAlias> m_characterAliases;  // 별칭들 (오래된 것이 앞)
        int m_characterIntervalSeconds{ DEFAULT_CHARACTER_INTERVAL_SECONDS };   // 현재 캐릭터 확인 간격
    };
}
