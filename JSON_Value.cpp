module LostArkMultiTool.Services.JSONValue;

import std;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr std::string_view JSON_FORMAT_ERROR_MESSAGE{ "JSON 형식이 올바르지 않습니다." };   // API 응답 · 저장 파일 공용
        constexpr std::string_view NULL_LITERAL{ "null" };
        constexpr std::string_view TRUE_LITERAL{ "true" };
        constexpr std::string_view FALSE_LITERAL{ "false" };
        constexpr char END_OF_TEXT{ '\0' };

        // \uXXXX 이스케이프 처리용
        constexpr std::size_t HEX_DIGIT_COUNT{ 4 };
        constexpr int HEX_RADIX{ 16 };
        constexpr unsigned HIGH_SURROGATE_FIRST{ 0xD800 };
        constexpr unsigned HIGH_SURROGATE_LAST{ 0xDBFF };
        constexpr unsigned LOW_SURROGATE_FIRST{ 0xDC00 };
        constexpr unsigned LOW_SURROGATE_LAST{ 0xDFFF };
        constexpr unsigned SURROGATE_PAIR_BASE{ 0x10000 };
        constexpr unsigned SURROGATE_PAYLOAD_BITS{ 10 };
        constexpr std::size_t UNICODE_ESCAPE_PREFIX_LENGTH{ 2 };
        constexpr std::string_view UNICODE_ESCAPE_PREFIX{ "\\u" };

        // 코드 포인트 → UTF-8 변환용
        constexpr unsigned UTF8_2BYTE_START{ 0x80 };
        constexpr unsigned UTF8_3BYTE_START{ 0x800 };
        constexpr unsigned UTF8_4BYTE_START{ 0x10000 };
        constexpr unsigned UTF8_2BYTE_LEAD{ 0xC0 };
        constexpr unsigned UTF8_3BYTE_LEAD{ 0xE0 };
        constexpr unsigned UTF8_4BYTE_LEAD{ 0xF0 };
        constexpr unsigned UTF8_CONTINUATION{ 0x80 };
        constexpr unsigned UTF8_PAYLOAD_MASK{ 0x3F };
        constexpr unsigned UTF8_SHIFT_6{ 6 };
        constexpr unsigned UTF8_SHIFT_12{ 12 };
        constexpr unsigned UTF8_SHIFT_18{ 18 };

        [[noreturn]] void ThrowFormatError()
        {
            throw std::runtime_error{ std::string{ JSON_FORMAT_ERROR_MESSAGE } };
        }

        bool IsJSONWhitespace(char c) noexcept
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        bool IsNumberChar(char c) noexcept
        {
            return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
        }

        // 코드 포인트 하나를 UTF-8 바이트열로 덧붙임
        void AppendUTF8(std::string& out, unsigned codepoint)
        {
            if (codepoint < UTF8_2BYTE_START)
            {
                out.push_back(static_cast<char>(codepoint));
            }
            else if (codepoint < UTF8_3BYTE_START)
            {
                out.push_back(static_cast<char>(UTF8_2BYTE_LEAD | (codepoint >> UTF8_SHIFT_6)));
                out.push_back(static_cast<char>(UTF8_CONTINUATION | (codepoint & UTF8_PAYLOAD_MASK)));
            }
            else if (codepoint < UTF8_4BYTE_START)
            {
                out.push_back(static_cast<char>(UTF8_3BYTE_LEAD | (codepoint >> UTF8_SHIFT_12)));
                out.push_back(static_cast<char>(UTF8_CONTINUATION | ((codepoint >> UTF8_SHIFT_6) & UTF8_PAYLOAD_MASK)));
                out.push_back(static_cast<char>(UTF8_CONTINUATION | (codepoint & UTF8_PAYLOAD_MASK)));
            }
            else
            {
                out.push_back(static_cast<char>(UTF8_4BYTE_LEAD | (codepoint >> UTF8_SHIFT_18)));
                out.push_back(static_cast<char>(UTF8_CONTINUATION | ((codepoint >> UTF8_SHIFT_12) & UTF8_PAYLOAD_MASK)));
                out.push_back(static_cast<char>(UTF8_CONTINUATION | ((codepoint >> UTF8_SHIFT_6) & UTF8_PAYLOAD_MASK)));
                out.push_back(static_cast<char>(UTF8_CONTINUATION | (codepoint & UTF8_PAYLOAD_MASK)));
            }
        }

        // 재귀 하강 방식의 JSON 파서 (내부 전용)
        class JSONParser
        {
        public:
            explicit JSONParser(std::string_view text) noexcept
                : m_text{ text }
            {}

            JSONValue ParseDocument()
            {
                JSONValue root{ ParseValue() };
                SkipWhitespace();
                if (!IsRawEnd()) ThrowFormatError();   // 값 뒤에 남은 글자가 있으면 형식 오류
                return root;
            }

        private:
            JSONValue ParseValue()
            {
                switch (PeekChar())
                {
                    case '{': return ParseObject();
                    case '[': return ParseArray();
                    case '"': return JSONValue::CreateString(ParseString());
                    case 't':
                        ExpectLiteral(TRUE_LITERAL);
                        return JSONValue::CreateBool(true);
                    case 'f':
                        ExpectLiteral(FALSE_LITERAL);
                        return JSONValue::CreateBool(false);
                    case 'n':
                        ExpectLiteral(NULL_LITERAL);
                        return JSONValue{};
                    default:
                        return JSONValue::CreateNumber(ParseNumber());
                }
            }

            JSONValue ParseObject()
            {
                ExpectChar('{');
                std::vector<std::string> keys;
                std::vector<JSONValue> values;

                if (PeekChar() == '}')
                {
                    ++m_position;
                    return JSONValue::CreateObject(std::move(keys), std::move(values));
                }

                while (true)
                {
                    keys.push_back(ParseString());
                    ExpectChar(':');
                    values.push_back(ParseValue());

                    const char next{ PeekChar() };
                    if (next == ',')
                    {
                        ++m_position;
                        continue;
                    }
                    if (next == '}')
                    {
                        ++m_position;
                        break;
                    }
                    ThrowFormatError();
                }
                return JSONValue::CreateObject(std::move(keys), std::move(values));
            }

            JSONValue ParseArray()
            {
                ExpectChar('[');
                std::vector<JSONValue> items;

                if (PeekChar() == ']')
                {
                    ++m_position;
                    return JSONValue::CreateArray(std::move(items));
                }

                while (true)
                {
                    items.push_back(ParseValue());

                    const char next{ PeekChar() };
                    if (next == ',')
                    {
                        ++m_position;
                        continue;
                    }
                    if (next == ']')
                    {
                        ++m_position;
                        break;
                    }
                    ThrowFormatError();
                }
                return JSONValue::CreateArray(std::move(items));
            }

            std::string ParseString()
            {
                ExpectChar('"');

                std::string result;
                while (true)
                {
                    if (IsRawEnd()) ThrowFormatError();
                    const char current{ m_text[m_position++] };

                    if (current == '"') return result;
                    if (current != '\\')
                    {
                        result.push_back(current);
                        continue;
                    }

                    if (IsRawEnd()) ThrowFormatError();
                    const char escaped{ m_text[m_position++] };
                    switch (escaped)
                    {
                        case '"':  result.push_back('"'); break;
                        case '\\': result.push_back('\\'); break;
                        case '/':  result.push_back('/'); break;
                        case 'b':  result.push_back('\b'); break;
                        case 'f':  result.push_back('\f'); break;
                        case 'n':  result.push_back('\n'); break;
                        case 'r':  result.push_back('\r'); break;
                        case 't':  result.push_back('\t'); break;
                        case 'u':  AppendUTF8(result, ReadUnicodeEscape()); break;
                        default:   ThrowFormatError();
                    }
                }
            }

            // \uXXXX 한 개 (상위·하위 대리 문자면 쌍으로 묶어서 코드 포인트를 만듦)
            unsigned ReadUnicodeEscape()
            {
                const unsigned first{ ReadHex4() };
                if (first < HIGH_SURROGATE_FIRST || first > LOW_SURROGATE_LAST) return first;   // 일반 글자
                if (first >= LOW_SURROGATE_FIRST) ThrowFormatError();                           // 짝 없는 하위 대리 문자

                // 상위 대리 문자 바로 뒤에는 하위 대리 문자 이스케이프가 반드시 와야 함
                if (m_text.substr(m_position, UNICODE_ESCAPE_PREFIX_LENGTH) != UNICODE_ESCAPE_PREFIX) ThrowFormatError();
                m_position += UNICODE_ESCAPE_PREFIX_LENGTH;

                const unsigned second{ ReadHex4() };
                if (second < LOW_SURROGATE_FIRST || second > LOW_SURROGATE_LAST) ThrowFormatError();

                return SURROGATE_PAIR_BASE
                    + ((first - HIGH_SURROGATE_FIRST) << SURROGATE_PAYLOAD_BITS)
                    + (second - LOW_SURROGATE_FIRST);
            }

            unsigned ReadHex4()
            {
                if (m_position + HEX_DIGIT_COUNT > m_text.size()) ThrowFormatError();

                const char* first{ m_text.data() + m_position };
                const char* last{ first + HEX_DIGIT_COUNT };
                unsigned value{};
                const auto [ptr, ec] { std::from_chars(first, last, value, HEX_RADIX) };
                if (ec != std::errc{} || ptr != last) ThrowFormatError();

                m_position += HEX_DIGIT_COUNT;
                return value;
            }

            double ParseNumber()
            {
                SkipWhitespace();
                const std::size_t start{ m_position };
                while (!IsRawEnd() && IsNumberChar(m_text[m_position])) ++m_position;

                const std::string_view token{ m_text.substr(start, m_position - start) };
                double value{};
                const auto [ptr, ec] { std::from_chars(token.data(), token.data() + token.size(), value) };
                if (token.empty() || ec != std::errc{} || ptr != token.data() + token.size()) ThrowFormatError();
                return value;
            }

            void ExpectLiteral(std::string_view literal)
            {
                SkipWhitespace();
                if (m_text.substr(m_position, literal.size()) != literal) ThrowFormatError();
                m_position += literal.size();
            }

            void ExpectChar(char expected)
            {
                if (PeekChar() != expected) ThrowFormatError();
                ++m_position;
            }

            // 공백을 건너뛴 다음 글자를 봄 (끝이면 END_OF_TEXT)
            char PeekChar()
            {
                SkipWhitespace();
                return IsRawEnd() ? END_OF_TEXT : m_text[m_position];
            }

            void SkipWhitespace() noexcept
            {
                while (!IsRawEnd() && IsJSONWhitespace(m_text[m_position])) ++m_position;
            }

            [[nodiscard]] bool IsRawEnd() const noexcept
            {
                return m_position >= m_text.size();
            }

            std::string_view m_text;
            std::size_t m_position{};
        };
    }

    JSONValue::JSONValue() noexcept = default;

    JSONValue JSONValue::CreateBool(bool value)
    {
        JSONValue result;
        result.m_type = JSONType::BOOL_VALUE;
        result.m_boolValue = value;
        return result;
    }

    JSONValue JSONValue::CreateNumber(double value)
    {
        JSONValue result;
        result.m_type = JSONType::NUMBER_VALUE;
        result.m_numberValue = value;
        return result;
    }

    JSONValue JSONValue::CreateString(std::string value)
    {
        JSONValue result;
        result.m_type = JSONType::STRING_VALUE;
        result.m_stringValue = std::move(value);
        return result;
    }

    JSONValue JSONValue::CreateArray(std::vector<JSONValue> items)
    {
        JSONValue result;
        result.m_type = JSONType::ARRAY_VALUE;
        result.m_items = std::move(items);
        return result;
    }

    JSONValue JSONValue::CreateObject(std::vector<std::string> keys, std::vector<JSONValue> values)
    {
        JSONValue result;
        result.m_type = JSONType::OBJECT_VALUE;
        result.m_keys = std::move(keys);
        result.m_items = std::move(values);
        return result;
    }

    JSONType JSONValue::GetType() const noexcept
    {
        return m_type;
    }

    bool JSONValue::GetBool() const noexcept
    {
        return m_type == JSONType::BOOL_VALUE && m_boolValue;
    }

    double JSONValue::GetNumber() const noexcept
    {
        return m_type == JSONType::NUMBER_VALUE ? m_numberValue : 0.0;
    }

    const std::string& JSONValue::GetString() const noexcept
    {
        static const std::string EMPTY_STRING{};
        return m_type == JSONType::STRING_VALUE ? m_stringValue : EMPTY_STRING;
    }

    const std::vector<JSONValue>& JSONValue::GetItems() const noexcept
    {
        static const std::vector<JSONValue> EMPTY_ITEMS{};
        return m_type == JSONType::ARRAY_VALUE ? m_items : EMPTY_ITEMS;
    }

    const std::vector<std::string>& JSONValue::GetKeys() const noexcept
    {
        static const std::vector<std::string> EMPTY_KEYS{};
        return m_type == JSONType::OBJECT_VALUE ? m_keys : EMPTY_KEYS;
    }

    const std::vector<JSONValue>& JSONValue::GetValues() const noexcept
    {
        static const std::vector<JSONValue> EMPTY_VALUES{};
        return m_type == JSONType::OBJECT_VALUE ? m_items : EMPTY_VALUES;
    }

    const JSONValue& JSONValue::FindMember(std::string_view key) const noexcept
    {
        static const JSONValue NULL_NODE{};
        if (m_type != JSONType::OBJECT_VALUE) return NULL_NODE;

        for (std::size_t index{}; index < m_keys.size(); ++index)
        {
            if (m_keys[index] == key) return m_items[index];
        }
        return NULL_NODE;
    }

    JSONValue ParseJSONText(std::string_view text)
    {
        JSONParser parser{ text };
        return parser.ParseDocument();
    }
}