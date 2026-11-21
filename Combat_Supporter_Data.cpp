module LostArkMultiTool.Models.CombatSupporterData;  // 서포터 전투력 계수 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 딜러 계수 데이터 모듈 가져오기 (구조체 · 검색 함수)

namespace LostArkMultiTool::Models::CombatSupporterData  // 서포터 계수 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 영역 시작
    {
        namespace Data = CombatCoefficientData;  // 딜러 계수 데이터 네임스페이스 별칭

        // ===== 판별: 아크 패시브 칭호 (ArkPassive.Title) =====
        constexpr std::array<std::string_view, 4> SUPPORTER_TITLES{ {   // 서포터 칭호 4개
            "축복의 오라",   // 홀리나이트
            "만개",          // 도화가
            "절실한 구원",   // 바드
            "해방자",        // 발키리
        } };   // SUPPORTER_TITLES 끝

        // ===== 장비 재련: S0 · W0 보정 (투구 · 무기 재련 곡선 9명 맞춤) =====
        constexpr double MAIN_STAT_EXTRA{ 45000.0 };     // S0(보정 후)에 더하는 주스탯 (투구 재련 rmse 0.00007)
        constexpr double WEAPON_ATTACK_EXTRA{ 62000.0 }; // 무기 툴팁 공격력에 더하는 값 (무기 재련 rmse 0.0006, 딜러는 21500)

        // ===== 방어구 품질 (5부위 같은 곡선, 캐릭터별 0.0047 ~ 0.0081 → 중앙값) =====
        constexpr double ARMOR_QUALITY_AMPLITUDE{ 0.0055 };   // 품질 0일 때 부위 하나의 감소 비율 (F(0) = 1 - a)
        constexpr double MAX_QUALITY{ 100.0 };                // 최대 품질

        // ===== 완갑 재련 (+25 → 단계, 서포터 8명 중앙값, 단계마다 등급 변화 포함) =====
        constexpr std::array<double, 26> VAMBRACE_LEVEL_LN{   // 총 26칸
            -0.13584, -0.13211, -0.12856, -0.12484, -0.12126, -0.11743, -0.11374, -0.11017, -0.10647, -0.10288,   // 단계 0 ~ 9
            -0.09767, -0.08603, -0.08242, -0.07871, -0.07508, -0.06786, -0.0563, -0.05265, -0.0489, -0.04525,     // 단계 10 ~ 19
            -0.02765, -0.02392, -0.02025, -0.01641, -0.01272, 0.0,                                                // 단계 20 ~ 25
        };   // VAMBRACE_LEVEL_LN 끝

        // ===== 보석: 레벨 1 ~ 10 계수 (보석 한 칸 기준, 종류는 영향 없음) =====
        constexpr std::array<double, 10> GEM_FACTORS{   // 총 10칸
            0.890841, 0.90985, 0.920355, 0.931234, 0.942129, 0.953417, 0.964731, 0.976452, 0.988208, 1.0,   // 레벨 1 ~ 10
        };   // GEM_FACTORS 끝
        constexpr int GEM_FIRST_LEVEL{ 1 };              // GEM_FACTORS[0]의 보석 레벨
        constexpr double NO_GEM_VALUE{ 0.889218 };       // 보석이 없을 때의 계수 (76칸 측정)

        // ===== 악세서리 연마 옵션 (없음 대비 ln, 표에 없는 문구 = 딜러 옵션 등 효과 없음) =====
        constexpr std::array<Data::KeyedValue, 27> ACCESSORY_OPTIONS{ {   // 문구 키 27개
            Data::KeyedValue{ "낙인력 +8.00%", 0.043085 },   // 문구 → ln
            Data::KeyedValue{ "아군 피해량 강화 효과 +7.50%", 0.0338453 },   // 문구 → ln
            Data::KeyedValue{ "아군 공격력 강화 효과 +5.00%", 0.0338453 },   // 문구 → ln
            Data::KeyedValue{ "세레나데, 신앙, 조화 게이지 획득량 +6.00%", 0.027217 },   // 문구 → ln
            Data::KeyedValue{ "낙인력 +4.80%", 0.026106 },   // 문구 → ln
            Data::KeyedValue{ "아군 피해량 강화 효과 +4.50%", 0.020445 },   // 문구 → ln
            Data::KeyedValue{ "아군 공격력 강화 효과 +3.00%", 0.0198354 },   // 문구 → ln
            Data::KeyedValue{ "세레나데, 신앙, 조화 게이지 획득량 +3.60%", 0.0159079 },   // 문구 → ln
            Data::KeyedValue{ "무기 공격력 +3.00%", 0.0119193 },   // 문구 → ln
            Data::KeyedValue{ "낙인력 +2.15%", 0.0117786 },   // 문구 → ln
            Data::KeyedValue{ "아군 공격력 강화 효과 +1.35%", 0.0092514 },   // 문구 → ln
            Data::KeyedValue{ "아군 피해량 강화 효과 +2.00%", 0.0091376 },   // 문구 → ln
            Data::KeyedValue{ "세레나데, 신앙, 조화 게이지 획득량 +1.60%", 0.0073204 },   // 문구 → ln
            Data::KeyedValue{ "무기 공격력 +1.80%", 0.0070541 },   // 문구 → ln
            Data::KeyedValue{ "무기 공격력 +0.80%", 0.003148 },   // 문구 → ln
            Data::KeyedValue{ "파티원 회복 효과 +3.50%", 0.0023587 },   // 문구 → ln
            Data::KeyedValue{ "파티원 보호막 효과 +3.50%", 0.0020721 },   // 문구 → ln
            Data::KeyedValue{ "최대 생명력 +6500", 0.002015 },   // 문구 → ln
            Data::KeyedValue{ "무기 공격력 +960", 0.0015661 },   // 문구 → ln
            Data::KeyedValue{ "파티원 회복 효과 +2.10%", 0.0012874 },   // 문구 → ln
            Data::KeyedValue{ "파티원 보호막 효과 +2.10%", 0.0011645 },   // 문구 → ln
            Data::KeyedValue{ "최대 생명력 +3250", 0.0011156 },   // 문구 → ln
            Data::KeyedValue{ "무기 공격력 +480", 0.0007798 },   // 문구 → ln
            Data::KeyedValue{ "파티원 회복 효과 +0.95%", 0.0005274 },   // 문구 → ln
            Data::KeyedValue{ "파티원 보호막 효과 +0.95%", 0.0005231 },   // 문구 → ln
            Data::KeyedValue{ "최대 생명력 +1300", 0.0004197 },   // 문구 → ln
            Data::KeyedValue{ "무기 공격력 +195", 0.0003175 },   // 문구 → ln
        } };   // ACCESSORY_OPTIONS 끝

        // ===== 팔찌 옵션 (공백 없는 문구 → ln, 무기 공격력 줄도 측정값 그대로) =====
        constexpr std::array<Data::KeyedValue, 33> BRACELET_OPTIONS{ {   // 문구 키 33개
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의치명타피해저항을4.8%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가3%증가한다.", 0.1110478 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의방어력을2.5%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가3%증가한다.", 0.1101013 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의치명타저항을2.5%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가3%증가한다.", 0.109025 },   // 문구 → ln
            Data::KeyedValue{ "파티효과로보호효과(보호막,생명력회복,받는피해감소)가적용된대상이5초동안적에게주는피해가1.3%증가한다.해당효과는한파티당하나만적용되며,지속시간이없는보호효과에는적용되지않는다.아군공격력강화효과가3%증가한다.", 0.1063933 },   // 문구 → ln
            Data::KeyedValue{ "파티효과로보호효과(보호막,생명력회복,받는피해감소)가적용된대상이5초동안적에게주는피해가1.1%증가한다.해당효과는한파티당하나만적용되며,지속시간이없는보호효과에는적용되지않는다.아군공격력강화효과가2.5%증가한다.", 0.0933957 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의치명타저항을2.1%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가2.5%증가한다.", 0.0933287 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의치명타피해저항을4.2%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가2.5%증가한다.", 0.0933287 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의방어력을2.1%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가2.5%증가한다.", 0.0931961 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의치명타저항을1.8%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가2%증가한다.", 0.0797318 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의방어력을1.8%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가2%증가한다.", 0.0794026 },   // 문구 → ln
            Data::KeyedValue{ "파티효과로보호효과(보호막,생명력회복,받는피해감소)가적용된대상이5초동안적에게주는피해가0.9%증가한다.해당효과는한파티당하나만적용되며,지속시간이없는보호효과에는적용되지않는다.아군공격력강화효과가2%증가한다.", 0.0789144 },   // 문구 → ln
            Data::KeyedValue{ "몬스터에게공격적중시8초동안대상의치명타피해저항을3.6%감소시킨다.해당효과는한파티당하나만적용된다.아군공격력강화효과가2%증가한다.", 0.0789144 },   // 문구 → ln
            Data::KeyedValue{ "아군공격력강화효과+6.00%", 0.0402537 },   // 문구 → ln
            Data::KeyedValue{ "아군피해량강화효과+9.00%", 0.0402537 },   // 문구 → ln
            Data::KeyedValue{ "아군공격력강화효과+5.00%", 0.0336554 },   // 문구 → ln
            Data::KeyedValue{ "아군피해량강화효과+7.50%", 0.0336554 },   // 문구 → ln
            Data::KeyedValue{ "아군공격력강화효과+4.00%", 0.0270143 },   // 문구 → ln
            Data::KeyedValue{ "아군피해량강화효과+6.00%", 0.0261253 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력이8700증가한다.공격적중시30초마다120초동안무기공격력이150증가한다.(최대30중첩)", 0.0248721 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력이7800증가한다.공격적중시30초마다120초동안무기공격력이140증가한다.(최대30중첩)", 0.0227338 },   // 문구 → ln
            Data::KeyedValue{ "공격적중시매초마다10초동안무기공격력이1480,공격및이동속도가1%증가한다.(최대6중첩)", 0.0216702 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력이6900증가한다.공격적중시30초마다120초동안무기공격력이130증가한다.(최대30중첩)", 0.0205884 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력이9000증가한다.자신의생명력이50%이상일경우적에게공격적중시5초동안무기공격력이2400증가한다.", 0.0202471 },   // 문구 → ln
            Data::KeyedValue{ "공격적중시매초마다10초동안무기공격력이1320,공격및이동속도가1%증가한다.(최대6중첩)", 0.0193438 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력이8100증가한다.자신의생명력이50%이상일경우적에게공격적중시5초동안무기공격력이2200증가한다.", 0.0182892 },   // 문구 → ln
            Data::KeyedValue{ "공격적중시매초마다10초동안무기공격력이1160,공격및이동속도가1%증가한다.(최대6중첩)", 0.0170132 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력이7200증가한다.자신의생명력이50%이상일경우적에게공격적중시5초동안무기공격력이2000증가한다.", 0.0164191 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력+9000", 0.0143022 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력+8100", 0.0126527 },   // 문구 → ln
            Data::KeyedValue{ "무기공격력+7200", 0.0114797 },   // 문구 → ln
            Data::KeyedValue{ "최대생명력+16800", 0.005787 },   // 문구 → ln
            Data::KeyedValue{ "최대생명력+14000", 0.004825 },   // 문구 → ln
            Data::KeyedValue{ "최대생명력+11200", 0.0038621 },   // 문구 → ln
        } };   // BRACELET_OPTIONS 끝

        // ===== 팔찌 스탯 (특화 · 신속만 반영, 치명은 효과 없음) =====
        constexpr double BRACELET_STAT_RATIO{ 0.0001843 };   // 유효 스탯 1포인트당 배율 증가 (165줄 측정)
        constexpr std::array<std::string_view, 2> BRACELET_EFFECTIVE_TYPES{ {   // 선택지 2개
            "특화",   // 선택지
            "신속",   // 선택지
        } };   // BRACELET_EFFECTIVE_TYPES 끝

        // ===== 각인서 (스톤 효과를 뺀 ln, 효과 없는 이름은 표에 없음) =====
        constexpr std::array<SupporterBook, 5> BOOKS{ {   // 각인서 5개
            SupporterBook{ "각성", 0.21652, 0.23793, 0.20928 },          // 이름 · 유물 X0 · 유물 X4 · 전설 X4
            SupporterBook{ "구슬동자", 0.19523, 0.22322, 0.17176 },      // 이름 · 유물 X0 · 유물 X4 · 전설 X4
            SupporterBook{ "마나의 흐름", 0.11946, 0.16659, 0.10732 },   // 이름 · 유물 X0 · 유물 X4 · 전설 X4
            SupporterBook{ "전문의", 0.02176, 0.02483, 0.02099 },        // 이름 · 유물 X0 · 유물 X4 · 전설 X4
            SupporterBook{ "분쇄의 주먹", 0.0213, 0.0213, 0.0213 },      // 이름 · 레벨과 무관한 고정 값
        } };   // BOOKS 끝

        // ===== 스톤 (레벨 1 효과, 레벨 배수 표는 딜러와 같음) =====
        constexpr std::array<Data::KeyedValue, 4> STONE_LEVEL_ONE{ {   // 문구 키 4개
            Data::KeyedValue{ "각성", 0.0269 },          // 키 → 값
            Data::KeyedValue{ "구슬동자", 0.0259 },      // 키 → 값
            Data::KeyedValue{ "전문의", 0.0018 },        // 키 → 값
            Data::KeyedValue{ "분쇄의 주먹", 0.008 },    // 키 → 값
        } };   // STONE_LEVEL_ONE 끝
        constexpr int STONE_SUM_MIN_LEVEL{ 5 };          // 스톤 레벨 합 보너스가 붙는 최소 합
        constexpr double STONE_SUM_LN{ 0.0117 };         // 스톤 레벨 합 보너스 ln (딜러 0.01259)
        constexpr Data::StoneSumBonus STONE_SUM_BONUS{ STONE_SUM_MIN_LEVEL, STONE_SUM_LN };   // 스톤 레벨 합 보너스 묶음

        // ===== 아바타 =====
        constexpr double AVATAR_MAIN_STAT_SCALE{ 0.008665 };   // 주스탯 % 합 계수 (딜러 0.009569)

        // ===== 아크 그리드 코어 (정확히 이 포인트일 때만 켜짐, 나머지 포인트 = 0, 서포터 7명 포인트 4 ~ 20 단일 측정) =====
        // 10 · 14포인트는 등급과 무관, 17포인트 이상은 고대만 추가 효과 (영웅 = 전설 = 유물)
        // 혼돈의 해 10포인트 = 20포인트와 같은 값은 lopec 측정 그대로 (7명 모두 같음, lopec 특이점으로 보임)
        constexpr std::array<Data::ArkCorePoint, 35> ARK_CORE_POINTS{ {   // 코어 포인트 칸 35개
            Data::ArkCorePoint{ 0, "", 10, 0.0109, 0.0109 },   // 질서의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 0, "", 14, 0.0109, 0.0109 },   // 질서의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 0, "", 17, 0.0691, 0.0792 },   // 질서의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 0, "", 18, 0.0706, 0.0807 },   // 질서의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 0, "", 19, 0.0716, 0.0817 },   // 질서의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 0, "", 20, 0.0726, 0.0827 },   // 질서의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 1, "", 10, 0.0109, 0.0109 },   // 질서의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 1, "", 14, 0.0109, 0.0109 },   // 질서의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 1, "", 17, 0.0691, 0.0792 },   // 질서의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 1, "", 18, 0.0706, 0.0807 },   // 질서의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 1, "", 19, 0.0716, 0.0817 },   // 질서의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 1, "", 20, 0.0726, 0.0827 },   // 질서의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 2, "", 14, 0.0055, 0.0055 },   // 질서의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 2, "", 17, 0.0192, 0.0272 },   // 질서의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 2, "", 18, 0.0201, 0.0281 },   // 질서의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 2, "", 19, 0.021, 0.029 },   // 질서의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 2, "", 20, 0.0219, 0.0299 },   // 질서의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 3, "", 10, 0.0536, 0.0536 },   // 혼돈의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 3, "", 14, 0.011, 0.011 },   // 혼돈의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 3, "", 17, 0.0326, 0.0484 },   // 혼돈의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 3, "", 18, 0.0341, 0.0499 },   // 혼돈의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 3, "", 19, 0.0357, 0.0515 },   // 혼돈의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 3, "", 20, 0.0378, 0.0536 },   // 혼돈의 해 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 4, "", 10, 0.0055, 0.0055 },   // 혼돈의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 4, "", 14, 0.011, 0.011 },   // 혼돈의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 4, "", 17, 0.0326, 0.0484 },   // 혼돈의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 4, "", 18, 0.0341, 0.0499 },   // 혼돈의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 4, "", 19, 0.0357, 0.0515 },   // 혼돈의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 4, "", 20, 0.0378, 0.0536 },   // 혼돈의 달 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 5, "", 10, 0.0022, 0.0022 },   // 혼돈의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 5, "", 14, 0.0052, 0.0052 },   // 혼돈의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 5, "", 17, 0.0155, 0.0206 },   // 혼돈의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 5, "", 18, 0.0164, 0.0215 },   // 혼돈의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 5, "", 19, 0.0173, 0.0224 },   // 혼돈의 별 · 포인트 · ln · 고대 ln
            Data::ArkCorePoint{ 5, "", 20, 0.0182, 0.0233 },   // 혼돈의 별 · 포인트 · ln · 고대 ln
        } };   // ARK_CORE_POINTS 끝

        // ===== 아크 그리드 젬 (레벨 1당 ln, 딜러 효과는 0) =====
        constexpr std::array<Data::KeyedValue, 3> ARK_GEM_PER_LEVEL{ {   // 문구 키 3개
            Data::KeyedValue{ "낙인력", 0.000761 },            // 키 → 값
            Data::KeyedValue{ "아군 공격 강화", 0.001042 },    // 키 → 값
            Data::KeyedValue{ "아군 피해 강화", 0.000447 },    // 키 → 값
        } };   // ARK_GEM_PER_LEVEL 끝

        // ===== 아크 패시브 =====
        constexpr std::array<double, 7> PASSIVE_RANK_RATIOS{   // 총 7칸
            0.93163, 0.9434, 0.95527, 0.96725, 0.97933, 0.99152, 1.0,   // 랭크 0 ~ 6
        };   // PASSIVE_RANK_RATIOS 끝
        constexpr std::array<Data::KeyedValue, 8> ENLIGHTENMENT_RATIOS{ {   // 문구 키 8개
            Data::KeyedValue{ "상급 + 일반2", 0.99235 },   // 키 → 값
            Data::KeyedValue{ "상급 + 일반1", 0.98088 },   // 키 → 값
            Data::KeyedValue{ "상급", 0.96942 },           // 키 → 값
            Data::KeyedValue{ "일반3", 0.9847 },           // 키 → 값
            Data::KeyedValue{ "일반2", 0.97323 },          // 키 → 값
            Data::KeyedValue{ "일반1", 0.96176 },          // 키 → 값
            Data::KeyedValue{ "없음", 0.95028 },           // 키 → 값
            Data::KeyedValue{ "상급 + 일반3", 1.0 },       // 키 → 값
        } };   // ENLIGHTENMENT_RATIOS 끝
        constexpr double EVOLUTION_SLOPE{ 0.0001238 };      // 진화 레벨 기울기 (레벨 1 = 0.99641)
        constexpr double ENLIGHTENMENT_SLOPE{ 0.000394 };   // 깨달음 레벨 기울기 (레벨 1 = 0.98858)
        constexpr double LEAP_SLOPE{ 0.0 };                 // 도약 레벨은 효과 없음
        constexpr SupporterPassiveSlopes PASSIVE_SLOPES{ EVOLUTION_SLOPE, ENLIGHTENMENT_SLOPE, LEAP_SLOPE };   // 기울기 묶음
        constexpr double INTERACTION_CONSTANT_LN{ -0.00405066 };   // 랭크 · 깨달음 둘 다 최대가 아닐 때 상수 ln (서포터 조합 205줄)
        constexpr double INTERACTION_CROSS_LN{ -0.42114313 };      // 교차항 계수 × (1 - 랭크 배율) × (1 - 깨달음 배율)
        constexpr Data::PassiveInteraction PASSIVE_INTERACTION{ INTERACTION_CONSTANT_LN, INTERACTION_CROSS_LN };   // 상호작용 묶음
    }

    // 서포터 칭호 목록
    std::span<const std::string_view> GetSupporterTitles() noexcept
    {
        return SUPPORTER_TITLES;   // 칭호 표 반환
    }

    // 칭호가 서포터 칭호인지
    bool IsSupporterTitle(std::string_view title) noexcept
    {
        return std::ranges::find(SUPPORTER_TITLES, title) != SUPPORTER_TITLES.end();   // 표에 있으면 서포터
    }

    // 서포터 S0 보정 주스탯
    double GetMainStatExtra() noexcept
    {
        return MAIN_STAT_EXTRA;   // 보정 주스탯 반환
    }

    // 서포터 W0 (툴팁 공격력을 모르면 0)
    double GetWeaponBase(double weaponAttack) noexcept
    {
        return weaponAttack > 0.0 ? weaponAttack + WEAPON_ATTACK_EXTRA : 0.0;   // 툴팁 공격력 + 보정, 모르면 0
    }

    // 방어구 품질 감소 비율 a
    double GetArmorQualityAmplitude() noexcept
    {
        return ARMOR_QUALITY_AMPLITUDE;   // 감소 비율 반환
    }

    // 최대 품질
    double GetMaxQuality() noexcept
    {
        return MAX_QUALITY;   // 최대 품질 반환
    }

    // 서포터 완갑 재련 단계별 ln
    std::span<const double> GetVambraceLevelLn() noexcept
    {
        return VAMBRACE_LEVEL_LN;   // 단계별 ln 표 반환
    }

    // 서포터 보석 레벨 계수 표
    std::span<const double> GetGemFactors() noexcept
    {
        return GEM_FACTORS;   // 레벨별 계수 반환
    }

    // 보석 계수 표의 첫 레벨
    int GetGemFirstLevel() noexcept
    {
        return GEM_FIRST_LEVEL;   // 첫 레벨 반환
    }

    // 보석이 없을 때의 계수
    double GetNoGemValue() noexcept
    {
        return NO_GEM_VALUE;   // 보석 없음 계수 반환
    }

    // 서포터 악세서리 옵션 표
    std::span<const Data::KeyedValue> GetAccessoryOptions() noexcept
    {
        return ACCESSORY_OPTIONS;   // 악세서리 옵션 표 반환
    }

    // 서포터 팔찌 옵션 표
    std::span<const Data::KeyedValue> GetBraceletOptions() noexcept
    {
        return BRACELET_OPTIONS;   // 팔찌 옵션 표 반환
    }

    // 서포터 팔찌 유효 스탯 1포인트당 배율
    double GetBraceletStatRatio() noexcept
    {
        return BRACELET_STAT_RATIO;   // 스탯 배율 반환
    }

    // 서포터 팔찌 유효 스탯 종류
    std::span<const std::string_view> GetBraceletEffectiveTypes() noexcept
    {
        return BRACELET_EFFECTIVE_TYPES;   // 유효 스탯 종류 반환
    }

    // 서포터 각인서 표
    std::span<const SupporterBook> GetBooks() noexcept
    {
        return BOOKS;   // 각인서 표 반환
    }

    // 서포터 스톤 레벨 1 효과 표
    std::span<const Data::KeyedValue> GetStoneLevelOne() noexcept
    {
        return STONE_LEVEL_ONE;   // 스톤 효과 표 반환
    }

    // 서포터 스톤 레벨 합 보너스
    const Data::StoneSumBonus& GetStoneSumBonus() noexcept
    {
        return STONE_SUM_BONUS;   // 레벨 합 보너스 반환
    }

    // 서포터 아바타 주스탯 % 합 계수
    double GetAvatarMainStatScale() noexcept
    {
        return AVATAR_MAIN_STAT_SCALE;   // 아바타 계수 반환
    }

    // 서포터 아크 코어 포인트 표
    std::span<const Data::ArkCorePoint> GetArkCorePoints() noexcept
    {
        return ARK_CORE_POINTS;   // 코어 포인트 표 반환
    }

    // 서포터 아크 젬 레벨 1당 ln 표
    std::span<const Data::KeyedValue> GetArkGemPerLevel() noexcept
    {
        return ARK_GEM_PER_LEVEL;   // 젬 효과 표 반환
    }

    // 서포터 아크 패시브 랭크 배율 표
    std::span<const double> GetPassiveRankRatios() noexcept
    {
        return PASSIVE_RANK_RATIOS;   // 랭크 배율 표 반환
    }

    // 서포터 깨달음 포인트 구성 배율 표
    std::span<const Data::KeyedValue> GetEnlightenmentRatios() noexcept
    {
        return ENLIGHTENMENT_RATIOS;   // 깨달음 구성 표 반환
    }

    // 서포터 랭크 · 깨달음 상호작용
    const Data::PassiveInteraction& GetPassiveInteraction() noexcept
    {
        return PASSIVE_INTERACTION;   // 상호작용 묶음 반환
    }

    // 서포터 아크 패시브 트리 레벨 기울기
    const SupporterPassiveSlopes& GetPassiveSlopes() noexcept
    {
        return PASSIVE_SLOPES;   // 기울기 묶음 반환
    }
}
