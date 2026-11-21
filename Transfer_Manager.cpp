module LostArkMultiTool.Views.TransferManager;  // 2부 이관 매니저 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPExceptions;  // 앱 예외 모듈 가져오기
import LostArkMultiTool.Core.BackgroundTask;  // 백그라운드 작업 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.LocalTime;  // 현지 시간 모듈 가져오기
import LostArkMultiTool.Core.MarketAPI;  // 시장 API 모듈 가져오기
import LostArkMultiTool.Core.Notifier;  // 알림 모듈 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // 글자 인코딩 모듈 가져오기
import LostArkMultiTool.Core.ToolContext;  // 도구 문맥 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 가져오기 (역할 · 스톤)
import LostArkMultiTool.Models.RateLimitState;  // 호출 한도 상태 모델 가져오기
import LostArkMultiTool.Models.TransferData;  // 2부 이관 표 가져오기
import LostArkMultiTool.Services.TransferCalculator;  // 2부 이관 계산기 가져오기
import LostArkMultiTool.Views.TransferView;  // 2부 이관 화면 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = Models::TransferData;   // 이관 표 네임스페이스 줄임

        constexpr std::wstring_view TAB_TITLE{ L"2부 이관 계산기" };   // 탭 제목

        // ===== 알림 =====
        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };   // 알림 제목
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };    // 오류 제목
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };   // 호출 한도 제목
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };   // 서버 점검 제목
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 조회는 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };   // 호출 부족 안내
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };   // API 키 오류 제목
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };   // 키 재입력 질문
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };   // 이름 입력 요청
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };   // 알 수 없는 오류
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };   // 일반 공백 · 전각 공백

        // ===== 상태 · 요약 =====
        constexpr std::wstring_view SEARCHING_FORMAT{ L"'{}' 무기고 조회 중..." };   // 조회 중
        constexpr std::wstring_view NOT_SEARCHED_TEXT{ L"캐릭터 이름을 넣고 [조회]를 누르면 2부에서 받을 장비 · 주문서를 계산해요." };   // 조회 전
        constexpr std::wstring_view SEARCH_ERROR_FORMAT{ L"조회하지 못했어요: {}" };   // 조회 실패
        constexpr std::wstring_view SEARCH_DONE_FORMAT{ L"'{}' 이관 계산을 마쳤어요." };   // 조회 완료 토스트
        constexpr std::wstring_view STATUS_FORMAT{ L"{} · {} · {} · 아이템 레벨 {:.2f}" };   // 상태 줄
        constexpr std::wstring_view DEALER_TEXT{ L"딜러" };       // 딜러
        constexpr std::wstring_view SUPPORTER_TEXT{ L"서포터" };  // 서포터
        constexpr std::wstring_view LEVEL_FORMAT{ L"{:.2f}" };    // 평균 레벨
        constexpr std::wstring_view SCROLL_SUMMARY_FORMAT{ L"{}등급" };   // 최고 주문서
        constexpr std::wstring_view NO_SCROLL_TEXT{ L"대상 없음" };       // 주문서 대상 없음
        constexpr std::wstring_view GEM_SUMMARY_FORMAT{ L"성휘 2Lv {} · 1Lv {}" };   // 성휘 요약
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };   // 빈 칸

        // ===== 카드 글자 =====
        constexpr std::wstring_view NEW_SLOT_FORMAT{ L"{} (신규)" };      // 새 슬롯 이름
        constexpr std::wstring_view LEVEL_INT_FORMAT{ L"{}" };            // 정수 레벨
        constexpr std::wstring_view GEAR_FORMAT{ L"{} {}연마" };          // "고대 3연마"
        constexpr std::wstring_view ESTHER_FORMAT{ L"엘라 {}단계" };      // 에스더 이관
        constexpr std::wstring_view ESTHER_POLISH_FORMAT{ L"엘라 {}단계 {}연마" };   // 에스더 이관 (연마)
        constexpr std::wstring_view OUT_OF_TABLE_TEXT{ L"1600 이하 (공지 대기)" };   // 표 밖
        constexpr std::wstring_view REFINE_FORMAT{ L"+{}" };              // 슬롯 재련
        constexpr std::wstring_view REFINE_ADVANCED_FORMAT{ L"+{} · 상급 {}" };   // 슬롯 재련 + 상급
        constexpr std::wstring_view QUALITY_FORMAT{ L"{}" };              // 품질
        constexpr std::wstring_view SUPPORTER_BASIS_FORMAT{ L"{} (서폿)" };   // 서포터 기준 판정
        constexpr std::wstring_view SCROLL_GRADE_FORMAT{ L"{}등급" };     // 주문서 등급
        constexpr std::wstring_view BRACELET_HEADER_FORMAT{ L"팔찌 ({})" };   // 팔찌 구분 행
        constexpr std::wstring_view BRACELET_REASON_FORMAT{ L"팔찌 — {}" };   // 팔찌 대상 아님
        constexpr std::wstring_view BRACELET_TIER_COUNT_FORMAT{ L"1티어 옵션  딜러 {}개 · 서폿 {}개" };   // 1티어 개수
        constexpr std::wstring_view BRACELET_PENDING_TEXT{ L"인장 1 ~ 21등급 계산식은 11/25 공지 후" };   // 등급 대기
        constexpr std::wstring_view FIRST_TIER_TEXT{ L"1티어" };   // 1티어 글자 (강조 · 개수 셈)
        constexpr std::wstring_view GEM_HEADER_TEXT{ L"보석 → 광휘 · 성휘" };   // 보석 구분 행
        constexpr std::wstring_view GEM_COUNT_LABEL{ L"보석 수" };   // 보석 수
        constexpr std::wstring_view GEM_COUNT_FORMAT{ L"{}개" };     // 개수
        constexpr std::wstring_view GEM_LUMINOUS_FORMAT{ L"광휘 {}개 (거래소 거래)" };   // 광휘
        constexpr std::wstring_view GEM_LEVELS_LABEL{ L"레벨 구성" };   // 레벨 구성
        constexpr std::wstring_view GEM_LEVEL_ITEM_FORMAT{ L"{}Lv×{}" };   // "10Lv×5"
        constexpr std::wstring_view GEM_LEVEL_SEPARATOR{ L" · " };   // 구분
        constexpr std::wstring_view GEM_SAME_LEVEL_TEXT{ L"같은 레벨 광휘" };   // 레벨 유지
        constexpr std::wstring_view GEM_TEN_LABEL{ L"10레벨 → 성휘" };   // 성휘 행
        constexpr std::wstring_view GEM_STAR_FORMAT{ L"2Lv {} · 1Lv {} · 남는 10Lv {}" };   // 성휘 결과
        constexpr std::wstring_view STONE_HEADER_TEXT{ L"어빌리티 스톤 → 각인 업그레이드" };   // 스톤 구분 행
        constexpr std::wstring_view STONE_LINE_FORMAT{ L"각인 {}" };       // 스톤 줄 이름
        constexpr std::wstring_view STONE_ENGRAVING_FORMAT{ L"{} Lv.{}" }; // 스톤 각인
        constexpr std::wstring_view STONE_VALID_TEXT{ L"유효 각인" };      // 유효
        constexpr std::wstring_view STONE_INVALID_TEXT{ L"유효 각인 아님 (계산 제외)" };   // 무효
        constexpr std::wstring_view STONE_POINT_LABEL{ L"활성화 포인트" }; // 포인트 행
        constexpr std::wstring_view STONE_POINT_FORMAT{ L"{}+{}" };        // "4+3"
        constexpr std::wstring_view STONE_NONE_TEXT{ L"스톤 없음" };       // 스톤 없음
        constexpr std::wstring_view STONE_OUT_TEXT{ L"표 밖 (공지 대기)" }; // 표 밖
        constexpr std::wstring_view ESTHER_LABEL{ L"에스더 무기" };        // 에스더 행
        constexpr std::wstring_view ESTHER_CURRENT_FORMAT{ L"+{} 엘라 {} ({})" };   // 에스더 지금
        constexpr std::wstring_view ESTHER_TARGET_FORMAT{ L"{} ({}) · {}" };       // 에스더 이관 + 환급
        constexpr std::wstring_view ESTHER_TARGET_NO_REFUND_FORMAT{ L"{} ({})" };  // 에스더 이관 (환급 없음)
        constexpr std::wstring_view ESTHER_NONE_TEXT{ L"없음" };           // 에스더 없음
        constexpr std::wstring_view WEAPON_BASIS_LABEL{ L"무기 기준 레벨" };   // 무기 기준 행
        constexpr std::wstring_view WEAPON_BASIS_TEXT{ L"완갑 · 허리띠 · 성물 · 인장 · 팔찌" };   // 무기 기준 부위

        // 그 밖 카드의 고정 행 (항목 · 지금 · 2부)
        struct FixedRow
        {
            std::wstring_view m_label;     // 항목
            std::wstring_view m_current;   // 지금
            std::wstring_view m_target;    // 2부
        };
        constexpr std::array<FixedRow, 6> OTHER_FIXED_ROWS{ {
            { L"품질 · 카르마", L"시스템 삭제", L"마력 주입 재료로 환산 (양은 공지 대기)" },
            { L"상급 재련", L"-", L"처리 방식 공지 대기" },
            { L"아크 패시브", L"진화 · 깨달음 · 도약", L"깨달음 중심 (진화 · 도약 삭제)" },
            { L"특성", L"6종", L"치명 · 특화 · 신속 · 기량 4종" },
            { L"룬", L"장착 중", L"전부 삭제 (일부는 보석 추가 효과)" },
            { L"파편 · 돌파석 · 특수 재료", L"보유", L"2부 재료 토큰 (귀속 그대로)" },
        } };

        constexpr std::wstring_view NOTE_TEXT{ L"※ {}. 16부위 평균은 1600 이하 부위를 0으로 계산해요. 장신구 세 번째 칸은 공격력+ · 무기 공격력+ 만 인정(추정)하고, 팔찌 인장 등급 · 품질/카르마 환산 · 상급 재련은 공지 후 반영해요." };   // 맨 아래 안내

        // API 부위 이름 → 화면 이름 ("어깨" → "견갑")
        constexpr std::string_view API_SHOULDER{ "어깨" };   // API 견갑 이름
        constexpr std::wstring_view SHOULDER_TEXT{ L"견갑" }; // 화면 견갑 이름

        // UTF-8 → 화면 글자 (변환할 수 없으면 기본 문구)
        [[nodiscard]] std::wstring ToWide(std::string_view text)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(text);   // 변환
            }
            catch (const std::exception&)   // 변환 실패
            {
                return std::wstring{ UNKNOWN_ERROR_TEXT };   // 기본 문구
            }
        }

        [[nodiscard]] std::wstring_view TrimSpaces(std::wstring_view text) noexcept   // 양 끝 공백 제거
        {
            const std::size_t first{ text.find_first_not_of(TRIM_CHARACTERS) };   // 첫 글자
            if (first == std::wstring_view::npos) return {};   // 전부 공백
            const std::size_t last{ text.find_last_not_of(TRIM_CHARACTERS) };     // 마지막 글자
            return text.substr(first, last - first + 1);   // 공백을 뺀 부분
        }

        [[nodiscard]] std::wstring PartText(std::string_view apiPart)   // 부위 화면 이름
        {
            return apiPart == API_SHOULDER ? std::wstring{ SHOULDER_TEXT } : ToWide(apiPart);   // 어깨 → 견갑
        }

        // 칸 만들기 도우미
        [[nodiscard]] TransferCell Cell(std::wstring text, TransferTone tone = TransferTone::TONE_NORMAL, std::string gradeName = {})
        {
            return TransferCell{ std::move(text), tone, std::move(gradeName) };   // 칸
        }

        // 슬롯의 "이관 장비" 칸 (에스더 · 일반 · 표 밖)
        [[nodiscard]] TransferCell TargetGearCell(const Services::TransferSlotResult& slot)
        {
            if (slot.m_esther.has_value())   // 에스더 무기
            {
                const std::wstring text{ slot.m_esther->m_polish > 0 ? std::format(ESTHER_POLISH_FORMAT, slot.m_esther->m_elaStage, slot.m_esther->m_polish)
                                                                     : std::format(ESTHER_FORMAT, slot.m_esther->m_elaStage) };   // "엘라 4단계 5연마"
                return Cell(text, TransferTone::TONE_GRADE, std::string{ Data::ESTHER_GRADE });   // 에스더 색
            }
            if (!slot.m_target.has_value()) return Cell(std::wstring{ OUT_OF_TABLE_TEXT }, TransferTone::TONE_MUTED);   // 표 밖
            return Cell(std::format(GEAR_FORMAT, ToWide(slot.m_target->m_grade), slot.m_target->m_polish), TransferTone::TONE_GRADE, std::string{ slot.m_target->m_grade });   // "고대 3연마"
        }

        // 슬롯의 "이관 Lv" 칸 (오른 만큼 함께)
        [[nodiscard]] TransferCell TargetLevelCell(const Services::TransferSlotResult& slot)
        {
            const int level{ slot.GetTargetLevel() };   // 이관 레벨
            if (level <= 0) return Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED);   // 표 밖
            return Cell(std::format(LEVEL_INT_FORMAT, level), TransferTone::TONE_HIGHLIGHT);   // "1815" (지금 Lv 칸과 나란히 보면 오른 만큼이 보임)
        }

        // 기준 레벨 칸
        [[nodiscard]] TransferCell SourceLevelCell(int level)
        {
            return level > 0 ? Cell(std::format(LEVEL_INT_FORMAT, level)) : Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED);   // 모르면 "-"
        }

        // 슬롯 이름 칸 (새 슬롯은 "(신규)")
        [[nodiscard]] TransferCell SlotNameCell(const Services::TransferSlotResult& slot)
        {
            const std::wstring name{ ToWide(slot.m_slotName) };   // 슬롯 이름
            return Cell(slot.m_isNewSlot ? std::format(NEW_SLOT_FORMAT, name) : name, TransferTone::TONE_LABEL);   // 이름 칸
        }

        // 팔찌 티어 칸 (1티어 강조, 없으면 "-")
        [[nodiscard]] TransferCell TierCell(std::string_view tier)
        {
            if (tier.empty()) return Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED);   // 점수 없음
            const std::wstring text{ ToWide(tier) };   // 티어 글자
            return Cell(text, text == FIRST_TIER_TEXT ? TransferTone::TONE_HIGHLIGHT : TransferTone::TONE_NORMAL);   // 1티어 강조
        }

        // 구분 제목 행 (첫 칸에만 글자)
        [[nodiscard]] TransferRow HeaderRow(std::wstring text, std::size_t columnCount)
        {
            TransferRow row(columnCount, Cell({}, TransferTone::TONE_HEADER));   // 모든 칸 = 구분 모양
            if (!row.empty()) row.front().m_text = std::move(text);   // 첫 칸 글자
            return row;   // 행
        }

        constexpr std::size_t THREE_COLUMNS{ 3 };   // 항목 · 지금 · 2부 카드 열 수
    }

    TransferManager::TransferManager() = default;    // 생성자 (싱글톤)
    TransferManager::~TransferManager() = default;   // 소멸자

    std::wstring_view TransferManager::GetTabTitle() const noexcept  // 탭 제목
    {
        return TAB_TITLE;  // 탭 제목 반환
    }

    void TransferManager::Initialize(Core::ToolContext& context)  // 초기화
    {
        m_context = &context;   // 도구 문맥 보관
        m_calculator = std::make_unique<Services::TransferCalculator>();   // 계산기
        m_view = std::make_unique<TransferView>(context.m_platform);       // 화면
        m_view->SetOnSearch([this](const std::wstring& name) { RequestSearch(name); });   // 조회 알림 연결
        m_view->SetNote(std::format(NOTE_TEXT, ToWide(Data::GetSourceText())));   // 맨 아래 안내 (출처 포함)
        RefreshAll();   // 처음 화면
    }

    void TransferManager::Shutdown()  // 종료
    {
        m_armoryTask.Wait();     // 진행 중인 조회가 끝날 때까지 대기 (공용 API를 지우기 전에)
        m_view.reset();          // 화면 해제
        m_calculator.reset();    // 계산기 해제
        m_context = nullptr;     // 문맥 해제
    }

    void TransferManager::OnActivated()  // 탭이 선택됨
    {
        if (m_view) m_view->RestoreFocus();  // 포커스 복원
    }

    void TransferManager::OnDeactivated()  // 탭 선택이 풀림
    {
        if (m_view) m_view->SuspendFocus();  // 포커스 해제
    }

    void TransferManager::Update(double deltaSeconds)  // 프레임 갱신
    {
        if (!m_view) return;   // 화면이 없으면 중단
        m_view->Update(deltaSeconds);   // 화면 갱신
        if (m_armoryTask.IsReady()) FinishSearch();   // 조회가 끝났으면 결과 처리
    }

    void TransferManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)  // 화면 그리기
    {
        if (m_view) m_view->Render(renderer, contentArea);  // 화면이 있으면 그리기
    }

    bool TransferManager::HandleInput(const Core::InputEvent& event)  // 입력 처리
    {
        return m_view && m_view->HandleInput(event);  // 화면에 전달
    }

    Core::CursorShape TransferManager::GetCursor() const noexcept  // 커서 모양
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;  // 화면이 없으면 화살표
    }

    // ===================== 조회 =====================

    void TransferManager::RequestSearch(const std::wstring& characterName)  // 무기고 조회 요청
    {
        const std::wstring_view trimmedName{ TrimSpaces(characterName) };   // 양 끝 공백을 뺀 이름
        if (trimmedName.empty())   // 이름이 비었으면
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);   // 안내
            return;   // 조회 안 함
        }
        if (m_armoryTask.IsRunning()) return;   // 이미 조회 중

        // 남은 호출이 안전 여유분 이하면 건너뜀 (다른 탭과 키를 함께 쓰므로)
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };   // 현재 시각
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };   // 호출 한도
        if (rate.IsExhausted(now))   // 부족
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE },
                std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))), Core::NoticeKind::NOTICE_INFO, nullptr);   // 안내
            return;   // 조회 안 함
        }

        m_searchError.reset();   // 이전 오류 지움
        m_view->SetSearchBusy(true);   // 조회 중 표시
        m_view->SetStatusText(std::format(SEARCHING_FORMAT, trimmedName), false);   // 조회 중 글자

        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };   // 시장 API (백그라운드에서 사용)
        m_armoryTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetArmory(name); });   // 백그라운드 조회
    }

    void TransferManager::FinishSearch()  // 조회가 끝남 → 계산 · 오류 처리
    {
        m_view->SetSearchBusy(false);   // 조회 중 표시 해제
        std::optional<std::string> keyError;   // API 키 오류 메시지

        try
        {
            Models::CharacterArmory armory{ m_armoryTask.TakeResult() };   // 백그라운드 예외는 여기서 다시 던져짐
            m_result = m_calculator->Calculate(armory);   // 이관 계산
            m_armory = std::move(armory);                 // 무기고 보관
            m_context->m_notifier.ShowToast(std::format(SEARCH_DONE_FORMAT, ToWide(m_armory->m_characterName)), Core::NoticeKind::NOTICE_SUCCESS);   // 완료 토스트
        }
        catch (const Core::APIKeyInvalidException& error)   // API 키 오류
        {
            keyError = error.what();   // 키 오류 보관
            m_searchError = ToWide(error.what());   // 상태 줄 오류
        }
        catch (const Core::APIRateLimitException& error)   // 호출 한도 초과
        {
            m_searchError = ToWide(error.what());   // 상태 줄 오류
            m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_WARNING, nullptr);   // 경고
        }
        catch (const Core::APIMaintenanceException& error)   // 서버 점검
        {
            m_searchError = ToWide(error.what());   // 상태 줄 오류
            m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_INFO, nullptr);   // 안내
        }
        catch (const std::exception& error)   // 없는 캐릭터 · 통신 오류 등
        {
            m_searchError = ToWide(error.what());   // 상태 줄 오류
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_ERROR, nullptr);   // 오류 알림
        }

        RefreshAll();   // 화면 갱신
        if (keyError.has_value()) HandleAPIKeyError(*keyError);   // 화면 갱신 뒤 키 재입력 질문
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (다른 탭과 같음)
    void TransferManager::HandleAPIKeyError(const std::string& message)
    {
        const std::wstring reason{ ToWide(message) };   // 오류 이유
        if (!m_context->m_platform.IsWindowShown())   // 창이 숨어 있으면
        {
            m_context->m_notifier.ShowToast(reason, Core::NoticeKind::NOTICE_ERROR);   // 풍선 알림
            return;   // 질문 안 함
        }
        m_context->m_notifier.AskYesNo(std::wstring{ API_KEY_ERROR_TITLE }, reason + std::wstring{ API_KEY_RETRY_TEXT },
            [openAPIKeyDialog = m_context->m_openAPIKeyDialog](bool isYes)   // 예 → 키 입력창
        {
            if (isYes && openAPIKeyDialog) openAPIKeyDialog();   // 입력창 열기
        });
    }

    // ===================== 화면 갱신 =====================

    void TransferManager::RefreshAll()  // 전체 갱신
    {
        RefreshStatus();     // 상태 줄
        RefreshSummary();    // 요약
        RefreshArmor();      // 장비
        RefreshAccessory();  // 장신구 슬롯
        RefreshScrolls();    // 장신구 주문서
        RefreshBracelet();   // 팔찌
        RefreshGemStone();   // 보석 · 스톤
        RefreshOther();      // 그 밖
    }

    void TransferManager::RefreshStatus()  // 상태 줄
    {
        if (m_searchError.has_value()) { m_view->SetStatusText(std::format(SEARCH_ERROR_FORMAT, *m_searchError), true); return; }   // 오류
        if (!m_armory.has_value()) { m_view->SetStatusText(std::wstring{ NOT_SEARCHED_TEXT }, false); return; }   // 조회 전
        const bool isSupporter{ m_result.has_value() && m_result->m_isSupporter };   // 서포터 여부
        m_view->SetStatusText(std::format(STATUS_FORMAT, ToWide(m_armory->m_characterName), ToWide(m_armory->m_className),
            isSupporter ? SUPPORTER_TEXT : DEALER_TEXT, m_armory->m_itemAvgLevel), false);   // "이름 · 직업 · 역할 · 레벨"
    }

    void TransferManager::RefreshSummary()  // 요약 큰 글자
    {
        TransferSummaryText summary;   // 요약
        if (m_result.has_value())   // 결과가 있으면
        {
            summary.m_currentLevel = std::format(LEVEL_FORMAT, m_result->m_currentAverageLevel);     // 지금 평균
            summary.m_transferLevel = std::format(LEVEL_FORMAT, m_result->m_transferAverageLevel);   // 이관 뒤 평균
            summary.m_scroll = m_result->m_bestScrollGrade.has_value() ? std::format(SCROLL_SUMMARY_FORMAT, *m_result->m_bestScrollGrade) : std::wstring{ NO_SCROLL_TEXT };   // 최고 주문서
            summary.m_gem = std::format(GEM_SUMMARY_FORMAT, m_result->m_gems.m_starTwoCount, m_result->m_gems.m_starOneCount);   // 성휘
        }
        else   // 조회 전
        {
            summary.m_currentLevel = summary.m_transferLevel = summary.m_scroll = summary.m_gem = std::wstring{ EMPTY_TEXT };   // 모두 "-"
        }
        m_view->ShowSummary(summary);   // 화면 반영
    }

    void TransferManager::RefreshArmor()  // 장비 카드
    {
        std::vector<TransferRow> rows;   // 행 목록
        if (m_result.has_value())   // 결과가 있으면
        {
            for (const Services::TransferSlotResult& slot : m_result->m_armorSlots)   // 장비 쪽 슬롯마다
            {
                TransferRow row;   // 행
                row.push_back(SlotNameCell(slot));   // 2부 슬롯
                row.push_back(Cell(PartText(slot.m_sourceName)));   // 기준 부위
                row.push_back(SourceLevelCell(slot.m_sourceLevel)); // 지금 Lv
                row.push_back(TargetGearCell(slot));                // 이관 장비
                row.push_back(TargetLevelCell(slot));               // 이관 Lv
                if (!slot.m_slotRefine.has_value()) row.push_back(Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED));   // 슬롯 재련 모름
                else row.push_back(Cell(slot.m_advancedRefine > 0 ? std::format(REFINE_ADVANCED_FORMAT, *slot.m_slotRefine, slot.m_advancedRefine)
                                                                  : std::format(REFINE_FORMAT, *slot.m_slotRefine)));   // "+20 · 상급 40"
                rows.push_back(std::move(row));   // 행 추가
            }
        }
        m_view->ShowCard(TransferCard::CARD_ARMOR, rows);   // 화면 반영
    }

    void TransferManager::RefreshAccessory()  // 장신구 슬롯 카드
    {
        std::vector<TransferRow> rows;   // 행 목록
        if (m_result.has_value())   // 결과가 있으면
        {
            for (const Services::TransferSlotResult& slot : m_result->m_accessorySlots)   // 장신구 쪽 슬롯마다
            {
                rows.push_back(TransferRow{ SlotNameCell(slot), Cell(PartText(slot.m_sourceName)), SourceLevelCell(slot.m_sourceLevel),
                    TargetGearCell(slot), TargetLevelCell(slot) });   // 슬롯 · 기준 · 기준 Lv · 이관 장비 · 이관 Lv
            }
        }
        m_view->ShowCard(TransferCard::CARD_ACCESSORY, rows);   // 화면 반영
    }

    void TransferManager::RefreshScrolls()  // 장신구 주문서 카드
    {
        std::vector<TransferRow> rows;   // 행 목록
        if (m_result.has_value())   // 결과가 있으면
        {
            for (const Services::ScrollResult& scroll : m_result->m_scrolls)   // 장신구마다
            {
                TransferRow row;   // 행
                row.push_back(Cell(ToWide(scroll.m_slotName), TransferTone::TONE_LABEL));   // 부위
                row.push_back(scroll.m_itemName.empty() ? Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED)
                                                        : Cell(ToWide(scroll.m_itemName), TransferTone::TONE_GRADE, scroll.m_grade));   // 장신구 (등급 색)
                row.push_back(scroll.m_quality.has_value() ? Cell(std::format(QUALITY_FORMAT, *scroll.m_quality)) : Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED));   // 품질
                const std::wstring polish{ ToWide(scroll.m_polishText) };   // 연마 판정
                row.push_back(Cell(polish.empty() ? std::wstring{ EMPTY_TEXT } : (scroll.m_isSupporterBasis ? std::format(SUPPORTER_BASIS_FORMAT, polish) : polish)));   // 연마 (서폿 기준 표시)
                row.push_back(scroll.m_scrollGrade.has_value() ? Cell(std::format(SCROLL_GRADE_FORMAT, *scroll.m_scrollGrade), TransferTone::TONE_HIGHLIGHT)
                                                               : Cell(ToWide(scroll.m_reason), TransferTone::TONE_MUTED));   // 주문서 등급 · 이유
                rows.push_back(std::move(row));   // 행 추가
            }
        }
        m_view->ShowCard(TransferCard::CARD_SCROLL, rows);   // 화면 반영
    }

    void TransferManager::RefreshBracelet()  // 팔찌 카드
    {
        std::vector<TransferRow> rows;   // 행 목록
        if (m_result.has_value())   // 결과가 있으면
        {
            const Services::TransferResult& result{ *m_result };   // 결과
            rows.push_back(HeaderRow(result.m_braceletReason.empty() ? std::format(BRACELET_HEADER_FORMAT, ToWide(result.m_braceletGrade))
                                                                     : std::format(BRACELET_REASON_FORMAT, ToWide(result.m_braceletReason)), THREE_COLUMNS));   // 팔찌 구분 행

            int dealerFirst{};      // 딜러 1티어 개수
            int supporterFirst{};   // 서포터 1티어 개수
            for (const Services::BraceletLineResult& line : result.m_braceletLines)   // 줄마다
            {
                const std::wstring label{ line.m_label.empty() ? ToWide(line.m_text) : ToWide(line.m_label) };   // 분류 이름 (모르면 원문)
                rows.push_back(TransferRow{ Cell(label, TransferTone::TONE_LABEL), TierCell(line.m_dealerTier), TierCell(line.m_supporterTier) });   // 옵션 · 딜러 · 서폿
                if (ToWide(line.m_dealerTier) == FIRST_TIER_TEXT) ++dealerFirst;         // 딜러 1티어 셈
                if (ToWide(line.m_supporterTier) == FIRST_TIER_TEXT) ++supporterFirst;   // 서포터 1티어 셈
            }
            rows.push_back(HeaderRow(std::format(BRACELET_TIER_COUNT_FORMAT, dealerFirst, supporterFirst), THREE_COLUMNS));   // 1티어 개수
            rows.push_back(TransferRow{ Cell(std::wstring{ BRACELET_PENDING_TEXT }, TransferTone::TONE_MUTED), Cell({}, TransferTone::TONE_MUTED), Cell({}, TransferTone::TONE_MUTED) });   // 등급 대기 안내
        }
        m_view->ShowCard(TransferCard::CARD_BRACELET, rows);   // 화면 반영
    }

    void TransferManager::RefreshGemStone()  // 보석 · 스톤 카드
    {
        std::vector<TransferRow> rows;   // 행 목록
        if (m_result.has_value())   // 결과가 있으면
        {
            const Services::GemResult& gems{ m_result->m_gems };   // 보석 결과
            rows.push_back(HeaderRow(std::wstring{ GEM_HEADER_TEXT }, THREE_COLUMNS));   // 보석 구분 행
            rows.push_back(TransferRow{ Cell(std::wstring{ GEM_COUNT_LABEL }, TransferTone::TONE_LABEL), Cell(std::format(GEM_COUNT_FORMAT, gems.m_totalCount)),
                Cell(std::format(GEM_LUMINOUS_FORMAT, gems.m_totalCount), TransferTone::TONE_HIGHLIGHT) });   // 보석 수 → 광휘

            std::wstring levels;   // 레벨 구성 ("10Lv×5 · 9Lv×6")
            for (int level{ Data::MAX_GEM_LEVEL }; level >= 1; --level)   // 높은 레벨부터
            {
                const int count{ gems.m_levelCounts[static_cast<std::size_t>(level)] };   // 그 레벨 개수
                if (count <= 0) continue;   // 없으면 건너뜀
                if (!levels.empty()) levels += GEM_LEVEL_SEPARATOR;   // 구분
                levels += std::format(GEM_LEVEL_ITEM_FORMAT, level, count);   // "10Lv×5"
            }
            rows.push_back(TransferRow{ Cell(std::wstring{ GEM_LEVELS_LABEL }, TransferTone::TONE_LABEL), Cell(levels.empty() ? std::wstring{ EMPTY_TEXT } : levels),
                Cell(std::wstring{ GEM_SAME_LEVEL_TEXT }) });   // 레벨 구성
            rows.push_back(TransferRow{ Cell(std::wstring{ GEM_TEN_LABEL }, TransferTone::TONE_LABEL),
                Cell(std::format(GEM_COUNT_FORMAT, gems.m_levelCounts[static_cast<std::size_t>(Data::MAX_GEM_LEVEL)])),
                Cell(std::format(GEM_STAR_FORMAT, gems.m_starTwoCount, gems.m_starOneCount, gems.m_leftTenCount), TransferTone::TONE_HIGHLIGHT) });   // 10레벨 → 성휘

            const Services::StoneResult& stone{ m_result->m_stone };   // 스톤 결과
            rows.push_back(HeaderRow(std::wstring{ STONE_HEADER_TEXT }, THREE_COLUMNS));   // 스톤 구분 행
            if (!stone.m_hasStone)   // 스톤 없음
            {
                rows.push_back(TransferRow{ Cell(std::wstring{ STONE_NONE_TEXT }, TransferTone::TONE_MUTED), Cell({}, TransferTone::TONE_MUTED), Cell({}, TransferTone::TONE_MUTED) });   // 안내
            }
            else
            {
                for (std::size_t index{}; index < stone.m_lines.size(); ++index)   // 스톤 줄마다
                {
                    const Models::StoneLine& line{ stone.m_lines[index] };   // 스톤 줄
                    if (line.m_name.empty()) continue;   // 빈 줄
                    rows.push_back(TransferRow{ Cell(std::format(STONE_LINE_FORMAT, index + 1), TransferTone::TONE_LABEL),
                        Cell(std::format(STONE_ENGRAVING_FORMAT, ToWide(line.m_name), line.m_level)),
                        stone.m_isValid[index] ? Cell(std::wstring{ STONE_VALID_TEXT }) : Cell(std::wstring{ STONE_INVALID_TEXT }, TransferTone::TONE_MUTED) });   // 각인 · 유효 여부
                }
                rows.push_back(TransferRow{ Cell(std::wstring{ STONE_POINT_LABEL }, TransferTone::TONE_LABEL), Cell(std::format(STONE_POINT_FORMAT, stone.m_high, stone.m_low)),
                    stone.m_result.empty() ? Cell(std::wstring{ STONE_OUT_TEXT }, TransferTone::TONE_MUTED) : Cell(ToWide(stone.m_result), TransferTone::TONE_HIGHLIGHT) });   // 포인트 → 결과
            }
        }
        m_view->ShowCard(TransferCard::CARD_GEM_STONE, rows);   // 화면 반영
    }

    void TransferManager::RefreshOther()  // 에스더 · 그 밖 카드
    {
        std::vector<TransferRow> rows;   // 행 목록
        if (m_result.has_value() && m_armory.has_value())   // 결과가 있으면
        {
            const Services::TransferResult& result{ *m_result };   // 결과
            const auto weaponSlot{ std::ranges::find_if(result.m_armorSlots, [](const Services::TransferSlotResult& slot) { return slot.m_esther.has_value(); }) };   // 에스더 무기 슬롯
            if (weaponSlot != result.m_armorSlots.end())   // 에스더 무기를 끼고 있으면
            {
                const std::optional<Models::GearItem>& weapon{ m_armory->m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)] };   // 무기
                const int evolution{ weapon.has_value() ? weapon->m_enhancement : 0 };   // 진화 단계 (이름 앞 +N)
                const int ela{ weapon.has_value() ? weapon->m_elaStage : 0 };            // 엘라 단계
                const Data::EstherTransferRow& esther{ *weaponSlot->m_esther };          // 이관 행
                const std::wstring target{ TargetGearCell(*weaponSlot).m_text };         // "엘라 4단계 5연마"
                rows.push_back(TransferRow{ Cell(std::wstring{ ESTHER_LABEL }, TransferTone::TONE_LABEL),
                    Cell(std::format(ESTHER_CURRENT_FORMAT, evolution, ela, weaponSlot->m_sourceLevel), TransferTone::TONE_GRADE, std::string{ Data::ESTHER_GRADE }),
                    Cell(esther.m_refund.empty() ? std::format(ESTHER_TARGET_NO_REFUND_FORMAT, target, esther.m_level) : std::format(ESTHER_TARGET_FORMAT, target, esther.m_level, ToWide(esther.m_refund)),
                        TransferTone::TONE_HIGHLIGHT) });   // 에스더 → 엘라 단계
            }
            else
            {
                rows.push_back(TransferRow{ Cell(std::wstring{ ESTHER_LABEL }, TransferTone::TONE_LABEL), Cell(std::wstring{ ESTHER_NONE_TEXT }, TransferTone::TONE_MUTED), Cell(std::wstring{ EMPTY_TEXT }, TransferTone::TONE_MUTED) });   // 에스더 없음
            }
            rows.push_back(TransferRow{ Cell(std::wstring{ WEAPON_BASIS_LABEL }, TransferTone::TONE_LABEL), SourceLevelCell(result.m_weaponBasisLevel), Cell(std::wstring{ WEAPON_BASIS_TEXT }) });   // 무기 기준 레벨
        }
        for (const FixedRow& fixed : OTHER_FIXED_ROWS)   // 고정 안내 행 (조회 전에도 보임)
        {
            rows.push_back(TransferRow{ Cell(std::wstring{ fixed.m_label }, TransferTone::TONE_LABEL), Cell(std::wstring{ fixed.m_current }), Cell(std::wstring{ fixed.m_target }, TransferTone::TONE_MUTED) });   // 항목 · 지금 · 2부
        }
        m_view->ShowCard(TransferCard::CARD_OTHER, rows);   // 화면 반영
    }
}
