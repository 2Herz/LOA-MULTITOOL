module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOUSER   // winuser.h 제외: GetClassName 같은 매크로 충돌 방지
#include <Windows.h>

module LostArkMultiTool.Services.JSONFileStore;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr std::string_view UTF8_BOM{ "\xEF\xBB\xBF" };
        constexpr std::string_view CORRUPT_FILE_SUFFIX{ ".corrupt_" };

        // 파일 읽기 결과 상태
        enum class LoadStatus
        {
            NOT_FOUND,
            LOADED,
            CORRUPTED
        };

        // 경로 뒤에 접미사를 붙인 새 경로 ("...json" → "...json.bak")
        std::filesystem::path AppendSuffix(const std::filesystem::path& path, std::string_view suffix)
        {
            std::filesystem::path result{ path };
            result += suffix;
            return result;
        }

        // 파일 전체를 문자열로 읽음 (UTF-8 BOM은 제거)
        std::string ReadTextFile(const std::filesystem::path& path)
        {
            std::ifstream in{ path, std::ios::binary };
            if (!in) throw std::runtime_error{ "저장 파일을 열지 못했습니다." };

            std::string text{ std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
            if (std::string_view{ text }.starts_with(UTF8_BOM)) text.erase(0, UTF8_BOM.size());
            return text;
        }

        void WriteTextFile(const std::filesystem::path& path, const std::string& text)
        {
            std::ofstream out{ path, std::ios::binary | std::ios::trunc };
            if (!out) throw std::runtime_error{ "저장 파일을 열지 못했습니다." };

            out << text;
            out.close();
            if (out.fail()) throw std::runtime_error{ "저장 파일 쓰기에 실패했습니다." };
        }

        // 파일 읽기: 없음 / 성공 / 손상 구분 (파싱이나 onLoaded 변환이 실패하면 예외 없이 CORRUPTED)
        LoadStatus TryLoad(const std::filesystem::path& path, const std::function<void(const JSONValue&)>& onLoaded)
        {
            std::error_code errorCode;
            if (!std::filesystem::exists(path, errorCode)) return LoadStatus::NOT_FOUND;

            try
            {
                onLoaded(ParseJSONText(ReadTextFile(path)));
                return LoadStatus::LOADED;
            }
            catch (const std::exception&)
            {
                return LoadStatus::CORRUPTED;
            }
        }

        // 손상본은 별도 이름으로 보관하고, 본 파일 자리는 비움 → 보관 위치(또는 실패 사유) 반환
        std::string PreserveCorruptFile(const std::filesystem::path& savePath)
        {
            std::filesystem::path corruptPath{ AppendSuffix(savePath, CORRUPT_FILE_SUFFIX) };
            corruptPath += Core::LocalTime::MakeTimestamp();

            try
            {
                std::filesystem::rename(savePath, corruptPath);
                return Core::TextEncoding::WideToUTF8(corruptPath.wstring());   // 한글 경로가 깨지지 않도록 UTF-16 → UTF-8
            }
            catch (const std::filesystem::filesystem_error& error)
            {
                return std::format("(보관 실패: {})", error.what());
            }
        }
    }

    JSONFileStore::JSONFileStore(std::string_view fileName)
        : m_savePath{ Core::APPConstants::GetDataFilePath(fileName) }
        , m_backupPath{ AppendSuffix(m_savePath, Core::APPConstants::BACKUP_FILE_SUFFIX) }
        , m_tempPath{ AppendSuffix(m_savePath, Core::APPConstants::TEMP_FILE_SUFFIX) }
    {}

    std::optional<std::string> JSONFileStore::Load(const std::function<void(const JSONValue&)>& onLoaded)
    {
        // 파일이 없거나 정상이면 안내할 것이 없음
        if (TryLoad(m_savePath, onLoaded) != LoadStatus::CORRUPTED) return std::nullopt;

        // 손상본을 먼저 치워야 다음 저장 때 .bak이 손상본으로 덮어써지지 않음
        const std::string corruptLocation{ PreserveCorruptFile(m_savePath) };
        const bool isRecovered{ TryLoad(m_backupPath, onLoaded) == LoadStatus::LOADED };

        return isRecovered
            ? std::format("저장 파일이 손상되어 백업에서 복구했습니다.\n손상된 원본: {}", corruptLocation)
            : std::format("저장 파일이 손상되어 새로 시작합니다.\n손상된 원본: {}", corruptLocation);
    }

    void JSONFileStore::Save(const std::string& jsonText)
    {
        // 1) 임시 파일에 먼저 전부 씀 (여기서 실패하면 기존 저장본은 그대로)
        WriteTextFile(m_tempPath, jsonText);

        // 2) 처음 저장이면 그냥 이름 변경
        if (!std::filesystem::exists(m_savePath))
        {
            std::filesystem::rename(m_tempPath, m_savePath);
            return;
        }

        // 3) 기존 저장본은 .bak으로 보관하면서 원자적으로 교체 (원본 File.Replace와 같은 역할)
        if (!::ReplaceFileW(m_savePath.c_str(), m_tempPath.c_str(), m_backupPath.c_str(), 0, nullptr, nullptr))
        {
            // 교체가 막히면 복사로 대신하고 임시 파일 정리 (원본 IOException 대응)
            std::filesystem::copy_file(m_tempPath, m_savePath, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(m_tempPath);
        }
    }
}