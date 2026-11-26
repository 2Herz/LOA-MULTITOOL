# 로스트아크 멀티툴 아이콘 만들기 (테마 색: 남색 바탕 · 금색 동전 고리 · 청록 보석)
# 큰 캔버스(1024)에 그린 뒤 크기별로 줄여 PNG 압축 ICO로 저장
import io
import struct
import sys
from PIL import Image, ImageDraw, ImageFilter

S = 1024                       # 그리는 캔버스 크기
OUT_ICO = sys.argv[1]          # 저장할 ICO 경로
OUT_PNG = sys.argv[2]          # 미리보기 PNG 경로
SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]   # ICO에 넣을 크기

NAVY_TOP = (24, 33, 46)        # 바탕 위쪽 (테마 HEADER_BACK보다 살짝 밝게)
NAVY_BOTTOM = (9, 12, 18)      # 바탕 아래쪽 (테마 BACKGROUND 톤)
TEAL_LIGHT = (150, 248, 232)   # 보석 밝은 면
TEAL = (94, 234, 212)          # 보석 기본 (#5EEAD4)
TEAL_MID = (56, 190, 176)      # 보석 중간 면
TEAL_DARK = (24, 120, 112)     # 보석 어두운 면
TEAL_DEEP = (14, 82, 78)       # 보석 가장 어두운 면
GOLD_LIGHT = (255, 222, 130)   # 금색 밝은 쪽
GOLD = (255, 196, 77)          # 금색 (#FFC44D)
GOLD_DARK = (196, 136, 30)     # 금색 어두운 쪽


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def vertical_gradient(size, top, bottom):
    img = Image.new('RGBA', (size, size))
    px = img.load()
    for y in range(size):
        c = lerp(top, bottom, y / (size - 1))
        for x in range(size):
            px[x, y] = c + (255,)
    return img


def diagonal_gradient(size, a, b):
    img = Image.new('RGBA', (size, size))
    px = img.load()
    for y in range(size):
        for x in range(size):
            t = (x + y) / (2 * (size - 1))
            px[x, y] = lerp(a, b, t) + (255,)
    return img


def rounded_mask(size, radius, inset=0):
    m = Image.new('L', (size, size), 0)
    ImageDraw.Draw(m).rounded_rectangle([inset, inset, size - 1 - inset, size - 1 - inset], radius=radius, fill=255)
    return m


canvas = Image.new('RGBA', (S, S), (0, 0, 0, 0))

# 1) 바탕: 둥근 사각형 + 위→아래 남색 그라데이션
radius = int(S * 0.225)
bg = vertical_gradient(S, NAVY_TOP, NAVY_BOTTOM)
canvas.paste(bg, (0, 0), rounded_mask(S, radius))

# 바탕 안쪽 은은한 청록 테두리
border = Image.new('RGBA', (S, S), (0, 0, 0, 0))
ImageDraw.Draw(border).rounded_rectangle([10, 10, S - 11, S - 11], radius=radius - 10, outline=TEAL + (70,), width=8)
canvas = Image.alpha_composite(canvas, border)

# 바탕 위쪽 빛 (유리 느낌)
glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
ImageDraw.Draw(glow).ellipse([int(S * 0.08), int(-S * 0.55), int(S * 0.92), int(S * 0.38)], fill=(255, 255, 255, 11))
glow_mask = rounded_mask(S, radius)
glow.putalpha(Image.composite(glow.getchannel('A'), Image.new('L', (S, S), 0), glow_mask))
canvas = Image.alpha_composite(canvas, glow)

cx, cy = S // 2, int(S * 0.5)

# 2) 금색 동전 고리 (자산) + 뒤쪽 은은한 금빛
halo = Image.new('RGBA', (S, S), (0, 0, 0, 0))
r_halo = int(S * 0.36)
ImageDraw.Draw(halo).ellipse([cx - r_halo, cy - r_halo, cx + r_halo, cy + r_halo], outline=GOLD + (90,), width=int(S * 0.05))
halo = halo.filter(ImageFilter.GaussianBlur(S * 0.05))
canvas = Image.alpha_composite(canvas, halo)

r_out = int(S * 0.335)
ring_w = int(S * 0.072)
ring_mask = Image.new('L', (S, S), 0)
d = ImageDraw.Draw(ring_mask)
d.ellipse([cx - r_out, cy - r_out, cx + r_out, cy + r_out], fill=255)
d.ellipse([cx - r_out + ring_w, cy - r_out + ring_w, cx + r_out - ring_w, cy + r_out - ring_w], fill=0)
ring = diagonal_gradient(S, GOLD_LIGHT, GOLD_DARK)
ring_layer = Image.new('RGBA', (S, S), (0, 0, 0, 0))
# 고리 안쪽은 어두운 남색으로 채워 보석이 돋보이게
ri0 = r_out - ring_w
inner_fill = Image.new('RGBA', (S, S), (0, 0, 0, 0))
ImageDraw.Draw(inner_fill).ellipse([cx - ri0, cy - ri0, cx + ri0, cy + ri0], fill=(13, 19, 28, 255))
canvas = Image.alpha_composite(canvas, inner_fill)
ring_layer.paste(ring, (0, 0), ring_mask)
canvas = Image.alpha_composite(canvas, ring_layer)

# 고리 안쪽 얇은 선 (동전 테두리 느낌)
inner = Image.new('RGBA', (S, S), (0, 0, 0, 0))
ri = r_out - ring_w - int(S * 0.022)
ImageDraw.Draw(inner).ellipse([cx - ri, cy - ri, cx + ri, cy + ri], outline=GOLD + (70,), width=int(S * 0.007))
canvas = Image.alpha_composite(canvas, inner)

# 3) 청록 보석 (재료) — 위쪽 크라운 + 아래쪽 파빌리온, 면마다 다른 밝기
gw = int(S * 0.40)          # 보석 가로 반폭의 2배
top_y = int(cy - S * 0.175)  # 윗면 높이
girdle_y = int(cy - S * 0.055)
tip_y = int(cy + S * 0.235)
half = gw // 2
table_half = int(gw * 0.27)
L = cx - half
R = cx + half
TL = cx - table_half
TR = cx + table_half

gem = Image.new('RGBA', (S, S), (0, 0, 0, 0))
g = ImageDraw.Draw(gem)
# 크라운 (윗부분): 왼쪽 면 · 윗면 · 오른쪽 면
g.polygon([(L, girdle_y), (TL, top_y), (cx, girdle_y)], fill=TEAL_MID)
g.polygon([(TL, top_y), (TR, top_y), (cx, girdle_y)], fill=TEAL_LIGHT)
g.polygon([(TR, top_y), (R, girdle_y), (cx, girdle_y)], fill=TEAL)
# 파빌리온 (아랫부분): 왼쪽 · 가운데 · 오른쪽 면
mid_l = cx - int(half * 0.42)
mid_r = cx + int(half * 0.42)
g.polygon([(L, girdle_y), (mid_l, girdle_y), (cx, tip_y)], fill=TEAL_DARK)
g.polygon([(mid_l, girdle_y), (mid_r, girdle_y), (cx, tip_y)], fill=TEAL_MID)
g.polygon([(mid_r, girdle_y), (R, girdle_y), (cx, tip_y)], fill=TEAL_DEEP)

# 보석 그림자 (아래로 살짝)
shadow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
shadow.paste((0, 0, 0, 150), (0, 0), gem.getchannel('A'))
shadow = shadow.transform(shadow.size, Image.AFFINE, (1, 0, 0, 0, 1, -int(S * 0.02))).filter(ImageFilter.GaussianBlur(S * 0.018))
canvas = Image.alpha_composite(canvas, shadow)
canvas = Image.alpha_composite(canvas, gem)

# 보석 면 경계선 (어두운 청록, 얇게)
edges = Image.new('RGBA', (S, S), (0, 0, 0, 0))
e = ImageDraw.Draw(edges)
lw = max(2, int(S * 0.006))
edge_color = (8, 60, 58, 200)
for a, b in [((L, girdle_y), (R, girdle_y)), ((TL, top_y), (cx, girdle_y)), ((TR, top_y), (cx, girdle_y)),
             ((mid_l, girdle_y), (cx, tip_y)), ((mid_r, girdle_y), (cx, tip_y))]:
    e.line([a, b], fill=edge_color, width=lw)
e.line([(L, girdle_y), (TL, top_y), (TR, top_y), (R, girdle_y), (cx, tip_y), (L, girdle_y)], fill=(6, 48, 46, 230), width=lw + 2, joint='curve')
canvas = Image.alpha_composite(canvas, edges)

# 윗면 반짝임
spark = Image.new('RGBA', (S, S), (0, 0, 0, 0))
sp = ImageDraw.Draw(spark)
sp.polygon([(TL + int(S * 0.02), top_y + int(S * 0.012)), (cx - int(S * 0.01), top_y + int(S * 0.012)),
            (TL + int(S * 0.06), girdle_y - int(S * 0.03))], fill=(255, 255, 255, 150))
canvas = Image.alpha_composite(canvas, spark)

# 4) 오른쪽 위 작은 별빛 (금색)
star = Image.new('RGBA', (S, S), (0, 0, 0, 0))
st = ImageDraw.Draw(star)
sx, sy = int(S * 0.735), int(S * 0.265)
sr = int(S * 0.062)
sw = int(S * 0.016)
st.polygon([(sx, sy - sr), (sx + sw, sy - sw), (sx + sr, sy), (sx + sw, sy + sw), (sx, sy + sr), (sx - sw, sy + sw), (sx - sr, sy), (sx - sw, sy - sw)], fill=(255, 240, 200, 255))
star_glow = star.filter(ImageFilter.GaussianBlur(S * 0.012))
canvas = Image.alpha_composite(canvas, star_glow)
canvas = Image.alpha_composite(canvas, star)

canvas.save(OUT_PNG)


def render(size):
    img = canvas.resize((size, size), Image.Resampling.LANCZOS)
    if size <= 32:
        # 작은 크기는 살짝 선명하게
        img = img.filter(ImageFilter.UnsharpMask(radius=0.6, percent=60, threshold=0))
    buf = io.BytesIO()
    img.save(buf, format='PNG')
    return buf.getvalue()


entries = [(s, render(s)) for s in SIZES]
header = struct.pack('<HHH', 0, 1, len(entries))
offset = 6 + 16 * len(entries)
directory = b''
payload = b''
for s, data in entries:
    dim = 0 if s >= 256 else s
    directory += struct.pack('<BBBBHHII', dim, dim, 0, 0, 1, 32, len(data), offset)
    offset += len(data)
    payload += data
with open(OUT_ICO, 'wb') as f:
    f.write(header + directory + payload)

# 크기별 미리보기 한 장 (사용자 확인용)
sheet = Image.new('RGBA', (sum(SIZES[4:]) + 40 * 7 + 40, 300), (30, 30, 30, 255))
x = 20
for s in [16, 24, 32, 48, 64, 128, 256]:
    tile = canvas.resize((s, s), Image.Resampling.LANCZOS)
    sheet.paste(tile, (x, 280 - s), tile)
    x += s + 30
sheet.save(OUT_PNG.replace('.png', '_sizes.png'))
print('ok', OUT_ICO)
