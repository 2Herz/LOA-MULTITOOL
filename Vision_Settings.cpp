module LostArkMultiTool.Vision.VisionSettings;  // 화면 인식 설정 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPConstants;  // 파일 이름 상수 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 트리 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::string_view KEY_ENABLED{ "Enabled" };   // 켜짐 키
    }

    VisionSettings::VisionSettings()
        : m_fileStore{ Core::APPConstants::VISION_SETTINGS_FILE_NAME }   // 저장 파일
    {
        std::optional<bool> loaded;                                      // 읽은 값 (끝까지 성공했을 때만)
        [[maybe_unused]] const auto result = m_fileStore.Load([&loaded](const Services::JSONValue& root)   // 파일 읽기 (안내 문구는 쓰지 않음)
            {
                const Services::JSONValue& enabled = root.FindMember(KEY_ENABLED);                   // 켜짐 값
                if (enabled.GetType() != Services::JSONType::BOOL_VALUE) throw std::runtime_error("VisionSettings: Enabled 형식 오류");   // 형식 검사
                loaded = enabled.GetBool();                                                          // 값
            });
        m_isEnabled = loaded.value_or(false);                            // 없으면 꺼짐
    }

    bool VisionSettings::IsEnabled() const noexcept
    {
        return m_isEnabled;   // 켜짐 여부
    }

    void VisionSettings::SetEnabled(bool isEnabled) noexcept
    {
        m_isEnabled = isEnabled;   // 값 바꾸기
    }

    void VisionSettings::Save()
    {
        m_fileStore.Save(Services::WriteJSONText(Services::JSONValue::CreateObject(
            { std::string{ KEY_ENABLED } }, { Services::JSONValue::CreateBool(m_isEnabled) })));   // { "Enabled": true }
    }
}
