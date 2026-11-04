export module LostArkMultiTool.Services.JSONValue;

import std;

export namespace LostArkMultiTool::Services
{
    // JSON 값의 종류
    enum class JSONType
    {
        NULL_VALUE,
        BOOL_VALUE,
        NUMBER_VALUE,
        STRING_VALUE,
        ARRAY_VALUE,
        OBJECT_VALUE
    };

    // JSON 트리의 값 하나 (프로젝트의 유일한 JSON 파서 결과: API 응답 · 저장 파일 모두 이걸로 읽음)
    class JSONValue
    {
    public:
        // null 값
        JSONValue() noexcept;

        [[nodiscard]] static JSONValue CreateBool(bool value);
        [[nodiscard]] static JSONValue CreateNumber(double value);
        [[nodiscard]] static JSONValue CreateString(std::string value);
        [[nodiscard]] static JSONValue CreateArray(std::vector<JSONValue> items);
        [[nodiscard]] static JSONValue CreateObject(std::vector<std::string> keys, std::vector<JSONValue> values);

        [[nodiscard]] JSONType GetType() const noexcept;
        [[nodiscard]] bool GetBool() const noexcept;                             // 불리언이 아니면 false
        [[nodiscard]] double GetNumber() const noexcept;                         // 숫자가 아니면 0
        [[nodiscard]] const std::string& GetString() const noexcept;             // 문자열이 아니면 빈 문자열
        [[nodiscard]] const std::vector<JSONValue>& GetItems() const noexcept;   // 배열이 아니면 빈 배열

        // 객체의 키 목록과 값 목록 (같은 순서, 객체가 아니면 빈 목록) → std::views::zip으로 함께 순회
        [[nodiscard]] const std::vector<std::string>& GetKeys() const noexcept;
        [[nodiscard]] const std::vector<JSONValue>& GetValues() const noexcept;

        // 객체에서 키로 값 찾기 (없거나 객체가 아니면 null 값을 참조)
        [[nodiscard]] const JSONValue& FindMember(std::string_view key) const noexcept;

    private:
        JSONType m_type{ JSONType::NULL_VALUE };
        bool m_boolValue{};
        double m_numberValue{};
        std::string m_stringValue;
        std::vector<JSONValue> m_items;      // 배열의 원소 또는 객체의 값들 (m_keys와 같은 순서)
        std::vector<std::string> m_keys;     // 객체의 키들
    };

    // JSON 문자열을 트리로 만듦 (형식이 잘못되면 std::runtime_error)
    [[nodiscard]] JSONValue ParseJSONText(std::string_view text);
}