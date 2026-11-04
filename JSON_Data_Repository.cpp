module LostArkMultiTool.Services.JSONDataRepository;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Models.Character;
import LostArkMultiTool.Models.ItemData;
import LostArkMultiTool.Models.UserData;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 저장 파일 키 이름 (원본 C# 저장 파일과 같아야 옛 기록을 그대로 읽음) =====
        constexpr std::string_view KEY_CHARACTERS{ "Characters" };
        constexpr std::string_view KEY_HISTORY{ "History" };
        constexpr std::string_view KEY_CLASS_NAME{ "ClassName" };
        constexpr std::string_view KEY_ITEM_LEVEL{ "ItemLevel" };
        constexpr std::string_view KEY_MATERIALS{ "Materials" };
        constexpr std::string_view KEY_TOP_QTY{ "TopQty" };
        constexpr std::string_view KEY_BOTTOM_QTY{ "BottomQty" };

        constexpr std::string_view STRUCTURE_ERROR_MESSAGE{ "저장 파일 구조가 올바르지 않습니다." };

        // ===== 쓰기 형식 =====
        constexpr unsigned CONTROL_CHAR_LIMIT{ 0x20 };
        constexpr std::size_t INDENT_WIDTH{ 2 };
        constexpr std::size_t ROOT_DEPTH{ 0 };
        constexpr std::size_t ROOT_MEMBER_DEPTH{ 1 };
        constexpr std::string_view HEX_DIGITS{ "0123456789abcdef" };
        constexpr unsigned NIBBLE_BITS{ 4 };
        constexpr unsigned NIBBLE_MASK{ 0xF };

        // ===== 읽기: JSONValue 트리 → 모델 (구조가 어긋나면 예외 → JSONFileStore가 손상으로 처리) =====
        [[noreturn]] void ThrowStructureError()
        {
            throw std::runtime_error{ std::string{ STRUCTURE_ERROR_MESSAGE } };
        }

        // 없는 키도 FindMember가 null을 돌려주므로 "없음"과 "null"은 같은 취급 (원본 C# 기본값과 같음)
        bool IsNullOrMissing(const JSONValue& value) noexcept
        {
            return value.GetType() == JSONType::NULL_VALUE;
        }

        void RequireType(const JSONValue& value, JSONType expected)
        {
            if (value.GetType() != expected) ThrowStructureError();
        }

        // 숫자 멤버 읽기 (없거나 null이면 기본값, 숫자가 아니면 구조 오류)
        double ReadNumberMember(const JSONValue& object, std::string_view key, double fallback)
        {
            const JSONValue& value{ object.FindMember(key) };
            if (IsNullOrMissing(value)) return fallback;

            RequireType(value, JSONType::NUMBER_VALUE);
            return value.GetNumber();
        }

        std::optional<std::string> ReadNullableString(const JSONValue& value)
        {
            if (IsNullOrMissing(value)) return std::nullopt;

            RequireType(value, JSONType::STRING_VALUE);
            return value.GetString();
        }

        Models::ItemData ReadItem(const JSONValue& itemValue)
        {
            Models::ItemData item;
            if (IsNullOrMissing(itemValue)) return item;   // null 재료는 빈 값 (원본 EnsureMaterials와 같은 결과)

            RequireType(itemValue, JSONType::OBJECT_VALUE);
            item.SetTopQty(ReadNumberMember(itemValue, KEY_TOP_QTY, item.GetTopQty()));
            item.SetBottomQty(ReadNumberMember(itemValue, KEY_BOTTOM_QTY, item.GetBottomQty()));
            return item;
        }

        std::unordered_map<std::string, Models::ItemData> ReadMaterials(const JSONValue& materialsValue)
        {
            std::unordered_map<std::string, Models::ItemData> materials;
            if (IsNullOrMissing(materialsValue)) return materials;

            RequireType(materialsValue, JSONType::OBJECT_VALUE);
            for (const auto& [key, value] : std::views::zip(materialsValue.GetKeys(), materialsValue.GetValues()))
            {
                materials.insert_or_assign(key, ReadItem(value));
            }
            return materials;
        }

        Models::Character ReadCharacter(const JSONValue& characterValue)
        {
            Models::Character character;   // 생성자가 전체 재료 키를 채움
            if (IsNullOrMissing(characterValue)) return character;   // null 캐릭터는 새 캐릭터로 (원본과 같음)

            RequireType(characterValue, JSONType::OBJECT_VALUE);
            character.SetClassName(ReadNullableString(characterValue.FindMember(KEY_CLASS_NAME)));
            character.SetItemLevel(ReadNumberMember(characterValue, KEY_ITEM_LEVEL, character.GetItemLevel()));
            character.SetMaterials(ReadMaterials(characterValue.FindMember(KEY_MATERIALS)));   // 빠진 재료 키는 SetMaterials가 채움
            return character;
        }

        Models::UserData ReadUserData(const JSONValue& root)
        {
            RequireType(root, JSONType::OBJECT_VALUE);

            std::unordered_map<std::string, Models::Character> characters;
            const JSONValue& charactersValue{ root.FindMember(KEY_CHARACTERS) };
            if (!IsNullOrMissing(charactersValue))
            {
                RequireType(charactersValue, JSONType::OBJECT_VALUE);
                for (const auto& [name, value] : std::views::zip(charactersValue.GetKeys(), charactersValue.GetValues()))
                {
                    characters.insert_or_assign(name, ReadCharacter(value));
                }
            }

            std::map<std::string, double> history;
            const JSONValue& historyValue{ root.FindMember(KEY_HISTORY) };
            if (!IsNullOrMissing(historyValue))
            {
                RequireType(historyValue, JSONType::OBJECT_VALUE);
                for (const auto& [date, value] : std::views::zip(historyValue.GetKeys(), historyValue.GetValues()))
                {
                    RequireType(value, JSONType::NUMBER_VALUE);
                    history.insert_or_assign(date, value.GetNumber());
                }
            }

            Models::UserData data;
            data.SetCharacters(std::move(characters));
            data.SetHistory(std::move(history));
            return data;
        }

        // ===== 쓰기 =====
        void AppendIndent(std::string& out, std::size_t depth)
        {
            out.append(depth * INDENT_WIDTH, ' ');
        }

        void AppendQuoted(std::string& out, std::string_view text)
        {
            out.push_back('"');
            for (const char c : text)
            {
                switch (c)
                {
                    case '"':  out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < CONTROL_CHAR_LIMIT)
                        {
                            // \u00XX 형식을 직접 만듦 (std::format 없이 16진수 두 자리)
                            const auto value{ static_cast<unsigned char>(c) };
                            out += "\\u00";
                            out.push_back(HEX_DIGITS[static_cast<std::size_t>(value >> NIBBLE_BITS)]);
                            out.push_back(HEX_DIGITS[static_cast<std::size_t>(value & NIBBLE_MASK)]);
                        }
                        else
                        {
                            out.push_back(c);   // 한글 등 UTF-8 바이트는 그대로 씀 (원본과 같음)
                        }
                        break;
                }
            }
            out.push_back('"');
        }

        // 객체 멤버 하나 시작 (첫 멤버는 쉼표 없이)
        void AppendMemberPrefix(std::string& out, bool& isFirst, std::size_t depth)
        {
            out += isFirst ? "\n" : ",\n";
            isFirst = false;
            AppendIndent(out, depth);
        }

        // 객체 닫기 (객체 자신의 들여쓰기 depth 기준)
        void AppendMemberClose(std::string& out, bool isFirst, std::size_t depth)
        {
            if (!isFirst)
            {
                out.push_back('\n');
                AppendIndent(out, depth);
            }
            out.push_back('}');
        }

        std::string FormatNumber(double value)
        {
            return std::format("{}", value);
        }

        void AppendItem(std::string& out, const Models::ItemData& item, std::size_t depth)
        {
            out.push_back('{');
            bool isFirst{ true };

            AppendMemberPrefix(out, isFirst, depth + 1);
            AppendQuoted(out, KEY_TOP_QTY);
            out += ": " + FormatNumber(item.GetTopQty());

            AppendMemberPrefix(out, isFirst, depth + 1);
            AppendQuoted(out, KEY_BOTTOM_QTY);
            out += ": " + FormatNumber(item.GetBottomQty());

            AppendMemberClose(out, isFirst, depth);
        }

        void AppendMaterials(std::string& out, const Models::Character& character, std::size_t depth)
        {
            out.push_back('{');
            bool isFirst{ true };

            for (const std::string_view key : Core::APPConstants::MAT_KEYS)
            {
                if (key.empty()) continue;

                AppendMemberPrefix(out, isFirst, depth + 1);
                AppendQuoted(out, key);
                out += ": ";
                AppendItem(out, character.GetMaterial(key), depth + 1);
            }

            AppendMemberClose(out, isFirst, depth);
        }

        void AppendCharacter(std::string& out, const Models::Character& character, std::size_t depth)
        {
            out.push_back('{');
            bool isFirst{ true };

            AppendMemberPrefix(out, isFirst, depth + 1);
            AppendQuoted(out, KEY_CLASS_NAME);
            out += ": ";
            if (character.GetClassName().has_value()) AppendQuoted(out, *character.GetClassName());
            else out += "null";

            AppendMemberPrefix(out, isFirst, depth + 1);
            AppendQuoted(out, KEY_ITEM_LEVEL);
            out += ": " + FormatNumber(character.GetItemLevel());

            AppendMemberPrefix(out, isFirst, depth + 1);
            AppendQuoted(out, KEY_MATERIALS);
            out += ": ";
            AppendMaterials(out, character, depth + 1);

            AppendMemberClose(out, isFirst, depth);
        }

        void AppendCharacters(std::string& out, const Models::UserData& data, std::size_t depth)
        {
            const auto& characters{ data.GetCharacters() };

            // 저장 순서를 고정하려고 이름순 정렬
            std::vector<std::string_view> names;
            names.reserve(characters.size());
            for (const auto& entry : characters) names.push_back(entry.first);
            std::sort(names.begin(), names.end());

            out.push_back('{');
            bool isFirst{ true };
            for (const std::string_view name : names)
            {
                AppendMemberPrefix(out, isFirst, depth + 1);
                AppendQuoted(out, name);
                out += ": ";
                AppendCharacter(out, characters.at(std::string{ name }), depth + 1);
            }
            AppendMemberClose(out, isFirst, depth);
        }

        void AppendHistory(std::string& out, const Models::UserData& data, std::size_t depth)
        {
            out.push_back('{');
            bool isFirst{ true };
            for (const auto& entry : data.GetHistory())   // std::map이라 이미 날짜순
            {
                AppendMemberPrefix(out, isFirst, depth + 1);
                AppendQuoted(out, entry.first);
                out += ": " + FormatNumber(entry.second);
            }
            AppendMemberClose(out, isFirst, depth);
        }

        std::string BuildJSONText(const Models::UserData& data)
        {
            std::string out;
            out.push_back('{');
            bool isFirst{ true };

            AppendMemberPrefix(out, isFirst, ROOT_MEMBER_DEPTH);
            AppendQuoted(out, KEY_CHARACTERS);
            out += ": ";
            AppendCharacters(out, data, ROOT_MEMBER_DEPTH);

            AppendMemberPrefix(out, isFirst, ROOT_MEMBER_DEPTH);
            AppendQuoted(out, KEY_HISTORY);
            out += ": ";
            AppendHistory(out, data, ROOT_MEMBER_DEPTH);

            AppendMemberClose(out, isFirst, ROOT_DEPTH);
            out.push_back('\n');
            return out;
        }
    }

    JSONDataRepository::JSONDataRepository()
        : m_fileStore{ Core::APPConstants::ROSTER_DATA_FILE_NAME }
    {
        // 변환이 끝까지 성공한 파일만 반영 (중간에 실패하면 loaded는 그대로 → 백업 파일로 다시 시도)
        std::optional<Models::UserData> loaded;
        m_loadWarning = m_fileStore.Load([&loaded](const JSONValue& root) {
            Models::UserData data{ ReadUserData(root) };
            data.MigrateLegacyHistory(Core::LocalTime::GetLocalToday());
            loaded = std::move(data);
        });

        m_data = loaded.has_value() ? std::move(*loaded) : Models::UserData{};

        // 캐릭터가 하나도 없으면 기본 캐릭터 추가 (원본과 같음)
        if (m_data.GetCharacters().empty())
        {
            auto characters{ m_data.GetCharacters() };
            characters.try_emplace(std::string{ Core::APPConstants::DEFAULT_CHARACTER_NAME }, Models::Character{});
            m_data.SetCharacters(std::move(characters));
        }
    }

    Models::UserData& JSONDataRepository::GetData()
    {
        return m_data;
    }

    const Models::UserData& JSONDataRepository::GetData() const
    {
        return m_data;
    }

    const std::optional<std::string>& JSONDataRepository::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void JSONDataRepository::Save()
    {
        m_fileStore.Save(BuildJSONText(m_data));
    }
}