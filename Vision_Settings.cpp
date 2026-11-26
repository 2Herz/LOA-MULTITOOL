module LostArkMultiTool.Vision.VisionSettings;  // 화면 인식 설정 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPConstants;  // 파일 이름 상수 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 트리 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::string_view KEY_ENABLED{ "Enabled" };                     // 켜짐 키
        constexpr std::string_view KEY_CHARACTER_AUTO{ "CharacterAuto" };        // 현재 캐릭터 자동 인식 키
        constexpr std::string_view KEY_CHARACTER_ALIASES{ "CharacterAliases" };  // 별칭 목록 키
        constexpr std::string_view KEY_ALIAS_READ{ "Read" };                     // 별칭: 읽은 글자
        constexpr std::string_view KEY_ALIAS_NAME{ "Name" };                     // 별칭: 캐릭터 이름
        constexpr std::string_view KEY_CHARACTER_INTERVAL{ "CharacterInterval" }; // 현재 캐릭터 확인 간격 (초) 키
    }

    VisionSettings::VisionSettings()
        : m_fileStore{ Core::APPConstants::VISION_SETTINGS_FILE_NAME }   // 저장 파일
    {
        std::optional<bool> loaded;                                      // 읽은 값 (끝까지 성공했을 때만)
        bool loadedAuto = true;                                          // 자동 인식 (없으면 켜짐 → 예전 파일도 그대로)
        int loadedInterval = DEFAULT_CHARACTER_INTERVAL_SECONDS;         // 확인 간격 (없으면 기본)
        std::vector<SavedCharacterAlias> loadedAliases;                  // 별칭 (없으면 빈 목록)
        [[maybe_unused]] const auto result = m_fileStore.Load([&](const Services::JSONValue& root)   // 파일 읽기 (안내 문구는 쓰지 않음)
            {
                const Services::JSONValue& enabled = root.FindMember(KEY_ENABLED);                   // 켜짐 값
                if (enabled.GetType() != Services::JSONType::BOOL_VALUE) throw std::runtime_error("VisionSettings: Enabled 형식 오류");   // 형식 검사
                const Services::JSONValue& autoValue = root.FindMember(KEY_CHARACTER_AUTO);          // 자동 인식 값 (없어도 됨)
                if (autoValue.GetType() == Services::JSONType::BOOL_VALUE) loadedAuto = autoValue.GetBool();   // 있으면 읽음
                const Services::JSONValue& intervalValue = root.FindMember(KEY_CHARACTER_INTERVAL);  // 확인 간격 (없어도 됨)
                if (intervalValue.GetType() == Services::JSONType::NUMBER_VALUE) loadedInterval = static_cast<int>(intervalValue.GetNumber());   // 있으면 읽음
                for (const Services::JSONValue& item : root.FindMember(KEY_CHARACTER_ALIASES).GetItems())   // 별칭마다 (없으면 빈 배열)
                {
                    const std::string& readText = item.FindMember(KEY_ALIAS_READ).GetString();      // 읽은 글자
                    const std::string& name = item.FindMember(KEY_ALIAS_NAME).GetString();          // 캐릭터 이름
                    if (!readText.empty() && !name.empty()) loadedAliases.push_back(SavedCharacterAlias{ readText, name });   // 빈 값은 버림
                }
                loaded = enabled.GetBool();                                                          // 값 (끝까지 성공)
            });
        m_isEnabled = loaded.value_or(false);                            // 없으면 꺼짐
        if (loaded)                                                      // 파일을 읽었으면
        {
            m_isCharacterAutoEnabled = loadedAuto;                       // 자동 인식
            SetCharacterIntervalSeconds(loadedInterval);                 // 확인 간격 (범위 제한)
            m_characterAliases = std::move(loadedAliases);               // 별칭
        }
    }

    bool VisionSettings::IsEnabled() const noexcept
    {
        return m_isEnabled;   // 켜짐 여부
    }

    void VisionSettings::SetEnabled(bool isEnabled) noexcept
    {
        m_isEnabled = isEnabled;   // 값 바꾸기
    }

    bool VisionSettings::IsCharacterAutoEnabled() const noexcept
    {
        return m_isCharacterAutoEnabled;   // 자동 인식 여부
    }

    void VisionSettings::SetCharacterAutoEnabled(bool isEnabled) noexcept
    {
        m_isCharacterAutoEnabled = isEnabled;   // 값 바꾸기
    }

    const std::vector<SavedCharacterAlias>& VisionSettings::GetCharacterAliases() const noexcept
    {
        return m_characterAliases;   // 별칭들
    }

    void VisionSettings::AddCharacterAlias(std::string readText, std::string name)
    {
        if (readText.empty() || name.empty()) return;                                         // 빈 값은 무시
        std::erase_if(m_characterAliases, [&readText](const SavedCharacterAlias& alias) { return alias.m_readText == readText; });   // 같은 글자는 바꿈
        m_characterAliases.push_back(SavedCharacterAlias{ std::move(readText), std::move(name) });   // 맨 뒤 (가장 새 것)
        if (m_characterAliases.size() > MAX_CHARACTER_ALIASES)                                 // 너무 많으면
            m_characterAliases.erase(m_characterAliases.begin(), m_characterAliases.end() - MAX_CHARACTER_ALIASES);   // 오래된 것부터 지움
    }

    void VisionSettings::ClearCharacterAliases() noexcept
    {
        m_characterAliases.clear();   // 모두 지움
    }

    int VisionSettings::GetCharacterIntervalSeconds() const noexcept
    {
        return m_characterIntervalSeconds;   // 확인 간격
    }

    void VisionSettings::SetCharacterIntervalSeconds(int seconds) noexcept
    {
        m_characterIntervalSeconds = std::clamp(seconds, MIN_CHARACTER_INTERVAL_SECONDS, MAX_CHARACTER_INTERVAL_SECONDS);   // 범위 제한
    }

    void VisionSettings::Save()
    {
        std::vector<Services::JSONValue> aliases;                                              // 별칭 배열
        aliases.reserve(m_characterAliases.size());                                            // 자리 확보
        for (const SavedCharacterAlias& alias : m_characterAliases)                            // 별칭마다
        {
            aliases.push_back(Services::JSONValue::CreateObject({ std::string{ KEY_ALIAS_READ }, std::string{ KEY_ALIAS_NAME } },
                { Services::JSONValue::CreateString(alias.m_readText), Services::JSONValue::CreateString(alias.m_name) }));   // { "Read": ..., "Name": ... }
        }
        m_fileStore.Save(Services::WriteJSONText(Services::JSONValue::CreateObject(
            { std::string{ KEY_ENABLED }, std::string{ KEY_CHARACTER_AUTO }, std::string{ KEY_CHARACTER_INTERVAL }, std::string{ KEY_CHARACTER_ALIASES } },
            { Services::JSONValue::CreateBool(m_isEnabled), Services::JSONValue::CreateBool(m_isCharacterAutoEnabled),
              Services::JSONValue::CreateNumber(m_characterIntervalSeconds), Services::JSONValue::CreateArray(std::move(aliases)) })));   // { "Enabled", "CharacterAuto", "CharacterInterval", "CharacterAliases" }
    }
}
