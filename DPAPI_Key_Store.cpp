module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOUSER   // winuser.h 제외: GetClassName 같은 매크로 충돌 방지
#include <Windows.h>
#include <wincrypt.h>   // DATA_BLOB (WIN32_LEAN_AND_MEAN이 빼므로 직접 포함)
#include <dpapi.h>      // CryptProtectData, CryptUnprotectData

#pragma comment(lib, "crypt32.lib")

module LostArkMultiTool.Services.DPAPIKeyStore;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr const wchar_t* KEY_DESCRIPTION{ L"LostArkMultiTool API Key" };   // 암호화 데이터에 함께 저장되는 설명

        // DPAPI가 LocalAlloc으로 만들어 준 출력 버퍼를 자동 해제 (해제 전에 내용을 지워 키가 메모리에 남지 않게 함)
        class LocalBlob
        {
        public:
            LocalBlob() noexcept = default;

            ~LocalBlob()
            {
                if (m_blob.pbData == nullptr) return;

                ::SecureZeroMemory(m_blob.pbData, m_blob.cbData);
                ::LocalFree(m_blob.pbData);
            }

            LocalBlob(const LocalBlob&) = delete;
            LocalBlob& operator=(const LocalBlob&) = delete;

            [[nodiscard]] DATA_BLOB* GetAddress() noexcept
            {
                return &m_blob;
            }

            [[nodiscard]] std::string_view GetBytes() const noexcept
            {
                return { reinterpret_cast<const char*>(m_blob.pbData), m_blob.cbData };
            }

        private:
            DATA_BLOB m_blob{};
        };

        // 입력 바이트를 DATA_BLOB으로 (DPAPI는 입력을 읽기만 하지만 선언이 non-const라서 const_cast 필요)
        DATA_BLOB MakeInputBlob(std::string_view bytes) noexcept
        {
            return DATA_BLOB{ static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE*>(const_cast<char*>(bytes.data())) };
        }

        void WriteBinaryFile(const std::filesystem::path& path, std::string_view bytes)
        {
            std::ofstream out{ path, std::ios::binary | std::ios::trunc };
            if (!out) throw std::runtime_error{ "API 키 파일을 열지 못했어요!" };

            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            out.close();
            if (out.fail()) throw std::runtime_error{ "API 키 파일 쓰기에 실패했어요!" };
        }
    }

    DPAPIKeyStore::DPAPIKeyStore()
        : m_keyPath{ Core::APPConstants::GetUserDataFilePath(Core::APPConstants::API_KEY_FILE_NAME) }
    {}

    std::optional<std::string> DPAPIKeyStore::Load() const
    {
        std::error_code errorCode;
        if (!std::filesystem::exists(m_keyPath, errorCode)) return std::nullopt;   // 첫 실행

        std::ifstream in{ m_keyPath, std::ios::binary };
        if (!in) return std::nullopt;   // 읽을 수 없음 · 권한 문제 → 종료하지 않고 다시 입력받음

        const std::string encrypted{ std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
        DATA_BLOB input{ MakeInputBlob(encrypted) };
        DATA_BLOB entropy{ MakeInputBlob(Core::APPConstants::KEY_ENTROPY_TEXT) };
        LocalBlob plain;

        // 다른 PC·계정에서 만든 파일이거나 깨진 파일이면 복호화 실패 → 다시 입력받음 (원본과 같음)
        if (!::CryptUnprotectData(&input, nullptr, &entropy, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, plain.GetAddress()))
        {
            return std::nullopt;
        }
        return std::string{ plain.GetBytes() };
    }

    void DPAPIKeyStore::Save(std::string_view apiKey)
    {
        DATA_BLOB input{ MakeInputBlob(apiKey) };
        DATA_BLOB entropy{ MakeInputBlob(Core::APPConstants::KEY_ENTROPY_TEXT) };
        LocalBlob encrypted;

        if (!::CryptProtectData(&input, KEY_DESCRIPTION, &entropy, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, encrypted.GetAddress()))
        {
            throw std::runtime_error{ std::format("API 키 암호화에 실패했어요! (코드 {})", ::GetLastError()) };
        }

        std::filesystem::create_directories(m_keyPath.parent_path());   // 폴더가 없으면 생성

        // 임시 파일에 먼저 쓰고 한 번에 교체 → 중간에 종료돼도 기존 키 파일이 깨지지 않음 (원본 보석 검색기와 같음)
        std::filesystem::path tempPath{ m_keyPath };
        tempPath += Core::APPConstants::TEMP_FILE_SUFFIX;

        WriteBinaryFile(tempPath, encrypted.GetBytes());
        std::filesystem::rename(tempPath, m_keyPath);   // 기존 키 파일이 있으면 교체됨
    }
}