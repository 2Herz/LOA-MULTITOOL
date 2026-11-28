# 버프 타이머 인식 템플릿 만들기 (Buff_Templates.cpp 생성)
# - 아이콘: 샘플 화면에서 찾은 버프 · 디버프 아이콘 안쪽을 16x16 RGB로 줄여 넣음 (암수 · 아드로핀 · 서폿 통합 아이콘 4종)
# - 초 글자: 아이콘 아래 남은 시간 글자("18초" · "0.7초" · "5분")를 글자마다 잘라 "#" · "." 모양으로 넣음
# - 만든 뒤 같은 방법(Buff_Detector와 같은 기준)으로 샘플을 다시 읽어 정답과 비교해 출력
# 사용: python tools/Make-BuffTemplates.py  (프로젝트 폴더에서 실행)
import sys
import numpy as np
from PIL import Image

BUFFS = 'Verify/buffs/'      # 버프 샘플 폴더
SCREENS = 'Verify/screens/'  # 화면 샘플 폴더
NORM = 16                    # 아이콘 비교 크기 (Buff_Detector와 같음)
TEXT_ROWS = 8                # 초 글자 줄 높이
BASE_ICON = 26               # 기준 아이콘 크기 (1080p)

# 아이콘 템플릿: (종류 이름, 샘플, 아이콘 왼쪽 위 근처 x, y)
ICONS = [
    ('GRENADE', BUFFS + 'combat_grenade_2.webp', 938, 185),          # 암흑 수류탄 (보스 디버프)
    ('GRENADE', BUFFS + 'combat_grenade_1.webp', 964, 180),
    ('GRENADE', BUFFS + 'combat_grenade_adrenaline.webp', 959, 179),  # 다른 전투 (점수 0.79였던 화면)
    ('ADRENALINE', BUFFS + 'combat_grenade_4.webp', 836, 938),       # 아드로핀 (내 버프)
    ('SUPPORT_AWAKENING', BUFFS + 'support_icons.png', 1, 1),        # 서폿 초각성기 (보라)
    ('SUPPORT_HYPER', BUFFS + 'support_icons.png', 37, 1),           # 서폿 초각성 스킬 (파랑)
    ('SUPPORT_ATTACK', BUFFS + 'support_icons.png', 72, 1),          # 서폿 공격력 증가 (빨강)
    ('SUPPORT_IDENTITY', BUFFS + 'support_icons.png', 107, 1),       # 서폿 아이덴티티 (노랑)
]

# 정답 샘플: (샘플, 디버프?, [(아이콘 x, y, 종류 또는 None, 남은 시간 글자 또는 None)])
SAMPLES = [
    (SCREENS + 'boss_hp_half.webp', True, [(932, 179, None, '4초'), (960, 179, None, None), (988, 179, None, '3초')]),
    (SCREENS + 'boss_hp_half.webp', False, [(744, 939, None, '25초'), (772, 939, None, '3초'), (801, 939, None, None), (830, 939, None, None)]),
    (SCREENS + 'boss_hp_low.webp', False, [(743, 942, None, '3초'), (772, 942, None, '27초'), (801, 942, None, None), (830, 942, None, None)]),
    (BUFFS + 'combat_grenade_1.webp', True, [(936, 180, None, '5초'), (964, 180, 'GRENADE', '18초')]),
    (BUFFS + 'combat_grenade_1.webp', False, [(777, 940, None, '5분'), (806, 940, None, None), (835, 940, None, None)]),
    (BUFFS + 'combat_grenade_2.webp', True, [(938, 185, 'GRENADE', '8초')]),
    (BUFFS + 'combat_grenade_2.webp', False, [(779, 945, None, '5분'), (808, 945, None, None), (837, 945, None, None)]),
    (BUFFS + 'combat_grenade_3.webp', True, [(936, 179, 'GRENADE', '0.7초')]),
    (BUFFS + 'combat_grenade_3.webp', False, [(777, 939, None, '4분'), (806, 939, None, None), (835, 939, None, None)]),
    (BUFFS + 'combat_grenade_4.webp', True, []),
    (BUFFS + 'combat_grenade_adrenaline.webp', True, [(931, 179, None, '4초'), (959, 179, 'GRENADE', '18초')]),
    (BUFFS + 'combat_grenade_adrenaline.webp', False, [(800, 939, None, '0.0초'), (829, 939, None, None)]),
    (BUFFS + 'combat_grenade_4.webp', False, [(749, 938, None, '4분'), (778, 938, None, None), (807, 938, None, None), (836, 938, 'ADRENALINE', '8초')]),
]

UNIT_SYMBOLS = {'초': 's', '분': 'm'}   # 단위 글자 → 템플릿 기호


def load(path):
    return np.asarray(Image.open(path).convert('RGB')).astype(np.int32)


def is_red(c):
    r, g, b = c
    return r >= 110 and r - g >= 55 and r - b >= 50


def is_green(c):
    r, g, b = c
    return g >= 120 and g - r >= 25 and g - b >= 40


def is_gold(c):
    r, g, b = c
    return r >= 140 and g >= 60 and b <= 80 and r - b >= 90 and r + 10 >= g


def find_icons(img, zone, is_debuff):
    tests = [is_red] if is_debuff else [is_green, is_gold]
    h, w, _ = img.shape
    x0, y0, x1, y1 = zone
    found = []
    for test in tests:
        mask = np.zeros((h, w), dtype=bool)
        ys, xs = slice(max(0, y0 - 2), min(h, y1 + 60)), slice(max(0, x0 - 2), min(w, x1 + 2))
        sub = img[ys, xs]
        mask[ys, xs] = np.apply_along_axis(lambda c: test(c), 2, sub) if False else vec_test(test, sub)
        for y in range(y0, y1):
            x = x0
            while x < x1:
                if not mask[y, x]:
                    x += 1
                    continue
                s = x
                while x < x1 and mask[y, x]:
                    x += 1
                n = x - s
                if not (h * 0.018 <= n <= h * 0.034):
                    continue
                rect = None
                for top in (y, y - n + 1):            # 이 줄이 위 테두리 · 아래 테두리라고 보고 차례로 확인
                    rows = range(top + 2, top + n - 2)
                    if top < 0 or top + n + 1 >= h:
                        continue
                    left = sum(mask[yy, max(0, s - 2):s + 1].any() for yy in rows)
                    right = sum(mask[yy, s + n - 1:s + n + 2].any() for yy in rows)
                    if left >= len(rows) * 0.75 and right >= len(rows) * 0.75:
                        rect = (s - 1, top, n + 2, n + 2)
                        break
                if rect and not any(abs(f[0] - rect[0]) < rect[2] * 0.6 and abs(f[1] - rect[1]) < rect[3] * 0.6 for f in found):
                    found.append(rect)
    found.sort()
    return found


def vec_test(test, sub):
    r, g, b = sub[..., 0], sub[..., 1], sub[..., 2]
    if test is is_red:
        return (r >= 110) & (r - g >= 55) & (r - b >= 50)
    if test is is_green:
        return (g >= 120) & (g - r >= 25) & (g - b >= 40)
    return (r >= 140) & (g >= 60) & (b <= 80) & (r - b >= 90) & (r + 10 >= g)


def zone_of(img, is_debuff):
    h, w, _ = img.shape
    if is_debuff:
        return (int(w * 0.35), int(h * 0.14), int(w * 0.65), int(h * 0.32))
    return (int(w * 0.25), int(h * 0.76), int(w * 0.75), int(h * 0.885))


def normalize(img, rect):
    x, y, w, h = rect
    inset = max(2, h // 8)
    crop = img[y + inset:y + h - inset, x + inset:x + w - inset]
    ch, cw = crop.shape[:2]
    out = np.zeros((NORM, NORM, 3), dtype=np.int32)
    for oy in range(NORM):                           # 상자 평균 (Buff_Detector와 같은 방법)
        y0 = oy * ch // NORM
        y1 = max(y0 + 1, (oy + 1) * ch // NORM)
        for ox in range(NORM):
            x0 = ox * cw // NORM
            x1 = max(x0 + 1, (ox + 1) * cw // NORM)
            out[oy, ox] = crop[y0:y1, x0:x1].reshape(-1, 3).sum(axis=0) // ((y1 - y0) * (x1 - x0))
    return out


def ncc(a, b):
    a = a.astype(np.float64).ravel()
    b = b.astype(np.float64).ravel()
    a -= a.mean()
    b -= b.mean()
    return float((a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-6))


def text_mask(img, rect):
    x, y, w, h = rect
    scale = h / BASE_ICON
    top = y + h + 1
    bottom = y + h + int(round(14 * scale))
    left = x - int(round(8 * scale))
    right = x + w + int(round(8 * scale))
    area = img[top:bottom, left:right]
    if abs(scale - 1.0) > 0.08:
        nh, nw = int(round(area.shape[0] / scale)), int(round(area.shape[1] / scale))
        area = np.asarray(Image.fromarray(area.astype(np.uint8)).resize((nw, nh), Image.NEAREST)).astype(np.int32)
    lum = area.mean(axis=2)
    low = area.min(axis=2)
    ah, aw = lum.shape
    pad = np.pad(lum, 2, mode='edge')
    local = np.full_like(lum, 255.0)
    for dy in range(5):
        for dx in range(5):
            local = np.minimum(local, pad[dy:dy + ah, dx:dx + aw])
    bits = (lum >= 125) & (low >= 90) & (local < lum - 60)
    best, best_top = -1, 0
    for t in range(0, max(1, ah - TEXT_ROWS + 1)):
        c = int(bits[t:t + TEXT_ROWS].sum())
        if c > best:
            best, best_top = c, t
    kept = np.zeros_like(bits)
    kept[best_top:best_top + TEXT_ROWS] = bits[best_top:best_top + TEXT_ROWS]
    return kept, int(round(8 * 1.0)) + int(round(w / scale / 2))   # (마스크, 아이콘 가운데 x)


UNIT_MIN_WIDTH = 7     # 단위 글자(초 · 분) 너비 하한
UNIT_MAX_WIDTH = 10    # 단위 글자 너비 상한
DIGIT_MAX_WIDTH = 6    # 숫자 너비 상한
DIGIT_MIN_INK = 6      # 숫자 잉크 하한 (이보다 적으면 잡음)
UNIT_WIDTH = 9         # 단위 글자 보통 너비
GROUP_MAX_GAP = 3      # 같은 글자 줄로 보는 빈 칸 상한
CENTER_MAX_DIST = 14   # 아이콘 가운데에서 이만큼 안에 글자가 있어야 함
DOT_ROWS = 3           # 소수점: 아래 3줄 안에만 잉크


def column_runs(bits, left, right):
    cols = bits[:, left:right].any(axis=0)
    runs, x = [], 0
    while x < len(cols):
        if not cols[x]:
            x += 1
            continue
        s0 = x
        while x < len(cols) and cols[x]:
            x += 1
        runs.append([left + s0, left + x])
    return runs


def trim(bits, l, r):
    g = bits[:, l:r]
    rows = np.where(g.any(axis=1))[0]
    return g[rows[0]:rows[-1] + 1], int(rows[0])


def cut_column(bits, lo, hi, want):
    # lo ~ hi 사이에서 잉크가 가장 적은 칸 (같으면 want에 가까운 칸)
    best, best_x = None, lo
    for x in range(lo, hi + 1):
        key = (int(bits[:, x].sum()), abs(x - want))
        if best is None or key < best:
            best, best_x = key, x
    return best_x


def split_text(bits, center):
    # 아이콘 아래 글자 줄 → [('digit'|'dot'|'unit', 모양)] (못 나누면 None)
    runs = column_runs(bits, 0, bits.shape[1])
    runs = [r for r in runs if bits[:, r[0]:r[1]].sum() >= 2 or bits[TEXT_ROWS - DOT_ROWS:, r[0]:r[1]].any()]   # 한 점은 소수점 자리일 때만
    if not runs:
        return None
    near = min(range(len(runs)), key=lambda i: abs((runs[i][0] + runs[i][1]) / 2 - center))
    if abs((runs[near][0] + runs[near][1]) / 2 - center) > CENTER_MAX_DIST:
        return None
    lo, hi = near, near
    while lo > 0 and runs[lo][0] - runs[lo - 1][1] <= GROUP_MAX_GAP:
        lo -= 1
    while hi + 1 < len(runs) and runs[hi + 1][0] - runs[hi][1] <= GROUP_MAX_GAP:
        hi += 1
    group = [list(r) for r in runs[lo:hi + 1]]
    last = group[-1]
    width = last[1] - last[0]
    if width < UNIT_MIN_WIDTH:
        return None
    if width > UNIT_MAX_WIDTH:                     # 숫자와 단위 글자가 붙음 → 오른쪽 9칸 근처에서 자름
        cut = cut_column(bits, last[1] - UNIT_MAX_WIDTH - 1, last[1] - UNIT_MIN_WIDTH, last[1] - UNIT_MAX_WIDTH)
        group[-1] = [last[0], cut]
        group.append([cut + (0 if bits[:, cut].any() else 1), last[1]])
    unit = group.pop()
    digits = []
    queue = group
    while queue:
        l, r = queue.pop(0)
        if r - l > DIGIT_MAX_WIDTH:                 # 숫자 두 개가 붙음
            cut = cut_column(bits, l + 3, r - 3, (l + r) // 2)
            if bits[:, l:cut].sum() >= DIGIT_MIN_INK and bits[:, cut:r].sum() >= DIGIT_MIN_INK:   # 양쪽 다 글자일 때만 나눔
                queue = [[l, cut], [cut + (0 if bits[:, cut].any() else 1), r]] + queue
                continue
        if r <= l or not bits[:, l:r].any():
            continue
        g, top = trim(bits, l, r)
        if r - l <= 2 and top >= TEXT_ROWS - DOT_ROWS:
            digits.append(('dot', g))
        elif int(g.sum()) >= DIGIT_MIN_INK:
            digits.append(('digit', g))       # 잉크가 너무 적으면 바탕 잡음
    ug, _ = trim(bits, unit[0], unit[1])
    return digits + [('unit', ug)]


def dice(a, b):
    h = max(a.shape[0], b.shape[0]) + 2
    w = max(a.shape[1], b.shape[1]) + 2
    best = 0.0
    ca = np.zeros((h, w), bool)
    ca[1:1 + a.shape[0], 1:1 + a.shape[1]] = a
    for dy in (0, 1, 2):
        for dx in (0, 1, 2):
            if dy + b.shape[0] > h or dx + b.shape[1] > w:
                continue
            cb = np.zeros((h, w), bool)
            cb[dy:dy + b.shape[0], dx:dx + b.shape[1]] = b
            inter = (ca & cb).sum()
            total = ca.sum() + cb.sum()
            if total:
                best = max(best, 2 * inter / total)
    return best


def classify(kind, bits, glyphs):
    if kind == 'dot':
        return '.', 1.0
    best, best_score = None, 0.0
    for sym, tb in glyphs:
        if (sym in 'sm') != (kind == 'unit'):
            continue
        sc = dice(bits, tb)
        if sc > best_score:
            best, best_score = sym, sc
    return best, best_score


def read_text(img, rect, glyphs):
    bits, center = text_mask(img, rect)
    parts = split_text(bits, center)
    if not parts:
        return None
    out = ''
    for kind, g in parts:
        sym, sc = classify(kind, g, glyphs)
        if sym is None or sc < 0.70:
            return None
        out += sym
    return out


def to_answer_symbols(text):
    return ''.join(UNIT_SYMBOLS.get(ch, ch) for ch in text)


def main():
    icon_templates = []
    for kind, path, ax, ay in ICONS:
        img = load(path)
        h, w, _ = img.shape
        zone = (0, 0, w, h) if 'support' in path else zone_of(img, kind == 'GRENADE')
        rects = find_icons(img, zone, kind == 'GRENADE')
        rect = min(rects, key=lambda r: abs(r[0] - ax) + abs(r[1] - ay)) if rects else None
        if rect is None or abs(rect[0] - ax) + abs(rect[1] - ay) > 6:
            sys.exit(f'아이콘을 못 찾음: {kind} {path} {rects}')
        icon_templates.append((kind, path, rect, normalize(img, rect)))

    glyphs = []
    for path, is_debuff, items in SAMPLES:
        img = load(path)
        rects = find_icons(img, zone_of(img, is_debuff), is_debuff)
        for ix, iy, kind, answer in items:
            if not answer:
                continue
            rect = min(rects, key=lambda r: abs(r[0] - ix) + abs(r[1] - iy))
            bits, center = text_mask(img, rect)
            parts = split_text(bits, center) or []
            symbols = to_answer_symbols(answer)
            if len(parts) != len(symbols):
                print(f'  ! 글자 수 다름 {path} {answer}: {len(parts)}')
                continue
            for (kind, g), sym in zip(parts, symbols):
                if sym == '.':
                    continue
                if not any(s == sym and dice(b, g) >= 0.95 for s, b in glyphs):
                    glyphs.append((sym, g))

    # 다시 읽어 확인
    total, correct = 0, 0
    worst = [0.0]
    for path, is_debuff, items in SAMPLES:
        img = load(path)
        rects = find_icons(img, zone_of(img, is_debuff), is_debuff)
        if len(rects) != len(items):
            print(f'  ! 아이콘 수 다름 {path} {"D" if is_debuff else "B"}: {rects}')
        for ix, iy, kind, answer in items:
            total += 1
            rect = min(rects, key=lambda r: abs(r[0] - ix) + abs(r[1] - iy)) if rects else None
            vec = normalize(img, rect)
            scores = sorted(((ncc(vec, t[3]), t[0]) for t in icon_templates), reverse=True)
            got_kind = scores[0][1] if scores[0][0] >= 0.70 else None
            if kind is None:
                worst[0] = max(worst[0], scores[0][0])          # 모르는 아이콘 중 가장 높은 점수 (오탐 여유)
            got_text = read_text(img, rect, glyphs)
            want_text = to_answer_symbols(answer) if answer else None
            ok = got_kind == kind and (want_text is None or got_text == want_text)
            correct += ok
            print(f'  {"OK " if ok else "BAD"} {path.split("/")[-1]} {rect} 종류 {got_kind}({scores[0][0]:.2f}/{scores[0][1]}) 글자 {got_text} 정답 {kind} {want_text}')
    print(f'모르는 아이콘 최고 점수 {worst[0]:.2f}')
    print(f'정답 {correct}/{total}, 글자 템플릿 {len(glyphs)}개: {sorted(set(s for s, _ in glyphs))}')

    write_cpp(icon_templates, glyphs)


def write_cpp(icon_templates, glyphs):
    lines = []
    out = lines.append
    out('module LostArkMultiTool.Vision.BuffTemplates;  // 버프 타이머 템플릿 모듈 구현 (tools/Make-BuffTemplates.py가 만든 파일, 손으로 고치지 말 것)')
    out('')
    out('import std;  // 표준 라이브러리 모듈 가져오기')
    out('')
    out('namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작')
    out('{')
    out('    namespace  // 이 파일 전용 데이터')
    out('    {')
    names = []
    for index, (kind, path, rect, vec) in enumerate(icon_templates):
        name = f'{kind}_{index}_PIXELS'
        names.append((kind, name))
        flat = vec.astype(np.uint8).ravel()
        out(f'        constexpr std::array<std::uint8_t, {len(flat)}> {name}{{   // {kind} 아이콘 RGB ({NORM}x{NORM}, {path.split("/")[-1]} {rect[:2]})')
        for row in range(NORM):
            part = flat[row * NORM * 3:(row + 1) * NORM * 3]
            out('            ' + ','.join(str(v) for v in part) + ',   // ' + f'{row + 1}행')
        out('        };')
    out('')
    out('        // 아이콘 목록')
    out(f'        constexpr std::array<BuffIconTemplate, {len(names)}> ICONS{{ {{')
    for kind, name in names:
        out(f'            BuffIconTemplate{{ BuffKind::BUFF_{kind}, {NORM}, {NORM}, {name} }},')
    out('        } };')
    out('')
    out('        // 초 글자 목록 (\'s\' = 초, \'m\' = 분)')
    out(f'        constexpr std::array<BuffGlyphTemplate, {len(glyphs)}> GLYPHS{{ {{')
    for sym, bits in glyphs:
        text = ''.join('#' if v else '.' for v in bits.ravel())
        out(f'            BuffGlyphTemplate{{ \'{sym}\', {bits.shape[1]}, {bits.shape[0]}, "{text}" }},')
    out('        } };')
    out('    }')
    out('')
    out('    std::span<const BuffIconTemplate> GetBuffIcons() noexcept')
    out('    {')
    out('        return ICONS;   // 아이콘 목록')
    out('    }')
    out('')
    out('    std::span<const BuffGlyphTemplate> GetBuffGlyphs() noexcept')
    out('    {')
    out('        return GLYPHS;   // 초 글자 목록')
    out('    }')
    out('}')
    with open('Buff_Templates.cpp', 'w', encoding='utf-8-sig', newline='\r\n') as f:
        f.write('\n'.join(lines) + '\n')
    print('Buff_Templates.cpp 생성')


if __name__ == '__main__':
    main()
