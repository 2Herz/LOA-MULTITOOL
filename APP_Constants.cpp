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

		//	사용자 데이터 폴더(%LOCALAPPDATA%\LostArkMultiTool)를 실제로 계산 (캐시가 비어 있을 때 한 번만 호출됨)
		std::filesystem::path ReadUserDataDirectory()
		{
			CoTaskString localAppData;
			if (FAILED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, localAppData.GetAddress())))
			{
				throw std::runtime_error{ "사용자 데이터 폴더를 찾지 못했어요!" };
			}
			return std::filesystem::path{ localAppData.Get() } / std::filesystem::path{ USER_DATA_FOLDER_NAME };
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
		return GetExecutableDirectory() / std::filesystem::path{ fileName };
	}

	std::filesystem::path GetUserDataFilePath(std::string_view fileName)
	{
		return GetUserDataDirectory() / std::filesystem::path{ fileName };
	}
}