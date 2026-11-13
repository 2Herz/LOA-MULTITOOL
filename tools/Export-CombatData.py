"""전투력 시뮬레이터 계수 JSON(data/CombatCoefficients.json) → C++ 상수 표(Combat_Coefficient_Data.cpp) 생성 도구.

사용법:
    python tools/Export-CombatData.py [입력 JSON] [출력 .cpp] [장비 재련 정보 CSV]
인자를 생략하면 프로젝트 폴더 기준 data/CombatCoefficients.json 을 읽어 Combat_Coefficient_Data.cpp 에 쓴다.
CSV(기본: 바탕화면\장비\장비데이터\장비_재련정보.csv)가 있으면 먼저 에스더 무기의 단계별 아이템 레벨(ItemLevels)을
JSON 에 채워 넣고 저장한다 (CSV 가 없으면 JSON 에 있던 값을 그대로 씀).

규칙:
    - 선언(모듈 인터페이스)은 손으로 쓴 Combat_Coefficient_Data.ixx 에만 있고, 이 도구는 정의(.cpp)만 만든다.
    - 생성된 .cpp 는 직접 고치지 말고 JSON 을 고친 뒤 이 도구를 다시 실행한다.
    - JSON 의 null(측정하지 않은 칸)은 MISSING_STAT(-1) 으로 바꿔 넣는다.
    - 모든 값은 이름 있는 constexpr 상수로 내보낸다 (매직넘버 금지 규칙).
"""
import csv
import json
import math
import os
import sys

# ===== 경로 =====
PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # tools 폴더의 상위 = 프로젝트 폴더
DEFAULT_INPUT = os.path.join(PROJECT_DIR, 'data', 'CombatCoefficients.json')
DEFAULT_OUTPUT = os.path.join(PROJECT_DIR, 'Combat_Coefficient_Data.cpp')
DEFAULT_CSV = os.path.join(os.path.expanduser('~'), 'Desktop', '장비', '장비데이터', '장비_재련정보.csv')

# ===== 장비 재련 정보 CSV =====
CSV_ENCODING = 'utf-8-sig'                 # 엑셀에서 저장한 BOM 있는 UTF-8
CSV_KEY_COLUMN = '분류'
CSV_KIND_COLUMN = '재련종류'
CSV_STEP_COLUMN = '재련단계'
CSV_LEVEL_COLUMN = '아이템레벨'            # "1830" 또는 "1675 (티어 4)"
CSV_NORMAL_KINDS = ('기본', '일반재련')     # 일반 단계 행 (상급재련 행은 제외)
ESTHER_KEY_PREFIX = '에스더'               # 단계마다 아이템 레벨이 불규칙한 분류
JSON_ITEM_LEVELS_KEY = 'ItemLevels'
JSON_SEPARATORS = (',', ':')               # 원본 JSON 과 같은 압축 형식

# ===== 스키마 =====
SUPPORTED_VERSION = 2                     # 읽을 수 있는 JSON 스키마 버전 (2 = 악세 · 팔찌 · 스톤 · 각인 · 아바타 · 아크 그리드 · 아크 패시브 · 완갑 추가)
STAT_KINDS = {                            # JSON Stat 값 → C++ 열거형 값
    'MAIN_STAT': 'StatKind::STAT_MAIN',
    'WEAPON_ATTACK': 'StatKind::STAT_WEAPON_ATTACK',
}
GEM_COMPONENT_ID = 'GEM_LEVEL'            # 보석 레벨 계수 표
QUALITY_COMPONENT_ID = 'WEAPON_QUALITY'   # 무기 품질 곡선
DEFAULT_WEAPON_ATTACK_EXTRA = 21500.0     # JSON에 값이 없을 때 쓰는 무기 공격력 보정 (lopec 측정 맞춤값)
VALUES_PER_LINE = 10                      # 생성 파일 한 줄에 넣을 숫자 수
INDENT = '    '                           # 생성 파일 들여쓰기 (4칸)

# ===== 추가 시스템 컴포넌트 Id (JSON Components 의 Id) =====
ACCESSORY_ID = 'ACCESSORY_OPTION'
BRACELET_STAT_ID = 'BRACELET_STAT'
BRACELET_OPTION_ID = 'BRACELET_OPTION'
STONE_ID = 'STONE_ENGRAVING'
BOOK_ID = 'ENGRAVING_BOOK'
AVATAR_ID = 'AVATAR'
CORE_GRADE_ID = 'ARK_CORE_GRADE'
CORE_POINT_ID = 'ARK_CORE_POINT'
ARK_GEM_ID = 'ARK_GEM'
PASSIVE_ID = 'ARK_PASSIVE'
VAMBRACE_ID = 'VAMBRACE_STATS'
CORE_COUNT = 6                            # 아크 그리드 코어 수
RANK_COUNT = 7                            # 아크 패시브 랭크 0 ~ 6

# ===== 분류 키 → C++ 식별자 (한글 키를 영문 상수 이름으로) =====
NAME_WORDS = {
    '결단': 'DECISION', '업화': 'KARMA', '전율': 'THRILL', '에스더': 'ESTHER',
    '머리': 'HEAD', '무기': 'WEAPON', '상의': 'CHEST', '하의': 'PANTS', '장갑': 'GLOVES', '어깨': 'SHOULDER',
    '완갑': 'VAMBRACE', '고대': 'ANCIENT', '유물': 'RELIC', '전설': 'LEGENDARY', '영웅': 'EPIC',
    '기가': 'GIGA', '네오': 'NEO', '에크혼': 'ECKHORN', '테라': 'TERRA',
}


def to_identifier(key, index):
    """'결단_머리' → 'DECISION_HEAD' (모르는 단어가 있으면 TABLE_번호)."""
    words = key.split('_')
    if all(word in NAME_WORDS for word in words):
        return '_'.join(NAME_WORDS[word] for word in words)
    return f'TABLE_{index}'


def format_number(value):
    """숫자 → C++ double 리터럴 (null 이면 MISSING_STAT)."""
    if value is None:
        return 'MISSING_STAT'
    number = float(value)
    if not math.isfinite(number):
        raise ValueError(f'유한하지 않은 값: {value}')
    return repr(number)


def format_array(name, values, comment, first_index=0, index_label='칸'):
    """constexpr std::array 한 개 (여러 줄로 나눠서, 줄마다 칸 번호 범위를 주석으로 붙임)."""
    lines = [f'{INDENT * 2}// {comment}',
             f'{INDENT * 2}constexpr std::array<double, {len(values)}> {name}{{   // 총 {len(values)}칸']
    for start in range(0, len(values), VALUES_PER_LINE):
        chunk = ', '.join(format_number(value) for value in values[start:start + VALUES_PER_LINE])
        last = min(start + VALUES_PER_LINE, len(values)) - 1   # 이 줄의 마지막 칸 번호
        lines.append(f'{INDENT * 3}{chunk},   // {index_label} {first_index + start} ~ {first_index + last}')
    lines.append(f'{INDENT * 2}}};   // {name} 끝')
    return '\n'.join(lines)


def find_min_advanced(values, first_level, base_level, normal_step, advanced_step, normal_count):
    """상급 재련 1단계 값이 있는 가장 낮은 일반 단계 (없으면 -1 → NO_ADVANCED_ENHANCEMENT 와 같은 뜻)."""
    for normal in range(normal_count):
        index = base_level + normal * normal_step + advanced_step - first_level
        if 0 <= index < len(values) and values[index] is not None:
            return normal
    return -1


def read_item_levels(csv_path):
    """CSV → {에스더 분류: {일반 단계: 아이템 레벨}} (CSV 가 없으면 None)."""
    if not os.path.exists(csv_path):
        return None
    levels = {}
    with open(csv_path, encoding=CSV_ENCODING, newline='') as file:
        for row in csv.DictReader(file):
            key = row.get(CSV_KEY_COLUMN, '')
            if not key.startswith(ESTHER_KEY_PREFIX) or row.get(CSV_KIND_COLUMN) not in CSV_NORMAL_KINDS:
                continue
            level_text = (row.get(CSV_LEVEL_COLUMN) or '').split()   # "1675 (티어 4)" → "1675"
            if level_text:
                levels.setdefault(key, {})[int(row[CSV_STEP_COLUMN])] = int(level_text[0])
    return levels


def merge_item_levels(data, levels):
    """에스더 표마다 0 ~ 마지막 단계 아이템 레벨이 모두 있으면 ItemLevels 로 넣음 (바뀌었으면 True)."""
    changed = False
    for key, table in data['Equipment']['Tables'].items():
        steps = levels.get(key)
        if not key.startswith(ESTHER_KEY_PREFIX) or not steps:
            continue
        count = len(table['Normal'])
        if any(step not in steps for step in range(count)):
            print(f'경고: {key} 의 아이템 레벨이 CSV 에 모두 있지 않아 기존 값을 유지합니다.')
            continue
        item_levels = [steps[step] for step in range(count)]
        if table.get(JSON_ITEM_LEVELS_KEY) != item_levels:
            table[JSON_ITEM_LEVELS_KEY] = item_levels
            changed = True
    return changed


def format_int_array(name, values, comment):
    """constexpr std::array<int> 한 개 (단계별 아이템 레벨)."""
    body = ', '.join(str(int(value)) for value in values)
    return '\n'.join([f'{INDENT * 2}// {comment}',
                      f'{INDENT * 2}constexpr std::array<int, {len(values)}> {name}{{ {body} }};   // 일반 재련 단계 0 ~ {len(values) - 1}의 아이템 레벨'])


def find_component(components, component_id):
    """Components 배열에서 Id 가 같은 항목 (없으면 오류)."""
    for component in components:
        if component.get('Id') == component_id:
            return component
    raise KeyError(f'Components 에 {component_id} 가 없습니다.')


def cpp_string(text):
    """문자열 → C++ 문자열 리터럴 (UTF-8 그대로, 따옴표 · 역슬래시만 이스케이프)."""
    return '"' + str(text).replace('\\', '\\\\').replace('"', '\\"') + '"'


def format_keyed(name, pairs, comment):
    """constexpr std::array<KeyedValue> 한 개 (문구 키 → 값)."""
    lines = [f'{INDENT * 2}// {comment}',
             f'{INDENT * 2}constexpr std::array<KeyedValue, {len(pairs)}> {name}{{ {{   // 문구 키 {len(pairs)}개']
    for key, value in pairs:
        lines.append(f'{INDENT * 3}KeyedValue{{ {cpp_string(key)}, {format_number(value)} }},   // 키 → 값')
    lines.append(f'{INDENT * 2}}} }};   // {name} 끝')
    return '\n'.join(lines)


def format_strings(name, values, comment):
    """constexpr std::array<std::string_view> 한 개 (선택지 목록)."""
    lines = [f'{INDENT * 2}// {comment}',
             f'{INDENT * 2}constexpr std::array<std::string_view, {len(values)}> {name}{{ {{   // 선택지 {len(values)}개']
    for value in values:
        lines.append(f'{INDENT * 3}{cpp_string(value)},   // 선택지')
    lines.append(f'{INDENT * 2}}} }};   // {name} 끝')
    return '\n'.join(lines)


def build_extra(data):
    """추가 시스템 표 → (익명 네임스페이스 안 상수 줄들, 접근 함수 (선언, 반환 식) 목록)."""
    components = data['Components']
    cands = data['Candidates']
    consts = []

    # ----- 악세서리 -----
    acc = find_component(components, ACCESSORY_ID)
    params = acc['Params']
    consts.append(format_keyed('ACCESSORY_OPTIONS', list(acc['Values'].items()), '악세서리 연마 옵션 (정확한 문구 → ln 배율, 무기 공격력 옵션은 식으로 따로 계산)'))
    consts.append(f'{INDENT * 2}constexpr AccessoryFormula ACCESSORY_FORMULA{{ {format_number(params["AddDamageBase"])}, {format_number(params["AttackFlatReference"])}, '
                  f'{format_number(params["CritRateBase"])}, {format_number(params["CritDamageBase"])}, {format_number(params["WeaponRatioEsther"])}, {format_number(params["WeaponRatioReforge"])} }};   // 악세서리 식 계수 (추가 피해 · 공격력 기준 · 치명 확률 · 치명 피해 · 에스더 무기 비율 · 재련 무기 비율)')
    consts.append(format_strings('ACCESSORY_NECKLACE_CANDIDATES', cands['AccessoryNecklace'], '목걸이 연마 옵션 선택지'))
    consts.append(format_strings('ACCESSORY_EARRING_CANDIDATES', cands['AccessoryEarring'], '귀걸이 연마 옵션 선택지'))
    consts.append(format_strings('ACCESSORY_RING_CANDIDATES', cands['AccessoryRing'], '반지 연마 옵션 선택지'))
    consts.append('')

    # ----- 팔찌 -----
    bst = find_component(components, BRACELET_STAT_ID)
    bopt = find_component(components, BRACELET_OPTION_ID)
    consts.append(f'{INDENT * 2}// ===== 팔찌 (스탯 1포인트당 배율 · 옵션은 모든 공백을 지운 문구가 키) =====')
    consts.append(f'{INDENT * 2}constexpr double BRACELET_STAT_RATIO{{ {format_number(bst["RatioPerPoint"])} }};   // 팔찌 스탯 1포인트당 배율')
    consts.append(format_strings('BRACELET_STAT_TYPES', bst['AllTypes'], '팔찌 스탯 종류 선택지'))
    consts.append(format_strings('BRACELET_EFFECTIVE_TYPES', bst['EffectiveTypes'], '전투력에 같은 비율로 반영되는 스탯 (치명 · 특화 · 신속)'))
    consts.append(format_strings('BRACELET_MAIN_STAT_TYPES', bst['MainStatTypes'], '주스탯으로 더해지는 스탯 (힘 · 민첩 · 지능)'))
    consts.append(format_keyed('BRACELET_OPTIONS', list(bopt['Values'].items()), '팔찌 옵션 (공백 없는 문구 → ln 배율)'))
    weapon_lines = bopt['WeaponAtkLines']
    consts.append(f'{INDENT * 2}// 무기 공격력이 들어간 팔찌 옵션 (ln = 더하는 값 + 0.5 × ln(1 + 무기 공격력 / W0))')
    consts.append(f'{INDENT * 2}constexpr std::array<BraceletWeaponLine, {len(weapon_lines)}> BRACELET_WEAPON_LINES{{ {{   // 무기 공격력 포함 팔찌 옵션 {len(weapon_lines)}개')
    for key, line in weapon_lines.items():
        consts.append(f'{INDENT * 3}BraceletWeaponLine{{ {cpp_string(key)}, {format_number(line["WeaponFlat"])}, {format_number(line["LogAdd"])} }},   // 문구 키 · 무기 공격력 고정값 · 더하는 ln')
    consts.append(f'{INDENT * 2}}} }};   // BRACELET_WEAPON_LINES 끝')
    consts.append(format_strings('BRACELET_OPTION_CANDIDATES', cands['BraceletOption'], '팔찌 옵션 선택지'))
    consts.append('')

    # ----- 스톤 · 각인서 -----
    stone = find_component(components, STONE_ID)
    consts.append(format_keyed('STONE_LEVEL_ONE', list(stone['LevelOne'].items()), '스톤 각인 레벨 1 효과 (각인 이름 → 같은 이름 각인서에 더하는 비율, 표에 없는 이름은 0)'))
    scale = stone['LevelScale']
    consts.append(f'{INDENT * 2}constexpr std::array<double, {len(scale)}> STONE_LEVEL_SCALE{{ {{ {", ".join(format_number(float(v)) for v in scale)} }} }};   // 스톤 레벨 0 ~ 4 → 레벨 1 효과 대비 배수')
    consts.append(f'{INDENT * 2}constexpr StoneSumBonus STONE_SUM_BONUS{{ {int(stone["SumBonusMinLevel"])}, {format_number(stone["SumBonusLn"])} }};   // 스톤 두 줄 레벨 합 보너스 (최소 합, ln)')
    consts.append(format_strings('STONE_CANDIDATES', cands['StoneEngraving'], '스톤 각인 이름 선택지'))
    book = find_component(components, BOOK_ID)
    consts.append(f'{INDENT * 2}// 각인서 (ln = 기본 + (최대 - 기본) / 4 × 각인서 레벨, 효과 없는 각인은 0)')
    entries = [(name, book['E0'][name], book['E4'][name]) for name in book['E0']] + [(name, 0.0, 0.0) for name in book['ZeroNames']]
    consts.append(f'{INDENT * 2}constexpr std::array<EngravingBook, {len(entries)}> ENGRAVING_BOOKS{{ {{   // 각인서 {len(entries)}개')
    for name, base_value, top_value in entries:
        consts.append(f'{INDENT * 3}EngravingBook{{ {cpp_string(name)}, {format_number(base_value)}, {format_number(top_value)} }},   // 이름 · 레벨 0 ln · 레벨 4 ln')
    consts.append(f'{INDENT * 2}}} }};   // ENGRAVING_BOOKS 끝')
    consts.append(format_strings('BOOK_CANDIDATES', cands['EngravingBook'], '각인서 이름 선택지'))
    legend = book['GradeLegendRatio']
    consts.append(f'{INDENT * 2}constexpr LevelRange LEGEND_RATIO{{ {format_number(legend["X0"])}, {format_number(legend["X4"])} }};   // 전설 각인서 효과 = 유물 레벨 0 효과 × 비율 (레벨 0, 레벨 4)')
    consts.append('')

    # ----- 아바타 -----
    avatar = find_component(components, AVATAR_ID)   # 아바타 요소 (부위별 주스탯 % · 주스탯 배율 계수)
    consts.append(format_keyed('AVATAR_PERCENT', list(avatar['PercentPerSlot'].items()), '아바타 한 부위의 주스탯 % (등급 이름 → 값)'))
    consts.append(f'{INDENT * 2}constexpr double AVATAR_MAIN_STAT_SCALE{{ {format_number(avatar["MainStatScale"])} }};   // 아바타 배율 = sqrt((1 + 이 값 × 목표 % 합) / (1 + 이 값 × 현재 % 합))')
    consts.append('')

    # ----- 아크 그리드 -----
    grade = find_component(components, CORE_GRADE_ID)['LnGainAncientVsOthers']
    gains = [grade[str(core)] for core in range(CORE_COUNT)]
    consts.append(format_array('ARK_CORE_ANCIENT_GAINS', gains, '코어 번호별 고대 등급 추가 ln'))
    kinds = find_component(components, CORE_POINT_ID)['Kinds']
    point_rows = sorted((kind['Core'], kind['Name'], int(points), values[0], values[1]) for kind in kinds for points, values in kind['Points'].items())
    consts.append(f'{INDENT * 2}// 코어 번호 · 코어 이름(비면 이름 무관) · 의지력 포인트 합 → ln (평탄 구간 기준, 정확히 이 포인트일 때만 켜짐), 마지막 값은 고대 등급')
    consts.append(f'{INDENT * 2}constexpr std::array<ArkCorePoint, {len(point_rows)}> ARK_CORE_POINTS{{ {{   // 코어 포인트 칸 {len(point_rows)}개')
    for core, name, points, normal, ancient in point_rows:
        consts.append(f'{INDENT * 3}ArkCorePoint{{ {core}, {cpp_string(name)}, {points}, {format_number(normal)}, {format_number(ancient)} }},   // 코어 번호 · 이름 · 포인트 · ln · 고대 ln')
    consts.append(f'{INDENT * 2}}} }};   // ARK_CORE_POINTS 끝')
    gem = find_component(components, ARK_GEM_ID)['LnPerLevel']
    consts.append(format_keyed('ARK_GEM_PER_LEVEL', list(gem.items()), '아크 그리드 젬 효과 레벨 1당 ln (효과 이름 → 값)'))
    consts.append('')

    # ----- 아크 패시브 -----
    passive = find_component(components, PASSIVE_ID)
    consts.append(format_keyed('ENLIGHTENMENT_RATIOS', list(passive['EnlightenmentPoint'].items()), '깨달음 포인트 구성 → 전투력 배율 (최대 구성 = 1)'))
    ranks = [passive['RankControl'][str(rank)] for rank in range(RANK_COUNT)]
    consts.append(format_array('PASSIVE_RANK_RATIOS', ranks, '아크 패시브 랭크 0 ~ 6 → 전투력 배율'))
    consts.append(f'{INDENT * 2}constexpr PassiveSlopes PASSIVE_SLOPES{{ {format_number(passive["Level1_RatioPerLevel"])}, {format_number(passive["Level2_RatioPerLevel"])} }};   // 레벨 1 · 2 감소 기울기')
    interaction = passive['RankEnlightenmentInteraction']   # 랭크 · 깨달음 포인트가 둘 다 최대가 아닐 때 상호작용
    consts.append(f'{INDENT * 2}constexpr PassiveInteraction PASSIVE_INTERACTION{{ {format_number(interaction["ConstantLn"])}, {format_number(interaction["CrossLn"])} }};   // 랭크 < 최대 · 깨달음 < 최대일 때 ln 상수 · 교차항')
    consts.append('')

    # ----- 완갑 (등급 · 재련 단계별 무기 공격력 / 주스탯 / 기본 공격력) -----
    vambrace = find_component(components, VAMBRACE_ID)
    consts.append(f'{INDENT * 2}// ===== 완갑 재련 (등급마다 재련 단계별 무기 공격력 · 주스탯 · 기본 공격력) =====')
    grade_rows = []
    for index, (name, grade) in enumerate(vambrace['Grades'].items()):
        array_name = f'VAMBRACE_STEPS_{index}'
        consts.append(f'{INDENT * 2}// {name} 등급 {grade["FirstStep"]}단계부터 (무기 공격력, 주스탯, 기본 공격력)')
        consts.append(f'{INDENT * 2}constexpr std::array<VambraceStep, {len(grade["Steps"])}> {array_name}{{ {{   // 완갑 단계 {len(grade["Steps"])}개')
        for weapon, main_stat, base_attack in grade['Steps']:
            consts.append(f'{INDENT * 3}VambraceStep{{ {format_number(float(weapon))}, {format_number(float(main_stat))}, {format_number(float(base_attack))} }},   // 무기 공격력 · 주스탯 · 기본 공격력')
        consts.append(f'{INDENT * 2}}} }};   // {array_name} 끝')
        grade_rows.append((name, grade['FirstStep'], array_name))
    consts.append(f'{INDENT * 2}constexpr std::array<VambraceGrade, {len(grade_rows)}> VAMBRACE_GRADES{{ {{   // 완갑 등급 {len(grade_rows)}개')
    for name, first_step, array_name in grade_rows:
        consts.append(f'{INDENT * 3}VambraceGrade{{ {cpp_string(name)}, {first_step}, {array_name} }},   // 등급 이름 · 첫 단계 · 단계 표')
    consts.append(f'{INDENT * 2}}} }};   // VAMBRACE_GRADES 끝')
    bonuses = vambrace['GradeBonus']
    consts.append(f'{INDENT * 2}// 완갑 한계 해방 추가 효과 (등급 이름, 확정 첫 단계, 기본 공격력 비율)')
    consts.append(f'{INDENT * 2}constexpr std::array<VambraceGradeBonus, {len(bonuses)}> VAMBRACE_GRADE_BONUSES{{ {{   // 완갑 등급 {len(bonuses)}개')
    for name, first_step, rate in bonuses:
        consts.append(f'{INDENT * 3}VambraceGradeBonus{{ {cpp_string(name)}, {first_step}, {format_number(float(rate))} }},   // 등급 이름 · 확정 첫 단계 · 추가 효과 기본 공격력 비율')
    consts.append(f'{INDENT * 2}}} }};   // VAMBRACE_GRADE_BONUSES 끝')
    consts.append(f'{INDENT * 2}constexpr double BASE_ATTACK_CALIBRATION{{ {format_number(vambrace["BaseAttackCalibration"])} }};   // 기본 공격력 A 추정식 보정 (한 측정점)')
    attack = data['AttackModel']
    consts.append(f'{INDENT * 2}// ===== S0 · W0 보정 ({attack["CalibrationNote"]}) =====')
    consts.append(f'{INDENT * 2}constexpr double S0_CALIBRATION{{ {format_number(attack["S0Calibration"])} }};   // 주스탯 합 S0 보정')
    consts.append(f'{INDENT * 2}constexpr double W0_CALIBRATION{{ {format_number(attack["W0Calibration"])} }};   // 무기 공격력 W0 보정')

    funcs = [
        ('std::span<const KeyedValue> GetAccessoryOptions() noexcept', 'ACCESSORY_OPTIONS', '악세서리 연마 옵션 ln 표'),
        ('const AccessoryFormula& GetAccessoryFormula() noexcept', 'ACCESSORY_FORMULA', '악세서리 식 계수'),
        ('double GetBraceletStatRatio() noexcept', 'BRACELET_STAT_RATIO', '팔찌 스탯 1포인트당 배율'),
        ('std::span<const std::string_view> GetBraceletStatTypes() noexcept', 'BRACELET_STAT_TYPES', '팔찌 스탯 종류 선택지'),
        ('std::span<const std::string_view> GetBraceletEffectiveTypes() noexcept', 'BRACELET_EFFECTIVE_TYPES', '전투력에 같은 비율로 반영되는 팔찌 스탯 종류'),
        ('std::span<const std::string_view> GetBraceletMainStatTypes() noexcept', 'BRACELET_MAIN_STAT_TYPES', '주스탯으로 더해지는 팔찌 스탯 종류'),
        ('std::span<const KeyedValue> GetBraceletOptions() noexcept', 'BRACELET_OPTIONS', '팔찌 옵션 ln 표'),
        ('std::span<const BraceletWeaponLine> GetBraceletWeaponLines() noexcept', 'BRACELET_WEAPON_LINES', '무기 공격력이 들어간 팔찌 옵션 줄'),
        ('std::span<const std::string_view> GetBraceletOptionCandidates() noexcept', 'BRACELET_OPTION_CANDIDATES', '팔찌 옵션 선택지'),
        ('std::span<const KeyedValue> GetStoneLevelOne() noexcept', 'STONE_LEVEL_ONE', '스톤 각인 이름별 레벨 1 효과 비율'),
        ('std::span<const double> GetStoneLevelScale() noexcept', 'STONE_LEVEL_SCALE', '스톤 레벨별 배수'),
        ('const StoneSumBonus& GetStoneSumBonus() noexcept', 'STONE_SUM_BONUS', '스톤 두 줄 레벨 합 보너스'),
        ('std::span<const std::string_view> GetStoneCandidates() noexcept', 'STONE_CANDIDATES', '스톤 각인 이름 선택지'),
        ('std::span<const EngravingBook> GetEngravingBooks() noexcept', 'ENGRAVING_BOOKS', '각인서 효과 표'),
        ('std::span<const std::string_view> GetBookCandidates() noexcept', 'BOOK_CANDIDATES', '각인서 이름 선택지'),
        ('const LevelRange& GetLegendRatio() noexcept', 'LEGEND_RATIO', '전설 각인서 효과 비율'),
        ('std::span<const KeyedValue> GetAvatarPercent() noexcept', 'AVATAR_PERCENT', '아바타 등급별 한 부위 주스탯 %'),
        ('double GetAvatarMainStatScale() noexcept', 'AVATAR_MAIN_STAT_SCALE', '아바타 주스탯 % 합 배율 계수'),
        ('std::span<const double> GetArkCoreAncientGains() noexcept', 'ARK_CORE_ANCIENT_GAINS', '코어 번호별 고대 등급 추가 ln'),
        ('std::span<const ArkCorePoint> GetArkCorePoints() noexcept', 'ARK_CORE_POINTS', '코어 의지력 포인트별 ln 표'),
        ('std::span<const KeyedValue> GetArkGemPerLevel() noexcept', 'ARK_GEM_PER_LEVEL', '아크 그리드 젬 효과 레벨 1당 ln'),
        ('std::span<const KeyedValue> GetEnlightenmentRatios() noexcept', 'ENLIGHTENMENT_RATIOS', '깨달음 포인트 구성별 배율'),
        ('std::span<const double> GetPassiveRankRatios() noexcept', 'PASSIVE_RANK_RATIOS', '아크 패시브 랭크별 배율'),
        ('const PassiveSlopes& GetPassiveSlopes() noexcept', 'PASSIVE_SLOPES', '아크 패시브 레벨 감소 기울기'),
        ('const PassiveInteraction& GetPassiveInteraction() noexcept', 'PASSIVE_INTERACTION', '아크 패시브 랭크 · 깨달음 상호작용'),
        ('std::span<const VambraceGrade> GetVambraceGrades() noexcept', 'VAMBRACE_GRADES', '완갑 등급별 재련 단계 표'),
        ('std::span<const VambraceGradeBonus> GetVambraceGradeBonuses() noexcept', 'VAMBRACE_GRADE_BONUSES', '완갑 등급별 한계 해방 추가 효과 표'),
        ('double GetBaseAttackCalibration() noexcept', 'BASE_ATTACK_CALIBRATION', '기본 공격력 추정식 보정'),
        ('double GetMainStatCalibration() noexcept', 'S0_CALIBRATION', '주스탯 합 S0 보정'),
    ]
    return consts, funcs


def build_special_functions():
    """고정 형태의 접근 함수: 악세서리 종류별 선택지 · 문구 키 표 찾기."""
    return [f'{INDENT}// 악세서리 종류별 연마 옵션 선택지',
            f'{INDENT}std::span<const std::string_view> GetAccessoryCandidates(AccessoryKind kind) noexcept',
            f'{INDENT}{{',
            f'{INDENT * 2}switch (kind)   // 악세서리 종류에 따라 분기',
            f'{INDENT * 2}{{',
            f'{INDENT * 3}case AccessoryKind::KIND_NECKLACE:   // 목걸이',
            f'{INDENT * 4}return ACCESSORY_NECKLACE_CANDIDATES;   // 목걸이 선택지 반환',
            f'{INDENT * 3}case AccessoryKind::KIND_EARRING:   // 귀걸이',
            f'{INDENT * 4}return ACCESSORY_EARRING_CANDIDATES;   // 귀걸이 선택지 반환',
            f'{INDENT * 3}case AccessoryKind::KIND_RING:   // 반지',
            f'{INDENT * 4}return ACCESSORY_RING_CANDIDATES;   // 반지 선택지 반환',
            f'{INDENT * 2}}}',
            f'{INDENT * 2}return {{}};   // 모르는 종류는 빈 목록',
            f'{INDENT}}}',
            '',
            f'{INDENT}// 보석 종류 계수 (표에 없는 종류 · 빈 글자는 1)',
            f'{INDENT}double GetGemTypeFactor(std::string_view kind) noexcept',
            f'{INDENT}{{',
            f'{INDENT * 2}const auto found{{ std::ranges::find(GEM_TYPE_FACTORS, kind, &KeyedValue::m_key) }};   // 종류가 같은 항목 검색',
            f'{INDENT * 2}return found != GEM_TYPE_FACTORS.end() ? found->m_value : 1.0;   // 없으면 계수 1',
            f'{INDENT}}}',
            '',
            f'{INDENT}// 무기 공격력 → 보정한 W0 ((툴팁 공격력 + 보정) × W0 보정, 공격력을 모르면 0)',
            f'{INDENT}double GetWeaponBase(double weaponAttack) noexcept',
            f'{INDENT}{{',
            f'{INDENT * 2}return weaponAttack > 0.0 ? (weaponAttack + WEAPON_ATTACK_EXTRA) * W0_CALIBRATION : 0.0;   // 공격력을 모르면 0',
            f'{INDENT}}}',
            '',
            f'{INDENT}// 완갑 재련 단계의 능력치 (등급 표에서 찾음, 0 ~ 25 밖이면 nullptr)',
            f'{INDENT}const VambraceStep* FindVambraceStep(int step) noexcept',
            f'{INDENT}{{',
            f'{INDENT * 2}for (const VambraceGrade& grade : VAMBRACE_GRADES)   // 등급 표를 차례로 확인',
            f'{INDENT * 2}{{',
            f'{INDENT * 3}const int index{{ step - grade.m_firstStep }};   // 등급 첫 단계 기준 칸 번호',
            f'{INDENT * 3}if (index >= 0 && static_cast<std::size_t>(index) < grade.m_steps.size()) return &grade.m_steps[static_cast<std::size_t>(index)];   // 범위 안이면 그 칸 주소',
            f'{INDENT * 2}}}',
            f'{INDENT * 2}return nullptr;   // 어느 등급에도 없으면 nullptr',
            f'{INDENT}}}',
            '',
            f'{INDENT}// 문구 키 표에서 키로 값 찾기 (없으면 nullptr)',
            f'{INDENT}const KeyedValue* FindKeyed(std::span<const KeyedValue> table, std::string_view key) noexcept',
            f'{INDENT}{{',
            f'{INDENT * 2}const auto found{{ std::ranges::find(table, key, &KeyedValue::m_key) }};   // 키가 같은 항목 검색',
            f'{INDENT * 2}return found != table.end() ? &*found : nullptr;   // 찾았으면 주소, 없으면 nullptr',
            f'{INDENT}}}',
            '']


def build_source(data, input_name):
    """JSON → 생성 .cpp 전체 텍스트."""
    if data.get('Version') != SUPPORTED_VERSION:
        raise ValueError(f'지원하지 않는 스키마 버전: {data.get("Version")}')

    equipment = data['Equipment']
    base_level = int(equipment['BaseItemLevel'])
    normal_step = int(equipment['NormalStepLevel'])
    advanced_step = int(equipment['AdvancedStepLevel'])
    bonuses = sorted(equipment['AdvancedBonus'], key=lambda bonus: bonus['MinStep'])
    max_advanced = int(bonuses[-1]['MinStep'])   # 마지막 보너스 구간 = 상급 재련 최대 단계

    # 보석: 첫 레벨부터 빈틈없이 이어져야 표 번호 = 레벨 - 첫 레벨
    gem = find_component(data['Components'], GEM_COMPONENT_ID)
    gem_keys = [int(key) for key in gem['Keys']]
    if not gem_keys or gem_keys != list(range(gem_keys[0], gem_keys[0] + len(gem_keys))):
        raise ValueError('보석 Keys 는 빈틈없이 이어져야 합니다.')

    # 무기 품질: F(q) = A + B·(q/최대)² → A = F(0), B = F(최대) - F(0)
    quality = find_component(data['Components'], QUALITY_COMPONENT_ID)
    points = sorted(quality['Points'], key=lambda point: point[0])
    quality_max = float(points[-1][0])
    quality_base = float(points[0][1])
    quality_scale = float(points[-1][1]) - quality_base

    weapon_extra = float(data.get('AttackModel', {}).get('ExtraWeaponAtk', DEFAULT_WEAPON_ATTACK_EXTRA))

    out = []
    out.append('// ⚠️ 자동 생성 파일 — 직접 고치지 말 것')
    out.append(f'// 원본: {input_name} → python tools/Export-CombatData.py 로 다시 만듦')
    out.append('// 선언은 Combat_Coefficient_Data.ixx (손으로 쓴 인터페이스), 여기는 값 · 접근 함수 정의만')
    out.append('// 구성: 익명 네임스페이스 안의 constexpr 상수 표(장비 · 보석 · 품질 · 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 · 완갑)')
    out.append('//       → 그 아래 네임스페이스에 접근 함수 (표를 span / 참조로 돌려줌, 실행할 때 파일을 읽지 않음)')
    out.append('// 표의 null(측정하지 않은 칸)은 MISSING_STAT(-1)으로 들어가고, 줄 끝 주석은 칸 번호 범위 · 항목 뜻을 알려 줌')
    out.append('module LostArkMultiTool.Models.CombatCoefficientData;   // 계수 데이터 모듈 구현 파일')
    out.append('')
    out.append('import std;   // 표준 라이브러리 모듈 가져오기')
    out.append('')
    out.append('namespace LostArkMultiTool::Models::CombatCoefficientData   // 계수 데이터 네임스페이스 시작')
    out.append('{')
    out.append(f'{INDENT}namespace   // 파일 내부 전용 상수 영역 시작')
    out.append(f'{INDENT}{{')
    out.append(f'{INDENT * 2}// ===== 아이템 레벨 규칙 (부위 레벨 = 기본 + 일반 단계 × 일반 계수 + 상급 단계 × 상급 계수) =====')
    out.append(f'{INDENT * 2}constexpr int BASE_ITEM_LEVEL{{ {base_level} }};   // 기본 아이템 레벨 (재련 0단계)')
    out.append(f'{INDENT * 2}constexpr int NORMAL_STEP_LEVEL{{ {normal_step} }};   // 일반 재련 한 단계당 오르는 아이템 레벨')
    out.append(f'{INDENT * 2}constexpr int ADVANCED_STEP_LEVEL{{ {advanced_step} }};   // 상급 재련 한 단계당 오르는 아이템 레벨')
    out.append(f'{INDENT * 2}constexpr int MAX_ADVANCED_ENHANCEMENT{{ {max_advanced} }};   // 상급 재련 최대 단계 (마지막 보너스 구간)')
    out.append(f'{INDENT * 2}constexpr ItemLevelRule ITEM_LEVEL_RULE{{ BASE_ITEM_LEVEL, NORMAL_STEP_LEVEL, ADVANCED_STEP_LEVEL, MAX_ADVANCED_ENHANCEMENT }};   // 아이템 레벨 규칙 묶음')
    out.append('')
    out.append(f'{INDENT * 2}// ===== 상급 재련 보너스 (상급 단계가 기준 이상이면 능력치 × 배율, 소수점 버림) =====')
    out.append(f'{INDENT * 2}constexpr std::array<AdvancedBonus, {len(bonuses)}> ADVANCED_BONUSES{{ {{   // 상급 보너스 구간 {len(bonuses)}개')
    for bonus in bonuses:
        out.append(f'{INDENT * 3}AdvancedBonus{{ {int(bonus["MinStep"])}, {format_number(bonus["Multiplier"])} }},   // 시작 단계 · 능력치 배율')
    out.append(f'{INDENT * 2}}} }};   // ADVANCED_BONUSES 끝')
    out.append('')

    # 장비 표: 세트_부위마다 일반 배열 + (있으면) 아이템 레벨별 배열
    out.append(f'{INDENT * 2}constexpr int NO_FIRST_LEVEL{{ 0 }};          // 레벨별 표가 없는 분류')
    out.append(f'{INDENT * 2}constexpr int NO_ADVANCED_ENHANCEMENT{{ -1 }};  // 상급 재련이 없는 분류 (최소 단계 없음)')
    out.append('')
    out.append(f'{INDENT * 2}// ===== 장비 능력치 표 (일반 = 일반 재련 단계별, 레벨별 = 아이템 레벨별 · 상급 재련 포함) =====')
    tables = equipment['Tables']
    entries = []
    for index, (key, table) in enumerate(tables.items()):
        identifier = to_identifier(key, index)
        if table['Stat'] not in STAT_KINDS:
            raise ValueError(f'{key}: 모르는 Stat {table["Stat"]}')

        normal = table['Normal']
        normal_name = f'{identifier}_NORMAL'
        out.append(format_array(normal_name, normal, f'{key} 일반 재련 0 ~ {len(normal) - 1}단계'))

        item_levels = table.get(JSON_ITEM_LEVELS_KEY) or []
        item_level_span = 'std::span<const int>{}'
        if item_levels:
            if len(item_levels) != len(normal):
                raise ValueError(f'{key}: ItemLevels 길이({len(item_levels)})가 Normal 길이({len(normal)})와 다릅니다.')
            item_level_span = f'{identifier}_ITEM_LEVELS'
            out.append(format_int_array(item_level_span, item_levels, f'{key} 일반 재련 단계별 아이템 레벨 (불규칙)'))

        by_level = table.get('ByLevel') or {}
        if by_level:
            values = by_level['Values']
            first_level = int(by_level['FirstLevel'])
            level_name = f'{identifier}_BY_LEVEL'
            out.append(format_array(level_name, values, f'{key} 아이템 레벨 {first_level} ~ {first_level + len(values) - 1}', first_level, '레벨'))
            min_advanced = find_min_advanced(values, first_level, base_level, normal_step, advanced_step, len(normal))
            first_name = f'{identifier}_FIRST_LEVEL'
            min_name = f'{identifier}_MIN_ADVANCED'
            out.append(f'{INDENT * 2}constexpr int {first_name}{{ {first_level} }};   // 레벨별 표 첫 칸의 아이템 레벨')
            out.append(f'{INDENT * 2}constexpr int {min_name}{{ {min_advanced} }};   // 상급 재련을 넣을 수 있는 최소 일반 단계')
            entries.append((key, table['Stat'], normal_name, first_name, level_name, min_name, item_level_span))
        else:
            entries.append((key, table['Stat'], normal_name, 'NO_FIRST_LEVEL', None, 'NO_ADVANCED_ENHANCEMENT', item_level_span))
        out.append('')

    out.append(f'{INDENT * 2}// 분류 키 → 표 (키는 UTF-8, 결단 · 업화 · 전율 · 에스더 + 부위)')
    out.append(f'{INDENT * 2}constexpr std::array<EquipmentTable, {len(entries)}> EQUIPMENT_TABLES{{ {{   // 장비 표 {len(entries)}개')
    for key, stat, normal_name, first_level, level_name, min_name, item_level_span in entries:
        level_span = level_name if level_name else 'std::span<const double>{}'
        out.append(f'{INDENT * 3}EquipmentTable{{ "{key}", {STAT_KINDS[stat]}, {normal_name}, {first_level}, {level_span}, {min_name}, {item_level_span} }},   // 분류 키 · 능력치 종류 · 일반 표 · 레벨 표 첫 레벨 · 레벨 표 · 상급 최소 단계 · 단계별 아이템 레벨')
    out.append(f'{INDENT * 2}}} }};   // EQUIPMENT_TABLES 끝')
    out.append('')

    out.append(f'{INDENT * 2}// ===== 보석 (레벨별 계수 F(level)/F(10), 표 번호 = 레벨 - 첫 레벨) =====')
    out.append(format_array('GEM_FACTORS', gem['Values'], f'보석 {gem_keys[0]} ~ {gem_keys[-1]}레벨', gem_keys[0], '레벨'))
    out.append(f'{INDENT * 2}constexpr int GEM_FIRST_LEVEL{{ {gem_keys[0]} }};   // GEM_FACTORS[0]의 보석 레벨')
    out.append(f'{INDENT * 2}constexpr double NO_GEM_VALUE{{ {format_number(gem["NoGemValue"])} }};   // 보석이 없을 때의 계수 (같은 척도)')
    out.append(format_keyed('GEM_TYPE_FACTORS', list(gem['TypeFactors'].items()), '보석 종류별 계수 (레벨 계수에 곱함, 겁화 · 작열 = 1)'))
    out.append('')
    out.append(f'{INDENT * 2}// ===== 무기 품질 곡선 F(q) = 기본 + 계수 × (q / 최대)² (F(최대) = 1) =====')
    out.append(f'{INDENT * 2}constexpr double QUALITY_BASE{{ {format_number(round(quality_base, 6))} }};   // 곡선 기본값 A = F(0)')
    out.append(f'{INDENT * 2}constexpr double QUALITY_SCALE{{ {format_number(round(quality_scale, 6))} }};   // 곡선 계수 B = F(최대) - F(0)')
    out.append(f'{INDENT * 2}constexpr double QUALITY_MAX{{ {format_number(quality_max)} }};   // 최대 품질')
    out.append(f'{INDENT * 2}constexpr QualityCurve WEAPON_QUALITY_CURVE{{ QUALITY_BASE, QUALITY_SCALE, QUALITY_MAX }};   // 품질 곡선 묶음')
    out.append('')
    out.append(f'{INDENT * 2}// ===== 무기 공격력 보정 (W0 = 무기 툴팁 공격력 + 이 값, 다른 무기 공격력 출처의 맞춤값) =====')
    out.append(f'{INDENT * 2}constexpr double WEAPON_ATTACK_EXTRA{{ {format_number(weapon_extra)} }};   // 무기 툴팁 공격력에 더하는 보정값')
    out.append('')
    extra_consts, extra_funcs = build_extra(data)
    out.append(f'{INDENT * 2}// ===== 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 · 완갑 =====')
    out.extend(extra_consts)
    out.append(f'{INDENT}}}   // 익명 네임스페이스 끝')
    out.append('')

    # 접근 함수 (인터페이스에 선언된 것)
    out.append(f'{INDENT}// 장비 능력치 표 전체')
    out.append(f'{INDENT}std::span<const EquipmentTable> GetEquipmentTables() noexcept')
    out.append(f'{INDENT}{{')
    out.append(f'{INDENT * 2}return EQUIPMENT_TABLES;   // 전체 표를 span으로 반환')
    out.append(f'{INDENT}}}')
    out.append('')
    out.append(f'{INDENT}// 분류 키로 표 찾기 (없으면 nullptr)')
    out.append(f'{INDENT}const EquipmentTable* FindEquipmentTable(std::string_view key) noexcept')
    out.append(f'{INDENT}{{')
    out.append(f'{INDENT * 2}const auto found{{ std::ranges::find(EQUIPMENT_TABLES, key, &EquipmentTable::m_key) }};   // 키가 같은 표 검색')
    out.append(f'{INDENT * 2}return found != EQUIPMENT_TABLES.end() ? &*found : nullptr;   // 찾았으면 주소, 없으면 nullptr')
    out.append(f'{INDENT}}}')
    out.append('')
    for signature, value, description in [
        ('std::span<const AdvancedBonus> GetAdvancedBonuses() noexcept', 'ADVANCED_BONUSES', '상급 재련 보너스 구간 목록'),
        ('const ItemLevelRule& GetItemLevelRule() noexcept', 'ITEM_LEVEL_RULE', '아이템 레벨 규칙'),
        ('std::span<const double> GetGemFactors() noexcept', 'GEM_FACTORS', '보석 레벨별 계수 표'),
        ('int GetGemFirstLevel() noexcept', 'GEM_FIRST_LEVEL', '보석 계수 표 첫 칸의 레벨'),
        ('double GetNoGemValue() noexcept', 'NO_GEM_VALUE', '보석이 없을 때의 계수'),
        ('const QualityCurve& GetWeaponQualityCurve() noexcept', 'WEAPON_QUALITY_CURVE', '무기 품질 곡선'),
        ('double GetWeaponAttackExtra() noexcept', 'WEAPON_ATTACK_EXTRA', '무기 공격력 보정값'),
    ]:
        out.append(f'{INDENT}// {description}')
        out.append(f'{INDENT}{signature}')
        out.append(f'{INDENT}{{')
        out.append(f'{INDENT * 2}return {value};   // {description} 반환')
        out.append(f'{INDENT}}}')
        out.append('')
    for signature, value, description in extra_funcs:
        out.append(f'{INDENT}// {description}')
        out.append(f'{INDENT}{signature}')
        out.append(f'{INDENT}{{')
        out.append(f'{INDENT * 2}return {value};   // {description} 반환')
        out.append(f'{INDENT}}}')
        out.append('')
    out.extend(build_special_functions())
    out[-1] = '}   // 계수 데이터 네임스페이스 끝'
    return '\n'.join(out) + '\n'


def main():
    """인자 읽기 → JSON 읽기 → .cpp 쓰기."""
    input_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_INPUT
    output_path = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUTPUT
    csv_path = sys.argv[3] if len(sys.argv) > 3 else DEFAULT_CSV

    with open(input_path, encoding='utf-8') as file:
        data = json.load(file)

    # 1단계: CSV → 에스더 ItemLevels 를 JSON 에 반영 (CSV 가 없으면 기존 JSON 값 그대로)
    levels = read_item_levels(csv_path)
    if levels is None:
        print(f'CSV 없음 → JSON 의 ItemLevels 를 그대로 씁니다: {csv_path}')
    elif merge_item_levels(data, levels):
        with open(input_path, 'w', encoding='utf-8', newline='') as file:
            json.dump(data, file, ensure_ascii=False, separators=JSON_SEPARATORS)
        print(f'ItemLevels 갱신: {input_path}')

    source = build_source(data, os.path.relpath(input_path, PROJECT_DIR).replace('\\', '/'))
    with open(output_path, 'w', encoding='utf-8', newline='\r\n') as file:
        file.write(source)
    print(f'생성 완료: {output_path} ({len(data["Equipment"]["Tables"])}개 장비 표)')


if __name__ == '__main__':
    main()
