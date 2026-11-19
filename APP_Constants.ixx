export module LostArkMultiTool.Core.APPConstants;

import std;

//	프로그램 전체에서 쓰는 상수 모음 (코드에 숫자·문자열을 직접 적지 않기 위함)
export namespace LostArkMultiTool::Core::APPConstants
{
	//	프로그램 버전 (원격 설정의 LatestVersion과 비교 → 패치노트 버전을 올릴 때 같이 올릴 것)
	inline constexpr std::string_view APP_VERSION{ "1.6.6" };

	//	툴별 저장 파일 이름 (툴마다 파일을 따로 씀, 전체 경로는 GetUserDataFilePath가 사용자 데이터 폴더 기준으로 계산)
	//	예전 버전은 실행 파일 폴더에 저장했음 → 처음 실행할 때 한 번만 사용자 데이터 폴더로 가져옴 (예전 파일은 그대로 둠)
	inline constexpr std::string_view ROSTER_DATA_FILE_NAME{ "LostArkRosterData.json" };	//	귀속재료 계산기 (원본 C#과 같은 파일 이름)
	inline constexpr std::string_view GEM_DATA_FILE_NAME{ "GemFinderData.json" };			//	보석 최저가 검색기 (멀티툴 전용 새 파일)
	inline constexpr std::string_view PARADISE_DATA_FILE_NAME{ "ParadiseTrackerData.json" };	//	낙원력 트래커 (멀티툴 전용 새 파일)

	//	원본 낙원력 트래커의 기록 위치 (%LOCALAPPDATA%\낙원력 트래커) → 처음 실행할 때 한 번만 읽어서 가져옴 (원본은 건드리지 않음)
	inline constexpr std::string_view PARADISE_ORIGINAL_FOLDER_NAME{ "낙원력 트래커" };
	inline constexpr std::string_view PARADISE_ORIGINAL_FILE_NAME{ "RosterGrowthData_v2.json" };	//	시즌별 기록 (v2)
	inline constexpr std::string_view PARADISE_LEGACY_FILE_NAME{ "RosterGrowthData.json" };		//	이전 버전 기록 (주차 목록만, v1)
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

	//	로스트아크 하루가 바뀌는 시각 (일일 컨텐츠 초기화 오전 6시 → 그래프 · 일별 기록의 날짜 기준)
	inline constexpr int DAILY_RESET_HOUR{ 6 };

	//	상위 재료(결정·위대한·상급) 사용 기준 아이템 레벨
	inline constexpr double TOP_MATERIAL_MIN_LEVEL{ 1730.00 };

	//	표와 열 번호(1~7)와 같은 순서의 재료 키 (0번은 구분 열이라 비워 둠)
	inline constexpr std::size_t MAT_COLUMN_COUNT{ 8 };
	inline constexpr std::array<std::string_view, MAT_COLUMN_COUNT> MAT_KEYS
	{
		"", "Shard", "Dest", "Guard", "Leap", "Fusion", "Yong", "Bing"
	};

	//	===== 보석 최저가 검색기 =====
	//	경매장 검색 조건 (겁화 · 작열은 4티어 보석, 7 ~ 10레벨만 조회 → 호출 수 절약)
	inline constexpr int GEM_CATEGORY_CODE{ 210000 };		//	경매장 '보석' 카테고리
	inline constexpr int GEM_TIER_4{ 4 };
	inline constexpr int MIN_GEM_LEVEL{ 7 };
	inline constexpr int MAX_GEM_LEVEL{ 10 };
	inline constexpr std::int64_t MAX_TARGET_PRICE{ 100'000'000 };	//	목표가 입력 최대값

	//	경매장 통신
	inline constexpr int MAX_CONCURRENT_REQUESTS{ 3 };		//	동시에 보내는 요청 수 (호출 한도 보호)
	inline constexpr int MAX_SEARCH_PAGES{ 20 };			//	정확한 이름의 매물을 찾을 때까지 넘겨볼 최대 페이지
	inline constexpr int MAX_TRANSIENT_RETRIES{ 1 };		//	일시적 오류(네트워크 · 5xx) 재시도 횟수
	inline constexpr int TRANSIENT_RETRY_DELAY_MS{ 500 };	//	재시도 전 대기 시간
	inline constexpr std::size_t ERROR_DETAIL_MAX_LENGTH{ 120 };	//	오류 메시지에 붙일 서버 설명 최대 길이 (바이트)

	//	자동 갱신 주기 (감시 주기마다 확인하고, 전체 주기가 되면 그 회차를 전체 갱신으로 실행)
	inline constexpr std::size_t WATCH_INTERVAL_OPTION_COUNT{ 4 };
	inline constexpr std::array<int, WATCH_INTERVAL_OPTION_COUNT> WATCH_INTERVAL_SECONDS_OPTIONS{ 10, 15, 30, 60 };
	inline constexpr int DEFAULT_WATCH_INTERVAL_SECONDS{ 10 };
	inline constexpr std::size_t FULL_INTERVAL_OPTION_COUNT{ 3 };
	inline constexpr std::array<int, FULL_INTERVAL_OPTION_COUNT> FULL_INTERVAL_MINUTES_OPTIONS{ 1, 3, 5 };
	inline constexpr int DEFAULT_FULL_INTERVAL_MINUTES{ 1 };

	//	호출 한도 (공식 가이드: 분당 100회)
	inline constexpr int RATE_LIMIT_PER_MINUTE{ 100 };
	inline constexpr int RATE_LIMIT_WINDOW_SECONDS{ 60 };				//	Reset 헤더가 없을 때 가정하는 한도 구간 길이
	inline constexpr int RATE_LIMIT_SAFETY_MARGIN{ 10 };				//	갱신 후에도 이만큼은 남아 있어야 시작함
	inline constexpr int RATE_LIMIT_FALLBACK_COOLDOWN_SECONDS{ 60 };	//	429인데 초기화 시각을 모를 때 쉬는 시간
	inline constexpr int MAINTENANCE_RETRY_MINUTES{ 5 };				//	API 서버 점검(503) 중 자동 갱신 재시도 간격

	//	알림 · 기록
	inline constexpr int HISTORY_KEEP_DAYS{ 365 };			//	시세 기록 보관 기간
	inline constexpr std::size_t MAX_BALLOON_LINES{ 4 };	//	트레이 풍선 글자 수 제한(약 255자) 때문에 줄 수 제한

	//	===== 유각 시세 검색기 =====
	//	거래소 검색 조건 (유물 등급 각인서 전체를 현재 최저가 순으로 받아옴)
	inline constexpr int ENGRAVING_CATEGORY_CODE{ 40000 };				//	거래소 '각인서' 카테고리
	inline constexpr std::string_view RELIC_GRADE_NAME{ "유물" };			//	유물 등급 (= 유각)
	inline constexpr int MAX_MARKET_PAGES{ 20 };							//	거래소 목록을 넘겨볼 최대 페이지 (한 페이지 10개 → 최대 200개)

	//	===== 악세 검색기 =====
	//	경매장 검색 조건 (4티어 악세서리, 부위 · 등급은 화면에서 고름)
	inline constexpr int NECKLACE_CATEGORY_CODE{ 200010 };				//	경매장 '목걸이' 카테고리
	inline constexpr int EARRING_CATEGORY_CODE{ 200020 };				//	경매장 '귀걸이' 카테고리
	inline constexpr int RING_CATEGORY_CODE{ 200030 };					//	경매장 '반지' 카테고리
	inline constexpr int ACCESSORY_TIER{ 4 };
	inline constexpr std::string_view ANCIENT_GRADE_NAME{ "고대" };
	inline constexpr std::string_view POLISH_OPTION_KEYWORD{ "연마" };	//	검색 옵션 중 연마 효과 묶음을 찾을 때 쓰는 글자
	inline constexpr std::size_t MAX_ACCESSORY_OPTION_FILTERS{ 3 };		//	한 번에 거는 연마 효과 조건 수
	inline constexpr int ACCESSORY_BASE_OPTION_GROUP{ 1 };				//	경매장 검색 옵션 '장신구 기본 효과' 묶음 (EtcOptions.Value)
	inline constexpr int MAIN_STAT_OPTION_CODE{ 11 };					//	그 묶음의 '힘 / 민첩 / 지능' (EtcSubs.Value, 최소 수치로 검색)

	//	===== 강화책 검색기 =====
	//	거래소 '강화 재료' 카테고리에서 이름으로 검색 (야금술 = 무기, 재봉술 = 방어구)
	inline constexpr int REFINE_MATERIAL_CATEGORY_CODE{ 50000 };		//	거래소 '강화 재료' 카테고리
	inline constexpr std::string_view METALLURGY_KEYWORD{ "야금술" };	//	무기 강화책
	inline constexpr std::string_view TAILORING_KEYWORD{ "재봉술" };		//	방어구 강화책

	//	===== 젬 시세 검색기 =====
	//	거래소 '강화 재료 > 아크 그리드 재료'의 영웅 등급 젬 (강화 재료 카테고리에서 이름으로 검색)
	inline constexpr std::string_view ORDER_GEM_KEYWORD{ "질서의 젬" };
	inline constexpr std::string_view CHAOS_GEM_KEYWORD{ "혼돈의 젬" };
	inline constexpr std::string_view EPIC_GRADE_NAME{ "영웅" };

	//	귀속 젬 계산기 (젬 탭 아래 표, 원정대 전체 보유 수량을 한 벌로 저장)
	inline constexpr std::string_view BOUND_GEM_DATA_FILE_NAME{ "BoundGemData.json" };	//	멀티툴 전용 새 파일
	inline constexpr int MAX_BOUND_GEM_COUNT{ 99'999 };								//	수량 입력 최대값

	//	===== 재련 시뮬레이터 =====
	inline constexpr std::string_view REFINE_SIMULATOR_DATA_FILE_NAME{ "RefineSimulatorData.json" };	//	시작 · 목표 · 재료 가격 · 귀속 설정 (멀티툴 전용 새 파일)

	//	===== 전투력 시뮬레이터 =====
	inline constexpr std::string_view COMBAT_SIMULATOR_DATA_FILE_NAME{ "CombatSimulatorData.json" };	//	마지막 캐릭터 이름 · 목표 재련 · 품질 · 보석 (멀티툴 전용 새 파일)

	//	실행 파일이 있는 폴더 기준 데이터 파일 전체 경로 (실행 파일 폴더는 처음 한 번만 계산해 캐싱, 예전 저장 위치를 찾을 때 사용)
	std::filesystem::path GetDataFilePath(std::string_view fileName);

	//	사용자 데이터 폴더 기준 파일 전체 경로 (폴더 위치는 처음 한 번만 계산해 캐싱, 폴더 생성은 저장하는 쪽에서)
	std::filesystem::path GetUserDataFilePath(std::string_view fileName);

	//	%LOCALAPPDATA%\폴더\파일 전체 경로 (다른 프로그램의 저장 파일을 찾을 때 사용, 이름은 UTF-8)
	std::filesystem::path GetLocalAppDataFilePath(std::string_view folderName, std::string_view fileName);
}