# 창고 · 소지품 슬롯 인식 템플릿 만들기 (Slot_Templates.cpp 생성)
# - 아이콘: Verify/screens/storage.png 에서 숫자 아래쪽 아이콘 부분(44x28)을 RGB로 잘라 넣음
# - 숫자 글자: 답을 아는 슬롯 숫자(흰 글자)를 글자마다 잘라 "#" · "." 모양 글자로 넣음
# 사용: python tools/Make-SlotTemplates.py  (프로젝트 폴더에서 실행)
import numpy as np
from PIL import Image

ICON_WIDTH = 44   # 아이콘 템플릿 가로
ICON_HEIGHT = 28  # 아이콘 템플릿 세로
SCREENS = 'Verify/screens/'  # 샘플 폴더

# 아이콘: (값 이름 키, 재료 구분 꼬리, 샘플, 왼쪽, 위쪽)
ICONS = [
    ('Fusion', 'level', 'storage.png', 383, 97),    # 아비도스 융화 재료 (금색)
    ('Yong', 'level', 'storage.png', 177, 306),     # 용암의 숨결 (빨간 불꽃)
    ('Bing', 'level', 'storage.png', 228, 306),     # 빙하의 숨결 (파란 결정)
]

# 숫자 글자 샘플: (샘플, 위, 아래, 왼쪽, 오른쪽, 답)
BANDS = [
    ('storage.png', 84, 96, 378, 432, '1178'),
    ('storage.png', 293, 305, 175, 222, '2720'),
    ('storage.png', 293, 305, 226, 273, '3024'),
    ('storage.png', 450, 463, 515, 535, '50'),
    ('storage.png', 184, 200, 460, 486, '6'),
    ('storage.png', 84, 96, 12, 64, '9999+'),
    ('tooltip_guard_top_bound.png', 88, 100, 379, 428, '207'),
    ('tooltip_guard_top_bound.png', 297, 309, 68, 114, '1423'),
    ('storage_inventory.webp', 250, 262, 116, 158, '920'),
]
GLYPH_ROWS = 10  # 글자 줄 창 높이 (Material_Slot_Detector와 같음)


def load(name):
    return np.asarray(Image.open(SCREENS + name).convert('RGB')).astype(np.int32)


def mask(area):
    low = area.min(axis=2)
    high = area.max(axis=2)
    bits = (low >= 150) & (high - low <= 70)   # 흰 글자 (Material_Slot_Detector와 같은 기준)
    best_count, best_top = -1, 0                # 글자가 가장 많은 10줄 창만 남김 (아래 · 위 아이콘 빛 지우기)
    for top in range(0, max(1, bits.shape[0] - GLYPH_ROWS + 1)):
        count = int(bits[top:top + GLYPH_ROWS].sum())
        if count > best_count:
            best_count, best_top = count, top
    kept = np.zeros_like(bits)
    kept[best_top:best_top + GLYPH_ROWS] = bits[best_top:best_top + GLYPH_ROWS]
    return kept


def segments(bits):
    columns = bits.any(axis=0)
    found = []
    x = 0
    while x < len(columns):
        if not columns[x]:
            x += 1
            continue
        start = x
        while x < len(columns) and columns[x]:
            x += 1
        glyph = bits[:, start:x]
        rows = np.where(glyph.any(axis=1))[0]
        glyph = glyph[rows[0]:rows[-1] + 1]
        if 7 <= glyph.shape[0] <= 10 and 2 <= glyph.shape[1] <= 7:
            found.append(glyph)
    return found


def main():
    lines = []
    out = lines.append
    out('module LostArkMultiTool.Vision.SlotTemplates;  // 슬롯 인식 템플릿 모듈 구현 (tools/Make-SlotTemplates.py가 만든 파일, 손으로 고치지 말 것)')
    out('')
    out('import std;  // 표준 라이브러리 모듈 가져오기')
    out('')
    out('namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작')
    out('{')
    out('    namespace  // 이 파일 전용 데이터')
    out('    {')
    icon_names = []
    for key, tier, sample, left, top in ICONS:
        image = load(sample)[top:top + ICON_HEIGHT, left:left + ICON_WIDTH]
        name = f'{key.upper()}_ICON_PIXELS'
        icon_names.append((key, tier, name))
        out(f'        constexpr std::array<std::uint8_t, {ICON_WIDTH * ICON_HEIGHT * 3}> {name}{{   // {key} 아이콘 RGB ({ICON_WIDTH}x{ICON_HEIGHT}, {sample} ({left}, {top}))')
        for row_index, row in enumerate(image):
            values = ','.join(str(int(v)) for v in row.reshape(-1))
            out(f'            {values},   // {row_index + 1}행')
        out('        };')
    glyph_rows = []
    for sample, top, bottom, left, right, answer in BANDS:
        glyphs = segments(mask(load(sample)[top:bottom, left:right]))
        if len(glyphs) != len(answer):
            raise SystemExit(f'글자 수가 다름: {sample} {answer} → {len(glyphs)}')
        for symbol, glyph in zip(answer, glyphs):
            bits = ''.join('#' if v else '.' for v in glyph.reshape(-1))
            glyph_rows.append((symbol, glyph.shape[1], glyph.shape[0], bits, answer))
    out('        constexpr std::array GLYPHS{   // 슬롯 숫자 글자 모양 (같은 글자 여러 모양: 바탕 · 번짐 차이)')
    for symbol, width, height, bits, answer in glyph_rows:
        out(f'            SlotGlyphTemplate{{ \'{symbol}\', {width}, {height}, "{bits}" }},   // "{answer}"의 {symbol}')
    out('        };')
    out('        constexpr std::array ICONS{   // 슬롯 아이콘 (값 이름 키 · 구분 · 픽셀)')
    for key, tier, name in icon_names:
        out(f'            SlotIconTemplate{{ "{key}/{tier}", {ICON_WIDTH}, {ICON_HEIGHT}, std::span<const std::uint8_t>{{ {name} }} }},   // {key}')
    out('        };')
    out('    }')
    out('')
    out('    std::span<const SlotIconTemplate> GetSlotIcons() noexcept')
    out('    {')
    out('        return ICONS;   // 아이콘 목록')
    out('    }')
    out('')
    out('    std::span<const SlotGlyphTemplate> GetSlotGlyphs() noexcept')
    out('    {')
    out('        return GLYPHS;   // 글자 목록')
    out('    }')
    out('}')
    text = '\r\n'.join(lines) + '\r\n'
    with open('Slot_Templates.cpp', 'wb') as file:
        file.write(b'\xef\xbb\xbf' + text.encode('utf-8'))
    print(f'Slot_Templates.cpp: 아이콘 {len(ICONS)}개 · 글자 {len(glyph_rows)}개')


if __name__ == '__main__':
    main()
