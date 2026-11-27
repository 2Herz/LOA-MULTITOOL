module LostArkMultiTool.Views.CalculatorManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.AssetCalculator;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.DataRepository;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.Character;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.ItemData;
import LostArkMultiTool.Models.PriceData;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Models.UserData;
import LostArkMultiTool.Services.AssetCalculationService;
import LostArkMultiTool.Services.JSONDataRepository;
import LostArkMultiTool.Views.CalculatorView;
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.TrackerManager;          // 낙원력을 기록할 트래커
import LostArkMultiTool.Views.ArkGemFinderManager;     // 귀속 젬 계산기 (화면 인식 젬 값)
import LostArkMultiTool.Models.BoundGemData;           // 젬 · 등급 이름
import LostArkMultiTool.Vision.GemTooltipDetector;     // 귀속 젬 툴팁 인식기
import LostArkMultiTool.Vision.MaterialSlotDetector;   // 창고 · 소지품 슬롯 인식기
import LostArkMultiTool.Vision.CharacterDetector;      // 현재 캐릭터 인식 결과 이름
import LostArkMultiTool.Vision.NameplateMemory;        // 이름표 모양 기억
import LostArkMultiTool.Vision.OCRTextParser;          // 인식 값 이름 해석 ("Dest/top")
import LostArkMultiTool.Vision.ParadiseDetector;       // 낙원력 인식기
import LostArkMultiTool.Vision.ScreenWatcher;          // 화면 인식 감시기
import LostArkMultiTool.Vision.ShardBarDetector;       // 상단바 파편 인식기
import LostArkMultiTool.Vision.TooltipDetector;        // 재료 툴팁 인식기
import LostArkMultiTool.Vision.VisionSettings;         // 화면 인식 켜기 설정

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"귀속재료 계산기" };

        constexpr std::size_t FIRST_CHARACTER_INDEX{ 0 };    // 캐릭터 선택 0번 = 레벨이 가장 높은 캐릭터
        constexpr std::size_t FIRST_MATERIAL_INDEX{ 1 };     // MAT_KEYS 0번은 구분 열
        constexpr int AMOUNT_DECIMALS{ 0 };

        // ★ 원정대 자산 ★ 행에서 상위 · 하위를 나누지 않고 총합만 보여 줄 재료 (운명의 파편 · 용암의 숨결 · 빙하의 숨결 = 상위 · 하위가 같은 아이템)
        constexpr std::size_t SUM_ONLY_MATERIAL_COUNT{ 3 };   // 총합만 표시할 재료 수
        constexpr std::array<std::string_view, SUM_ONLY_MATERIAL_COUNT> SUM_ONLY_MATERIAL_KEYS{ "Shard", "Yong", "Bing" };

        // ===== 문구 =====
        constexpr std::wstring_view CHART_TITLE_TEXT{ L"날짜별 원정대 총 자산 (골드)" };   // 총자산 숫자는 카드 머리말 오른쪽에 따로 표시
        constexpr std::wstring_view SYNC_RUNNING_TEXT{ L"원정대 불러오는 중..." };          // 동기화 진행 상태
        constexpr std::wstring_view SYNC_DONE_FORMAT{ L"마지막 동기화 {}" };                 // 동기화 성공 시각
        constexpr std::wstring_view SYNC_FAILED_TEXT{ L"동기화 실패" };                      // 동기화 실패 상태
        constexpr std::wstring_view RESET_TITLE{ L"입력 초기화" };
        constexpr std::wstring_view RESET_QUESTION_FORMAT{ L"{}의 보유 수량(상위 · 하위)을 모두 0으로 되돌릴까요?\n입력한 수량이 지워져요." };
        constexpr std::wstring_view RESET_DONE_TEXT{ L"입력한 수량을 초기화했어요." };
        constexpr std::wstring_view RESET_NO_CHARACTER_TEXT{ L"먼저 캐릭터를 골라 주세요." };
        constexpr std::wstring_view CHART_EMPTY_TEXT{ L"아직 자산 기록이 없어요 · [시세 갱신] 후 기록돼요" };
        constexpr std::wstring_view TRAY_TOOLTIP_FORMAT{ L"원정대 자산: {} 골드" };
        constexpr std::wstring_view BACKGROUND_STATUS_FORMAT{ L"{}분마다 시세를 자동으로 갱신해요." };

        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };   // 손상 복구 · 이전 위치 기록 가져오기
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"API 호출 한도가 부족해서 이번 시세 갱신은 건너뛰었어요.\n{}쯤 다시 시도해 주세요." };
        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"저장에 실패했어요.\n" };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view REFRESH_DONE_TEXT{ L"시세 동기화 완료!" };
        constexpr std::wstring_view AUTO_REFRESH_FAILED_TEXT{ L"시세 자동 갱신 실패: " };
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };
        constexpr std::wstring_view ROSTER_NOT_FOUND_TEXT{ L"캐릭터 정보를 찾을 수 없어요." };
        constexpr std::wstring_view ROSTER_ERROR_TEXT{ L"오류 발생: " };
        constexpr std::wstring_view ROSTER_DONE_FORMAT{ L"총 {}개의 캐릭터 연동 완료!" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" \u3000" };   // 일반 공백 · 전각 공백

        // ===== 화면 인식 안내 =====
        constexpr std::wstring_view VISION_ON_TEXT{ L"화면 인식을 켰어요. 게임에서 재료 툴팁 · 상단바 파편 · 현재 낙원력 위에 마우스를 잠깐 올려 두면, 지금 접속한 캐릭터에 반영돼요." };
        constexpr std::wstring_view VISION_WAIT_CHARACTER_TEXT{ L"화면 인식: 지금 접속한 캐릭터를 확인하는 중이라 반영하지 않았어요. 머리 위 이름이 보이게 하거나 캐릭터 정보 창(P)을 한 번 열어 주세요." };
        constexpr std::wstring_view CURRENT_CHARACTER_FORMAT{ L"현재 캐릭터: {}" };   // 캐릭터를 확인했을 때 안내
        constexpr std::wstring_view MANUAL_CHARACTER_FORMAT{ L"현재 캐릭터를 {}(으)로 바꿨어요." };   // [변경]으로 골랐을 때
        constexpr std::wstring_view ALIAS_SAVED_FORMAT{ L"'{}'(으)로 읽히는 이름을 {}(으)로 기억했어요. 다음부터는 바로 알아봐요." };   // 별칭 기억
        constexpr std::wstring_view SHAPE_SAVED_FORMAT{ L"{}의 머리 위 이름표 모양을 기억했어요. 다음부터는 바로 알아봐요." };   // 이름표 모양 기억
        constexpr std::wstring_view REFRESH_NO_CHARACTER_TEXT{ L"현재 캐릭터를 아직 모르겠어요. 먼저 [변경]으로 골라 주세요." };       // [갱신] 실패
        constexpr std::chrono::seconds ALIAS_CANDIDATE_LIFETIME{ 120 };   // 못 맞춘 이름을 별칭 후보로 쓰는 시간 (정보 창을 본 뒤 이 안에 고르면 기억)
        constexpr double ITEM_LEVEL_TOLERANCE{ 0.005 };                   // 아이템 레벨이 이만큼 넘게 다르면 [갱신] 필요
        constexpr std::wstring_view VISION_OFF_TEXT{ L"화면 인식을 껐어요." };
        constexpr std::wstring_view VISION_NO_CHARACTER_TEXT{ L"화면 인식: 먼저 캐릭터를 골라 주세요." };
        constexpr std::wstring_view VISION_SAVE_FAILED_TEXT{ L"화면 인식 설정을 저장하지 못했어요." };
        constexpr std::wstring_view SCAN_MATERIAL_FORMAT{ L"{} · {}{} {}개 반영" };          // 캐릭터 · 재료 · (상/하) · 수량
        constexpr std::wstring_view SCAN_PARADISE_FORMAT{ L"{} · 낙원력 {} 기록" };           // 캐릭터 · 낙원력
        constexpr std::wstring_view SCAN_GEM_FORMAT{ L"{} · {} ({}) {}개 반영 ({})" };         // 캐릭터 · 젬 · 등급 · 합계 · 읽은 종류
        constexpr std::array<std::wstring_view, 3> GEM_KIND_LABELS{ L"귀속", L"거래 가능", L"가공 젬 +1" };   // GemStockKind 순서
        constexpr std::wstring_view SCAN_GEM_NO_CHARACTER_FORMAT{ L"귀속 젬 계산기에 '{}' 캐릭터가 없어요. 젬 시세 검색기에서 원정대 동기화를 먼저 해 주세요." };
        constexpr std::wstring_view SCAN_GEM_SAVE_FAILED_TEXT{ L"귀속 젬 수량을 저장하지 못했어요." };
        constexpr std::wstring_view SCAN_PARADISE_NO_CHARACTER_FORMAT{ L"낙원력 트래커에 '{}' 캐릭터가 없어서 기록하지 못했어요." };
        constexpr std::wstring_view SCAN_PARADISE_NO_WEEK_TEXT{ L"낙원력 트래커에 이번 주차가 없어서 기록하지 못했어요." };
        constexpr std::wstring_view SCAN_PARADISE_DECREASED_FORMAT{ L"읽은 낙원력 {}이 기록보다 작아서 건너뛰었어요." };
        constexpr std::wstring_view TIER_TOP_LABEL{ L"(상)" };        // 상위 재료 표시
        constexpr std::wstring_view TIER_BOTTOM_LABEL{ L"(하)" };     // 하위 재료 표시
        constexpr int SCAN_NUMBER_DECIMALS{ 0 };                       // 안내 숫자 소수 자리
        constexpr std::array<std::wstring_view, Core::APPConstants::MAT_COLUMN_COUNT> MATERIAL_LABELS{
            L"", L"운명의 파편", L"파괴석", L"수호석", L"돌파석", L"융화 재료", L"용암의 숨결", L"빙하의 숨결" };   // MAT_KEYS 순서의 안내 이름

        // 예외 메시지(UTF-8) → 화면 문구 (변환할 수 없으면 기본 문구)
        [[nodiscard]] std::wstring ToWideMessage(const char* message)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(message);
            }
            catch (const std::exception&)
            {
                return std::wstring{ UNKNOWN_ERROR_TEXT };
            }
        }

        [[nodiscard]] std::wstring_view TrimSpaces(std::wstring_view text) noexcept
        {
            const std::size_t first{ text.find_first_not_of(TRIM_CHARACTERS) };
            if (first == std::wstring_view::npos) return {};

            const std::size_t last{ text.find_last_not_of(TRIM_CHARACTERS) };
            return text.substr(first, last - first + 1);
        }

        // 캐릭터 선택에 보일 글자: "이름 (직업, Lv.1740.83)" (원본 GetDisplayName)
        [[nodiscard]] std::wstring MakeDisplayName(const std::string& name, const Models::Character& character)
        {
            const std::wstring wideName{ Core::TextEncoding::UTF8ToWide(name) };
            const std::optional<std::string>& className{ character.GetClassName() };
            if (!className.has_value() || className->empty()) return wideName;

            return std::format(L"{} ({}, Lv.{})", wideName, Core::TextEncoding::UTF8ToWide(*className), character.GetItemLevel());
        }

        // 저장 키 "yyyy-MM-dd" → 그래프 글자 "MM/dd" (형식이 다르면 그대로)
        [[nodiscard]] std::wstring ToChartDateLabel(const std::string& historyKey)
        {
            if (const auto date{ Models::UserData::ParseHistoryKey(historyKey) }; date.has_value())
            {
                return std::format(L"{:02}/{:02}", static_cast<unsigned>(date->month()), static_cast<unsigned>(date->day()));
            }
            return Core::TextEncoding::UTF8ToWide(historyKey);
        }
    }

    CalculatorManager::CalculatorManager() = default;

    CalculatorManager::~CalculatorManager() = default;

    std::wstring_view CalculatorManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void CalculatorManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 저장 파일(LostArkRosterData.json) → 시세 · 계산
        m_repository = std::make_unique<Services::JSONDataRepository>();
        m_calculator = std::make_unique<Services::AssetCalculationService>(*m_repository);
        m_calculator->AddAssetCalculatedListener([this] { RefreshView(); });

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<CalculatorView>(context.m_platform);
        m_view->SetOnSyncRoster([this](const std::wstring& mainCharacterName) { SyncRoster(mainCharacterName); });
        m_view->SetOnRefresh([this] { StartRefresh(false); });
        m_view->SetOnAutoRefreshChanged([this](bool isEnabled) { SetAutoRefresh(isEnabled); });
        m_view->SetOnCharacterSelected([this] { SelectCharacter(); });
        m_view->SetOnResetInput([this] { ResetInput(); });
        m_view->SetOnQuantityEdited([this](MaterialTier tier, std::size_t materialIndex, double quantity) { EditQuantity(tier, materialIndex, quantity); });
        m_view->SetOnVisionChanged([this](bool isEnabled) { SetVision(isEnabled); });

        // 화면 인식: 지난번에 켜 두었으면 바로 시작
        m_visionSettings = std::make_unique<Vision::VisionSettings>();
        m_nameplateMemory = std::make_shared<Vision::NameplateMemory>();   // 이름표 모양 기억 (꺼져 있어도 설정 탭에서 개수 · 초기화)
        m_view->SetVisionChecked(m_visionSettings->IsEnabled());
        if (m_visionSettings->IsEnabled()) StartVision();

        // 저장 파일이 손상돼 복구했으면 알림 (원본 Form1_Load)
        if (const std::optional<std::string>& warning{ m_repository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(warning->c_str()), Core::NoticeKind::NOTICE_WARNING, nullptr);
        }

        LoadCharacterList();
        RefreshView();
    }

    void CalculatorManager::Shutdown()
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_autoRefreshTimer.Stop();
        m_isAutoRefreshEnabled = false;
        m_refreshTask.Wait();
        m_rosterTask.Wait();
        m_screenWatcher.reset();     // 감시 스레드를 멈추고 기다림 (저장소를 지우기 전에)
        m_visionSettings.reset();
        m_nameplateMemory.reset();   // 감시 스레드가 끝난 뒤 놓음

        m_view.reset();
        m_calculator.reset();
        m_repository.reset();
        m_context = nullptr;
    }

    void CalculatorManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void CalculatorManager::OnDeactivated()
    {
        if (m_view) m_view->SuspendFocus();   // 편집 중인 칸 확정 · IME 끔
    }

    void CalculatorManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);

        // 백그라운드 작업이 끝났으면 결과 반영 (다른 탭을 보거나 트레이에 숨어 있어도 진행)
        if (m_refreshTask.IsReady()) FinishRefresh();
        if (m_rosterTask.IsReady()) FinishRosterSync();
        if (m_autoRefreshTimer.Poll()) StartRefresh(true);
        if (m_screenWatcher) ApplyScanResults();   // 화면 인식 결과 반영
    }

    // 인게임 오버레이 재료 패널용 요약 (고른 캐릭터의 레벨에 맞는 재료 줄 + 캐릭터 · 원정대 자산)
    CalculatorOverlaySummary CalculatorManager::GetOverlaySummary() const
    {
        CalculatorOverlaySummary summary;                                                       // 결과
        if (!m_repository || !m_calculator) return summary;                                     // 초기화 전
        summary.m_hasPrices = m_calculator->HasPrices();                                        // 시세 여부
        summary.m_isVisionOn = m_screenWatcher != nullptr;                                      // 화면 인식 여부
        summary.m_rosterTotal = CalculateRosterTotal();                                         // 원정대 자산

        const std::optional<std::string> name{ GetSelectedCharacterName() };                    // 고른 캐릭터
        const Models::Character* character{ name.has_value() ? m_repository->GetData().FindCharacter(*name) : nullptr };
        if (character == nullptr) return summary;                                               // 캐릭터 없음
        summary.m_characterName = Core::TextEncoding::UTF8ToWide(*name);                        // 이름

        const bool isTop{ character->GetItemLevel() <= 0.0 || character->GetItemLevel() >= Core::APPConstants::TOP_MATERIAL_MIN_LEVEL };   // 레벨에 맞는 구분
        const auto prices{ isTop ? m_calculator->GetTopPrices() : m_calculator->GetBotPrices() };   // 단가 표
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)   // 재료마다
        {
            const std::string key{ Core::APPConstants::MAT_KEYS[index] };                      // 재료 키
            const Models::ItemData& item{ character->GetMaterial(key) };                        // 보유 정보
            CalculatorOverlayRow row;                                                           // 한 줄
            row.m_label = std::wstring{ MATERIAL_LABELS[index] };                               // 이름
            row.m_quantity = isTop ? item.GetTopQty() : item.GetBottomQty();                    // 수량
            row.m_value = item.GetCalculatedTopValue() + item.GetCalculatedBotValue();          // 재료값
            if (const auto price{ prices.find(key) }; price != prices.end())                    // 단가가 있으면
            {
                row.m_unitPrice = price->second.GetCurrent();                                   // 단가
                row.m_priceDiff = price->second.GetDiff();                                      // 변동
            }
            summary.m_characterTotal += row.m_value;                                            // 캐릭터 자산 누적
            summary.m_rows.push_back(std::move(row));                                           // 추가
        }
        return summary;   // 요약
    }

    // [화면 인식] 켜기 · 끄기 → 감시 시작 · 멈춤 + 설정 저장
    void CalculatorManager::SetVision(bool isEnabled)
    {
        if (!m_visionSettings) return;                                      // 초기화 전
        m_visionSettings->SetEnabled(isEnabled);                            // 설정 바꾸기
        try
        {
            m_visionSettings->Save();                                       // 저장
        }
        catch (const std::exception&)
        {
            m_context->m_notifier.ShowToast(std::wstring{ VISION_SAVE_FAILED_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 저장 실패 안내 (켜기 · 끄기는 그대로)
        }

        if (isEnabled) StartVision();                                       // 감시 시작
        else m_screenWatcher.reset();                                       // 감시 멈춤 (스레드 정리)
        ClearCurrentCharacter();                                            // 켜고 끌 때마다 현재 캐릭터는 다시 확인
        m_context->m_notifier.ShowToast(std::wstring{ isEnabled ? VISION_ON_TEXT : VISION_OFF_TEXT }, Core::NoticeKind::NOTICE_INFO);   // 안내
    }

    // 감시기를 만들고 인식기(툴팁 · 상단바 파편 · 낙원력)를 등록해 시작
    void CalculatorManager::StartVision()
    {
        if (m_screenWatcher) return;                                        // 이미 감시 중
        m_screenWatcher = std::make_unique<Vision::ScreenWatcher>();        // 감시기
        m_screenWatcher->AddDetector(std::make_unique<Vision::TooltipDetector>());    // 재료 툴팁
        m_screenWatcher->AddDetector(std::make_unique<Vision::ShardBarDetector>());   // 상단바 운명의 파편
        m_screenWatcher->AddDetector(std::make_unique<Vision::ParadiseDetector>());   // 현재 낙원력
        m_screenWatcher->AddDetector(std::make_unique<Vision::GemTooltipDetector>()); // 귀속 젬 툴팁
        m_screenWatcher->AddFrameDetector(std::make_unique<Vision::MaterialSlotDetector>());   // 창고 · 소지품 슬롯 (융화 재료 · 숨결, 마우스 없이)
        PushCharacterNames();                                               // 현재 캐릭터 비교용 원정대 이름 · 별칭
        m_screenWatcher->SetCharacterScanEnabled(m_visionSettings->IsCharacterAutoEnabled());   // 자동 인식을 껐으면 캐릭터 확인 안 함
        m_screenWatcher->SetCharacterInterval(std::chrono::seconds{ m_visionSettings->GetCharacterIntervalSeconds() });   // 확인 간격
        m_screenWatcher->SetPreviewEnabled(m_isPreviewWanted);              // 설정 탭 미리 보기
        m_screenWatcher->SetCombatScanEnabled(m_isCombatWanted);            // 딜 지분 패널 전투 분석기
        m_screenWatcher->Start();                                           // 감시 스레드 시작
    }

    // 감시기에 원정대 이름 넘기기 (원정대를 다시 불러오면 다시 부름)
    void CalculatorManager::PushCharacterNames()
    {
        if (!m_screenWatcher) return;                                       // 화면 인식이 꺼져 있음
        std::vector<std::wstring> names;                                    // 넓은 글자 이름들
        names.reserve(m_characterNames.size());                             // 자리 확보
        for (const std::string& name : m_characterNames) names.push_back(Core::TextEncoding::UTF8ToWide(name));   // UTF-8 → 넓은 글자
        std::vector<Vision::CharacterAlias> aliases;                        // 별칭 (넓은 글자)
        for (const Vision::SavedCharacterAlias& alias : m_visionSettings->GetCharacterAliases())   // 저장된 별칭마다
        {
            if (std::ranges::find(m_characterNames, alias.m_name) == m_characterNames.end()) continue;   // 원정대에 없는 캐릭터는 뺌
            aliases.push_back(Vision::CharacterAlias{ Core::TextEncoding::UTF8ToWide(alias.m_readText), Core::TextEncoding::UTF8ToWide(alias.m_name) });
        }
        m_screenWatcher->SetCharacterNames(std::move(names), std::move(aliases), m_nameplateMemory);   // 넘기기 (모양 기억은 공유)
    }

    // 현재 캐릭터 바꾸기 → 계산기 선택 · 오버레이 패널도 그 캐릭터로 (같은 캐릭터면 알아본 방법 · 읽은 레벨만 갱신)
    void CalculatorManager::SetCurrentCharacter(const std::string& name, CurrentCharacterSource source, std::optional<double> readItemLevel)
    {
        const bool isChanged{ m_currentCharacter != name };                 // 다른 캐릭터로 바뀌는지
        m_currentSource = source;                                           // 알아본 방법
        if (isChanged) m_readItemLevel = readItemLevel;                     // 새 캐릭터면 읽은 레벨도 새로
        else if (readItemLevel.has_value()) m_readItemLevel = readItemLevel;   // 같은 캐릭터면 새로 읽었을 때만 바꿈
        if (!isChanged) return;                                             // 같은 캐릭터
        m_currentCharacter = name;                                          // 기록
        m_isWaitNoticeShown = false;                                        // 다음 확인 대기 때 다시 안내
        SelectCharacterByName(name);                                        // 계산기에서도 고름
        if (source != CurrentCharacterSource::SOURCE_MANUAL)                // 자동으로 알아봤을 때만 (직접 고르면 따로 안내)
            m_context->m_notifier.ShowToast(std::format(CURRENT_CHARACTER_FORMAT, Core::TextEncoding::UTF8ToWide(name)), Core::NoticeKind::NOTICE_INFO);
    }

    void CalculatorManager::ClearCurrentCharacter()
    {
        m_currentCharacter.reset();                                         // 다시 확인
        m_readItemLevel.reset();                                            // 읽은 레벨
        m_isWaitNoticeShown = false;                                        // 안내 초기화
    }

    // [변경]: 후보 번호로 현재 캐릭터를 직접 고르기 (방금 정보 창에서 못 맞춘 이름이 있으면 그 글자를 이 캐릭터로 기억)
    void CalculatorManager::PickCurrentCharacter(std::size_t index)
    {
        if (index >= m_characterNames.size()) return;                       // 경계 검사
        const std::string name{ m_characterNames[index] };                  // 고른 캐릭터
        const bool isRecentUnknown{ m_lastUnknownName.has_value()
            && std::chrono::steady_clock::now() - m_lastUnknownAt <= ALIAS_CANDIDATE_LIFETIME };   // 방금 못 맞춘 이름
        if (isRecentUnknown && m_visionSettings)                            // 별칭으로 기억
        {
            m_visionSettings->AddCharacterAlias(*m_lastUnknownName, name);  // "읽은 글자 → 캐릭터"
            try
            {
                m_visionSettings->Save();                                   // 저장
            }
            catch (const std::exception&)
            {
                m_context->m_notifier.ShowToast(std::wstring{ VISION_SAVE_FAILED_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 저장 실패 안내
            }
            m_context->m_notifier.ShowToast(std::format(ALIAS_SAVED_FORMAT, Core::TextEncoding::UTF8ToWide(*m_lastUnknownName),
                Core::TextEncoding::UTF8ToWide(name)), Core::NoticeKind::NOTICE_SUCCESS);   // 안내
            m_lastUnknownName.reset();                                      // 한 번만
            PushCharacterNames();                                           // 감시기에 새 별칭
        }
        if (m_nameplateMemory && m_nameplateMemory->LearnPending(name, ALIAS_CANDIDATE_LIFETIME))   // 방금 본 내 이름표 모양을 이 캐릭터로 기억
            m_context->m_notifier.ShowToast(std::format(SHAPE_SAVED_FORMAT, Core::TextEncoding::UTF8ToWide(name)), Core::NoticeKind::NOTICE_SUCCESS);   // 안내
        SetCurrentCharacter(name, CurrentCharacterSource::SOURCE_MANUAL, std::nullopt);   // 직접 고름
        if (m_screenWatcher) m_screenWatcher->NotifyCharacterPicked();      // 이름표를 못 봐도 붙잡은 값을 내보내게
        m_context->m_notifier.ShowToast(std::format(MANUAL_CHARACTER_FORMAT, Core::TextEncoding::UTF8ToWide(name)), Core::NoticeKind::NOTICE_INFO);   // 안내
    }

    // [갱신]: 현재 캐릭터 이름으로 원정대 정보를 다시 받음 (아이템 레벨 · 직업 · 새 캐릭터)
    void CalculatorManager::RefreshCurrentCharacter()
    {
        if (!m_currentCharacter.has_value())                                // 현재 캐릭터를 모름
        {
            m_context->m_notifier.ShowToast(std::wstring{ REFRESH_NO_CHARACTER_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 안내
            return;                                                         // 끝
        }
        SyncRoster(Core::TextEncoding::UTF8ToWide(*m_currentCharacter));    // 원정대 동기화와 같은 흐름 (끝나면 목록 · 레벨 갱신)
    }

    // 이름으로 캐릭터 고르기 (이미 고른 캐릭터면 그대로)
    void CalculatorManager::SelectCharacterByName(const std::string& name)
    {
        const auto found{ std::ranges::find(m_characterNames, name) };     // 선택 목록에서 찾기
        if (found == m_characterNames.end()) return;                        // 목록에 없음
        const std::size_t index{ static_cast<std::size_t>(found - m_characterNames.begin()) };   // 선택 번호 = 이름 번호
        if (m_view->GetSelectedCharacterIndex() == index) return;           // 이미 고름
        m_view->SelectCharacterIndex(index);                                // 표시 바꾸기
        SelectCharacter();                                                  // 수량 표시 · 계산 (손으로 고를 때와 같은 흐름)
    }

    // 인식 값을 넣을 캐릭터 = 화면에서 확인한 현재 캐릭터 (모르면 다른 캐릭터에 잘못 넣지 않도록 건너뜀)
    std::optional<std::string> CalculatorManager::GetScanTarget()
    {
        if (!m_currentCharacter.has_value())                                // 아직 확인 못 함
        {
            if (!m_isWaitNoticeShown)                                       // 안내는 한 번만
            {
                m_context->m_notifier.ShowToast(std::wstring{ VISION_WAIT_CHARACTER_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 안내
                m_isWaitNoticeShown = true;                                 // 띄움
            }
            return std::nullopt;                                            // 넣지 않음
        }
        SelectCharacterByName(*m_currentCharacter);                         // 손으로 다른 캐릭터를 골라 뒀어도 현재 캐릭터로
        return m_currentCharacter;                                          // 현재 캐릭터
    }

    // 오버레이 현재 캐릭터 패널용 상태
    CurrentCharacterSummary CalculatorManager::GetCurrentCharacter() const
    {
        CurrentCharacterSummary summary;                                    // 결과 (기본: 화면 인식 꺼짐)
        if (!m_screenWatcher) return summary;                               // 꺼짐
        if (m_characterNames.empty()) { summary.m_state = CurrentCharacterState::CHARACTER_NO_ROSTER; return summary; }       // 원정대 없음
        summary.m_isAutoEnabled = m_visionSettings && m_visionSettings->IsCharacterAutoEnabled();   // 자동 인식
        summary.m_aliasCount = m_visionSettings ? m_visionSettings->GetCharacterAliases().size() : 0;   // 별칭 수
        summary.m_shapeCount = m_nameplateMemory ? m_nameplateMemory->GetCharacterCount() : 0;           // 모양을 기억한 캐릭터 수
        summary.m_isSyncing = m_rosterTask.IsRunning();                     // 원정대 갱신 중
        summary.m_candidates.reserve(m_characterNames.size());              // 자리 확보
        for (const std::string& name : m_characterNames) summary.m_candidates.push_back(Core::TextEncoding::UTF8ToWide(name));   // [변경] 후보
        if (!m_currentCharacter.has_value()) { summary.m_state = CurrentCharacterState::CHARACTER_CHECKING; return summary; }  // 확인 중
        summary.m_state = CurrentCharacterState::CHARACTER_KNOWN;           // 확인됨
        summary.m_source = m_currentSource;                                 // 알아본 방법
        summary.m_name = Core::TextEncoding::UTF8ToWide(*m_currentCharacter);   // 이름
        if (const auto found{ std::ranges::find(m_characterNames, *m_currentCharacter) }; found != m_characterNames.end())
            summary.m_currentIndex = static_cast<std::size_t>(found - m_characterNames.begin());   // 후보 중 번호
        summary.m_readItemLevel = m_readItemLevel;                          // 정보 창에서 읽은 레벨
        if (const Models::Character* character{ m_repository ? m_repository->GetData().FindCharacter(*m_currentCharacter) : nullptr }; character != nullptr)   // 저장된 캐릭터
        {
            if (const auto& className{ character->GetClassName() }; className.has_value()) summary.m_className = Core::TextEncoding::UTF8ToWide(*className);   // 직업
            summary.m_itemLevel = character->GetItemLevel();                // 레벨
            summary.m_isOutdated = m_readItemLevel.has_value() && std::abs(*m_readItemLevel - summary.m_itemLevel) > ITEM_LEVEL_TOLERANCE;   // 저장된 정보가 옛날 것
        }
        return summary;   // 상태
    }

    bool CalculatorManager::IsVisionEnabled() const noexcept
    {
        return m_screenWatcher != nullptr;   // 감시 중이면 켜짐
    }

    void CalculatorManager::SetVisionEnabled(bool isEnabled)
    {
        if (IsVisionEnabled() == isEnabled) return;   // 그대로
        SetVision(isEnabled);                         // 계산기 [화면 인식]과 같은 흐름 (설정 저장 · 안내)
        if (m_view) m_view->SetVisionChecked(isEnabled);   // 계산기 체크 상자도 맞춤
    }

    void CalculatorManager::RestartVision()
    {
        if (!m_screenWatcher) return;                 // 꺼져 있으면 할 일 없음
        m_screenWatcher.reset();                      // 감시 멈춤 (스레드 정리)
        ClearCurrentCharacter();                      // 현재 캐릭터도 다시 확인
        StartVision();                                // 처음부터 다시 (게임 창 다시 찾기)
    }

    void CalculatorManager::SetCharacterAutoEnabled(bool isEnabled)
    {
        if (!m_visionSettings || m_visionSettings->IsCharacterAutoEnabled() == isEnabled) return;   // 그대로
        m_visionSettings->SetCharacterAutoEnabled(isEnabled);                         // 바꾸기
        try
        {
            m_visionSettings->Save();                                                 // 저장
        }
        catch (const std::exception&)
        {
            m_context->m_notifier.ShowToast(std::wstring{ VISION_SAVE_FAILED_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 저장 실패 안내
        }
        if (m_screenWatcher) m_screenWatcher->SetCharacterScanEnabled(isEnabled);     // 감시 반영 (끄면 화면 전체 OCR 안 함)
    }

    void CalculatorManager::ClearNameplateShapes()
    {
        if (m_nameplateMemory) m_nameplateMemory->Clear();   // 모두 지움 (저장 포함)
    }

    void CalculatorManager::ClearCharacterAliases()
    {
        if (!m_visionSettings) return;                       // 초기화 전
        m_visionSettings->ClearCharacterAliases();           // 모두 지움
        try
        {
            m_visionSettings->Save();                        // 저장
        }
        catch (const std::exception&)
        {
            m_context->m_notifier.ShowToast(std::wstring{ VISION_SAVE_FAILED_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 저장 실패 안내
        }
        PushCharacterNames();                                // 감시기에도 반영
    }

    int CalculatorManager::GetCharacterIntervalSeconds() const noexcept
    {
        return m_visionSettings ? m_visionSettings->GetCharacterIntervalSeconds() : Vision::VisionSettings::DEFAULT_CHARACTER_INTERVAL_SECONDS;   // 확인 간격
    }

    void CalculatorManager::SetCharacterIntervalSeconds(int seconds)
    {
        if (!m_visionSettings) return;                                                // 초기화 전
        m_visionSettings->SetCharacterIntervalSeconds(seconds);                       // 바꾸기 (범위 제한)
        try
        {
            m_visionSettings->Save();                                                 // 저장
        }
        catch (const std::exception&)
        {
            m_context->m_notifier.ShowToast(std::wstring{ VISION_SAVE_FAILED_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 저장 실패 안내
        }
        if (m_screenWatcher) m_screenWatcher->SetCharacterInterval(std::chrono::seconds{ m_visionSettings->GetCharacterIntervalSeconds() });   // 감시 반영
    }

    void CalculatorManager::SetVisionPreview(bool isEnabled)
    {
        m_isPreviewWanted = isEnabled;                                    // 기억 (감시를 다시 만들 때도)
        if (m_screenWatcher) m_screenWatcher->SetPreviewEnabled(isEnabled);   // 감시 반영
    }

    std::optional<VisionStatusSnapshot> CalculatorManager::GetVisionStatus() const
    {
        if (!m_screenWatcher) return std::nullopt;                        // 꺼짐
        VisionStatusSnapshot snapshot;                                    // 결과
        snapshot.m_status = m_screenWatcher->GetStatus();                 // 상태
        snapshot.m_diagnostics = m_screenWatcher->GetDiagnostics();       // 진단
        snapshot.m_topBar = Vision::ShardBarDetector::GetBarArea(snapshot.m_diagnostics.m_frameWidth, snapshot.m_diagnostics.m_frameHeight);   // 재화 줄
        return snapshot;                                                  // 상태
    }

    std::optional<Vision::BossHealthSample> CalculatorManager::GetBossHealth() const
    {
        return m_screenWatcher ? m_screenWatcher->GetBossHealth() : std::nullopt;   // 감시 중일 때만
    }

    std::optional<Vision::CombatSample> CalculatorManager::GetCombatStats() const
    {
        return m_screenWatcher ? m_screenWatcher->GetCombatStats() : std::nullopt;  // 감시 중일 때만
    }

    void CalculatorManager::SetCombatScan(bool isEnabled)
    {
        if (m_isCombatWanted == isEnabled) return;                                  // 그대로
        m_isCombatWanted = isEnabled;                                               // 기억 (감시를 다시 만들 때도)
        if (m_screenWatcher) m_screenWatcher->SetCombatScanEnabled(isEnabled);      // 감시 반영
    }

    void CalculatorManager::RequestRefresh()
    {
        if (m_view) StartRefresh(false);   // [시세 갱신]과 같은 흐름 (끝나면 알림)
    }

    std::size_t CalculatorManager::GetRosterCount() const noexcept
    {
        return m_characterNames.size();   // 원정대 캐릭터 수
    }

    // 확정된 인식 결과를 화면 스레드에서 반영 (모델은 이 스레드에서만 바뀜)
    void CalculatorManager::ApplyScanResults()
    {
        const Vision::WatcherStatus status{ m_screenWatcher->GetStatus() };   // 감시 상태
        if (status.m_state == Vision::WatcherState::WATCHER_UNSUPPORTED)      // 이 PC에서 못 씀 (창 캡처 · OCR 없음)
        {
            m_context->m_notifier.ShowToast(ToWideMessage(status.m_message.c_str()), Core::NoticeKind::NOTICE_WARNING);   // 이유 안내
            m_screenWatcher.reset();                                          // 감시 정리
            m_view->SetVisionChecked(false);                                  // 체크 해제 (설정은 그대로 → 환경을 고치면 다음 실행에 다시 시도)
            return;                                                           // 끝
        }

        for (const Vision::ScanResult& result : m_screenWatcher->TakeResults())   // 확정 결과마다 (들어온 순서대로)
        {
            const bool isAuto{ m_visionSettings->IsCharacterAutoEnabled() };      // 자동 인식 (끄면 직접 고른 캐릭터 유지)
            if (result.m_screenName == Vision::CharacterDetector::LOADING_SCREEN_NAME)   // 로딩 화면 → 캐릭터가 바뀌었을 수 있음
            {
                if (isAuto) ClearCurrentCharacter();                              // 다시 확인
                continue;                                                         // 다음 결과
            }
            if (result.m_screenName == Vision::CharacterDetector::UNKNOWN_SCREEN_NAME)   // 정보 창 이름을 못 맞춤 → [변경] 때 별칭 후보
            {
                m_lastUnknownName = result.m_characterName;                       // 읽은 글자
                m_lastUnknownAt = std::chrono::steady_clock::now();               // 시각
                continue;                                                         // 다음 결과
            }
            if (result.m_screenName == Vision::CharacterDetector::SCREEN_NAME)    // 현재 캐릭터 확인
            {
                if (!isAuto) continue;                                            // 자동 인식 꺼짐
                CurrentCharacterSource source{ CurrentCharacterSource::SOURCE_NAMEPLATE };   // 알아본 방법
                std::optional<double> readItemLevel;                              // 정보 창 레벨
                for (const Vision::ScanValue& value : result.m_values)            // 값마다
                {
                    if (value.m_key == Vision::CharacterDetector::SOURCE_VALUE_KEY
                        && value.m_value == static_cast<std::int64_t>(Vision::CharacterSource::SOURCE_INFO_WINDOW)) source = CurrentCharacterSource::SOURCE_INFO_WINDOW;   // 정보 창
                    if (value.m_key == Vision::CharacterDetector::ITEM_LEVEL_VALUE_KEY)
                        readItemLevel = static_cast<double>(value.m_value) / Vision::CharacterDetector::ITEM_LEVEL_SCALE;   // 아이템 레벨
                }
                SetCurrentCharacter(result.m_characterName, source, readItemLevel);   // 반영
                continue;                                                         // 다음 결과
            }
            for (const Vision::ScanValue& value : result.m_values)                // 값마다
            {
                if (const auto gem{ Vision::GemTooltipDetector::ParseValueKey(value.m_key) })   // 젬
                    ApplyGemScan(*gem, static_cast<int>(value.m_value));
                else if (value.m_key == Vision::ParadiseDetector::VALUE_KEY) ApplyParadiseScan(static_cast<double>(value.m_value));   // 낙원력
                else ApplyMaterialScan(value.m_key, static_cast<double>(value.m_value));                                         // 재료 수량
            }
        }
    }

    // 재료 수량 → 고른 캐릭터의 계산기 칸 (손 입력과 같은 EditQuantity 흐름 → 증감 표시 · 저장 · 재계산)
    void CalculatorManager::ApplyMaterialScan(const std::string& valueKey, double quantity)
    {
        const auto parsed{ Vision::OCRTextParser::SplitMaterialValueKey(valueKey) };   // "Dest/top" → 키 · 구분
        if (!parsed.has_value()) return;                                               // 모르는 값
        const auto keyIt{ std::ranges::find(Core::APPConstants::MAT_KEYS, std::string_view{ parsed->first }) };   // 재료 열
        if (keyIt == Core::APPConstants::MAT_KEYS.end() || parsed->first.empty()) return;                        // 모르는 재료
        const std::size_t materialIndex{ static_cast<std::size_t>(keyIt - Core::APPConstants::MAT_KEYS.begin()) };   // 열 번호

        const std::optional<std::string> name{ GetScanTarget() };                      // 현재 캐릭터 (모르면 안내 후 건너뜀)
        if (!name.has_value()) return;                                                 // 반영 안 함
        const Models::Character* character{ m_repository->GetData().FindCharacter(*name) };   // 캐릭터 데이터
        if (character == nullptr)                                                      // 저장소에 없음 (원정대를 다시 불러온 직후 등)
        {
            m_context->m_notifier.ShowToast(std::wstring{ VISION_NO_CHARACTER_TEXT }, Core::NoticeKind::NOTICE_WARNING);   // 안내
            return;                                                                    // 반영 안 함
        }

        // 상위 · 하위: 이름으로 정해진 재료는 그대로, 숨결 · 파편은 캐릭터 레벨로 (레벨을 모르면 상위)
        const bool isLevelTop{ character->GetItemLevel() <= 0.0 || character->GetItemLevel() >= Core::APPConstants::TOP_MATERIAL_MIN_LEVEL };
        const bool isTop{ parsed->second == Vision::OCRTextParser::MaterialTierHint::TIER_TOP
            || (parsed->second == Vision::OCRTextParser::MaterialTierHint::TIER_BY_LEVEL && isLevelTop) };
        const Models::ItemData& item{ character->GetMaterial(Core::APPConstants::MAT_KEYS[materialIndex]) };     // 지금 수량
        if ((isTop ? item.GetTopQty() : item.GetBottomQty()) == quantity) return;                               // 같은 수량이면 조용히 넘어감 (같은 재료를 다시 올려 봄)
        EditQuantity(isTop ? MaterialTier::TIER_TOP : MaterialTier::TIER_BOTTOM, materialIndex, quantity);   // 반영

        const std::wstring_view tierLabel{ parsed->second == Vision::OCRTextParser::MaterialTierHint::TIER_BY_LEVEL ? std::wstring_view{}
            : (isTop ? TIER_TOP_LABEL : TIER_BOTTOM_LABEL) };                          // (상) · (하) (숨결 · 파편은 생략)
        m_context->m_notifier.ShowToast(std::format(SCAN_MATERIAL_FORMAT, Core::TextEncoding::UTF8ToWide(*name), MATERIAL_LABELS[materialIndex],
            tierLabel, Core::NumberFormat::FormatNumber(quantity, SCAN_NUMBER_DECIMALS)), Core::NoticeKind::NOTICE_SUCCESS);   // 반영 안내
    }

    // 귀속 젬 → 젬 시세 검색기의 귀속 젬 계산기 (현재 캐릭터 칸)
    void CalculatorManager::ApplyGemScan(const Vision::GemValueKey& key, int quantity)
    {
        const std::size_t gemIndex{ key.m_gemIndex };                                  // 젬
        const std::size_t gradeIndex{ key.m_gradeIndex };                              // 등급
        const std::optional<std::string> name{ GetScanTarget() };                      // 현재 캐릭터 (모르면 안내 후 건너뜀)
        if (!name.has_value()) return;                                                 // 반영 안 함
        const std::wstring wideName{ Core::TextEncoding::UTF8ToWide(*name) };          // 안내용 이름
        using GemScanResult = ArkGemFinderManager::GemScanResult;                      // 결과 종류
        const auto outcome{ ArkGemFinderManager::GetInstance().ApplyScannedGem(*name, gemIndex, gradeIndex, key.m_kind, quantity, key.m_processedKey) };   // 젬 계산기에 넣기
        switch (outcome.m_result)
        {
        case GemScanResult::GEM_APPLIED:                                               // 넣음
            m_context->m_notifier.ShowToast(std::format(SCAN_GEM_FORMAT, wideName, Core::TextEncoding::UTF8ToWide(Models::BoundGemData::GEM_NAMES[gemIndex]),
                Core::TextEncoding::UTF8ToWide(Models::BoundGemData::GRADE_NAMES[gradeIndex]), outcome.m_total,
                GEM_KIND_LABELS[static_cast<std::size_t>(key.m_kind)]), Core::NoticeKind::NOTICE_SUCCESS);
            break;
        case GemScanResult::GEM_NO_CHARACTER:                                          // 계산기에 없는 캐릭터
            m_context->m_notifier.ShowToast(std::format(SCAN_GEM_NO_CHARACTER_FORMAT, wideName), Core::NoticeKind::NOTICE_WARNING);
            break;
        case GemScanResult::GEM_SAVE_FAILED:                                           // 저장 실패
            m_context->m_notifier.ShowToast(std::wstring{ SCAN_GEM_SAVE_FAILED_TEXT }, Core::NoticeKind::NOTICE_WARNING);
            break;
        case GemScanResult::GEM_UNCHANGED:                                             // 같은 수량 → 조용히
            break;
        }
    }

    // 낙원력 → 낙원력 트래커의 고른 캐릭터 이번 주차
    void CalculatorManager::ApplyParadiseScan(double score)
    {
        const std::optional<std::string> name{ GetScanTarget() };                      // 현재 캐릭터 (모르면 안내 후 건너뜀)
        if (!name.has_value()) return;                                                 // 반영 안 함

        const std::wstring wideName{ Core::TextEncoding::UTF8ToWide(*name) };          // 안내용 이름
        const std::wstring scoreText{ Core::NumberFormat::FormatNumber(score, SCAN_NUMBER_DECIMALS) };   // 안내용 점수
        switch (TrackerManager::GetInstance().ApplyScannedScore(*name, score))         // 트래커에 기록
        {
        case ScanApplyResult::SCAN_APPLIED:                                            // 기록함
            m_context->m_notifier.ShowToast(std::format(SCAN_PARADISE_FORMAT, wideName, scoreText), Core::NoticeKind::NOTICE_SUCCESS);
            break;
        case ScanApplyResult::SCAN_NO_CHARACTER:                                       // 트래커에 없는 캐릭터
            m_context->m_notifier.ShowToast(std::format(SCAN_PARADISE_NO_CHARACTER_FORMAT, wideName), Core::NoticeKind::NOTICE_WARNING);
            break;
        case ScanApplyResult::SCAN_NO_WEEK:                                            // 이번 주차 없음
            m_context->m_notifier.ShowToast(std::wstring{ SCAN_PARADISE_NO_WEEK_TEXT }, Core::NoticeKind::NOTICE_WARNING);
            break;
        case ScanApplyResult::SCAN_DECREASED:                                          // 줄어든 값 (오인식으로 봄)
            m_context->m_notifier.ShowToast(std::format(SCAN_PARADISE_DECREASED_FORMAT, scoreText), Core::NoticeKind::NOTICE_WARNING);
            break;
        case ScanApplyResult::SCAN_UNCHANGED:                                          // 같은 값 → 조용히 넘어감
            break;
        }
    }

    void CalculatorManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool CalculatorManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape CalculatorManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    std::optional<std::wstring> CalculatorManager::GetBackgroundStatus() const
    {
        if (!m_isAutoRefreshEnabled) return std::nullopt;
        return std::format(BACKGROUND_STATUS_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES);
    }

    // 대표 캐릭터 이름으로 원정대 목록 받기 (원본 Button2_Click 앞부분)
    void CalculatorManager::SyncRoster(const std::wstring& mainCharacterName)
    {
        const std::wstring_view trimmedName{ TrimSpaces(mainCharacterName) };
        if (trimmedName.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        if (m_rosterTask.IsRunning()) return;

        m_view->SetSyncBusy(true);
        m_view->SetSyncStatus(std::wstring{ SYNC_RUNNING_TEXT });   // 조회 진행 상태 표시
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_rosterTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetRoster(name); });
    }

    // 시세 갱신 시작 (원본 RefreshMarketAsync 앞부분) — 수동 · 자동 갱신이 겹치지 않음
    void CalculatorManager::StartRefresh(bool isAuto)
    {
        if (m_refreshTask.IsRunning()) return;

        m_view->CommitEdit();   // 입력 중이던 수량부터 반영

        // 세 툴이 API 키 하나(분당 호출 한도 하나)를 함께 씀 → 보석 검색기 자동 갱신과 겹쳐도 한도를 넘지 않게 먼저 확인
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };
        if (!rate.CanAfford(m_calculator->GetRequestCount(), now))
        {
            const std::wstring message{ std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))) };
            if (isAuto) m_context->m_notifier.ShowToast(std::wstring{ AUTO_REFRESH_FAILED_TEXT } + message, Core::NoticeKind::NOTICE_WARNING);
            else m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, message, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        m_isAutoRefreshRun = isAuto;
        m_view->SetRefreshBusy(true);

        Core::AssetCalculator* calculator{ m_calculator.get() };
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_refreshTask.Start([calculator, marketAPI] { calculator->FetchMarketPrices(*marketAPI); });
    }

    // 자동 갱신 켜면 바로 한 번 + 5분마다 (원본 CheckBox1_CheckedChanged)
    void CalculatorManager::SetAutoRefresh(bool isEnabled)
    {
        m_isAutoRefreshEnabled = isEnabled;
        if (!isEnabled)
        {
            m_autoRefreshTimer.Stop();
            return;
        }

        m_autoRefreshTimer.Start(Core::APPConstants::AUTO_REFRESH_INTERVAL_MS);
        StartRefresh(true);
    }

    // 고른 캐릭터의 수량 표시 · 잠금 → 계산 (원본 ComboBox1_SelectedIndexChanged)
    void CalculatorManager::SelectCharacter()
    {
        const std::vector<const Models::Character*> characters{ GetDisplayedCharacters() };
        if (characters.empty()) return;

        const Models::Character& character{ *characters.front() };
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
        {
            const std::string_view key{ Core::APPConstants::MAT_KEYS[index] };
            const Models::ItemData& item{ character.GetMaterial(key) };
            m_view->ShowQuantity(MaterialTier::TIER_TOP, index, item.GetTopQty(), item.GetTopQtyChange());
            m_view->ShowQuantity(MaterialTier::TIER_BOTTOM, index, item.GetBottomQty(), item.GetBottomQtyChange());
        }

        ApplyQuantityLock(character);
        if (const std::optional<std::string> name{ GetSelectedCharacterName() }; name.has_value()) m_calculator->CalculateCharacterAssets(*name);
    }

    // 보유수량 칸 하나 수정 → 저장 → 다시 계산 (원본 DataGridView1_CellValueChanged)
    void CalculatorManager::EditQuantity(MaterialTier tier, std::size_t materialIndex, double quantity)
    {
        if (materialIndex < FIRST_MATERIAL_INDEX || materialIndex >= Core::APPConstants::MAT_KEYS.size()) return;

        const std::optional<std::string> name{ GetSelectedCharacterName() };
        if (!name.has_value()) return;

        Models::Character* character{ m_repository->GetData().FindCharacter(*name) };
        if (character == nullptr) return;

        Models::ItemData& item{ character->GetMaterial(Core::APPConstants::MAT_KEYS[materialIndex]) };
        const bool isTop{ tier == MaterialTier::TIER_TOP };
        const double previousQuantity{ isTop ? item.GetTopQty() : item.GetBottomQty() };

        // 같은 수량을 다시 입력함 ("1000" → "1,000") → 지난 증감 표시를 그대로 되돌려 놓고 끝
        if (quantity == previousQuantity)
        {
            m_view->ShowQuantity(tier, materialIndex, quantity, isTop ? item.GetTopQtyChange() : item.GetBottomQtyChange());
            return;
        }

        // 바뀌기 전 수량을 직전 수량으로 남겨서 "지난 입력 대비 증감"을 보여 줌 (저장되어 다시 실행해도 유지)
        if (isTop)
        {
            item.SetPreviousTopQty(previousQuantity);
            item.SetTopQty(quantity);
        }
        else
        {
            item.SetPreviousBottomQty(previousQuantity);
            item.SetBottomQty(quantity);
        }
        m_view->ShowQuantity(tier, materialIndex, quantity, quantity - previousQuantity);

        TrySave();
        m_calculator->CalculateCharacterAssets(*name);
    }

    // 시세 갱신이 끝남 → 원정대 기록 · 완료 또는 오류 알림 (원본 RefreshMarketAsync 뒷부분)
    void CalculatorManager::FinishRefresh()
    {
        m_view->SetRefreshBusy(false);
        const bool isAuto{ m_isAutoRefreshRun };

        try
        {
            m_refreshTask.TakeResult();   // 백그라운드에서 난 예외는 여기서 다시 던져짐
            m_calculator->CalculateRosterHistory();
            if (!isAuto) m_context->m_notifier.ShowToast(std::wstring{ REFRESH_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            HandleAPIKeyError(error.what());
        }
        catch (const std::exception& error)
        {
            // 자동 갱신 오류는 작업을 막지 않게 알림만 (트레이에 숨어 있으면 풍선 알림), 직접 누른 갱신은 대화상자
            const std::wstring message{ ToWideMessage(error.what()) };
            if (isAuto) m_context->m_notifier.ShowToast(std::wstring{ AUTO_REFRESH_FAILED_TEXT } + message, Core::NoticeKind::NOTICE_ERROR);
            else m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, message, Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }

    // 원정대 목록을 받음 → 캐릭터 목록 교체 (기존 캐릭터는 수량 유지) (원본 Button2_Click 뒷부분)
    void CalculatorManager::FinishRosterSync()
    {
        m_view->SetSyncBusy(false);

        std::vector<Models::CharacterInfo> roster;
        try
        {
            roster = m_rosterTask.TakeResult();
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            m_view->SetSyncStatus(std::wstring{ SYNC_FAILED_TEXT });
            HandleAPIKeyError(error.what());
            return;
        }
        catch (const std::exception& error)
        {
            m_view->SetSyncStatus(std::wstring{ SYNC_FAILED_TEXT });
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, std::wstring{ ROSTER_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
            return;
        }

        if (roster.empty())
        {
            m_view->SetSyncStatus(std::wstring{ SYNC_FAILED_TEXT });
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ ROSTER_NOT_FOUND_TEXT }, Core::NoticeKind::NOTICE_WARNING, nullptr);
            return;
        }

        m_view->SetSyncStatus(std::format(SYNC_DONE_FORMAT, Core::LocalTime::FormatClock(Core::LocalTime::GetUnixSeconds())));   // 마지막 동기화 시각
        std::ranges::stable_sort(roster, std::ranges::greater{}, &Models::CharacterInfo::GetNumericItemLevel);

        Models::UserData& data{ m_repository->GetData() };
        std::unordered_map<std::string, Models::Character> characters;
        for (const Models::CharacterInfo& info : roster)
        {
            // 기존 캐릭터면 보유 수량은 그대로 두고 직업 · 레벨만 갱신
            const Models::Character* existing{ data.FindCharacter(info.GetCharacterName()) };
            Models::Character character{ existing != nullptr ? *existing : Models::Character{} };
            character.SetClassName(info.GetCharacterClassName());
            character.SetItemLevel(info.GetNumericItemLevel());
            characters.insert_or_assign(info.GetCharacterName(), std::move(character));
        }
        data.SetCharacters(std::move(characters));

        LoadCharacterList();

        try
        {
            if (m_calculator->HasPrices()) m_calculator->CalculateRosterHistory();
            else m_repository->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }

        m_context->m_notifier.ShowToast(std::format(ROSTER_DONE_FORMAT, roster.size()), Core::NoticeKind::NOTICE_SUCCESS);
    }

    // API 키 오류(401): 자동 갱신 끄기 → 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (원본 PromptApiKey)
    void CalculatorManager::HandleAPIKeyError(const std::string& message)
    {
        m_view->SetAutoRefreshChecked(false);
        SetAutoRefresh(false);

        const std::wstring reason{ ToWideMessage(message.c_str()) };
        if (!m_context->m_platform.IsWindowShown())
        {
            m_context->m_notifier.ShowToast(reason, Core::NoticeKind::NOTICE_ERROR);
            return;
        }

        m_context->m_notifier.AskYesNo(std::wstring{ API_KEY_ERROR_TITLE }, reason + std::wstring{ API_KEY_RETRY_TEXT },
            [openAPIKeyDialog = m_context->m_openAPIKeyDialog](bool isYes)
        {
            if (isYes && openAPIKeyDialog) openAPIKeyDialog();
        });
    }

    // 캐릭터 선택 목록: 아이템 레벨 높은 순 (원정대 전체 합계는 표 맨 아래 ★ 원정대 자산 ★ 행이 맡음)
    void CalculatorManager::LoadCharacterList()
    {
        std::vector<std::pair<const std::string*, const Models::Character*>> sorted;
        for (const auto& [name, character] : m_repository->GetData().GetCharacters()) sorted.emplace_back(&name, &character);

        // 레벨이 같으면 이름순 (실행할 때마다 순서가 바뀌지 않게)
        std::ranges::sort(sorted, [](const auto& left, const auto& right)
        {
            if (left.second->GetItemLevel() != right.second->GetItemLevel()) return left.second->GetItemLevel() > right.second->GetItemLevel();
            return *left.first < *right.first;
        });

        std::vector<std::wstring> items;
        items.reserve(sorted.size());
        m_characterNames.clear();
        for (const auto& [name, character] : sorted)
        {
            m_characterNames.push_back(*name);
            items.push_back(MakeDisplayName(*name, *character));
        }

        // 기본 선택은 첫 번째 캐릭터, 화면 인식으로 확인한 현재 캐릭터가 있으면 그 캐릭터 (캐릭터가 없으면 선택 없음)
        std::optional<std::size_t> selectedIndex{ m_characterNames.empty() ? std::nullopt : std::optional<std::size_t>{ FIRST_CHARACTER_INDEX } };
        if (m_currentCharacter.has_value())                                    // 현재 캐릭터를 알면
        {
            if (const auto found{ std::ranges::find(m_characterNames, *m_currentCharacter) }; found != m_characterNames.end())
                selectedIndex = static_cast<std::size_t>(found - m_characterNames.begin());   // 현재 캐릭터
            else m_currentCharacter.reset();                                                     // 원정대에서 빠짐 → 다시 확인
        }
        m_view->SetCharacterItems(std::move(items), selectedIndex);
        SelectCharacter();
        if (m_nameplateMemory) m_nameplateMemory->RemoveMissing(m_characterNames);   // 원정대에서 빠진 캐릭터 모양은 지움
        PushCharacterNames();   // 화면 인식에 새 이름 목록
    }

    // 단가 · 재료값 · 합계 · 그래프 · 트레이 글자 갱신 (원본 RefreshUI)
    void CalculatorManager::RefreshView()
    {
        if (!m_view) return;

        const auto topPrices{ m_calculator->GetTopPrices() };
        const auto bottomPrices{ m_calculator->GetBotPrices() };
        const std::vector<const Models::Character*> characters{ GetDisplayedCharacters() };

        double topTotal{};
        double bottomTotal{};
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
        {
            const std::string key{ Core::APPConstants::MAT_KEYS[index] };
            if (const auto top{ topPrices.find(key) }; top != topPrices.end())
            {
                m_view->ShowPrice(MaterialTier::TIER_TOP, index, top->second.GetCurrent(), top->second.GetDiff());
            }
            if (const auto bottom{ bottomPrices.find(key) }; bottom != bottomPrices.end())
            {
                m_view->ShowPrice(MaterialTier::TIER_BOTTOM, index, bottom->second.GetCurrent(), bottom->second.GetDiff());
            }

            // 고른 캐릭터의 재료값 (캐릭터가 없으면 건너뜀)
            if (characters.empty()) continue;

            double topValue{};
            double bottomValue{};
            for (const Models::Character* character : characters)
            {
                topValue += character->GetMaterial(key).GetCalculatedTopValue();
                bottomValue += character->GetMaterial(key).GetCalculatedBotValue();
            }
            m_view->ShowMaterialValue(MaterialTier::TIER_TOP, index, topValue);
            m_view->ShowMaterialValue(MaterialTier::TIER_BOTTOM, index, bottomValue);
            topTotal += topValue;
            bottomTotal += bottomValue;
        }
        if (!characters.empty()) m_view->ShowValueTotals(topTotal, bottomTotal);
        m_view->ShowRosterTotal(CalculateRosterTotal());   // 어떤 캐릭터를 골라도 표 맨 아래는 원정대 전체
        ShowRosterQuantities();

        const double currentTotal{ UpdateChart() };
        m_context->m_platform.SetTrayTooltip(std::format(TRAY_TOOLTIP_FORMAT, Core::NumberFormat::FormatNumber(currentTotal, AMOUNT_DECIMALS)));
    }

    // 날짜별 원정대 자산 기록 → 그래프 (가장 최근 총 자산 반환)
    double CalculatorManager::UpdateChart()
    {
        std::vector<UI::ChartPoint> points;
        double currentTotal{};
        for (const auto& [dateKey, total] : m_repository->GetData().GetHistory())
        {
            points.push_back(UI::ChartPoint{ ToChartDateLabel(dateKey), total });
            currentTotal = total;
        }

        std::wstring title{ points.empty() ? std::wstring{ CHART_EMPTY_TEXT } : std::wstring{ CHART_TITLE_TEXT } };
        m_view->SetTotalAsset(points.empty() ? std::nullopt : std::optional<double>{ currentTotal });   // 카드 머리말 오른쪽 "총자산 N G"
        m_view->SetChart(std::move(title), std::move(points));
        return currentTotal;
    }

    // 고른 캐릭터의 보유 수량(상위 · 하위)을 모두 0으로 (지워지므로 먼저 확인 → 예: 저장 · 다시 계산, 원본에 없는 새 기능)
    void CalculatorManager::ResetInput()
    {
        m_view->CommitEdit();   // 입력 중이던 칸은 확정한 뒤 지움

        const std::optional<std::string> name{ GetSelectedCharacterName() };
        if (!name.has_value())
        {
            m_context->m_notifier.ShowToast(std::wstring{ RESET_NO_CHARACTER_TEXT }, Core::NoticeKind::NOTICE_INFO);
            return;
        }

        m_context->m_notifier.AskYesNo(std::wstring{ RESET_TITLE }, std::format(RESET_QUESTION_FORMAT, Core::TextEncoding::UTF8ToWide(*name)),
            [this, name = *name](bool isYes)
        {
            if (!isYes || !m_repository || !m_view) return;   // 답하기 전에 프로그램이 종료됐거나 아니요

            Models::Character* character{ m_repository->GetData().FindCharacter(name) };
            if (character == nullptr) return;

            // 재료마다 직전 수량을 남기고 0으로 (지난 입력 대비 증감 표시와 같은 규칙)
            for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
            {
                Models::ItemData& item{ character->GetMaterial(Core::APPConstants::MAT_KEYS[index]) };
                item.SetPreviousTopQty(item.GetTopQty());
                item.SetTopQty(0.0);
                item.SetPreviousBottomQty(item.GetBottomQty());
                item.SetBottomQty(0.0);
            }

            TrySave();
            SelectCharacter();   // 표 수량 다시 표시 + 재계산 (총자산 즉시 반영)
            m_context->m_notifier.ShowToast(std::wstring{ RESET_DONE_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);
        });
    }

    // 레벨에 따라 보유수량 행 하나를 잠금 (1730 이상 = 상위만, 미만 = 하위만, 레벨 모름 = 둘 다) (원본 ApplyQtyRowLock)
    void CalculatorManager::ApplyQuantityLock(const Models::Character& character)
    {
        const bool isLevelKnown{ character.GetItemLevel() > 0.0 };
        const bool isTopTier{ character.GetItemLevel() >= Core::APPConstants::TOP_MATERIAL_MIN_LEVEL };

        m_view->SetQuantityEditable(MaterialTier::TIER_TOP, !isLevelKnown || isTopTier);
        m_view->SetQuantityEditable(MaterialTier::TIER_BOTTOM, !isLevelKnown || !isTopTier);
    }

    // 화면에 표시할 캐릭터 (고른 캐릭터 하나, 없으면 빈 목록) — 포인터는 저장소 데이터를 가리키기만 함
    std::vector<const Models::Character*> CalculatorManager::GetDisplayedCharacters() const
    {
        std::vector<const Models::Character*> characters;
        const Models::UserData& data{ m_repository->GetData() };

        if (const std::optional<std::string> name{ GetSelectedCharacterName() }; name.has_value())
        {
            if (const Models::Character* character{ data.FindCharacter(*name) }; character != nullptr) characters.push_back(character);
        }
        return characters;
    }

    // 모든 캐릭터의 상위 + 하위 재료값 합계 (시세 갱신 때 모든 캐릭터가 함께 계산되므로 저장된 계산값을 더하기만 함)
    double CalculatorManager::CalculateRosterTotal() const
    {
        double rosterTotal{};
        for (const auto& entry : m_repository->GetData().GetCharacters())
        {
            for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
            {
                const Models::ItemData& item{ entry.second.GetMaterial(Core::APPConstants::MAT_KEYS[index]) };
                rosterTotal += item.GetCalculatedTopValue() + item.GetCalculatedBotValue();
            }
        }
        return rosterTotal;
    }

    // 어떤 캐릭터를 골라도 원정대 자산 행은 원정대 전체 수량 (원정대 자산 골드와 같은 기준)
    void CalculatorManager::ShowRosterQuantities()
    {
        for (std::size_t index{ FIRST_MATERIAL_INDEX }; index < Core::APPConstants::MAT_KEYS.size(); ++index)
        {
            double topQuantity{};
            double bottomQuantity{};
            for (const auto& entry : m_repository->GetData().GetCharacters())
            {
                const Models::ItemData& item{ entry.second.GetMaterial(Core::APPConstants::MAT_KEYS[index]) };
                topQuantity += item.GetTopQty();
                bottomQuantity += item.GetBottomQty();
            }

            // 파편 · 숨결은 상위 · 하위 구분 없이 총합 하나로 표시 (하위를 0으로 넘기면 View가 숫자 하나만 씀)
            if (std::ranges::contains(SUM_ONLY_MATERIAL_KEYS, Core::APPConstants::MAT_KEYS[index]))
            {
                topQuantity += bottomQuantity;   // 상위 + 하위 합계
                bottomQuantity = 0.0;            // 하위 칸은 비움
            }
            m_view->ShowRosterQuantity(index, topQuantity, bottomQuantity);
        }
    }

    std::optional<std::string> CalculatorManager::GetSelectedCharacterName() const
    {
        if (!m_view) return std::nullopt;

        // 선택 번호 = 이름 번호 (목록 맨 위에 다른 항목이 없음)
        const std::optional<std::size_t> selectedIndex{ m_view->GetSelectedCharacterIndex() };
        if (!selectedIndex.has_value() || *selectedIndex >= m_characterNames.size()) return std::nullopt;   // 경계 검사
        return m_characterNames[*selectedIndex];
    }

    void CalculatorManager::TrySave()
    {
        try
        {
            m_repository->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }
}
