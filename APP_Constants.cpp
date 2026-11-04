module;

#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>		//	SHGetKnownFolderPath, FOLDERID_LocalAppData

#pragma comment(lib, "shell32.lib")		//	SHGetKnownFolderPath
#pragma comment(lib, "ole32.lib")		//	CoTaskMemFree

module LostArkMultiTool.Core.APPConstants;

import std;

namespace LostArkMultiTool::Core::APPConstants
{
	namespace
	{
		//	윈도우가 지원하는 가장 긴 경로 길이 (긴 경로 기능 기준)
		inline constexpr std::size_t EXE_PATH_BUFFER_SIZE{ 32767 };

		//	SHGetKnownFolderPath가 돌려준 문자열을 자동 해제 (성공·실패 모두 CoTaskMemFree가 필요함)
		class CoTaskString
		{
		public:
			CoTaskString() noexcept = default;

			~CoTaskString()
			{
				::CoTaskMemFree(m_text);	//	nullptr이면 아무것도 하지 않음
			}

			CoTaskString(const CoTaskString&) = delete;
			CoTaskString& operator=(const CoTaskString&) = delete;

			[[nodiscard]] PWSTR* GetAddress() noexcept
			{
				return &m_text;
			}

			[[nodiscard]] PCWSTR Get() const noexcept
			{
				return m_text;
			}

		private:
			PWSTR m_text{ nullptr };
		};

		//	실행 파일 폴더를 실제로 읽음 (캐시가 비어 있을 때 딱 한 번만 호출됨)
		std::filesystem::path ReadExecutableDirectory()
		{
			std::wstring buffer(EXE_PATH_BUFFER_SIZE, L'\0');
			const DWORD length{ ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) };
			if (length == 0) throw std::runtime_error{ "실행 파일 경로를 가져오지 못했어요!" };

			buffer.resize(length);
			return std::filesystem::path{ buffer }.parent_path();
		}

		//	%LOCALAPPDATA% 폴더를 실제로 계산 (캐시가 비어 있을 때 한 번만 호출됨)
		std::filesystem::path ReadLocalAppDataDirectory()
		{
			CoTaskString localAppData;
			if (FAILED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, localAppData.GetAddress())))
			{
				throw std::runtime_error{ "사용자 데이터 폴더를 찾지 못했어요!" };
			}
			return std::filesystem::path{ localAppData.Get() };
		}

		//	UTF-8 이름 → 경로 (std::filesystem::path에 char 문자열을 바로 넣으면 ANSI 코드 페이지로 해석돼 한글이 깨짐)
		std::filesystem::path ToPath(std::string_view utf8Name)
		{
			if (utf8Name.empty()) return {};

			const int sourceLength{ static_cast<int>(utf8Name.size()) };
			const int wideLength{ ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8Name.data(), sourceLength, nullptr, 0) };
			if (wideLength <= 0) throw std::runtime_error{ "경로 이름을 변환하지 못했어요!" };

			std::wstring wideName(static_cast<std::size_t>(wideLength), L'\0');
			::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8Name.data(), sourceLength, wideName.data(), wideLength);
			return std::filesystem::path{ wideName };
		}

		//	%LOCALAPPDATA% 폴더 캐시 (함수 안 static은 처음 호출 때 한 번만, 스레드 안전하게 초기화됨)
		const std::filesystem::path& GetLocalAppDataDirectory()
		{
			static const std::filesystem::path LOCAL_APP_DATA_DIRECTORY{ ReadLocalAppDataDirectory() };
			return LOCAL_APP_DATA_DIRECTORY;
		}

		//	사용자 데이터 폴더(%LOCALAPPDATA%\LostArkMultiTool)를 실제로 계산 (캐시가 비어 있을 때 한 번만 호출됨)
		std::filesystem::path ReadUserDataDirectory()
		{
			return GetLocalAppDataDirectory() / ToPath(USER_DATA_FOLDER_NAME);
		}

		//	실행 파일 폴더 캐시 (함수 안 static은 처음 호출 때 한 번만, 스레드 안전하게 초기화됨)
		//	초기화 중 예외가 나면 캐시가 비어 있는 상태로 남아서 다음 호출 때 다시 시도함
		const std::filesystem::path& GetExecutableDirectory()
		{
			static const std::filesystem::path EXECUTABLE_DIRECTORY{ ReadExecutableDirectory() };
			return EXECUTABLE_DIRECTORY;
		}

		//	사용자 데이터 폴더 캐시 (위와 같은 방식)
		const std::filesystem::path& GetUserDataDirectory()
		{
			static const std::filesystem::path USER_DATA_DIRECTORY{ ReadUserDataDirectory() };
			return USER_DATA_DIRECTORY;
		}
	}

	std::filesystem::path GetDataFilePath(std::string_view fileName)
	{
		return GetExecutableDirectory() / ToPath(fileName);
	}

	std::filesystem::path GetUserDataFilePath(std::string_view fileName)
	{
		return GetUserDataDirectory() / ToPath(fileName);
	}

	std::filesystem::path GetLocalAppDataFilePath(std::string_view folderName, std::string_view fileName)
	{
		return GetLocalAppDataDirectory() / ToPath(folderName) / ToPath(fileName);
	}
}