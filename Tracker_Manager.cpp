module LostArkMultiTool.Views.TrackerManager;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.GrowthCalculator;
import LostArkMultiTool.Core.GrowthDataRepository;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.RosterService;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.GrowthData;
import LostArkMultiTool.Models.Season;
import LostArkMultiTool.Models.WeeklyRecord;
import LostArkMultiTool.Services.GrowthCalculationService;
import LostArkMultiTool.Services.JSONGrowthDataRepository;
import LostArkMultiTool.Services.RosterChangeService;
import LostArkMultiTool.Views.PageLayout;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.TrackerView;
import LostArkMultiTool.Views.UI.LineChart;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view TAB_TITLE{ L"낙원력 트래커" };

        constexpr double UNRECORDED_SCORE{ 0.0 };
        constexpr std::size_t FIRST_WEEK_INDEX{ 0 };

        // ===== 문구 =====
        constexpr std::wstring_view WEEK_LABEL_FORMAT{ L"{}주차" };              // 표의 주차 열
        constexpr std::wstring_view CHART_LABEL_FORMAT{ L"{}주" };               // 그래프 x축
        constexpr std::wstring_view CHARACTER_HEADER_FORMAT{ L"{}\n(Lv.{})" };   // 현재 원정대 열: 직업 + 레벨 (원본과 같음)
        constexpr std::wstring_view PAST_CHARACTER_HEADER_FORMAT{ L"{}\n(원정대 제외)" };   // 원정대에서 빠졌지만 기록이 있는 캐릭터
        constexpr std::wstring_view OVERLAY_WEEK_FORMAT{ L"{}주차" };   // 인게임 오버레이 낙원력 패널 주차 이름

        // 이번 주차 번호 (0부터): 기간 있는 시즌은 그 기간, 기간 없는 시즌은 지금 게임 시즌 기간(LIVE_SEASON_*)으로 계산 (수요일 06시에 바뀜)
        [[nodiscard]] std::optional<std::size_t> FindThisWeekIndex(const Models::Season& season)
        {
            const auto today = Core::LocalTime::GetGameToday();                            // 06시 기준 오늘
            if (season.HasPeriod()) return season.FindWeekIndex(today);                     // 시즌 기간
            Models::Season live;                                                            // 지금 게임 시즌 (계산용)
            live.SetPeriod(Core::APPConstants::LIVE_SEASON_START, Core::APPConstants::LIVE_SEASON_END);
            return live.FindWeekIndex(today);                                               // 9/9부터 몇 번째 주
        }
        constexpr std::wstring_view CHART_TITLE_FORMAT{ L"{} · 주차별 평균 낙원력" };

        // ===== 통계 카드 · 목록 (새 디자인) =====
        constexpr std::size_t ROSTER_TREND_WEEKS{ 12 };                    // 목록 추이 선에 쓰는 최근 기록 수
        constexpr int AVERAGE_DECIMALS{ 0 };
        constexpr int CARD_RATE_DECIMALS{ 1 };
        constexpr std::wstring_view NO_VALUE_TEXT{ L"-" };                 // 기록이 없을 때 큰 값
        constexpr std::wstring_view NO_RECORD_TEXT{ L"기록 없음" };
        constexpr std::wstring_view PLUS_TEXT{ L"+" };
        constexpr std::wstring_view PERCENT_TEXT{ L"%" };
        constexpr std::wstring_view CARD_AVERAGE_ICON{ L"▤" };
        constexpr std::wstring_view CARD_GROWTH_ICON{ L"↗" };
        constexpr std::wstring_view CARD_COUNT_ICON{ L"◎" };
        constexpr std::wstring_view CARD_BEST_ICON{ L"★" };
        constexpr std::wstring_view CARD_AVERAGE_LABEL{ L"원정대 평균 낙원력" };
        constexpr std::wstring_view CARD_GROWTH_LABEL{ L"주차 총합 증감" };
        constexpr std::wstring_view CARD_COUNT_LABEL{ L"원정대 캐릭터" };
        constexpr std::wstring_view CARD_BEST_LABEL{ L"시즌 최고 평균" };
        constexpr std::wstring_view CARD_VS_PREVIOUS_FORMAT{ L"{}주차 · 전주 대비" };   // 증감 카드 작은 글자
        constexpr std::wstring_view CARD_WEEK_FORMAT{ L"{}주차" };
        constexpr std::wstring_view CARD_COUNT_FORMAT{ L"{}개" };
        constexpr std::wstring_view CARD_RECORDED_FORMAT{ L"기록 {}주차" };
        constexpr std::wstring_view ROSTER_DETAIL_FORMAT{ L"{} · Lv.{}" };
        constexpr std::wstring_view ROSTER_PAST_TEXT{ L"원정대 제외" };
        constexpr std::wstring_view CHART_EMPTY_FORMAT{ L"{} · 아직 기록이 없어요 · 주차 칸에 낙원력을 입력해 주세요" };

        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };   // 손상 복구 · 원본 낙원력 트래커 기록 가져오기
        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"저장에 실패했어요. 입력한 값은 화면에 남아 있고 다음 저장 때 다시 시도해요.\n" };
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"서버 점검" };
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };
        constexpr std::wstring_view RENAMING_TEXT{ L"닉네임 변경 확인을 먼저 마쳐 주세요." };
        constexpr std::wstring_view ROSTER_NOT_FOUND_TEXT{ L"캐릭터 정보를 찾을 수 없어요." };
        constexpr std::wstring_view ROSTER_ERROR_TEXT{ L"오류 발생: " };
        constexpr std::wstring_view ROSTER_DONE_FORMAT{ L"총 {}개의 캐릭터 연동 완료!" };
        constexpr std::wstring_view ROSTER_RENAMED_FORMAT{ L" (닉네임 변경 {}건의 기록을 옮겼어요)" };
        constexpr std::wstring_view RENAME_TITLE{ L"닉네임 변경 확인" };
        constexpr std::wstring_view RENAME_QUESTION_FORMAT{
            L"'{}'({}) 캐릭터가 원정대에서 사라지고,\n같은 서버 · 직업의 '{}' 캐릭터가 새로 생겼어요.\n\n닉네임을 바꾼 같은 캐릭터라면 예전 기록을 '{}'(으)로 옮길까요?" };
        constexpr std::wstring_view ADD_SEASON_TITLE_FORMAT{ L"시즌 {} 추가" };
        constexpr std::wstring_view ADD_SEASON_FORMAT{
            L"시즌 {}의 기간을 입력해 주세요. (이전 시즌 기록은 그대로 남아요)\n주차는 시작일(수요일)부터 7일씩(수요일 ~ 화요일) 나뉘고, 이번 주차만 다음 주 수요일 오전 6시 점검 전까지 입력할 수 있어요." };
        constexpr std::wstring_view EDIT_PERIOD_TITLE_FORMAT{ L"시즌 {} 기간" };
        constexpr std::wstring_view EDIT_PERIOD_TEXT{ L"시즌 기간을 바꾸면 주차 날짜가 다시 계산돼요. (입력한 기록은 주차 번호 그대로 남아요)" };
        constexpr std::wstring_view NOT_WEDNESDAY_TEXT{ L"시작일은 수요일이어야 해요. (로스트아크의 한 주 = 수요일 ~ 화요일)" };
        constexpr std::wstring_view SEASON_EXISTS_FORMAT{ L"시즌 {}은(는) 이미 있어요." };

        // ===== 주차 잠금 · 안내 =====
        constexpr std::wstring_view WEEK_CLOSED_TEXT{ L"이 주차는 입력이 마감됐어요. 고치려면 [지난 주차 수정]을 눌러 주세요." };
        constexpr std::wstring_view UNLOCK_TITLE{ L"지난 주차 수정" };
        constexpr std::wstring_view UNLOCK_SELECT_TEXT{ L"수정할 지난 주차의 칸을 먼저 골라 주세요." };
        constexpr std::wstring_view UNLOCK_QUESTION_FORMAT{ L"{}주차 기록을 수정할까요?\n다른 시즌 · 다른 탭으로 가거나 주가 바뀌면 다시 잠겨요." };
        constexpr std::wstring_view SCHEDULE_BEFORE_FORMAT{ L"{} 시작 전이에요 · {}부터 1주차를 입력할 수 있어요" };
        constexpr std::wstring_view SCHEDULE_CURRENT_FORMAT{ L"{}주차 입력 중 · 마감 {} 05:59 (정기점검 전) · 지난 주차는 잠겨요" };
        constexpr std::wstring_view SCHEDULE_ENDED_FORMAT{ L"{} 종료 · 모든 주차가 잠겼어요" };
        constexpr std::wstring_view SCHEDULE_UNLOCKED_FORMAT{ L" · {}주차 수정 중" };
        constexpr std::wstring_view DATE_LABEL_FORMAT{ L"{:02}/{:02}({})" };
        constexpr std::array<std::wstring_view, 7> WEEKDAY_NAMES{ L"일", L"월", L"화", L"수", L"목", L"금", L"토" };   // c_encoding 순서 (일요일 = 0)

        constexpr int DEFAULT_SEASON_WEEKS{ 8 };              // 새 시즌 기간 입력칸의 처음 종료일 (시작일 + 8주 - 1일)
        constexpr int LAST_DAY_OFFSET{ 1 };                    // 종료일 = 다음 시작일 - 1일
        constexpr std::size_t NEXT_WEEK_STEP{ 1 };
        constexpr int SCHEDULE_CHECK_INTERVAL_MS{ Core::APPConstants::MS_PER_MINUTE };   // 주차가 바뀌었는지 1분마다 확인
        constexpr std::wstring_view INVALID_SCORE_TEXT{ L"0 이상의 숫자만 입력할 수 있어요." };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };   // 일반 공백 · 전각 공백

        // 예외 메시지 · 저장 글자(UTF-8) → 화면 문구 (변환할 수 없으면 기본 문구)
        [[nodiscard]] std::wstring ToWide(std::string_view text)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(text);
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

        // "11/12(수)"
        [[nodiscard]] std::wstring ToDateLabel(const std::chrono::year_month_day& date)
        {
            const std::chrono::weekday weekday{ std::chrono::sys_days{ date } };
            return std::format(DATE_LABEL_FORMAT, static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()),
                WEEKDAY_NAMES.at(weekday.c_encoding()));
        }

        [[nodiscard]] bool IsWednesday(const std::chrono::year_month_day& date) noexcept
        {
            return std::chrono::weekday{ std::chrono::sys_days{ date } } == std::chrono::Wednesday;
        }

        // 이번 주의 수요일 (로스트아크 날짜 기준, 오늘이 수요일이면 오늘)
        [[nodiscard]] std::chrono::year_month_day GetThisWednesday(const std::chrono::year_month_day& gameDay)
        {
            const std::chrono::sys_days day{ gameDay };
            return std::chrono::year_month_day{ day - (std::chrono::weekday{ day } - std::chrono::Wednesday) };
        }

        // 현재 원정대 열 머리글: 직업이 없으면 이름으로 (원본 "{직업}\n(Lv.{레벨})")
        [[nodiscard]] std::wstring MakeCharacterHeader(const Models::CharacterInfo& character)
        {
            const std::optional<std::string>& className{ character.GetCharacterClassName() };
            const std::wstring title{ ToWide(className.has_value() && !className->empty() ? *className : character.GetCharacterName()) };
            const double level{ character.GetNumericItemLevel() };
            return std::format(CHARACTER_HEADER_FORMAT, title, level);
        }
    }

    TrackerManager::TrackerManager() = default;

    TrackerManager::~TrackerManager() = default;

    std::wstring_view TrackerManager::GetTabTitle() const noexcept
    {
        return TAB_TITLE;
    }

    void TrackerManager::Initialize(Core::ToolContext& context)
    {
        m_context = &context;

        // 모델: 저장 파일(ParadiseTrackerData.json, 없으면 원본 낙원력 트래커 기록을 가져옴) → 통계 · 원정대 변화
        m_repository = std::make_unique<Services::JSONGrowthDataRepository>();
        m_calculator = std::make_unique<Services::GrowthCalculationService>();
        m_rosterService = std::make_unique<Services::RosterChangeService>();

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_view = std::make_unique<TrackerView>(context.m_platform);
        m_view->SetOnSyncRoster([this](const std::wstring& characterName) { SyncRoster(characterName); });
        m_view->SetOnAddWeek([this] { AddWeek(); });
        m_view->SetOnAddSeason([this] { AddSeason(); });
        m_view->SetOnEditPeriod([this] { EditPeriod(); });
        m_view->SetOnUnlockWeek([this] { UnlockWeek(); });
        m_view->SetOnSeasonSelected([this] { SelectSeason(); });
        m_view->SetOnChartRangeChanged([this] { UpdateChart(); });   // 그래프 기간(4 · 8 · 12주)이 바뀌면 다시 그림
        m_view->SetOnScoreEdited([this](std::size_t weekIndex, std::size_t characterIndex, std::optional<double> score)
        {
            EditScore(weekIndex, characterIndex, score);
        });

        // 저장 파일 복구 · 원본 기록 가져오기 안내 (원본 Form1_Load)
        if (const std::optional<std::string>& warning{ m_repository->GetLoadWarning() }; warning.has_value())
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWide(*warning), Core::NoticeKind::NOTICE_INFO, nullptr);
        }

        RefreshSeasonList();
        RefreshGridStructure();
        PopulateGridData();

        // 기간 있는 시즌: 이번 주차까지 행 추가 · 잠금 · 이번 주차로 포커스 → 1분마다 주차가 바뀌었는지 확인
        ApplyWeekSchedule(true);
        m_scheduleTimer.Start(SCHEDULE_CHECK_INTERVAL_MS);
    }

    void TrackerManager::Shutdown()
    {
        m_scheduleTimer.Stop();
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에 → 사라진 객체를 쓰지 않게)
        m_rosterTask.Wait();

        m_pendingRenames.clear();
        m_isRenaming = false;
        m_view.reset();
        m_rosterService.reset();
        m_calculator.reset();
        m_repository.reset();
        m_context = nullptr;
    }

    void TrackerManager::OnActivated()
    {
        if (m_view) m_view->RestoreFocus();
    }

    void TrackerManager::OnDeactivated()
    {
        if (!m_view) return;
        m_view->SuspendFocus();   // 편집 중인 칸 확정 · IME 끔

        // 다른 탭으로 가면 [지난 주차 수정]으로 푼 주차는 다시 잠금 (확정이 끝난 뒤)
        if (m_unlockedWeek.has_value())
        {
            m_unlockedWeek.reset();
            ApplyWeekSchedule(false);
        }
    }

    void TrackerManager::Update(double deltaSeconds)
    {
        if (!m_view) return;

        m_view->Update(deltaSeconds);
        if (m_rosterTask.IsReady()) FinishRosterSync();   // 다른 탭을 보거나 트레이에 숨어 있어도 결과 반영
        if (m_scheduleTimer.Poll()) CheckSchedule();      // 수요일 06시 점검 시각이 지나면 주차 전환
    }

    void TrackerManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        if (m_view) m_view->Render(renderer, contentArea);
    }

    bool TrackerManager::HandleInput(const Core::InputEvent& event)
    {
        return m_view && m_view->HandleInput(event);
    }

    Core::CursorShape TrackerManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    // 원정대 동기화 시작 (원본 Button1_Click 앞부분)
    void TrackerManager::SyncRoster(const std::wstring& characterName)
    {
        const std::wstring_view trimmedName{ TrimSpaces(characterName) };
        if (trimmedName.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        if (m_isRenaming)
        {
            m_context->m_notifier.ShowToast(std::wstring{ RENAMING_TEXT }, Core::NoticeKind::NOTICE_INFO);
            return;
        }
        if (m_rosterTask.IsRunning()) return;

        m_view->CommitEdit();   // 입력 중이던 낙원력부터 반영
        m_view->SetSyncBusy(true);
        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };
        m_rosterTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetRoster(name); });
    }

    // 원정대 목록을 받음 → 레벨 높은 순 정렬 → 닉네임 변경 후보 확인 → 표 다시 구성 (원본 Button1_Click 뒷부분)
    void TrackerManager::FinishRosterSync()
    {
        m_view->SetSyncBusy(false);

        std::vector<Models::CharacterInfo> roster;
        try
        {
            roster = m_rosterTask.TakeResult();
        }
        catch (const Core::APIKeyInvalidException& error)
        {
            HandleAPIKeyError(error.what());   // 요청이 끝난 뒤에 재입력 안내
            return;
        }
        catch (const Core::APIRateLimitException& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, ToWide(error.what()), Core::NoticeKind::NOTICE_WARNING, nullptr);
            return;
        }
        catch (const Core::APIMaintenanceException& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, ToWide(error.what()), Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, std::wstring{ ROSTER_ERROR_TEXT } + ToWide(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
            return;
        }

        if (roster.empty())
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ ROSTER_NOT_FOUND_TEXT }, Core::NoticeKind::NOTICE_WARNING, nullptr);
            return;
        }

        // 템렙 기준으로 가장 높은 캐릭터부터 (같은 레벨은 API 순서 유지 → 원본 OrderByDescending은 안정 정렬)
        std::ranges::stable_sort(roster, std::ranges::greater{}, &Models::CharacterInfo::GetNumericItemLevel);

        // 원정대를 바꾸기 전에 닉네임 변경 후보를 찾아 둠 (확인받은 것만 기록 이전)
        Models::GrowthData& data{ m_repository->GetData() };
        m_pendingRenames = m_rosterService->FindRenameCandidates(data.GetCharacters(), roster, data);
        m_syncedCount = roster.size();
        data.SetCharacters(std::move(roster));
        TrySave();

        RefreshGridStructure();
        PopulateGridData();

        // 후보를 하나씩 물어봄 (대화상자 답을 받은 뒤 다음 후보 → 모두 끝나면 완료 안내)
        m_renameIndex = 0;
        m_renamedCount = 0;
        m_isRenaming = !m_pendingRenames.empty();
        AskNextRename();
    }

    void TrackerManager::AskNextRename()
    {
        if (m_renameIndex >= m_pendingRenames.size())
        {
            FinishRenames();
            return;
        }

        const Core::RenameCandidate& candidate{ m_pendingRenames[m_renameIndex] };
        const std::wstring oldName{ ToWide(candidate.m_oldName) };
        const std::wstring newName{ ToWide(candidate.m_newName) };
        const std::wstring className{ ToWide(candidate.m_className.value_or(std::string{})) };

        m_context->m_notifier.AskYesNo(std::wstring{ RENAME_TITLE }, std::format(RENAME_QUESTION_FORMAT, oldName, className, newName, newName),
            [this](bool isYes)
        {
            if (!m_repository) return;   // 답하기 전에 프로그램이 종료됨

            const Core::RenameCandidate& answered{ m_pendingRenames[m_renameIndex] };
            if (isYes)
            {
                static_cast<void>(m_rosterService->RenameCharacter(m_repository->GetData(), answered.m_oldName, answered.m_newName));
                ++m_renamedCount;
            }
            ++m_renameIndex;
            AskNextRename();
        });
    }

    // 확인이 모두 끝남 → 옮긴 기록이 있으면 저장 · 표 다시 구성 → 완료 안내
    void TrackerManager::FinishRenames()
    {
        m_isRenaming = false;
        m_pendingRenames.clear();

        if (m_renamedCount > 0)
        {
            TrySave();
            RefreshGridStructure();   // 옮긴 캐릭터가 "원정대 제외" 열에서 빠짐
            PopulateGridData();
        }

        std::wstring message{ std::format(ROSTER_DONE_FORMAT, m_syncedCount) };
        if (m_renamedCount > 0) message += std::format(ROSTER_RENAMED_FORMAT, m_renamedCount);
        m_context->m_notifier.ShowToast(std::move(message), Core::NoticeKind::NOTICE_SUCCESS);
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (원본 PromptApiKey)
    void TrackerManager::HandleAPIKeyError(const std::string& message)
    {
        const std::wstring reason{ ToWide(message) };
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

    // 주차 추가 (지금 보고 있는 시즌에, 원본 Button2_Click) — 기간 있는 시즌은 날짜에 맞춰 자동으로 늘어나므로 손으로 추가하지 않음
    void TrackerManager::AddWeek()
    {
        if (GetCurrentSeason().HasPeriod()) return;

        m_view->CommitEdit();
        GetCurrentSeason().AddWeek();
        TrySave();
        PopulateGridData();
    }

    // 시즌 추가 = 가장 큰 번호 + 1 (시즌 4 다음은 시즌 5) · 기간(시작일 ~ 종료일)을 함께 입력
    void TrackerManager::AddSeason()
    {
        m_view->CommitEdit();
        const int newSeasonNumber{ m_repository->GetData().GetNextSeasonNumber() };

        // 처음 값: 이번 주 수요일 ~ 8주 뒤 화요일
        const std::chrono::year_month_day startDate{ GetThisWednesday(Core::LocalTime::GetGameToday()) };
        const std::chrono::year_month_day endDate{ Core::LocalTime::AddDays(startDate, DEFAULT_SEASON_WEEKS * Models::Season::DAYS_PER_WEEK - LAST_DAY_OFFSET) };

        m_context->m_notifier.AskDateRange(std::format(ADD_SEASON_TITLE_FORMAT, newSeasonNumber), std::format(ADD_SEASON_FORMAT, newSeasonNumber),
            startDate, endDate, [this, newSeasonNumber](const std::chrono::year_month_day& start, const std::chrono::year_month_day& end) -> std::optional<std::wstring>
        {
            if (!m_repository) return std::nullopt;
            if (!IsWednesday(start)) return std::wstring{ NOT_WEDNESDAY_TEXT };   // 창을 연 채 다시 고르게

            Models::GrowthData& data{ m_repository->GetData() };
            if (data.HasSeason(newSeasonNumber)) return std::format(SEASON_EXISTS_FORMAT, newSeasonNumber);

            Models::Season season{ newSeasonNumber };
            season.SetPeriod(start, end);
            data.GetSeasons().push_back(std::move(season));
            data.SetSelectedSeasonNumber(newSeasonNumber);
            m_unlockedWeek.reset();
            m_lastWeekIndex.reset();
            TrySave();

            RefreshSeasonList();
            RefreshGridStructure();
            PopulateGridData();
            ApplyWeekSchedule(true);
            return std::nullopt;
        });
    }

    // 지금 시즌의 기간 바꾸기 (기간이 있는 시즌만 → 시즌 4처럼 기간 없는 시즌은 지금처럼 자유 입력)
    void TrackerManager::EditPeriod()
    {
        const Models::Season& season{ GetCurrentSeason() };
        if (!season.HasPeriod()) return;

        m_view->CommitEdit();
        const int seasonNumber{ season.GetSeasonNumber() };
        m_context->m_notifier.AskDateRange(std::format(EDIT_PERIOD_TITLE_FORMAT, seasonNumber), std::wstring{ EDIT_PERIOD_TEXT },
            *season.GetStartDate(), *season.GetEndDate(),
            [this, seasonNumber](const std::chrono::year_month_day& start, const std::chrono::year_month_day& end) -> std::optional<std::wstring>
        {
            if (!m_repository) return std::nullopt;
            if (!IsWednesday(start)) return std::wstring{ NOT_WEDNESDAY_TEXT };

            // 묻는 사이에 시즌을 바꿨을 수 있음 → 번호로 다시 찾음
            std::vector<Models::Season>& seasons{ m_repository->GetData().GetSeasons() };
            const auto found{ std::ranges::find(seasons, seasonNumber, &Models::Season::GetSeasonNumber) };
            if (found == seasons.end()) return std::nullopt;

            found->SetPeriod(start, end);
            m_unlockedWeek.reset();
            m_lastWeekIndex.reset();
            TrySave();
            ApplyWeekSchedule(true);
            return std::nullopt;
        });
    }

    // 고른 지난 주차를 확인 후 잠깐 풀기 (다른 시즌 · 다른 탭으로 가거나 주가 바뀌면 다시 잠김)
    void TrackerManager::UnlockWeek()
    {
        Models::Season& season{ GetCurrentSeason() };
        if (!season.HasPeriod()) return;

        m_view->CommitEdit();
        const std::optional<std::size_t> selectedWeek{ m_view->GetSelectedWeek() };
        const std::optional<std::size_t> currentWeek{ season.FindWeekIndex(Core::LocalTime::GetGameToday()) };

        // 지난 주차 = 행이 있고, 이번 주차보다 앞 (시즌이 끝났으면 모든 주차)
        const bool isPastWeek{ selectedWeek.has_value() && *selectedWeek < season.GetRecords().size()
            && (!currentWeek.has_value() || *selectedWeek < *currentWeek) };
        if (!isPastWeek)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ UNLOCK_TITLE }, std::wstring{ UNLOCK_SELECT_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        const std::size_t weekIndex{ *selectedWeek };
        const int seasonNumber{ season.GetSeasonNumber() };
        m_context->m_notifier.AskYesNo(std::wstring{ UNLOCK_TITLE },
            std::format(UNLOCK_QUESTION_FORMAT, season.GetRecords()[weekIndex].GetWeekNumber()), [this, weekIndex, seasonNumber](bool isYes)
        {
            if (!isYes || !m_repository || GetCurrentSeason().GetSeasonNumber() != seasonNumber) return;   // 묻는 사이에 시즌이 바뀌면 무시

            m_unlockedWeek = weekIndex;
            ApplyWeekSchedule(false);
            m_view->FocusWeek(weekIndex);
        });
    }

    // 시즌 선택이 바뀌면 그 시즌의 표 · 그래프로 전환 (마지막으로 본 시즌은 저장, 원본 CboSeason_SelectedIndexChanged)
    void TrackerManager::SelectSeason()
    {
        const std::optional<std::size_t> selectedIndex{ m_view->GetSelectedSeasonIndex() };
        if (!selectedIndex.has_value() || *selectedIndex >= m_seasonNumbers.size()) return;   // 경계 검사

        Models::GrowthData& data{ m_repository->GetData() };
        const int seasonNumber{ m_seasonNumbers[*selectedIndex] };
        if (seasonNumber == data.GetSelectedSeasonNumber()) return;

        m_view->CommitEdit();
        data.SetSelectedSeasonNumber(seasonNumber);
        m_unlockedWeek.reset();     // 다른 시즌으로 가면 다시 잠금
        m_lastWeekIndex.reset();
        TrySave();

        RefreshGridStructure();
        PopulateGridData();
        ApplyWeekSchedule(true);    // 그 시즌의 이번 주차로 포커스
    }

    // 낙원력 칸 하나 수정 → 통계 · 그래프 · 저장 (표 전체를 다시 만들지 않음 → 선택 위치 · 스크롤 유지, 원본 CellValueChanged)
    void TrackerManager::EditScore(std::size_t weekIndex, std::size_t characterIndex, std::optional<double> score)
    {
        std::vector<Models::WeeklyRecord>& records{ GetCurrentSeason().GetRecords() };
        if (weekIndex >= records.size() || characterIndex >= m_columnCharacterNames.size()) return;   // 경계 검사

        Models::WeeklyRecord& record{ records[weekIndex] };
        const std::string& characterName{ m_columnCharacterNames[characterIndex] };

        // 입력 마감된 주차 (편집 중에 수요일 06시가 지난 경우 등) → 안내하고 원래 값으로 되돌림
        if (!IsWeekEditable(weekIndex))
        {
            m_context->m_notifier.ShowToast(std::wstring{ WEEK_CLOSED_TEXT }, Core::NoticeKind::NOTICE_WARNING);
            m_view->ShowScore(weekIndex, characterIndex, record.GetScore(characterName));
            return;
        }

        // 숫자가 아님 → 안내하고 원래 값으로 되돌림 (원본: 편집 상태 유지 + 칸 아래 안내)
        if (!score.has_value())
        {
            m_context->m_notifier.ShowToast(std::wstring{ INVALID_SCORE_TEXT }, Core::NoticeKind::NOTICE_WARNING);
            m_view->ShowScore(weekIndex, characterIndex, record.GetScore(characterName));
            return;
        }

        record.SetScore(characterName, *score);   // 0 · 빈칸 = 미기록 (평균 계산에서 제외)
        m_view->ShowScore(weekIndex, characterIndex, record.GetScore(characterName));

        UpdateStatCells();
        UpdateChart();
        UpdateOverview();
        TrySave();
    }

    // 시즌 선택 목록 다시 채우기 (번호순, 지금 시즌 선택, 원본 RefreshSeasonList)
    void TrackerManager::RefreshSeasonList()
    {
        const Models::GrowthData& data{ m_repository->GetData() };
        const int currentNumber{ GetCurrentSeason().GetSeasonNumber() };

        m_seasonNumbers.clear();
        for (const Models::Season& season : data.GetSeasons()) m_seasonNumbers.push_back(season.GetSeasonNumber());
        std::ranges::sort(m_seasonNumbers);

        std::vector<std::wstring> items;
        items.reserve(m_seasonNumbers.size());
        std::optional<std::size_t> selectedIndex;
        for (std::size_t index{}; index < m_seasonNumbers.size(); ++index)
        {
            items.push_back(ToWide(Models::Season{ m_seasonNumbers[index] }.GetDisplayName()));
            if (m_seasonNumbers[index] == currentNumber) selectedIndex = index;
        }
        m_view->SetSeasonItems(std::move(items), selectedIndex);
    }

    // 열 구성: 주차 | 현재 원정대(입력) | 원정대 제외(읽기 전용) | 총합 · 평균 · 전주대비 · 첫주대비 (원본 RefreshGridStructure)
    void TrackerManager::RefreshGridStructure()
    {
        const Models::GrowthData& data{ m_repository->GetData() };
        m_columnCharacterNames.clear();

        std::vector<std::wstring> headers;
        for (const Models::CharacterInfo& character : data.GetCharacters())
        {
            headers.push_back(MakeCharacterHeader(character));
            m_columnCharacterNames.push_back(character.GetCharacterName());
        }
        const std::size_t editableCount{ m_columnCharacterNames.size() };

        // 이 시즌에 기록이 있지만 원정대에서 빠진 캐릭터 → 보이는 칸과 통계가 항상 일치
        for (std::string& pastName : m_rosterService->GetPastCharacters(GetCurrentSeason(), data.GetCharacters()))
        {
            headers.push_back(std::format(PAST_CHARACTER_HEADER_FORMAT, ToWide(pastName)));
            m_columnCharacterNames.push_back(std::move(pastName));
        }

        m_view->SetCharacterColumns(std::move(headers), editableCount);
    }

    // 지금 시즌의 주차별 점수를 표에 채우고 통계 · 그래프 갱신 (구조가 바뀔 때만: 시작 · 동기화 · 주차/시즌 추가 · 시즌 전환)
    void TrackerManager::PopulateGridData()
    {
        const std::vector<Models::WeeklyRecord>& records{ GetCurrentSeason().GetRecords() };
        m_view->SetWeekCount(records.size());

        for (std::size_t weekIndex{}; weekIndex < records.size(); ++weekIndex)
        {
            const Models::WeeklyRecord& record{ records[weekIndex] };
            m_view->ShowWeekLabel(weekIndex, std::format(WEEK_LABEL_FORMAT, record.GetWeekNumber()));

            // 캐릭터별 저장된 점수 (기록이 없으면 0)
            for (std::size_t characterIndex{}; characterIndex < m_columnCharacterNames.size(); ++characterIndex)
            {
                m_view->ShowScore(weekIndex, characterIndex, record.GetScore(m_columnCharacterNames[characterIndex]));
            }
        }

        UpdateStatCells();
        UpdateChart();
        UpdateOverview();
    }

    // 통계 칸만 다시 계산 (첫 주차는 증감률 없음, 원본 UpdateStatCells)
    void TrackerManager::UpdateStatCells()
    {
        const std::vector<Models::WeeklyRecord>& records{ GetCurrentSeason().GetRecords() };
        if (records.empty()) return;

        const double firstTotal{ m_calculator->GetTotal(records[FIRST_WEEK_INDEX]) };
        double previousTotal{};
        for (std::size_t weekIndex{}; weekIndex < records.size(); ++weekIndex)
        {
            WeekStats stats;
            stats.m_total = m_calculator->GetTotal(records[weekIndex]);
            stats.m_average = std::round(m_calculator->GetAverage(records[weekIndex]));   // 원본 Math.Round(avg, 0)
            if (weekIndex > FIRST_WEEK_INDEX)
            {
                stats.m_previousRate = m_calculator->GetChangeRate(stats.m_total, previousTotal);
                stats.m_firstRate = m_calculator->GetChangeRate(stats.m_total, firstTotal);
            }

            m_view->ShowStats(weekIndex, stats);
            previousTotal = stats.m_total;
        }
    }

    // 지금 시즌의 주차별 원정대 평균 낙원력 (기록이 없는 주차는 건너뜀 → 선이 0까지 떨어지지 않음, 원본 UpdateChart)
    void TrackerManager::UpdateChart()
    {
        const Models::Season& season{ GetCurrentSeason() };

        std::vector<UI::ChartPoint> points;
        for (const Models::WeeklyRecord& record : season.GetRecords())
        {
            const double average{ m_calculator->GetAverage(record) };   // 표와 같은 계산식
            if (average <= UNRECORDED_SCORE) continue;

            points.push_back(UI::ChartPoint{ std::format(CHART_LABEL_FORMAT, record.GetWeekNumber()), average });
        }

        // 고른 기간(최근 4 · 8 · 12주)만 남김 (기록된 주차 기준, 전체면 그대로)
        if (const std::optional<std::size_t> weekLimit{ m_view->GetChartWeekLimit() }; weekLimit.has_value() && points.size() > *weekLimit)
        {
            points.erase(points.begin(), points.end() - static_cast<std::ptrdiff_t>(*weekLimit));
        }

        const std::wstring seasonName{ ToWide(season.GetDisplayName()) };
        std::wstring title{ points.empty() ? std::format(CHART_EMPTY_FORMAT, seasonName) : std::format(CHART_TITLE_FORMAT, seasonName) };
        m_view->SetChart(std::move(title), std::move(points));
    }

    // 통계 카드 4장 + 캐릭터별 목록 (지금 시즌에 저장된 기록에서만 계산 → 기록이 없으면 "-" · "기록 없음")
    void TrackerManager::UpdateOverview()
    {
        const Models::GrowthData& data{ m_repository->GetData() };
        const std::vector<Models::WeeklyRecord>& records{ GetCurrentSeason().GetRecords() };

        // 평균이 기록된 주차 번호 (오래된 것부터)
        std::vector<std::size_t> recorded;
        for (std::size_t weekIndex{}; weekIndex < records.size(); ++weekIndex)
        {
            if (m_calculator->GetAverage(records[weekIndex]) > UNRECORDED_SCORE) recorded.push_back(weekIndex);
        }

        // "+384,520" / "-1,200" 숫자 글자 (0 이하는 부호 없이 FormatNumber가 "-"를 붙임)
        const auto formatSigned{ [](double value, int decimals)
        {
            return std::wstring{ value > 0.0 ? PLUS_TEXT : std::wstring_view{} } + Core::NumberFormat::FormatNumber(value, decimals);
        } };
        // 증감 글자 색 (상승 청록 · 하락 분홍 · 변동 없음 회색)
        const auto pickColor{ [](double value)
        {
            if (value > 0.0) return Theme::SPARKLINE_UP_COLOR;
            return value < 0.0 ? Theme::SPARKLINE_DOWN_COLOR : Theme::MUTED_TEXT_COLOR;
        } };

        PageLayout::StatCard averageCard{ std::wstring{ CARD_AVERAGE_ICON }, Theme::ACCENT_COLOR, std::wstring{ CARD_AVERAGE_LABEL },
            std::wstring{ NO_VALUE_TEXT }, std::wstring{ NO_RECORD_TEXT }, Theme::MUTED_TEXT_COLOR };
        PageLayout::StatCard growthCard{ std::wstring{ CARD_GROWTH_ICON }, Theme::GEM_CHART_LINE_COLOR, std::wstring{ CARD_GROWTH_LABEL },
            std::wstring{ NO_VALUE_TEXT }, std::wstring{ NO_RECORD_TEXT }, Theme::MUTED_TEXT_COLOR };
        PageLayout::StatCard countCard{ std::wstring{ CARD_COUNT_ICON }, Theme::HIGHLIGHT_TEXT_COLOR, std::wstring{ CARD_COUNT_LABEL },
            std::format(CARD_COUNT_FORMAT, data.GetCharacters().size()), std::format(CARD_RECORDED_FORMAT, recorded.size()), Theme::MUTED_TEXT_COLOR };
        PageLayout::StatCard bestCard{ std::wstring{ CARD_BEST_ICON }, Theme::DOWN_TEXT_COLOR, std::wstring{ CARD_BEST_LABEL },
            std::wstring{ NO_VALUE_TEXT }, std::wstring{ NO_RECORD_TEXT }, Theme::MUTED_TEXT_COLOR };

        if (!recorded.empty())
        {
            const Models::WeeklyRecord& latest{ records[recorded.back()] };
            const double latestAverage{ std::round(m_calculator->GetAverage(latest)) };
            averageCard.m_value = Core::NumberFormat::FormatNumber(latestAverage, AVERAGE_DECIMALS);
            averageCard.m_sub = std::format(CARD_WEEK_FORMAT, latest.GetWeekNumber());

            // 시즌 최고 평균 (가장 높은 주차)
            std::size_t bestIndex{ recorded.front() };
            for (const std::size_t weekIndex : recorded)
            {
                if (m_calculator->GetAverage(records[weekIndex]) > m_calculator->GetAverage(records[bestIndex])) bestIndex = weekIndex;
            }
            bestCard.m_value = Core::NumberFormat::FormatNumber(std::round(m_calculator->GetAverage(records[bestIndex])), AVERAGE_DECIMALS);
            bestCard.m_sub = std::format(CARD_WEEK_FORMAT, records[bestIndex].GetWeekNumber());

            if (recorded.size() >= 2)   // 직전 기록된 주차와 비교
            {
                const Models::WeeklyRecord& previous{ records[recorded[recorded.size() - 2]] };
                const double rate{ m_calculator->GetChangeRate(m_calculator->GetAverage(latest), m_calculator->GetAverage(previous)) };
                averageCard.m_sub = formatSigned(rate, CARD_RATE_DECIMALS) + std::wstring{ PERCENT_TEXT };
                averageCard.m_subColor = pickColor(rate);

                const double totalChange{ m_calculator->GetTotal(latest) - m_calculator->GetTotal(previous) };
                growthCard.m_value = formatSigned(totalChange, AVERAGE_DECIMALS);
                growthCard.m_sub = std::format(CARD_VS_PREVIOUS_FORMAT, latest.GetWeekNumber());
                growthCard.m_subColor = pickColor(totalChange);
            }
            else
            {
                growthCard.m_sub = std::wstring{ NO_RECORD_TEXT };   // 비교할 지난 기록이 아직 없음
            }
        }
        m_view->SetStatCards(std::vector<PageLayout::StatCard>{ std::move(averageCard), std::move(growthCard), std::move(countCard), std::move(bestCard) });

        // 캐릭터별 목록: 현재 원정대 + 이 시즌에 기록이 있는 원정대 제외 캐릭터 (m_columnCharacterNames 순서)
        const std::vector<Models::CharacterInfo>& roster{ data.GetCharacters() };
        std::vector<RosterEntry> entries;
        entries.reserve(m_columnCharacterNames.size());
        for (std::size_t index{}; index < m_columnCharacterNames.size(); ++index)
        {
            const std::string& name{ m_columnCharacterNames[index] };

            RosterEntry entry;
            entry.m_name = ToWide(name);
            if (index < roster.size())   // 현재 원정대: 직업 · 레벨
            {
                const std::optional<std::string>& className{ roster[index].GetCharacterClassName() };
                const std::wstring level{ Core::NumberFormat::FormatNumber(roster[index].GetNumericItemLevel(), CARD_RATE_DECIMALS) };
                entry.m_detail = className.has_value() && !className->empty() ? std::format(ROSTER_DETAIL_FORMAT, ToWide(*className), level) : std::format(L"Lv.{}", level);
            }
            else
            {
                entry.m_detail = std::wstring{ ROSTER_PAST_TEXT };
            }

            // 기록된(0보다 큰) 낙원력만 오래된 순으로 → 최근 값 · 직전 대비 · 추이 선
            for (const Models::WeeklyRecord& record : records)
            {
                if (const double score{ record.GetScore(name) }; score > UNRECORDED_SCORE) entry.m_history.push_back(score);
            }
            if (!entry.m_history.empty()) entry.m_score = entry.m_history.back();
            if (entry.m_history.size() >= 2)
            {
                entry.m_changeRate = m_calculator->GetChangeRate(entry.m_history.back(), entry.m_history[entry.m_history.size() - 2]);
            }
            if (entry.m_history.size() > ROSTER_TREND_WEEKS)
            {
                entry.m_history.erase(entry.m_history.begin(), entry.m_history.end() - static_cast<std::ptrdiff_t>(ROSTER_TREND_WEEKS));
            }
            entries.push_back(std::move(entry));
        }
        std::ranges::stable_sort(entries, std::ranges::greater{}, &RosterEntry::m_score);   // 낙원력 높은 순
        m_view->SetRoster(std::move(entries));
    }

    // 저장 실패 시 안내 (화면의 값은 유지 → 다음 저장 때 다시 시도, 원본 TrySave)
    void TrackerManager::TrySave()
    {
        try
        {
            m_repository->Save();
        }
        catch (const std::exception& error)
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWide(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }

    // 1분마다: 로스트아크 날짜(06시 기준)가 바뀌었을 때만 다시 적용 (수요일 06시 = 주차 전환)
    void TrackerManager::CheckSchedule()
    {
        if (!m_view || !GetCurrentSeason().HasPeriod()) return;
        if (m_lastGameDay == Core::LocalTime::GetGameToday()) return;

        m_view->CommitEdit();   // 편집 중이던 값은 지금 시각 기준으로 받거나(이번 주) 마감 안내(지난 주)
        ApplyWeekSchedule(false);
    }

    void TrackerManager::ApplyWeekSchedule(bool isFocusRequested)
    {
        if (!m_view) return;

        Models::Season& season{ GetCurrentSeason() };
        m_view->SetPeriodMode(season.HasPeriod());

        // 기간 없는 시즌 (시즌 4 이하): 잠금 · 안내 없음 (지금처럼 [주차 추가] · 자유 입력)
        if (!season.HasPeriod())
        {
            m_view->SetWeekLock(std::nullopt, std::nullopt);
            m_view->SetScheduleText({});
            m_lastWeekIndex.reset();
            m_unlockedWeek.reset();
            return;
        }

        const std::chrono::year_month_day today{ Core::LocalTime::GetGameToday() };
        m_lastGameDay = today;
        const std::optional<std::size_t> currentWeek{ season.FindWeekIndex(today) };
        const bool isBeforeStart{ std::chrono::sys_days{ today } < std::chrono::sys_days{ *season.GetStartDate() } };

        // 주차가 바뀌면 [지난 주차 수정]으로 푼 주차는 다시 잠금
        const bool isWeekChanged{ currentWeek != m_lastWeekIndex };
        if (isWeekChanged) m_unlockedWeek.reset();
        m_lastWeekIndex = currentWeek;

        // 이번 주차까지 행 자동 추가 (시즌이 끝났으면 기간의 모든 주차, 시작 전이면 없음)
        std::size_t neededWeeks{};
        if (currentWeek.has_value()) neededWeeks = *currentWeek + NEXT_WEEK_STEP;
        else if (!isBeforeStart) neededWeeks = season.GetPeriodWeekCount();

        if (season.GetRecords().size() < neededWeeks)
        {
            while (season.GetRecords().size() < neededWeeks) season.AddWeek();
            TrySave();
            PopulateGridData();
        }

        // 잠금: 이번 주차 + 잠깐 푼 주차만 입력 가능
        std::vector<bool> editableWeeks(season.GetRecords().size(), false);
        if (currentWeek.has_value() && *currentWeek < editableWeeks.size()) editableWeeks[*currentWeek] = true;
        if (m_unlockedWeek.has_value() && *m_unlockedWeek < editableWeeks.size()) editableWeeks[*m_unlockedWeek] = true;
        m_view->SetWeekLock(std::move(editableWeeks), currentWeek);

        // 안내 줄: 시작 전 / N주차 입력 중 · 마감 / 종료 (+ 수정 중인 지난 주차)
        const std::wstring seasonName{ ToWide(season.GetDisplayName()) };
        std::wstring scheduleText;
        if (isBeforeStart)
        {
            scheduleText = std::format(SCHEDULE_BEFORE_FORMAT, seasonName, ToDateLabel(*season.GetStartDate()));
        }
        else if (currentWeek.has_value())
        {
            // 마감 = 다음 주차 시작일(수요일) 오전 6시 → 화면에는 "05:59"
            const std::optional<std::chrono::year_month_day> deadline{ season.GetWeekStartDate(*currentWeek + NEXT_WEEK_STEP) };
            scheduleText = std::format(SCHEDULE_CURRENT_FORMAT, *currentWeek + NEXT_WEEK_STEP, deadline.has_value() ? ToDateLabel(*deadline) : std::wstring{});
        }
        else
        {
            scheduleText = std::format(SCHEDULE_ENDED_FORMAT, seasonName);
        }
        if (m_unlockedWeek.has_value() && *m_unlockedWeek < season.GetRecords().size())
        {
            scheduleText += std::format(SCHEDULE_UNLOCKED_FORMAT, season.GetRecords()[*m_unlockedWeek].GetWeekNumber());
        }
        m_view->SetScheduleText(std::move(scheduleText));

        // 이번 주차로 포커스 (처음 · 시즌 전환 · 새 주차 시작)
        if (currentWeek.has_value() && (isFocusRequested || isWeekChanged)) m_view->FocusWeek(*currentWeek);
    }

    // 화면 인식 낙원력 → 이번 주차 칸 (줄어든 값은 오인식으로 보고 무시, 기록은 손 입력과 같은 EditScore 흐름)
    ScanApplyResult TrackerManager::ApplyScannedScore(const std::string& characterName, double score)
    {
        if (!m_repository) return ScanApplyResult::SCAN_NO_WEEK;                                   // 아직 초기화 전
        const auto column = std::ranges::find(m_columnCharacterNames, characterName);              // 캐릭터 열
        if (column == m_columnCharacterNames.end()) return ScanApplyResult::SCAN_NO_CHARACTER;     // 트래커에 없는 캐릭터
        const std::size_t characterIndex = static_cast<std::size_t>(column - m_columnCharacterNames.begin());   // 열 번호

        Models::Season& season{ GetCurrentSeason() };                                              // 고른 시즌
        std::vector<Models::WeeklyRecord>& records{ season.GetRecords() };                         // 주차 기록
        std::optional<std::size_t> weekIndex{ FindThisWeekIndex(season) };                         // 이번 주차 (날짜 기준, 표의 마지막 칸이 아님)
        if (!weekIndex && !season.HasPeriod() && !records.empty()) weekIndex = records.size() - 1; // 게임 시즌 기간 밖 + 기간 없는 시즌: 마지막 주차
        if (weekIndex && *weekIndex >= records.size() && !season.HasPeriod())                      // 기간 없는 시즌에 이번 주차 칸이 아직 없음 → 늘림
        {
            while (records.size() <= *weekIndex) season.AddWeek();                                 // 이번 주차까지
            PopulateGridData();                                                                     // 표 다시 채우기
        }
        if (!weekIndex || *weekIndex >= records.size() || !IsWeekEditable(*weekIndex)) return ScanApplyResult::SCAN_NO_WEEK;   // 기록할 주차 없음 (경계 검사)

        const double recorded{ records[*weekIndex].GetScore(characterName) };                      // 지금 기록
        if (recorded == score) return ScanApplyResult::SCAN_UNCHANGED;                             // 같은 값
        if (recorded > 0.0 && score < recorded) return ScanApplyResult::SCAN_DECREASED;            // 줄어든 값 (낙원력은 주중에 줄지 않음)
        EditScore(*weekIndex, characterIndex, score);                                              // 기록 · 통계 · 저장
        return ScanApplyResult::SCAN_APPLIED;                                                      // 반영함
    }

    // 인게임 오버레이 낙원력 패널: 이번 주차의 지금 원정대 기록 (높은 순, 미기록은 뒤)
    TrackerOverlaySummary TrackerManager::GetOverlaySummary()
    {
        TrackerOverlaySummary summary;                                                          // 결과
        if (!m_repository) return summary;                                                      // 초기화 전
        Models::Season& season{ GetCurrentSeason() };                                          // 고른 시즌
        const std::vector<Models::WeeklyRecord>& records{ season.GetRecords() };                // 주차 기록
        std::optional<std::size_t> weekIndex{ FindThisWeekIndex(season) };                      // 이번 주차 (날짜 기준)
        if (!weekIndex && !season.HasPeriod() && !records.empty()) weekIndex = records.size() - 1;   // 게임 시즌 기간 밖: 마지막 주차
        if (!weekIndex || *weekIndex >= records.size()) return summary;                         // 이번 주차 없음 (경계 검사)

        summary.m_weekLabel = std::format(OVERLAY_WEEK_FORMAT, *weekIndex + 1);                 // "N주차"
        for (const Models::CharacterInfo& character : m_repository->GetData().GetCharacters())  // 지금 원정대
        {
            const double score{ records[*weekIndex].GetScore(character.GetCharacterName()) };  // 이번 주 기록
            TrackerOverlayRow row{ ToWide(character.GetCharacterName()) };                      // 이름
            if (score > 0.0) { row.m_score = score; ++summary.m_recordedCount; }                // 기록 있음
            summary.m_rows.push_back(std::move(row));                                           // 추가
        }
        std::ranges::stable_sort(summary.m_rows, std::ranges::greater{},
            [](const TrackerOverlayRow& row) { return row.m_score.value_or(-1.0); });           // 높은 순 (미기록은 뒤)
        return summary;   // 요약
    }

    bool TrackerManager::IsWeekEditable(std::size_t weekIndex) const
    {
        const Models::Season& season{ m_repository->GetData().GetCurrentSeason() };
        if (!season.HasPeriod()) return true;
        if (m_unlockedWeek == weekIndex) return true;
        return season.FindWeekIndex(Core::LocalTime::GetGameToday()) == weekIndex;   // 지금 시각 기준 이번 주차만
    }

    Models::Season& TrackerManager::GetCurrentSeason()
    {
        return m_repository->GetData().GetCurrentSeason();   // 저장소가 EnsureValid로 시즌 1개 이상 보장
    }
}
