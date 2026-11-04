export module LostArkMultiTool.Core.APPConstants;

import std;

//	프로그램 전체에서 쓰는 상수 모음 (코드에 숫자·문자열을 직접 적지 않기 위함)
export namespace LostArkMultiTool::Core::APPConstants
{
	//	프로그램 버전 (원격 설정의 LatestVersion과 비교 → 패치노트 버전을 올릴 때 같이 올릴 것)
	inline constexpr std::string_view APP_VERSION{ "0.5.0" };

	//	툴별 저장 파일 이름 (툴마다 파일을 따로 씀, 전체 경로는 GetDataFilePath가 실행 파일 위치 기준으로 계산)
	inline constexpr std::string_view ROSTER_DATA_FILE_NAME{ "LostArkRosterData.json" };	//	귀속재료 계산기 (원본 C#과 같은 파일)
	inline constexpr std::string_view BACKUP_FILE_SUFFIX{ ".bak" };
	inline constexpr std::string_view TEMP_FILE_SUFFIX{ ".tmp" };

	//	사용자 데이터 폴더 (%LOCALAPPDATA% 아래 → exe 위치가 바뀌어도 같은 폴더, 원본 보석·낙원력과 같은 방식)
	//	⚠️ USER_DATA_FOLDER_NAME · KEY_ENTROPY_TEXT는 한 번 정한 뒤 바꾸지 말 것 (저장된 API 키를 못 찾거나 못 풀게 됨)
	inline constexpr std::string_view USER_DATA_FOLDER_NAME{ "LostArkMultiTool" };
	inline constexpr std::string_view API_KEY_FILE_NAME{ "APIKey.dat" };		//	DPAPI로 암호화된 API 키 (세 툴 공용)
	inline constexpr std::string_view KEY_ENTROPY_TEXT{ "LostArkMultiTool" };	//	DPAPI 추가 암호화 보조값

	inline constexpr std::string_view DEFAULT_CHARACTER_NAME{ "본캐" };

	inline constexpr int SECONDS_PER_MINUTE{ 60 };
	inline constexpr int MS_PER_SECOND{ 1000 };
	inline constexpr int MS_PER_MINUTE{ SECONDS_PER_MINUTE * MS_PER_SECOND };

	inline constexpr int AUTO_REFRESH_MINUTES{ 5 };
	inline constexpr int AUTO_REFRESH_INTERVAL_MS{ AUTO_REFRESH_MINUTES * MS_PER_MINUTE };

	inline constexpr int API_TIMEOUT_SECONDS{ 15 };
	inline constexpr int CONFIG_TIMEOUT_SECONDS{ 5 };
	inline constexpr std::string_view DEFAULT_MAINTENANCE_MESSAGE{ "서버 점검중!, 점검이 완료되면 새 파일 다운로드 후에 실행하세요!" };

	//	실행 중 점검 스위치 확인 주기
	inline constexpr int CONFIG_CHECK_INTERVAL_MINUTES{ 30 };
	inline constexpr int CONFIG_CHECK_INTERVAL_MS{ CONFIG_CHECK_INTERVAL_MINUTES * MS_PER_MINUTE };

	//	상위 재료(결정·위대한·상급) 사용 기준 아이템 레벨
	inline constexpr double TOP_MATERIAL_MIN_LEVEL{ 1730.00 };

	//	표와 열 번호(1~7)와 같은 순서의 재료 키 (0번은 구분 열이라 비워 둠)
	inline constexpr std::size_t MAT_COLUMN_COUNT{ 8 };
	inline constexpr std::array<std::string_view, MAT_COLUMN_COUNT> MAT_KEYS
	{
		"", "Shard", "Dest", "Guard", "Leap", "Fusion", "Yong", "Bing"
	};

	//	실행 파일이 있는 폴더 기준 데이터 파일 전체 경로 (실행 파일 폴더는 처음 한 번만 계산해 캐싱)
	std::filesystem::path GetDataFilePath(std::string_view fileName);

	//	사용자 데이터 폴더 기준 파일 전체 경로 (폴더 위치는 처음 한 번만 계산해 캐싱, 폴더 생성은 저장하는 쪽에서)
	std::filesystem::path GetUserDataFilePath(std::string_view fileName);
}