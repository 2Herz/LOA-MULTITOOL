module LostArkMultiTool.Views.OverlaySettings;  // 인게임 오버레이 설정 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPConstants;  // 파일 이름 상수 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 트리 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        using Services::JSONType;    // JSON 값 종류
        using Services::JSONValue;   // JSON 값

        constexpr std::string_view KEY_ENABLED{ "Enabled" };        // 켜짐 키
        constexpr std::string_view KEY_OPACITY{ "Opacity" };        // 불투명도 키
        constexpr std::string_view KEY_MINI_TOOL{ "MiniTool" };     // 미니 탭 키
        constexpr std::string_view KEY_PANELS{ "Panels" };          // 패널 목록 키
        constexpr std::string_view KEY_ID{ "Id" };                  // 패널 이름 키
        constexpr std::string_view KEY_VISIBLE{ "Visible" };        // 보임 키
        constexpr std::string_view KEY_X{ "X" };                    // 왼쪽 키
        constexpr std::string_view KEY_Y{ "Y" };                    // 위쪽 키
        constexpr std::string_view KEY_WIDTH{ "Width" };            // 가로 키
        constexpr std::string_view KEY_HEIGHT{ "Height" };          // 세로 키
        constexpr std::string_view KEY_ALPHA{ "Alpha" };            // 요소 투명도 키

        // 멤버가 숫자면 그 값, 아니면 기본값
        [[nodiscard]] double ReadNumber(const JSONValue& object, std::string_view key, double fallback)
        {
            const JSONValue& member = object.FindMember(key);                                   // 멤버
            return member.GetType() == JSONType::NUMBER_VALUE ? member.GetNumber() : fallback;  // 숫자 또는 기본값
        }

        // 멤버가 참거짓이면 그 값, 아니면 기본값
        [[nodiscard]] bool ReadBool(const JSONValue& object, std::string_view key, bool fallback)
        {
            const JSONValue& member = object.FindMember(key);                                   // 멤버
            return member.GetType() == JSONType::BOOL_VALUE ? member.GetBool() : fallback;      // 값 또는 기본값
        }

        // 키 목록 (string_view → string)
        [[nodiscard]] std::vector<std::string> MakeKeys(std::initializer_list<std::string_view> keys)
        {
            std::vector<std::string> result;                                 // 결과
            for (std::string_view key : keys) result.emplace_back(key);     // 복사
            return result;                                                   // 키 목록
        }
    }

    OverlaySettings::OverlaySettings()
        : m_fileStore{ Core::APPConstants::OVERLAY_SETTINGS_FILE_NAME }   // 저장 파일
    {
        [[maybe_unused]] const auto result = m_fileStore.Load([this](const JSONValue& root)   // 파일 읽기 (형식이 조금 달라도 기본값으로 채움)
            {
                m_isEnabled = ReadBool(root, KEY_ENABLED, false);                                          // 켜짐
                SetOpacity(static_cast<float>(ReadNumber(root, KEY_OPACITY, DEFAULT_OPACITY)));             // 불투명도
                m_miniToolIndex = static_cast<std::size_t>(std::max(ReadNumber(root, KEY_MINI_TOOL, 0.0), 0.0));   // 미니 탭
                for (const JSONValue& item : root.FindMember(KEY_PANELS).GetItems())                       // 패널마다
                {
                    const JSONValue& id = item.FindMember(KEY_ID);                                         // 이름
                    if (id.GetType() != JSONType::STRING_VALUE) continue;                                  // 이름 없으면 건너뜀
                    m_panels.push_back(OverlayPanelState{ id.GetString(), ReadBool(item, KEY_VISIBLE, false),
                        static_cast<float>(ReadNumber(item, KEY_X, 0.0)), static_cast<float>(ReadNumber(item, KEY_Y, 0.0)),
                        static_cast<float>(ReadNumber(item, KEY_WIDTH, 0.0)), static_cast<float>(ReadNumber(item, KEY_HEIGHT, 0.0)),
                        std::clamp(static_cast<float>(ReadNumber(item, KEY_ALPHA, MAX_ELEMENT_ALPHA)), MIN_ELEMENT_ALPHA, MAX_ELEMENT_ALPHA) });   // 상태 (투명도가 없던 예전 파일은 100%)
                }
            });
    }

    bool OverlaySettings::IsEnabled() const noexcept
    {
        return m_isEnabled;   // 켜짐
    }

    void OverlaySettings::SetEnabled(bool isEnabled) noexcept
    {
        m_isEnabled = isEnabled;   // 바꾸기
    }

    float OverlaySettings::GetOpacity() const noexcept
    {
        return m_opacity;   // 불투명도
    }

    void OverlaySettings::SetOpacity(float opacity) noexcept
    {
        m_opacity = std::clamp(opacity, MIN_OPACITY, MAX_OPACITY);   // 범위 제한
    }

    std::size_t OverlaySettings::GetMiniToolIndex() const noexcept
    {
        return m_miniToolIndex;   // 미니 탭
    }

    void OverlaySettings::SetMiniToolIndex(std::size_t index) noexcept
    {
        m_miniToolIndex = index;   // 바꾸기
    }

    std::optional<OverlayPanelState> OverlaySettings::FindPanel(std::string_view id) const
    {
        const auto found = std::ranges::find(m_panels, id, &OverlayPanelState::m_id);   // 이름으로 찾기
        if (found == m_panels.end()) return std::nullopt;                               // 없음
        return *found;                                                                  // 복사본
    }

    void OverlaySettings::SetPanel(OverlayPanelState state)
    {
        const auto found = std::ranges::find(m_panels, state.m_id, &OverlayPanelState::m_id);   // 같은 패널
        if (found != m_panels.end()) *found = std::move(state);                                // 교체
        else m_panels.push_back(std::move(state));                                             // 추가
    }

    void OverlaySettings::RemovePanel(std::string_view id)
    {
        std::erase_if(m_panels, [id](const OverlayPanelState& state) { return state.m_id == id; });   // 이름이 같은 상태 지움
    }

    void OverlaySettings::Save()
    {
        std::vector<JSONValue> panels;                                                           // 패널 목록
        for (const OverlayPanelState& panel : m_panels)                                          // 패널마다
            panels.push_back(JSONValue::CreateObject(MakeKeys({ KEY_ID, KEY_VISIBLE, KEY_X, KEY_Y, KEY_WIDTH, KEY_HEIGHT, KEY_ALPHA }),
                { JSONValue::CreateString(panel.m_id), JSONValue::CreateBool(panel.m_isVisible), JSONValue::CreateNumber(panel.m_x),
                  JSONValue::CreateNumber(panel.m_y), JSONValue::CreateNumber(panel.m_width), JSONValue::CreateNumber(panel.m_height),
                  JSONValue::CreateNumber(panel.m_alpha) }));   // 한 패널
        m_fileStore.Save(Services::WriteJSONText(JSONValue::CreateObject(MakeKeys({ KEY_ENABLED, KEY_OPACITY, KEY_MINI_TOOL, KEY_PANELS }),
            { JSONValue::CreateBool(m_isEnabled), JSONValue::CreateNumber(m_opacity),
              JSONValue::CreateNumber(static_cast<double>(m_miniToolIndex)), JSONValue::CreateArray(std::move(panels)) })));   // 저장
    }
}
