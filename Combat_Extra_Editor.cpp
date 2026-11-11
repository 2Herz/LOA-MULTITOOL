module LostArkMultiTool.Views.CombatExtraEditor;  // 추가 장착 편집기 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.NumberFormat;  // 숫자 서식 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // 글자 인코딩 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 가져오기
import LostArkMultiTool.Views.CombatSimulatorView;  // 전투력 시뮬레이터 화면 모듈 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = Models::CombatCoefficientData;  // 계수 데이터 네임스페이스 별칭

        // ===== 글자 =====
        constexpr std::wstring_view NONE_LABEL{ L"없음" };  // 옵션 없음 표시 글자
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };  // 공백으로 보는 글자들 (반각 · 전각 공백)
        constexpr std::wstring_view NUMBER_FORMAT{ L"{}" };  // 정수 서식
        constexpr std::wstring_view MAIN_STAT_FORMAT{ L"{:.0f}" };  // 주스탯 서식 (소수점 없음)
        constexpr std::wstring_view BOOK_FORMAT{ L"{} X{}" };               // "유물 X2"
        constexpr std::wstring_view RANK_FORMAT{ L"{}랭크" };  // 랭크 서식
        constexpr std::wstring_view LEGEND_NAME{ L"전설" };  // 전설 등급 이름
        constexpr std::wstring_view RELIC_NAME{ L"유물" };  // 유물 등급 이름
        constexpr std::wstring_view EPIC_NAME{ L"영웅" };  // 영웅 등급 이름
        constexpr std::wstring_view ANCIENT_NAME{ L"고대" };  // 고대 등급 이름
        constexpr std::size_t BOOK_LEVEL_COUNT{ static_cast<std::size_t>(Data::ENGRAVING_BOOK_MAX_LEVEL) + 1 };   // 각인서 레벨 0 ~ 4
        constexpr std::wstring_view ACCESSORY_STAT_FORMAT{ L"{} 주스탯" };  // 악세서리 주스탯 행 이름 서식
        constexpr std::wstring_view ACCESSORY_OPTION_FORMAT{ L"{} 연마 {}" };  // 악세서리 연마 옵션 행 이름 서식
        constexpr std::wstring_view BRACELET_STAT_HEADER{ L"팔찌 스탯" };  // 팔찌 스탯 구분 제목
        constexpr std::wstring_view BRACELET_OPTION_HEADER{ L"팔찌 옵션 (공백 없는 문구)" };  // 팔찌 옵션 구분 제목
        constexpr std::wstring_view BRACELET_STAT_TYPE_FORMAT{ L"스탯 {} 종류" };  // 팔찌 스탯 종류 행 이름 서식
        constexpr std::wstring_view BRACELET_STAT_VALUE_FORMAT{ L"스탯 {} 수치" };  // 팔찌 스탯 수치 행 이름 서식
        constexpr std::wstring_view BRACELET_OPTION_FORMAT{ L"옵션 {}" };  // 팔찌 옵션 행 이름 서식
        constexpr std::wstring_view STONE_HEADER{ L"어빌리티 스톤 (각인 두 줄)" };  // 스톤 구분 제목
        constexpr std::wstring_view STONE_NAME_FORMAT{ L"스톤 각인 {}" };  // 스톤 각인 이름 행 서식
        constexpr std::wstring_view STONE_LEVEL_FORMAT{ L"스톤 각인 {} 레벨" };  // 스톤 각인 레벨 행 서식
        constexpr std::wstring_view BOOK_HEADER{ L"각인서 (유물 각인 5개)" };  // 각인서 구분 제목
        constexpr std::wstring_view BOOK_NAME_FORMAT{ L"각인서 {}" };  // 각인서 이름 행 서식
        constexpr std::wstring_view BOOK_LEVEL_FORMAT{ L"각인서 {} 등급 · 레벨" };  // 각인서 등급 · 레벨 행 서식
        constexpr std::wstring_view CORE_HEADER{ L"코어 (등급 · 의지력 포인트 합)" };  // 코어 구분 제목
        constexpr std::wstring_view CORE_GRADE_FORMAT{ L"{} 등급" };  // 코어 등급 행 서식
        constexpr std::wstring_view CORE_POINT_FORMAT{ L"{} 포인트" };  // 코어 포인트 행 서식
        constexpr std::wstring_view GEM_EFFECT_HEADER{ L"젬 효과 레벨 합 (장착한 젬 전체)" };  // 젬 효과 구분 제목
        constexpr std::wstring_view PASSIVE_HEADER{ L"아크 패시브" };  // 아크 패시브 구분 제목
        constexpr std::wstring_view RANK_LABEL{ L"랭크" };  // 랭크 행 이름
        constexpr std::wstring_view ENLIGHTENMENT_LABEL{ L"깨달음 포인트 구성" };  // 깨달음 포인트 구성 행 이름
        constexpr std::wstring_view AVATAR_HEADER{ L"아바타 (장착 중인 것)" };  // 아바타 구분 제목
        constexpr std::wstring_view VAMBRACE_HEADER{ L"완갑 (한계 해방)" };  // 완갑 구분 제목
        constexpr std::wstring_view VAMBRACE_GRADE_LABEL{ L"완갑 등급" };  // 완갑 등급 행 이름 (재련 단계는 장비 표에서 입력)

        // 악세서리 슬롯 이름 · 종류 (목걸이, 귀걸이 1 · 2, 반지 1 · 2)
        constexpr std::array<std::wstring_view, Models::ACCESSORY_SLOT_COUNT> ACCESSORY_NAMES{ L"목걸이", L"귀걸이 1", L"귀걸이 2", L"반지 1", L"반지 2" };  // 악세서리 슬롯 이름 표
        constexpr std::array<Data::AccessoryKind, Models::ACCESSORY_SLOT_COUNT> ACCESSORY_KINDS{  // 악세서리 슬롯 종류 표 (후보 옵션 선택용)
            Data::AccessoryKind::KIND_NECKLACE, Data::AccessoryKind::KIND_EARRING, Data::AccessoryKind::KIND_EARRING,  // 목걸이와 귀걸이 두 개
            Data::AccessoryKind::KIND_RING, Data::AccessoryKind::KIND_RING };  // 반지 두 개

        constexpr std::array<std::wstring_view, Models::ARK_CORE_COUNT> CORE_NAMES{  // 아크 코어 이름 표
            L"질서의 해 코어", L"질서의 달 코어", L"질서의 별 코어", L"혼돈의 해 코어", L"혼돈의 달 코어", L"혼돈의 별 코어" };
        constexpr std::array<std::wstring_view, Models::ARK_GEM_EFFECT_COUNT> GEM_EFFECT_NAMES{ L"공격력 레벨 합", L"추가 피해 레벨 합", L"보스 피해 레벨 합" };  // 젬 효과 행 이름 표
        constexpr std::array<std::wstring_view, Models::PASSIVE_TREE_COUNT> PASSIVE_LEVEL_NAMES{ L"진화 레벨 (효과 없음)", L"깨달음 레벨", L"도약 레벨" };  // 패시브 트리 레벨 행 이름 표
        constexpr std::array<std::wstring_view, Models::AVATAR_SLOT_COUNT> AVATAR_NAMES{ L"무기 아바타", L"머리 아바타", L"상의 아바타", L"하의 아바타" };  // 아바타 부위 행 이름 표

        // 아바타 · 코어 등급 선택지 (열거형 순서와 같음)
        constexpr std::array<std::wstring_view, 3> AVATAR_CHOICES{ NONE_LABEL, EPIC_NAME, LEGEND_NAME };  // 아바타 등급 선택지
        constexpr std::array<std::wstring_view, 4> CORE_GRADE_CHOICES{ EPIC_NAME, LEGEND_NAME, RELIC_NAME, ANCIENT_NAME };  // 코어 등급 선택지
        constexpr std::array<std::wstring_view, Models::VAMBRACE_GRADE_COUNT> VAMBRACE_GRADE_CHOICES{ EPIC_NAME, LEGEND_NAME, RELIC_NAME, ANCIENT_NAME };  // 완갑 등급 선택지 (VambraceGrade 순서)

        // UTF-8 → 화면 글자 (변환할 수 없으면 빈 글자)
        [[nodiscard]] std::wstring Wide(std::string_view text)  // UTF-8을 화면 글자로 변환
        {
            try  // 변환 시도
            {
                return Core::TextEncoding::UTF8ToWide(text);  // 화면 글자로 변환해 반환
            }
            catch (const std::exception&)  // 변환 실패 시
            {
                return std::wstring{};  // 빈 글자 반환
            }
        }

        [[nodiscard]] std::string Narrow(const std::wstring& text)  // 화면 글자를 UTF-8로 변환
        {
            try  // 변환 시도
            {
                return Core::TextEncoding::WideToUTF8(text);  // UTF-8로 변환해 반환
            }
            catch (const std::exception&)  // 변환 실패 시
            {
                return std::string{};  // 빈 글자 반환
            }
        }

        // 옵션 글자를 화면용으로 (비었으면 "없음")
        [[nodiscard]] std::wstring Display(std::string_view text)  // 옵션 글자를 화면용으로 변환
        {
            return Models::IsNone(text) ? std::wstring{ NONE_LABEL } : Wide(text);  // 비었으면 "없음", 아니면 변환한 글자
        }

        [[nodiscard]] bool IsBlank(const std::wstring& text) noexcept  // 글자가 공백뿐인지 검사
        {
            return text.find_first_not_of(TRIM_CHARACTERS) == std::wstring::npos;  // 공백이 아닌 글자가 없으면 true
        }

        [[nodiscard]] std::vector<std::wstring> ToWideList(std::span<const std::string_view> list)  // 문자열 목록을 화면 글자 목록으로 변환
        {
            std::vector<std::wstring> items;  // 결과 목록
            items.reserve(list.size());  // 개수만큼 메모리 미리 확보
            for (const std::string_view item : list) items.push_back(Display(item));  // 각 항목을 화면용으로 변환해 추가
            return items;  // 변환한 목록 반환
        }

        template <std::size_t Count>  // 고정 크기 글자 배열을 목록으로 변환하는 템플릿
        [[nodiscard]] std::vector<std::wstring> ToWideList(const std::array<std::wstring_view, Count>& list)  // 배열 크기를 템플릿 인자로 받음
        {
            return std::vector<std::wstring>(list.begin(), list.end());  // 배열 내용으로 목록 생성
        }

        // 각인서 이름이 두 칸 이상 같은지 ("없음"은 제외)
        [[nodiscard]] bool HasDuplicateBook(const Models::ExtraState& state)  // 각인서 이름 중복 검사
        {
            for (std::size_t first{}; first < Models::BOOK_COUNT; ++first)  // 첫 번째 칸 순회
            {
                if (Models::IsNone(state.m_books[first].m_name)) continue;  // 이름이 없는 칸은 건너뜀
                for (std::size_t second{ first + 1 }; second < Models::BOOK_COUNT; ++second)  // 뒤의 칸과 비교
                {
                    if (state.m_books[first].m_name == state.m_books[second].m_name) return true;  // 같은 이름이 있으면 중복
                }
            }
            return false;  // 중복이 없음
        }

        // ===== 행 명세 만들기 =====
        // 접근 함수는 generic lambda: 상태 참조를 받아 해당 칸 참조를 돌려줌 (const 상태면 const 참조)
        // 구분 제목 행
        template <typename Spec>  // 구분 제목 행 추가 템플릿
        void AddHeader(std::vector<Spec>& specs, std::wstring_view label)  // 구분 제목 행 추가
        {
            Spec spec;  // 새 행 명세
            spec.m_label = std::wstring{ label };  // 행 이름 지정
            spec.m_kind = CombatRowKind::ROW_HEADER;  // 구분 행 종류 지정
            specs.push_back(std::move(spec));  // 명세 목록에 추가
        }

        // 문구 선택 행 (선택지는 표의 목록, 현재 값이 목록에 없으면 "없음" 다음에 끼움)
        template <typename Spec, typename Accessor>  // 문구 선택 행 템플릿
        void AddTextChoice(std::vector<Spec>& specs, std::wstring_view label, Accessor accessor, std::span<const std::string_view> candidates)  // 문구 선택 행 추가
        {
            Spec spec;  // 새 행 명세
            spec.m_label = std::wstring{ label };  // 행 이름 지정
            spec.m_kind = CombatRowKind::ROW_CHOICE;  // 선택 행 종류 지정
            spec.m_read = [accessor](const Models::ExtraState& state) { return Display(accessor(state)); };  // 표시 글자 읽기 함수
            spec.m_choices = [accessor, candidates](const Models::ExtraState& state)  // 선택지 목록 함수
            {
                std::vector<std::wstring> items{ ToWideList(candidates) };  // 표 후보 목록을 화면 글자로 변환
                const std::wstring currentText{ Display(accessor(state)) };  // 현재 값의 화면 글자
                if (std::ranges::find(items, currentText) == items.end()) items.insert(items.empty() ? items.begin() : items.begin() + 1, currentText);  // 현재 값이 목록에 없으면 "없음" 다음에 끼움
                return items;  // 선택지 목록 반환
            };
            spec.m_write = [accessor](Models::ExtraState& state, const std::wstring& text) { accessor(state) = Narrow(text); };  // 선택한 글자를 UTF-8로 바꿔 상태에 반영
            specs.push_back(std::move(spec));  // 명세 목록에 추가
        }

        // 숫자 행 (정수 또는 실수 칸, 범위 맞춤은 CombatExtra::Clamp가 함)
        template <typename Spec, typename Accessor, typename Number>  // 숫자 행 템플릿
        void AddNumber(std::vector<Spec>& specs, std::wstring_view label, Accessor accessor, std::type_identity<Number>)  // 숫자 행 추가
        {
            Spec spec;  // 새 행 명세
            spec.m_label = std::wstring{ label };  // 행 이름 지정
            spec.m_kind = CombatRowKind::ROW_NUMBER;  // 숫자 행 종류 지정
            spec.m_read = [accessor](const Models::ExtraState& state)  // 표시 글자 읽기 함수
            {
                if constexpr (std::is_floating_point_v<Number>) return std::format(MAIN_STAT_FORMAT, static_cast<double>(accessor(state)));  // 실수 칸이면 소수점 없는 서식으로 표시
                else return std::format(NUMBER_FORMAT, accessor(state));  // 정수 칸이면 기본 서식으로 표시
            };
            spec.m_write = [accessor](Models::ExtraState& state, const std::wstring& text)  // 입력 반영 함수
            {
                if (IsBlank(text)) return;   // 비우면 그대로
                accessor(state) = static_cast<Number>(Core::NumberFormat::ParseNumber(text));  // 입력 글자를 숫자로 변환해 반영
            };
            specs.push_back(std::move(spec));  // 명세 목록에 추가
        }

        // 번호로 고르는 행 (선택지 글자 목록 + 현재 번호 읽기 / 쓰기)
        template <typename Spec, typename Getter, typename Setter>  // 번호 선택 행 템플릿
        void AddIndexChoice(std::vector<Spec>& specs, std::wstring_view label, std::vector<std::wstring> items, Getter getter, Setter setter)  // 번호 선택 행 추가
        {
            Spec spec;  // 새 행 명세
            spec.m_label = std::wstring{ label };  // 행 이름 지정
            spec.m_kind = CombatRowKind::ROW_CHOICE;  // 선택 행 종류 지정
            spec.m_read = [items, getter](const Models::ExtraState& state)  // 표시 글자 읽기 함수
            {
                const std::size_t index{ getter(state) };  // 현재 번호
                return index < items.size() ? items[index] : std::wstring{};  // 범위 안이면 해당 선택지, 아니면 빈 글자
            };
            spec.m_choices = [items](const Models::ExtraState&) { return items; };  // 선택지 목록 함수 (항상 같은 목록)
            spec.m_write = [items, setter](Models::ExtraState& state, const std::wstring& text)  // 입력 반영 함수
            {
                const auto found{ std::ranges::find(items, text) };  // 선택한 글자의 위치 찾기
                if (found != items.end()) setter(state, static_cast<std::size_t>(std::distance(items.begin(), found)));  // 찾았으면 번호로 변환해 쓰기 함수 호출
            };
            specs.push_back(std::move(spec));  // 명세 목록에 추가
        }
    }

    CombatExtraEditor::CombatExtraEditor()  // 생성자: 카드별 행 명세 만들기
    {
        BuildAccessoryRows();  // 악세서리 카드 행 만들기
        BuildBraceletRows();  // 팔찌 · 스톤 · 각인서 카드 행 만들기
        BuildArkGridRows();  // 아크 그리드 카드 행 만들기
        BuildPassiveRows();  // 아크 패시브 · 아바타 카드 행 만들기
    }

    CombatExtraEditor::~CombatExtraEditor() = default;  // 소멸자 기본 구현

    // 악세서리 5종: 슬롯마다 주스탯(숫자) + 연마 옵션 3줄(선택)
    void CombatExtraEditor::BuildAccessoryRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_ACCESSORY)] };  // 악세서리 카드의 명세 목록
        for (std::size_t slot{}; slot < Models::ACCESSORY_SLOT_COUNT; ++slot)  // 악세서리 슬롯 순회
        {
            AddNumber(specs, std::format(ACCESSORY_STAT_FORMAT, ACCESSORY_NAMES[slot]),  // 주스탯 숫자 행 추가
                [slot](auto& state) -> auto& { return state.m_accessories[slot].m_mainStat; }, std::type_identity<double>{});  // 슬롯의 주스탯 칸 접근 함수 (실수형)
            for (std::size_t option{}; option < Models::ACCESSORY_OPTION_COUNT; ++option)  // 연마 옵션 줄 순회
            {
                AddTextChoice(specs, std::format(ACCESSORY_OPTION_FORMAT, ACCESSORY_NAMES[slot], option + 1),  // 연마 옵션 선택 행 추가
                    [slot, option](auto& state) -> auto& { return state.m_accessories[slot].m_options[option]; }, Data::GetAccessoryCandidates(ACCESSORY_KINDS[slot]));  // 슬롯 · 옵션 번호의 칸 접근 함수와 종류별 후보 목록
            }
        }
    }

    // 팔찌 스탯 · 옵션, 어빌리티 스톤 두 줄, 각인서 5개
    void CombatExtraEditor::BuildBraceletRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_BRACELET)] };  // 팔찌 카드의 명세 목록

        AddHeader(specs, BRACELET_STAT_HEADER);  // 팔찌 스탯 구분 제목 추가
        for (std::size_t line{}; line < Models::BRACELET_STAT_COUNT; ++line)  // 팔찌 스탯 줄 순회
        {
            AddTextChoice(specs, std::format(BRACELET_STAT_TYPE_FORMAT, line + 1),  // 스탯 종류 선택 행 추가
                [line](auto& state) -> auto& { return state.m_braceletStats[line].m_type; }, Data::GetBraceletStatTypes());  // 종류 칸 접근 함수와 종류 후보
            AddNumber(specs, std::format(BRACELET_STAT_VALUE_FORMAT, line + 1),  // 스탯 수치 숫자 행 추가
                [line](auto& state) -> auto& { return state.m_braceletStats[line].m_value; }, std::type_identity<int>{});  // 수치 칸 접근 함수 (정수형)
        }

        AddHeader(specs, BRACELET_OPTION_HEADER);  // 팔찌 옵션 구분 제목 추가
        for (std::size_t line{}; line < Models::BRACELET_OPTION_COUNT; ++line)  // 팔찌 옵션 줄 순회
        {
            AddTextChoice(specs, std::format(BRACELET_OPTION_FORMAT, line + 1),  // 옵션 선택 행 추가
                [line](auto& state) -> auto& { return state.m_braceletOptions[line]; }, Data::GetBraceletOptionCandidates());  // 옵션 칸 접근 함수와 옵션 후보
        }

        AddHeader(specs, STONE_HEADER);  // 스톤 구분 제목 추가
        for (std::size_t line{}; line < Models::STONE_LINE_COUNT; ++line)  // 스톤 각인 줄 순회
        {
            AddTextChoice(specs, std::format(STONE_NAME_FORMAT, line + 1),  // 각인 이름 선택 행 추가
                [line](auto& state) -> auto& { return state.m_stone[line].m_name; }, Data::GetStoneCandidates());  // 이름 칸 접근 함수와 각인 후보
            AddNumber(specs, std::format(STONE_LEVEL_FORMAT, line + 1),  // 각인 레벨 숫자 행 추가
                [line](auto& state) -> auto& { return state.m_stone[line].m_level; }, std::type_identity<int>{});  // 레벨 칸 접근 함수 (정수형)
        }

        AddHeader(specs, BOOK_HEADER);  // 각인서 구분 제목 추가
        std::vector<std::wstring> gradeLevels;   // "유물 X0" ... "유물 X4", "전설 X0" ... "전설 X4"
        for (const std::wstring_view grade : { RELIC_NAME, LEGEND_NAME })  // 등급 · 레벨 조합 글자를 만들 등급 순회
        {
            for (std::size_t level{}; level < BOOK_LEVEL_COUNT; ++level) gradeLevels.push_back(std::format(BOOK_FORMAT, grade, level));  // 등급 · 레벨마다 조합 글자 추가
        }
        for (std::size_t book{}; book < Models::BOOK_COUNT; ++book)  // 각인서 5개 순회
        {
            AddTextChoice(specs, std::format(BOOK_NAME_FORMAT, book + 1),  // 각인서 이름 선택 행 추가
                [book](auto& state) -> auto& { return state.m_books[book].m_name; }, Data::GetBookCandidates());  // 이름 칸 접근 함수와 각인서 후보
            AddIndexChoice(specs, std::format(BOOK_LEVEL_FORMAT, book + 1), gradeLevels,  // 등급 · 레벨 조합 선택 행 추가
                [book](const Models::ExtraState& state)  // 현재 조합 번호를 구하는 함수
                {
                    const Models::BookState& item{ state.m_books[book] };  // 현재 각인서
                    return (item.m_isLegendary ? BOOK_LEVEL_COUNT : 0) + static_cast<std::size_t>(std::clamp(item.m_level, Models::MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL));  // 전설이면 유물 구간 뒤로 밀어 번호 계산
                },
                [book](Models::ExtraState& state, std::size_t index)  // 조합 번호를 상태에 쓰는 함수
                {
                    state.m_books[book].m_isLegendary = index >= BOOK_LEVEL_COUNT;  // 번호가 유물 구간 이상이면 전설로 표시
                    state.m_books[book].m_level = static_cast<int>(index % BOOK_LEVEL_COUNT);  // 레벨은 구간 안 나머지
                });
        }
    }

    // 아크 그리드: 코어 6개 (등급 선택 + 포인트 숫자) + 젬 효과 레벨 합 3개
    void CombatExtraEditor::BuildArkGridRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_ARK_GRID)] };  // 아크 그리드 카드의 명세 목록

        AddHeader(specs, CORE_HEADER);  // 코어 구분 제목 추가
        for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core)  // 코어 6개 순회
        {
            AddIndexChoice(specs, std::format(CORE_GRADE_FORMAT, CORE_NAMES[core]), ToWideList(CORE_GRADE_CHOICES),  // 코어 등급 선택 행 추가
                [core](const Models::ExtraState& state) { return static_cast<std::size_t>(state.m_cores[core].m_grade); },  // 현재 등급 번호 읽기 함수
                [core](Models::ExtraState& state, std::size_t index) { state.m_cores[core].m_grade = static_cast<Models::ArkCoreGrade>(index); });  // 등급 번호 쓰기 함수
            AddNumber(specs, std::format(CORE_POINT_FORMAT, CORE_NAMES[core]),  // 코어 포인트 숫자 행 추가
                [core](auto& state) -> auto& { return state.m_cores[core].m_points; }, std::type_identity<int>{});  // 포인트 칸 접근 함수 (정수형)
        }

        AddHeader(specs, GEM_EFFECT_HEADER);  // 젬 효과 구분 제목 추가
        for (std::size_t effect{}; effect < Models::ARK_GEM_EFFECT_COUNT; ++effect)  // 젬 효과 종류 순회
        {
            AddNumber(specs, GEM_EFFECT_NAMES[effect], [effect](auto& state) -> auto& { return state.m_arkGemLevels[effect]; }, std::type_identity<int>{});  // 효과별 레벨 합 숫자 행 추가
        }
    }

    // 아크 패시브 (랭크 · 깨달음 포인트 구성 · 트리 레벨) + 아바타 4부위
    void CombatExtraEditor::BuildPassiveRows()
    {
        std::vector<RowSpec>& specs{ m_specs[static_cast<std::size_t>(CombatCard::CARD_PASSIVE)] };  // 아크 패시브 카드의 명세 목록

        AddHeader(specs, PASSIVE_HEADER);  // 아크 패시브 구분 제목 추가
        std::vector<std::wstring> ranks;  // 랭크 선택지 글자 목록
        for (int rank{ Models::MIN_VALUE }; rank <= Models::MAX_PASSIVE_RANK; ++rank) ranks.push_back(std::format(RANK_FORMAT, rank));  // 0 ~ 최대 랭크 글자 만들기
        AddIndexChoice(specs, RANK_LABEL, std::move(ranks),  // 랭크 선택 행 추가
            [](const Models::ExtraState& state) { return static_cast<std::size_t>(std::clamp(state.m_passive.m_rank, Models::MIN_VALUE, Models::MAX_PASSIVE_RANK)); },  // 현재 랭크 번호 읽기 함수
            [](Models::ExtraState& state, std::size_t index) { state.m_passive.m_rank = static_cast<int>(index); });  // 랭크 번호 쓰기 함수

        // 깨달음 포인트 구성: 배율이 큰 구성부터
        std::vector<Data::KeyedValue> enlightenments{ Data::GetEnlightenmentRatios().begin(), Data::GetEnlightenmentRatios().end() };  // 깨달음 구성 표 복사
        std::ranges::sort(enlightenments, std::greater<>{}, &Data::KeyedValue::m_value);  // 배율이 큰 구성부터 정렬
        std::vector<std::wstring> enlightenmentItems;  // 깨달음 선택지 글자 목록
        for (const Data::KeyedValue& item : enlightenments) enlightenmentItems.push_back(Wide(item.m_key));  // 구성 키를 화면 글자로 변환해 추가
        AddIndexChoice(specs, ENLIGHTENMENT_LABEL, enlightenmentItems,  // 깨달음 구성 선택 행 추가
            [enlightenmentItems](const Models::ExtraState& state)  // 현재 구성 번호 읽기 함수
            {
                const auto found{ std::ranges::find(enlightenmentItems, Wide(state.m_passive.m_enlightenment)) };  // 현재 구성의 위치 찾기
                return static_cast<std::size_t>(std::distance(enlightenmentItems.begin(), found));   // 목록에 없으면 끝(빈 글자)
            },
            [enlightenmentItems](Models::ExtraState& state, std::size_t index)
            {
                if (index < enlightenmentItems.size()) state.m_passive.m_enlightenment = Narrow(enlightenmentItems[index]);  // 번호가 범위 안이면 구성 글자를 상태에 반영
            });

        for (std::size_t tree{}; tree < Models::PASSIVE_TREE_COUNT; ++tree)  // 패시브 트리 순회
        {
            AddNumber(specs, PASSIVE_LEVEL_NAMES[tree], [tree](auto& state) -> auto& { return state.m_passive.m_levels[tree]; }, std::type_identity<int>{});  // 트리 레벨 숫자 행 추가
        }

        AddHeader(specs, AVATAR_HEADER);  // 아바타 구분 제목 추가
        for (std::size_t slot{}; slot < Models::AVATAR_SLOT_COUNT; ++slot)  // 아바타 부위 순회
        {
            AddIndexChoice(specs, AVATAR_NAMES[slot], ToWideList(AVATAR_CHOICES),  // 아바타 등급 선택 행 추가
                [slot](const Models::ExtraState& state) { return static_cast<std::size_t>(state.m_avatars[slot]); },  // 현재 등급 번호 읽기 함수
                [slot](Models::ExtraState& state, std::size_t index) { state.m_avatars[slot] = static_cast<Models::AvatarGrade>(index); });  // 등급 번호 쓰기 함수
        }

        // 완갑 등급: 고르면 재련 단계를 그 등급의 범위 안으로 맞춤 (예: +17에서 고대 → +20)
        AddHeader(specs, VAMBRACE_HEADER);  // 완갑 구분 제목 추가
        AddIndexChoice(specs, VAMBRACE_GRADE_LABEL, ToWideList(VAMBRACE_GRADE_CHOICES),  // 완갑 등급 선택 행 추가
            [](const Models::ExtraState& state)  // 현재 등급 번호 읽기 함수 (완갑이 없으면 범위 밖 = 빈 글자)
            {
                if (!state.m_vambraceLevel.has_value()) return Models::VAMBRACE_GRADE_COUNT;  // 완갑이 없으면 빈 글자
                return static_cast<std::size_t>(state.m_vambraceGrade.value_or(Models::CombatExtra::GetVambraceGradeForLevel(*state.m_vambraceLevel)));  // 등급이 없으면 단계의 기본 등급
            },
            [](Models::ExtraState& state, std::size_t index)  // 등급 번호 쓰기 함수
            {
                if (!state.m_vambraceLevel.has_value() || index >= Models::VAMBRACE_GRADE_COUNT) return;  // 완갑이 없거나 번호가 범위 밖이면 무시
                state.m_vambraceGrade = static_cast<Models::VambraceGrade>(index);  // 고른 등급 기록
                state.m_vambraceLevel = std::clamp(*state.m_vambraceLevel, Models::VAMBRACE_GRADE_MIN_LEVELS[index], Models::VAMBRACE_GRADE_MAX_LEVELS[index]);  // 단계를 등급 범위 안으로 맞춤
            });
    }

    std::vector<CombatListRow> CombatExtraEditor::BuildRows(CombatCard card, const Models::ExtraState& current, const Models::ExtraState& target) const  // 카드의 화면 행 만들기
    {
        std::vector<CombatListRow> rows;  // 결과 행 목록
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };  // 카드 번호
        if (cardIndex >= m_specs.size()) return rows;  // 범위 밖 카드면 빈 목록 반환

        rows.reserve(m_specs[cardIndex].size());  // 행 개수만큼 메모리 미리 확보
        for (const RowSpec& spec : m_specs[cardIndex])  // 카드의 행 명세 순회
        {
            CombatListRow& row{ rows.emplace_back() };  // 새 화면 행 추가
            row.m_label = spec.m_label;  // 행 이름 복사
            row.m_kind = spec.m_kind;  // 편집 방식 복사
            if (spec.m_kind == CombatRowKind::ROW_HEADER) continue;  // 구분 행은 값이 없으므로 다음으로

            row.m_current = spec.m_read(current);  // 현재 값 글자
            row.m_target = spec.m_read(target);  // 목표 값 글자
            row.m_hasChange = row.m_current != row.m_target;  // 현재와 목표가 다르면 바뀐 행으로 표시
            if (spec.m_kind != CombatRowKind::ROW_CHOICE) continue;  // 선택 행이 아니면 다음으로

            row.m_choices = spec.m_choices(current);  // 선택지 목록
            auto found{ std::ranges::find(row.m_choices, row.m_target) };  // 목표 글자가 선택지에서 어느 위치인지 찾기
            if (found == row.m_choices.end() && !row.m_target.empty())  // 목표가 선택지에 없고 비어 있지 않으면
            {
                // 목표가 선택지에 없으면(저장 파일 등) 끼워서 고를 수 있게
                found = row.m_choices.insert(row.m_choices.empty() ? row.m_choices.begin() : row.m_choices.begin() + 1, row.m_target);  // 선택지에 끼워 넣음
            }
            if (found != row.m_choices.end()) row.m_selectedChoice = static_cast<std::size_t>(std::distance(row.m_choices.begin(), found));  // 선택된 번호 기록
        }
        return rows;  // 완성한 행 목록 반환
    }

    bool CombatExtraEditor::Apply(CombatCard card, std::size_t row, const std::wstring& text, Models::ExtraState& target) const  // 사용자가 고친 값을 목표에 반영
    {
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };  // 카드 번호
        if (cardIndex >= m_specs.size() || row >= m_specs[cardIndex].size()) return false;  // 카드나 행 번호가 범위 밖이면 실패

        const RowSpec& spec{ m_specs[cardIndex][row] };  // 해당 행 명세
        if (spec.m_kind == CombatRowKind::ROW_HEADER || !spec.m_write) return false;  // 구분 행이거나 쓰기 함수가 없으면 실패

        // 각인서는 같은 이름을 두 칸에 쓸 수 없음 (게임 · lopec 모두 무시함) → 중복이 새로 생기면 반영하지 않음
        Models::ExtraState edited{ target };  // 복사본에 먼저 반영해 검사
        spec.m_write(edited, text);  // 입력 반영
        if (HasDuplicateBook(edited) && !HasDuplicateBook(target)) return false;  // 중복 각인서가 새로 생기면 반영하지 않음
        target = std::move(edited);  // 검사를 통과한 값으로 목표 교체
        return true;  // 반영 성공
    }
}
