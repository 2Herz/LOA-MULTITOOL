module LostArkMultiTool.Models.RefineData;

import std;

namespace LostArkMultiTool::Models::RefineData
{
    namespace
    {
        // ===== 재료 정보 (RefineMaterial 순서) =====
        constexpr std::array<MaterialInfo, MATERIAL_COUNT> MATERIAL_INFOS{
            MaterialInfo{ "골드", "", false },
            MaterialInfo{ "아비도스 융화 재료", "아비도스 융화 재료", false },
            MaterialInfo{ "상급 아비도스 융화 재료", "상급 아비도스 융화 재료", false },
            MaterialInfo{ "운명의 파편", "운명의 파편 주머니(대)", true },
            MaterialInfo{ "운명의 돌파석", "운명의 돌파석", false },
            MaterialInfo{ "위대한 운명의 돌파석", "위대한 운명의 돌파석", false },
            MaterialInfo{ "운명의 수호석", "운명의 수호석", false },
            MaterialInfo{ "운명의 파괴석", "운명의 파괴석", false },
            MaterialInfo{ "운명의 수호석 결정", "운명의 수호석 결정", false },
            MaterialInfo{ "운명의 파괴석 결정", "운명의 파괴석 결정", false },
            MaterialInfo{ "빙하의 숨결", "빙하의 숨결", false },
            MaterialInfo{ "용암의 숨결", "용암의 숨결", false },
            MaterialInfo{ "재봉술 : 업화 [11-14]", "재봉술 : 업화 [11-14]", false },
            MaterialInfo{ "재봉술 : 업화 [15-18]", "재봉술 : 업화 [15-18]", false },
            MaterialInfo{ "재봉술 : 업화 [19-20]", "재봉술 : 업화 [19-20]", false },
            MaterialInfo{ "야금술 : 업화 [11-14]", "야금술 : 업화 [11-14]", false },
            MaterialInfo{ "야금술 : 업화 [15-18]", "야금술 : 업화 [15-18]", false },
            MaterialInfo{ "야금술 : 업화 [19-20]", "야금술 : 업화 [19-20]", false },
            MaterialInfo{ "재봉술 : 전율 [12-15]", "재봉술 : 전율 [12-15]", false },
            MaterialInfo{ "재봉술 : 전율 [16-19]", "재봉술 : 전율 [16-19]", false },
            MaterialInfo{ "야금술 : 전율 [12-15]", "야금술 : 전율 [12-15]", false },
            MaterialInfo{ "야금술 : 전율 [16-19]", "야금술 : 전율 [16-19]", false },
            MaterialInfo{ "장인의 재봉술 : 1단계", "장인의 재봉술 : 1단계", false },
            MaterialInfo{ "장인의 재봉술 : 2단계", "장인의 재봉술 : 2단계", false },
            MaterialInfo{ "장인의 재봉술 : 3단계", "장인의 재봉술 : 3단계", false },
            MaterialInfo{ "장인의 재봉술 : 4단계", "장인의 재봉술 : 4단계", false },
            MaterialInfo{ "장인의 야금술 : 1단계", "장인의 야금술 : 1단계", false },
            MaterialInfo{ "장인의 야금술 : 2단계", "장인의 야금술 : 2단계", false },
            MaterialInfo{ "장인의 야금술 : 3단계", "장인의 야금술 : 3단계", false },
            MaterialInfo{ "장인의 야금술 : 4단계", "장인의 야금술 : 4단계", false }
        };

        // ===== 일반 재련표 =====
        // 한 줄 = 목표 단계 · 기본 확률 · 1회 재료 (강화석 · 돌파석 · 융화 재료 · 파편 · 골드)
        // 강화석 · 돌파석 · 융화 재료는 장비 단계 · 종류에 따라 다른 재료 (StepMaterials 참고)
        constexpr std::size_t NORMAL_MATERIAL_COUNT{ 5 };
        constexpr std::size_t STONE_SLOT{ 0 };
        constexpr std::size_t LEAPSTONE_SLOT{ 1 };
        constexpr std::size_t FUSION_SLOT{ 2 };
        constexpr std::size_t SHARD_SLOT{ 3 };
        constexpr std::size_t GOLD_SLOT{ 4 };

        struct NormalStepRow
        {
            int m_targetLevel{};
            double m_baseProbability{};
            std::array<double, NORMAL_MATERIAL_COUNT> m_amounts{};
        };

        constexpr std::array AEGIR_ARMOR_ROWS{
            NormalStepRow{ 11, 0.1, { 750.0, 11.0, 7.0, 3000.0, 776.0 } },
            NormalStepRow{ 12, 0.1, { 780.0, 13.0, 7.0, 3180.0, 856.0 } },
            NormalStepRow{ 13, 0.05, { 840.0, 14.0, 9.0, 4560.0, 952.0 } },
            NormalStepRow{ 14, 0.05, { 930.0, 16.0, 9.0, 4920.0, 1056.0 } },
            NormalStepRow{ 15, 0.04, { 1020.0, 18.0, 11.0, 5280.0, 1168.0 } },
            NormalStepRow{ 16, 0.04, { 1170.0, 20.0, 11.0, 5640.0, 1280.0 } },
            NormalStepRow{ 17, 0.03, { 1320.0, 22.0, 15.0, 7200.0, 1408.0 } },
            NormalStepRow{ 18, 0.03, { 1470.0, 23.0, 15.0, 7740.0, 1544.0 } },
            NormalStepRow{ 19, 0.03, { 1620.0, 25.0, 15.0, 8220.0, 2110.0 } },
            NormalStepRow{ 20, 0.015, { 1770.0, 27.0, 21.0, 9600.0, 2300.0 } },
            NormalStepRow{ 21, 0.015, { 1920.0, 29.0, 21.0, 10260.0, 2500.0 } },
            NormalStepRow{ 22, 0.01, { 2220.0, 31.0, 21.0, 10920.0, 2710.0 } },
            NormalStepRow{ 23, 0.01, { 2400.0, 34.0, 21.0, 11520.0, 2920.0 } },
            NormalStepRow{ 24, 0.005, { 2520.0, 36.0, 30.0, 12240.0, 3150.0 } },
            NormalStepRow{ 25, 0.005, { 2700.0, 40.0, 30.0, 12900.0, 3390.0 } },
        };

        constexpr std::array AEGIR_WEAPON_ROWS{
            NormalStepRow{ 11, 0.1, { 1250.0, 18.0, 12.0, 5000.0, 1620.0 } },
            NormalStepRow{ 12, 0.1, { 1300.0, 21.0, 12.0, 5300.0, 1790.0 } },
            NormalStepRow{ 13, 0.05, { 1400.0, 24.0, 15.0, 7600.0, 1990.0 } },
            NormalStepRow{ 14, 0.05, { 1550.0, 27.0, 15.0, 8200.0, 2200.0 } },
            NormalStepRow{ 15, 0.04, { 1700.0, 30.0, 18.0, 8800.0, 2430.0 } },
            NormalStepRow{ 16, 0.04, { 1950.0, 33.0, 18.0, 9400.0, 2670.0 } },
            NormalStepRow{ 17, 0.03, { 2200.0, 36.0, 25.0, 12000.0, 2940.0 } },
            NormalStepRow{ 18, 0.03, { 2450.0, 39.0, 25.0, 12900.0, 3220.0 } },
            NormalStepRow{ 19, 0.03, { 2700.0, 42.0, 25.0, 13700.0, 3510.0 } },
            NormalStepRow{ 20, 0.015, { 2950.0, 45.0, 35.0, 16000.0, 3830.0 } },
            NormalStepRow{ 21, 0.015, { 3200.0, 48.0, 35.0, 17100.0, 4160.0 } },
            NormalStepRow{ 22, 0.01, { 3700.0, 52.0, 35.0, 18200.0, 4510.0 } },
            NormalStepRow{ 23, 0.01, { 4000.0, 56.0, 35.0, 19200.0, 4870.0 } },
            NormalStepRow{ 24, 0.005, { 4200.0, 60.0, 50.0, 20400.0, 5250.0 } },
            NormalStepRow{ 25, 0.005, { 4500.0, 65.0, 50.0, 21500.0, 5650.0 } },
        };

        constexpr std::array SERCA_ARMOR_ROWS{
            NormalStepRow{ 12, 0.05, { 930.0, 11.0, 11.0, 9570.0, 2450.0 } },
            NormalStepRow{ 13, 0.05, { 1030.0, 12.0, 12.0, 10540.0, 2700.0 } },
            NormalStepRow{ 14, 0.04, { 1120.0, 13.0, 13.0, 11520.0, 2950.0 } },
            NormalStepRow{ 15, 0.04, { 1240.0, 14.0, 15.0, 12690.0, 3250.0 } },
            NormalStepRow{ 16, 0.04, { 1330.0, 15.0, 16.0, 13670.0, 3500.0 } },
            NormalStepRow{ 17, 0.03, { 1450.0, 17.0, 17.0, 14840.0, 3800.0 } },
            NormalStepRow{ 18, 0.03, { 1560.0, 18.0, 19.0, 16010.0, 4100.0 } },
            NormalStepRow{ 19, 0.03, { 1700.0, 20.0, 20.0, 17380.0, 4450.0 } },
            NormalStepRow{ 20, 0.015, { 1810.0, 21.0, 22.0, 18550.0, 4750.0 } },
            NormalStepRow{ 21, 0.015, { 1950.0, 23.0, 23.0, 19920.0, 5100.0 } },
            NormalStepRow{ 22, 0.01, { 2080.0, 24.0, 25.0, 21280.0, 5450.0 } },
            NormalStepRow{ 23, 0.01, { 2200.0, 26.0, 26.0, 22460.0, 5750.0 } },
            NormalStepRow{ 24, 0.005, { 2330.0, 27.0, 28.0, 23820.0, 6100.0 } },
            NormalStepRow{ 25, 0.005, { 2450.0, 29.0, 30.0, 25000.0, 6400.0 } },
        };

        constexpr std::array SERCA_WEAPON_ROWS{
            NormalStepRow{ 12, 0.05, { 1700.0, 17.0, 18.0, 15890.0, 4050.0 } },
            NormalStepRow{ 13, 0.05, { 1890.0, 19.0, 21.0, 17660.0, 4500.0 } },
            NormalStepRow{ 14, 0.04, { 2080.0, 21.0, 23.0, 19420.0, 4950.0 } },
            NormalStepRow{ 15, 0.04, { 2270.0, 23.0, 25.0, 21190.0, 5400.0 } },
            NormalStepRow{ 16, 0.04, { 2460.0, 25.0, 27.0, 22960.0, 5850.0 } },
            NormalStepRow{ 17, 0.03, { 2690.0, 28.0, 29.0, 25120.0, 6400.0 } },
            NormalStepRow{ 18, 0.03, { 2900.0, 30.0, 32.0, 27080.0, 6900.0 } },
            NormalStepRow{ 19, 0.03, { 3110.0, 32.0, 34.0, 29040.0, 7400.0 } },
            NormalStepRow{ 20, 0.015, { 3340.0, 34.0, 37.0, 31200.0, 7950.0 } },
            NormalStepRow{ 21, 0.015, { 3570.0, 37.0, 39.0, 33360.0, 8500.0 } },
            NormalStepRow{ 22, 0.01, { 3800.0, 39.0, 42.0, 35520.0, 9050.0 } },
            NormalStepRow{ 23, 0.01, { 4030.0, 42.0, 44.0, 37680.0, 9600.0 } },
            NormalStepRow{ 24, 0.005, { 4260.0, 44.0, 47.0, 39840.0, 10150.0 } },
            NormalStepRow{ 25, 0.005, { 4500.0, 47.0, 50.0, 42000.0, 10700.0 } },
        };

        // 숨결: 기본 확률마다 최대 개수 · 1개당 확률이 정해져 있음
        struct BreathRule
        {
            double m_baseProbability{};
            int m_maxCount{};
            double m_probability{};
        };

        constexpr std::array BREATH_RULES{
            BreathRule{ 0.1, 20, 0.005 },
            BreathRule{ 0.05, 20, 0.0025 },
            BreathRule{ 0.04, 20, 0.002 },
            BreathRule{ 0.03, 20, 0.0015 },
            BreathRule{ 0.015, 25, 0.0006 },
            BreathRule{ 0.01, 25, 0.0004 },
            BreathRule{ 0.005, 50, 0.0002 }
        };

        constexpr double PROBABILITY_EPSILON{ 1e-9 };   // 기본 확률 비교 허용 오차

        // 강화책 구간 (목표 단계 범위 → 책, 책 1권 확률 = 기본 확률)
        struct BookRange
        {
            int m_minLevel{};
            int m_maxLevel{};
            RefineMaterial m_book{};
        };

        constexpr std::array AEGIR_ARMOR_BOOKS{
            BookRange{ 11, 14, RefineMaterial::MATERIAL_TAILORING_HELLFIRE_11_14 },
            BookRange{ 15, 18, RefineMaterial::MATERIAL_TAILORING_HELLFIRE_15_18 },
            BookRange{ 19, 20, RefineMaterial::MATERIAL_TAILORING_HELLFIRE_19_20 }
        };
        constexpr std::array AEGIR_WEAPON_BOOKS{
            BookRange{ 11, 14, RefineMaterial::MATERIAL_METALLURGY_HELLFIRE_11_14 },
            BookRange{ 15, 18, RefineMaterial::MATERIAL_METALLURGY_HELLFIRE_15_18 },
            BookRange{ 19, 20, RefineMaterial::MATERIAL_METALLURGY_HELLFIRE_19_20 }
        };
        constexpr std::array SERCA_ARMOR_BOOKS{
            BookRange{ 12, 15, RefineMaterial::MATERIAL_TAILORING_TREMOR_12_15 },
            BookRange{ 16, 19, RefineMaterial::MATERIAL_TAILORING_TREMOR_16_19 }
        };
        constexpr std::array SERCA_WEAPON_BOOKS{
            BookRange{ 12, 15, RefineMaterial::MATERIAL_METALLURGY_TREMOR_12_15 },
            BookRange{ 16, 19, RefineMaterial::MATERIAL_METALLURGY_TREMOR_16_19 }
        };

        constexpr int BOOK_COUNT_PER_TRY{ 1 };   // 책은 한 번에 1권

        // ===== 상급 재련표 =====
        // 기대 시도 수 표 (K = 숨결 + 2 × 책) — 강화 선조 턴이 없는 묶음(1~20) / 있는 묶음(21~40)
        constexpr std::array NO_ENHANCED_TRY_ROWS{
            AdvancedTryRow{ 0, 0, 0, 4.7999674134, 32.2021878529, 14.0379083029, 0 },
            AdvancedTryRow{ 0, 1, 0, 4.0421386627, 27.320514648, 11.8907467636, 0 },
            AdvancedTryRow{ 0, 2, 0, 3.7414403403, 25.3846510166, 11.0386032183, 0 },
            AdvancedTryRow{ 0, 3, 0, 3.2564448136, 22.2625629904, 9.6641829443, 0 },
            AdvancedTryRow{ 1, 0, 0, 3.9125078081, 26.618383927, 11.4449515838, 0 },
            AdvancedTryRow{ 1, 1, 0, 3.3904071591, 23.2372414995, 9.9764432112, 0 },
            AdvancedTryRow{ 1, 2, 0, 3.1747045868, 21.8413205224, 9.3695102373, 0 },
            AdvancedTryRow{ 1, 3, 0, 2.8160185219, 19.520300115, 8.3602066816, 0 },
            AdvancedTryRow{ 2, 0, 0, 3.5750483808, 24.4951369196, 10.4591048172, 0 },
            AdvancedTryRow{ 2, 1, 0, 3.1325762612, 21.6218338138, 9.2192345154, 0 },
            AdvancedTryRow{ 2, 2, 0, 2.946850216, 20.4166948158, 8.6985608009, 0 },
            AdvancedTryRow{ 2, 3, 0, 2.6341734459, 18.3880110067, 7.8219282361, 0 },
            AdvancedTryRow{ 3, 0, 0, 3.0417877556, 21.1398424076, 8.9012160661, 0 },
            AdvancedTryRow{ 3, 1, 0, 2.7131990864, 18.9942876605, 7.987568621, 0 },
            AdvancedTryRow{ 3, 2, 0, 2.5717065799, 18.0711221243, 7.5938743998, 0 },
            AdvancedTryRow{ 3, 3, 0, 2.3285430805, 16.4850023075, 6.917303311, 0 },
        };

        constexpr std::array ENHANCED_TRY_ROWS{
            AdvancedTryRow{ 0, 0, 0, 3.2303957841, 28.4120173182, 12.0330558296, 1.4860305586 },
            AdvancedTryRow{ 0, 0, 1, 3.1552449426, 27.77167191, 11.7604367094, 1.4524117775 },
            AdvancedTryRow{ 0, 0, 2, 3.1183542973, 27.4578529553, 11.6266727361, 1.4359113289 },
            AdvancedTryRow{ 0, 0, 3, 3.0480419338, 26.8588014874, 11.3715350529, 1.4044438506 },
            AdvancedTryRow{ 0, 1, 0, 2.8367581488, 25.1136453772, 10.6222664049, 1.3050463759 },
            AdvancedTryRow{ 0, 1, 1, 2.7779747374, 24.611745393, 10.4084292959, 1.278801937 },
            AdvancedTryRow{ 0, 1, 2, 2.749027397, 24.3650634692, 10.3032052506, 1.2658852892 },
            AdvancedTryRow{ 0, 1, 3, 2.6936407257, 23.8921836678, 10.1016761392, 1.2411487505 },
            AdvancedTryRow{ 0, 2, 0, 2.6718275242, 23.7293708358, 10.0308611225, 1.2292382732 },
            AdvancedTryRow{ 0, 2, 1, 2.6192954839, 23.2805649574, 9.8395235442, 1.2058076178 },
            AdvancedTryRow{ 0, 2, 2, 2.5933861561, 23.0597305988, 9.7452659572, 1.1942636244 },
            AdvancedTryRow{ 0, 2, 3, 2.5437456571, 22.6356393685, 9.5644257998, 1.1721164431 },
            AdvancedTryRow{ 0, 3, 0, 2.3931750996, 21.3948064131, 9.0320802491, 1.1011880445 },
            AdvancedTryRow{ 0, 3, 1, 2.3504705991, 21.0287500173, 8.8758386505, 1.0821501467 },
            AdvancedTryRow{ 0, 3, 2, 2.3293536593, 20.8482512621, 8.7987143842, 1.0727539115 },
            AdvancedTryRow{ 0, 3, 3, 2.2887918583, 20.5006375888, 8.6503298582, 1.0546706987 },
            AdvancedTryRow{ 1, 0, 0, 2.7142964556, 24.1807924849, 10.1238467675, 1.2500428853 },
            AdvancedTryRow{ 1, 0, 1, 2.6601000012, 23.7136048108, 9.9268280626, 1.225752506 },
            AdvancedTryRow{ 1, 0, 2, 2.6333606141, 23.4834588443, 9.8297058814, 1.2137776587 },
            AdvancedTryRow{ 1, 0, 3, 2.5821727479, 23.0423394459, 9.6436142857, 1.1908314849 },
            AdvancedTryRow{ 1, 1, 0, 2.4226140164, 21.7557379226, 9.0949260797, 1.1161754178 },
            AdvancedTryRow{ 1, 1, 1, 2.3792271889, 21.3792822713, 8.9362202441, 1.0967526192 },
            AdvancedTryRow{ 1, 1, 2, 2.3577484181, 21.1931887014, 8.8577313536, 1.0871438884 },
            AdvancedTryRow{ 1, 1, 3, 2.316524269, 20.8356490402, 8.7069507554, 1.0686873448 },
            AdvancedTryRow{ 1, 2, 0, 2.2971907575, 20.7134104955, 8.6527794037, 1.0587501645 },
            AdvancedTryRow{ 1, 2, 1, 2.2581087297, 20.3730238357, 8.5093117795, 1.0412570676 },
            AdvancedTryRow{ 1, 2, 2, 2.2387321111, 20.2044997009, 8.4382597234, 1.0325891372 },
            AdvancedTryRow{ 1, 2, 3, 2.2014991722, 19.8803789058, 8.3016032286, 1.0159226968 },
            AdvancedTryRow{ 1, 3, 0, 2.0808282289, 18.9143559139, 7.8888667152, .9594552418 },
            AdvancedTryRow{ 1, 3, 1, 2.0486748231, 18.6321972482, 7.7700317554, .9450769058 },
            AdvancedTryRow{ 1, 3, 2, 2.0326842081, 18.4920552324, 7.7110045949, .9379279948 },
            AdvancedTryRow{ 1, 3, 3, 2.0019073885, 18.2221395572, 7.5972869186, .9241648949 },
            AdvancedTryRow{ 2, 0, 0, 2.5041640186, 22.4910185477, 9.3595359956, 1.1550747014 },
            AdvancedTryRow{ 2, 0, 1, 2.4579479314, 22.0880281606, 9.1907589868, 1.1342922227 },
            AdvancedTryRow{ 2, 0, 2, 2.4350898672, 21.8889637142, 9.1073853784, 1.1240206001 },
            AdvancedTryRow{ 2, 0, 3, 2.3912394114, 21.5068203491, 8.947264894, 1.1043010004 },
            AdvancedTryRow{ 2, 1, 0, 2.2500006687, 20.376371535, 8.4650661492, 1.0378496201 },
            AdvancedTryRow{ 2, 1, 1, 2.2126332694, 20.0484565091, 8.3278867061, 1.0210917461 },
            AdvancedTryRow{ 2, 1, 2, 2.1940919511, 19.8859539764, 8.2598961836, 1.0127788878 },
            AdvancedTryRow{ 2, 1, 3, 2.1584463316, 19.5733316281, 8.129056943, .9967922483 },
            AdvancedTryRow{ 2, 2, 0, 2.1393736397, 19.4567714305, 8.0756186759, .986897272 },
            AdvancedTryRow{ 2, 2, 1, 2.1055882029, 19.1592257068, 7.9512380213, .9717635439 },
            AdvancedTryRow{ 2, 2, 2, 2.0888027318, 19.0115765748, 7.8895055892, .9642448345 },
            AdvancedTryRow{ 2, 2, 3, 2.0564970111, 18.7272281602, 7.7705906176, .9497728062 },
            AdvancedTryRow{ 2, 3, 0, 1.947609909, 17.8602456066, 7.4000559173, .8985247863 },
            AdvancedTryRow{ 2, 3, 1, 1.9196038175, 17.612003558, 7.2964258476, .8860155044 },
            AdvancedTryRow{ 2, 3, 2, 1.9056575222, 17.4885177243, 7.2448615959, .8797831249 },
            AdvancedTryRow{ 2, 3, 3, 1.8787679853, 17.2502945158, 7.1453741665, .8677706585 },
            AdvancedTryRow{ 3, 0, 0, 2.1487364531, 19.6805222309, 8.0511674037, .9933945235 },
            AdvancedTryRow{ 3, 0, 1, 2.1151996033, 19.382158117, 7.9288833679, .978311799 },
            AdvancedTryRow{ 3, 0, 2, 2.0985531459, 19.2342281694, 7.8681558001, .9708132624 },
            AdvancedTryRow{ 3, 0, 3, 2.0664684563, 18.9489405912, 7.7511570386, .956382095 },
            AdvancedTryRow{ 3, 1, 0, 1.9577723597, 18.0485057243, 7.375679535, .9032553276 },
            AdvancedTryRow{ 3, 1, 1, 1.9297440198, 17.7986286629, 7.2731375133, .8907456403 },
            AdvancedTryRow{ 3, 1, 2, 1.9158005285, 17.6744384863, 7.2220772271, .8845144458 },
            AdvancedTryRow{ 3, 1, 3, 1.8888901435, 17.4346707196, 7.1236317812, .8725019017 },
            AdvancedTryRow{ 3, 2, 0, 1.8725221812, 17.3190988851, 7.0732570308, .8627615331 },
            AdvancedTryRow{ 3, 2, 1, 1.8467690851, 17.0890574752, 6.9788144539, .8513034942 },
            AdvancedTryRow{ 3, 2, 2, 1.8339309819, 16.9744506315, 6.9316622973, .8455832609 },
            AdvancedTryRow{ 3, 2, 3, 1.8091635859, 16.7533291733, 6.8408340724, .8345610363 },
            AdvancedTryRow{ 3, 3, 0, 1.722654467, 16.0418169829, 6.5433310654, .7921417783 },
            AdvancedTryRow{ 3, 3, 1, 1.7007114228, 15.844775038, 6.4624117225, .7824116585 },
            AdvancedTryRow{ 3, 3, 2, 1.6897398789, 15.7461795724, 6.4218206044, .7775322809 },
            AdvancedTryRow{ 3, 3, 3, 1.6685700781, 15.556043171, 6.3436950961, .7681351731 },
        };

        // 묶음 하나 (1회 재료: 강화석 · 돌파석 · 융화 재료 · 파편 · 골드, 숨결 수량, 책)
        struct AdvancedBlockRow
        {
            std::array<double, NORMAL_MATERIAL_COUNT> m_amounts{};
            double m_breathAmount{};
            RefineMaterial m_book{};
            bool m_hasEnhancedBonus{};
        };

        constexpr std::array<AdvancedBlockRow, ADVANCED_BLOCK_COUNT> ADVANCED_ARMOR_ROWS{
            AdvancedBlockRow{ { 150.0, 4.0, 5.0, 300.0, 475.0 }, 4.0, RefineMaterial::MATERIAL_MASTER_TAILORING_1, false },
            AdvancedBlockRow{ { 270.0, 5.0, 5.0, 600.0, 900.0 }, 6.0, RefineMaterial::MATERIAL_MASTER_TAILORING_2, false },
            AdvancedBlockRow{ { 1000.0, 18.0, 17.0, 7000.0, 2000.0 }, 20.0, RefineMaterial::MATERIAL_MASTER_TAILORING_3, true },
            AdvancedBlockRow{ { 1200.0, 23.0, 19.0, 8000.0, 2400.0 }, 24.0, RefineMaterial::MATERIAL_MASTER_TAILORING_4, true }
        };

        constexpr std::array<AdvancedBlockRow, ADVANCED_BLOCK_COUNT> ADVANCED_WEAPON_ROWS{
            AdvancedBlockRow{ { 180.0, 5.0, 8.0, 500.0, 563.0 }, 4.0, RefineMaterial::MATERIAL_MASTER_METALLURGY_1, false },
            AdvancedBlockRow{ { 330.0, 7.0, 9.0, 1000.0, 1250.0 }, 6.0, RefineMaterial::MATERIAL_MASTER_METALLURGY_2, false },
            AdvancedBlockRow{ { 1200.0, 25.0, 28.0, 11500.0, 3000.0 }, 20.0, RefineMaterial::MATERIAL_MASTER_METALLURGY_3, true },
            AdvancedBlockRow{ { 1400.0, 32.0, 30.0, 13000.0, 4000.0 }, 24.0, RefineMaterial::MATERIAL_MASTER_METALLURGY_4, true }
        };

        // ===== 아이템 레벨 공식 =====
        constexpr int AEGIR_BASE_ITEM_LEVEL{ 1730 };      // 에기르 +20 · 상급 40
        constexpr int AEGIR_BASE_ENHANCEMENT{ 20 };
        constexpr int SERCA_BASE_ITEM_LEVEL{ 1725 };      // 세르카 +10
        constexpr int SERCA_BASE_ENHANCEMENT{ 10 };
        constexpr int ITEM_LEVEL_PER_ENHANCEMENT{ 5 };    // 일반 재련 1단계 = 아이템 레벨 5 (상급 1단계 = 1)

        // ===== 계승 =====
        constexpr std::array INHERIT_START_LEVELS{ 11, 12, 13, 14, 16, 18 };   // 에기르 +20 ~ +25 → 세르카 시작 단계

        // 장비 단계 · 종류 → 일반 재련표 · 책 구간
        [[nodiscard]] std::span<const NormalStepRow> GetNormalRows(EquipmentTier tier, EquipmentKind kind) noexcept
        {
            if (tier == EquipmentTier::TIER_AEGIR) return kind == EquipmentKind::KIND_ARMOR ? std::span<const NormalStepRow>{ AEGIR_ARMOR_ROWS } : std::span<const NormalStepRow>{ AEGIR_WEAPON_ROWS };
            return kind == EquipmentKind::KIND_ARMOR ? std::span<const NormalStepRow>{ SERCA_ARMOR_ROWS } : std::span<const NormalStepRow>{ SERCA_WEAPON_ROWS };
        }

        [[nodiscard]] std::span<const BookRange> GetBookRanges(EquipmentTier tier, EquipmentKind kind) noexcept
        {
            if (tier == EquipmentTier::TIER_AEGIR) return kind == EquipmentKind::KIND_ARMOR ? std::span<const BookRange>{ AEGIR_ARMOR_BOOKS } : std::span<const BookRange>{ AEGIR_WEAPON_BOOKS };
            return kind == EquipmentKind::KIND_ARMOR ? std::span<const BookRange>{ SERCA_ARMOR_BOOKS } : std::span<const BookRange>{ SERCA_WEAPON_BOOKS };
        }

        // 표의 다섯 칸이 가리키는 실제 재료 (에기르: 수호석 · 파괴석 / 세르카: 결정 · 위대한 돌파석 · 상급 융화 재료)
        [[nodiscard]] std::array<RefineMaterial, NORMAL_MATERIAL_COUNT> GetStepMaterials(EquipmentTier tier, EquipmentKind kind) noexcept
        {
            const bool isArmor{ kind == EquipmentKind::KIND_ARMOR };
            if (tier == EquipmentTier::TIER_AEGIR)
            {
                return { isArmor ? RefineMaterial::MATERIAL_DESTINY_GUARDIAN_STONE : RefineMaterial::MATERIAL_DESTINY_DESTRUCTION_STONE,
                    RefineMaterial::MATERIAL_DESTINY_LEAPSTONE, RefineMaterial::MATERIAL_ABIDOS_FUSION,
                    RefineMaterial::MATERIAL_DESTINY_SHARD, RefineMaterial::MATERIAL_GOLD };
            }
            return { isArmor ? RefineMaterial::MATERIAL_GUARDIAN_CRYSTAL : RefineMaterial::MATERIAL_DESTRUCTION_CRYSTAL,
                RefineMaterial::MATERIAL_GREAT_DESTINY_LEAPSTONE, RefineMaterial::MATERIAL_SUPERIOR_ABIDOS_FUSION,
                RefineMaterial::MATERIAL_DESTINY_SHARD, RefineMaterial::MATERIAL_GOLD };
        }

        // 표의 다섯 칸 → 재료 목록
        [[nodiscard]] std::vector<MaterialAmount> MakeMaterials(const std::array<RefineMaterial, NORMAL_MATERIAL_COUNT>& materials,
            const std::array<double, NORMAL_MATERIAL_COUNT>& amounts)
        {
            std::vector<MaterialAmount> result;
            result.reserve(NORMAL_MATERIAL_COUNT);
            for (std::size_t slot{}; slot < NORMAL_MATERIAL_COUNT; ++slot)
            {
                result.push_back(MaterialAmount{ materials[slot], amounts[slot] });
            }
            return result;
        }
    }

    const MaterialInfo& GetMaterialInfo(RefineMaterial material)
    {
        return MATERIAL_INFOS.at(ToIndex(material));
    }

    std::optional<NormalStep> FindNormalStep(EquipmentTier tier, EquipmentKind kind, int fromLevel)
    {
        const int targetLevel{ fromLevel + 1 };
        const std::span<const NormalStepRow> rows{ GetNormalRows(tier, kind) };
        const auto found{ std::ranges::find(rows, targetLevel, &NormalStepRow::m_targetLevel) };
        if (found == rows.end()) return std::nullopt;

        NormalStep step{ targetLevel, found->m_baseProbability, MakeMaterials(GetStepMaterials(tier, kind), found->m_amounts), std::nullopt, std::nullopt };

        // 숨결: 무기 = 용암, 방어구 = 빙하 (기본 확률별 규칙)
        const RefineMaterial breath{ kind == EquipmentKind::KIND_WEAPON ? RefineMaterial::MATERIAL_LAVA_BREATH : RefineMaterial::MATERIAL_GLACIER_BREATH };
        const auto rule{ std::ranges::find_if(BREATH_RULES, [&step](const BreathRule& candidate)
        {
            return std::abs(candidate.m_baseProbability - step.m_baseProbability) < PROBABILITY_EPSILON;
        }) };
        if (rule != BREATH_RULES.end()) step.m_breath = SupportMaterial{ breath, rule->m_maxCount, rule->m_probability };

        // 강화책: 구간 안의 단계만 (1권 = 기본 확률만큼)
        for (const BookRange& range : GetBookRanges(tier, kind))
        {
            if (targetLevel < range.m_minLevel || targetLevel > range.m_maxLevel) continue;
            step.m_book = SupportMaterial{ range.m_book, BOOK_COUNT_PER_TRY, step.m_baseProbability };
            break;
        }
        return step;
    }

    std::optional<AdvancedBlock> FindAdvancedBlock(EquipmentKind kind, int blockIndex)
    {
        if (blockIndex < 0 || blockIndex >= ADVANCED_BLOCK_COUNT) return std::nullopt;

        const bool isArmor{ kind == EquipmentKind::KIND_ARMOR };
        const AdvancedBlockRow& row{ (isArmor ? ADVANCED_ARMOR_ROWS : ADVANCED_WEAPON_ROWS)[static_cast<std::size_t>(blockIndex)] };
        const RefineMaterial breath{ isArmor ? RefineMaterial::MATERIAL_GLACIER_BREATH : RefineMaterial::MATERIAL_LAVA_BREATH };

        return AdvancedBlock{ MakeMaterials(GetStepMaterials(EquipmentTier::TIER_AEGIR, kind), row.m_amounts),
            MaterialAmount{ breath, row.m_breathAmount }, row.m_book, row.m_hasEnhancedBonus,
            row.m_hasEnhancedBonus ? std::span<const AdvancedTryRow>{ ENHANCED_TRY_ROWS } : std::span<const AdvancedTryRow>{ NO_ENHANCED_TRY_ROWS } };
    }

    int CalculateItemLevel(const EquipmentState& state) noexcept
    {
        if (state.m_tier == EquipmentTier::TIER_SERCA)
        {
            return SERCA_BASE_ITEM_LEVEL + (state.m_enhancement - SERCA_BASE_ENHANCEMENT) * ITEM_LEVEL_PER_ENHANCEMENT;
        }
        return AEGIR_BASE_ITEM_LEVEL + (state.m_enhancement - AEGIR_BASE_ENHANCEMENT) * ITEM_LEVEL_PER_ENHANCEMENT
            + (state.m_advancedEnhancement - MAX_ADVANCED_ENHANCEMENT);
    }

    int GetInheritStartLevel(int aegirEnhancement) noexcept
    {
        const int clamped{ std::clamp(aegirEnhancement, INHERIT_ENHANCEMENT, MAX_ENHANCEMENT) };
        return INHERIT_START_LEVELS[static_cast<std::size_t>(clamped - INHERIT_ENHANCEMENT)];
    }

    bool IsInheritPrepared(const EquipmentState& state) noexcept
    {
        return state.m_tier == EquipmentTier::TIER_SERCA
            || (state.m_enhancement >= INHERIT_ENHANCEMENT && state.m_advancedEnhancement >= INHERIT_ADVANCED_ENHANCEMENT);
    }

    bool IsValidState(const EquipmentState& state) noexcept
    {
        if (state.m_tier == EquipmentTier::TIER_SERCA)
        {
            return state.m_enhancement >= MIN_SERCA_ENHANCEMENT && state.m_enhancement <= MAX_ENHANCEMENT && state.m_advancedEnhancement == 0;
        }
        return state.m_enhancement >= MIN_AEGIR_ENHANCEMENT && state.m_enhancement <= MAX_ENHANCEMENT
            && state.m_advancedEnhancement >= 0 && state.m_advancedEnhancement <= MAX_ADVANCED_ENHANCEMENT;
    }
}
