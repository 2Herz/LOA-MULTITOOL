"""로펙(lopec.kr) 전투력 랭킹에서 직업별 보정용 샘플 캐릭터를 고른다 → CSV.

고르는 규칙 (정재 조건, 2026-11-19 · 로아와는 Cloudflare 때문에 로펙으로 변경):
    - 직업마다 직업 각인(깨달음) 두 가지 A · B (딜러 각인 + 서포터 각인 모두, 로펙 깨달음 목록 순서)
    - 각인마다 아이템 레벨 1800 구간 1명 + 1750 구간 1명 → 직업당 4명
    - 구간 안에서는 로펙 랭킹 순위가 가장 높은 캐릭터
    - 1750 구간 캐릭터가 없으면 그 각인의 랭킹 최하위 캐릭터로 대신 (레벨 구간 칸에 '최하위')

동작 원리 (2026-11-19 확인):
    - 랭킹 주소 /rank/specPoint?role=<deal|sup>&baseClass=<직업>&characterClass=<각인 줄임말>&page=<n>
      (한 페이지 20명, 점수 높은 순, 레벨 거르기는 없음)
    - 직업 목록 = #class-select, 각인 줄임말 = 직업을 고른 뒤 #enlightenment-select (첫 항목 '전체' 제외)
    - 1800 구간은 보통 첫 페이지에 있다. 1750 구간은 뒤쪽이라 페이지를 1 → 2 → 4 → 8 … 로 건너뛰며
      구간 캐릭터가 처음 보이는 곳을 찾고, 바로 앞 건너뛴 구간을 차례로 다시 읽어 가장 앞 순위를 고른다.
    - 페이지 사이마다 쉬는 시간 (사이트 부담 줄이기). 확인 화면이 뜨면 우회하지 않고 사람이 통과할 때까지 기다림.

사용법 (PowerShell):
    python tools/Pick-LopecSamples.py                     # 모든 직업 (딜러 + 서포터 각인)
    python tools/Pick-LopecSamples.py --class 데빌헌터     # 한 직업만 (쉼표로 여러 개)
결과: Verify/lopec_results/samples/lopec_samples.csv (utf-8-sig, gitignore · 다른 사람 캐릭터 이름이 들어 있음)
"""
import argparse
import csv
import os
import re
import sys
import time
import urllib.parse
from datetime import datetime

from playwright.sync_api import sync_playwright

# ===== 접속 · 출력 =====
BASE_URL = "https://lopec.kr"                                                     # 로펙 주소
RANK_URL = BASE_URL + "/rank/specPoint?role={role}&baseClass={cls}&characterClass={engraving}&page={page}"   # 랭킹 (직업 · 각인 · 페이지)
CLASS_LIST_URL = BASE_URL + "/rank/specPoint?role={role}&baseClass={cls}"        # 각인 목록을 읽을 주소
ROLES = (("deal", ""), ("sup", "Y"))                                              # (로펙 역할 값, 서포터 표시)
OUTPUT_DIR = "Verify/lopec_results/samples"                                       # 결과 폴더 (gitignore, 회귀 검사 CSV 폴더와 분리)
OUTPUT_FILE = "lopec_samples.csv"                                                 # 결과 파일
PROFILE_DIR = "Verify/lopec_browser_profile"                                      # 브라우저 프로필 (로펙 크롤러와 같은 쿠키)
VIEWPORT = {"width": 1440, "height": 960}                                         # 창 크기

# ===== 고르는 조건 =====
LEVEL_BANDS = (("1800", 1800.0, 1804.99), ("1750", 1750.0, 1754.99))              # (구간 이름, 최소, 최대) 아이템 레벨
FALLBACK_BAND = "1750"                                                            # 이 구간에 캐릭터가 없으면
FALLBACK_BAND_NAME = "최하위"                                                     # 그 각인의 랭킹 최하위 캐릭터로 대신 (정재 조건)
ENGRAVING_LABELS = ("A", "B")                                                     # 각인 두 개의 이름 (로펙 목록 순서)
RANK_LIMIT = 500                                                                  # 로펙 랭킹은 500등까지만 보여 줌
ROWS_PER_PAGE = 20                                                                # 한 페이지 줄 수
MAX_PAGE = RANK_LIMIT // ROWS_PER_PAGE                                            # 마지막 페이지 (25)
FIRST_PAGE = 1                                                                    # 첫 페이지 번호
PAGE_GROWTH = 2                                                                   # 건너뛸 때 페이지 배수

# ===== 대기 =====
PAGE_TIMEOUT_MS = 60000          # 페이지 로드 최대 대기
LIST_TIMEOUT = 20.0              # 목록이 그려질 때까지 최대 대기 (초)
CHALLENGE_TIMEOUT = 300.0        # 확인 화면을 사람이 통과할 때까지 최대 대기 (초)
POLL = 0.3                       # 확인 간격 (초)
PAGE_INTERVAL = 2.0              # 페이지 사이 쉬는 시간 (초)

# ===== 목록 해석 =====
LIST_HEADER = "순위\n닉네임\n점수\n레벨\n직업\n"                                     # 목록 머리글 (이 뒤부터 캐릭터 줄)
LEVEL_PATTERN = re.compile(r"^\d{4}\.\d{2}$")                                     # "1800.00"
SCORE_PATTERN = re.compile(r"^\d+\.\d{2}$")                                       # "9208.91"
RANK_PATTERN = re.compile(r"^\d+$")                                               # "41"
ROW_FIELD_COUNT = 6                                                               # 순위 · 닉네임 · 서버 · 점수 · 레벨 · 각인+직업
CHALLENGE_TITLES = ("Just a moment", "잠시만 기다리십시오")                       # 확인 화면 제목
EMPTY_ENGRAVING = "전체"                                                          # 각인 목록 첫 항목 (거르지 않음)
SUPPORT_ENGRAVING = "서폿"                                                        # 서포터 직업의 서포터 각인 (딜러 목록에도 이 이름으로 나옴)

COLUMNS = ["직업", "서포터", "각인 구분", "직업 각인", "레벨 구간", "캐릭터", "서버", "아이템 레벨", "로펙 점수", "랭킹 순위", "수집 시각"]


def wait_ready(page) -> None:
    """확인 화면이면 사람이 통과할 때까지 기다린다 (자동으로 풀지 않음)."""
    started = time.time()
    warned = False
    while any(title in page.title() for title in CHALLENGE_TITLES):
        if not warned:
            print("  확인 화면이에요. 브라우저 창에서 직접 통과해 주세요 (자동으로 풀지 않아요).")
            warned = True
        if time.time() - started > CHALLENGE_TIMEOUT:
            raise RuntimeError("확인 화면을 통과하지 못했어요.")
        time.sleep(POLL)


def open_page(page, url: str) -> None:
    """주소를 열고 확인 화면 · 로드를 기다린 뒤 쉬는 시간."""
    page.goto(url, timeout=PAGE_TIMEOUT_MS)
    wait_ready(page)
    time.sleep(PAGE_INTERVAL)


def select_options(page, selector: str) -> list[str]:
    """선택 상자의 값 목록 (그려질 때까지 기다림)."""
    deadline = time.time() + LIST_TIMEOUT
    while time.time() < deadline:
        values = page.evaluate("s => { const e = document.querySelector(s); return e ? [...e.options].map(o => o.value) : []; }", selector)
        if len(values) > 1:
            return values
        time.sleep(POLL)
    return []


def read_rows(page) -> list[dict]:
    """지금 페이지의 랭킹 줄 (순위 · 이름 · 서버 · 점수 · 레벨). 목록이 비면 빈 목록."""
    deadline = time.time() + LIST_TIMEOUT
    while time.time() < deadline:
        text = page.evaluate("() => document.body.innerText")
        start = text.find(LIST_HEADER)
        if start >= 0:
            lines = [line.strip() for line in text[start + len(LIST_HEADER):].split("\n") if line.strip()]
            rows = []
            index = 0
            while index + ROW_FIELD_COUNT <= len(lines):
                rank, name, server, score, level, engraving = lines[index:index + ROW_FIELD_COUNT]
                if not (RANK_PATTERN.match(rank) and SCORE_PATTERN.match(score) and LEVEL_PATTERN.match(level)):
                    break                                                         # 목록 끝 (페이지 번호 등)
                rows.append({"rank": int(rank), "name": name, "server": server, "score": float(score), "level": float(level)})
                index += ROW_FIELD_COUNT
            if rows:
                return rows
        time.sleep(POLL)
    return []


def rank_page(page, role: str, cls: str, engraving: str, number: int) -> list[dict]:
    """랭킹 한 페이지 읽기."""
    open_page(page, RANK_URL.format(role=role, cls=urllib.parse.quote(cls), engraving=urllib.parse.quote(engraving), page=number))
    return read_rows(page)


def first_in_band(rows: list[dict], minimum: float, maximum: float) -> dict | None:
    """순위가 가장 앞인 구간 캐릭터."""
    return next((row for row in rows if minimum <= row["level"] <= maximum), None)


def pick(page, role: str, cls: str, engraving: str, band: tuple, cache: dict) -> dict | None:
    """직업 · 각인 · 레벨 구간에서 순위가 가장 앞인 캐릭터 (페이지는 건너뛰며 찾고 앞 구간을 다시 훑음)."""
    _, minimum, maximum = band

    def rows_of(number: int) -> list[dict]:
        if number not in cache:                                                   # 같은 각인 안에서 페이지 재사용
            cache[number] = rank_page(page, role, cls, engraving, number)
        return cache[number]

    previous = 0                                                                  # 마지막으로 구간을 못 찾은 페이지
    number = FIRST_PAGE
    while number <= MAX_PAGE:
        rows = rows_of(number)
        if not rows:                                                              # 마지막 페이지를 넘음
            break
        if first_in_band(rows, minimum, maximum) is not None or rows[-1]["level"] < minimum:
            for scan in range(previous + 1, number + 1):                          # 건너뛴 구간을 앞에서부터
                found = first_in_band(rows_of(scan), minimum, maximum)
                if found is not None:
                    return found
            return None                                                           # 레벨이 이미 구간 아래로 내려감
        if number == MAX_PAGE:                                                    # 마지막 페이지까지 봤는데 없음
            return None
        previous = number
        number = min(number * PAGE_GROWTH, MAX_PAGE)                              # 건너뛰되 마지막 페이지를 넘지 않게
    for scan in range(previous + 1, number):                                      # 끝 페이지 근처를 차례로 (목록이 500명보다 짧을 때)
        rows = rows_of(scan)
        if not rows:
            break
        found = first_in_band(rows, minimum, maximum)
        if found is not None:
            return found
    return None


def last_ranked(page, role: str, cls: str, engraving: str, cache: dict) -> dict | None:
    """랭킹 최하위 캐릭터 (보통 500등 = 25페이지 마지막 줄, 1750 구간이 없을 때 대신 씀)."""
    def rows_of(number: int) -> list[dict]:
        if number not in cache:
            cache[number] = rank_page(page, role, cls, engraving, number)
        return cache[number]

    if rows_of(MAX_PAGE):                                                         # 500명이 다 차 있으면 바로 마지막 페이지
        return rows_of(MAX_PAGE)[-1]
    low = FIRST_PAGE                                                              # 목록이 500명보다 짧을 때: 목록이 있는 페이지
    if not rows_of(low):
        return None
    high = low * PAGE_GROWTH                                                      # 목록이 없는 페이지를 찾을 때까지 건너뜀
    while high <= MAX_PAGE and rows_of(high):
        low, high = high, high * PAGE_GROWTH
    while high - low > 1:                                                         # 마지막 페이지를 반씩 좁혀 찾음
        middle = (low + high) // 2
        if rows_of(middle):
            low = middle
        else:
            high = middle
    return rows_of(low)[-1]


def main() -> int:
    parser = argparse.ArgumentParser(description="로펙 전투력 랭킹 → 직업별 보정 샘플")
    parser.add_argument("--class", dest="classes", default="", help="직업 이름 (쉼표로 여러 개, 비우면 전체)")
    parser.add_argument("--profile", default=PROFILE_DIR, help="브라우저 프로필 폴더")
    args = parser.parse_args()
    wanted = {item.strip() for item in args.classes.split(",") if item.strip()}

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    output_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
    picked = 0
    missing = []

    with sync_playwright() as playwright:
        context = playwright.chromium.launch_persistent_context(args.profile, headless=False, viewport=VIEWPORT)
        page = context.pages[0] if context.pages else context.new_page()
        open_page(page, BASE_URL + "/rank/specPoint?role=deal")
        classes = [value for value in select_options(page, "#class-select") if value != "직업선택"]   # 직업 목록

        with open(output_path, "w", encoding="utf-8-sig", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(COLUMNS)
            for cls in classes:
                if wanted and cls not in wanted:
                    continue
                engravings = []                                                   # (역할, 서포터 표시, 각인 줄임말)
                for role, supporter in ROLES:
                    open_page(page, CLASS_LIST_URL.format(role=role, cls=urllib.parse.quote(cls)))
                    for value in select_options(page, "#enlightenment-select"):
                        if value != EMPTY_ENGRAVING and value not in [item[2] for item in engravings]:
                            engravings.append((role, "Y" if value == SUPPORT_ENGRAVING else supporter, value))   # 서폿 각인은 서포터로 표시
                for label, (role, supporter, engraving) in zip(ENGRAVING_LABELS, engravings):
                    cache = {}                                                    # 이 각인의 페이지 저장
                    for band in LEVEL_BANDS:
                        row = pick(page, role, cls, engraving, band, cache)
                        band_name = band[0]                                       # 기록할 구간 이름
                        if row is None and band[0] == FALLBACK_BAND:              # 1750 구간이 없으면 랭킹 최하위로 대신
                            row = last_ranked(page, role, cls, engraving, cache)
                            band_name = FALLBACK_BAND_NAME
                        if row is None:
                            missing.append(f"{cls} {engraving} {band[0]}")
                            print(f"  [없음] {cls} / {engraving} / {band[0]}")
                            continue
                        writer.writerow([cls, supporter, label, engraving, band_name, row["name"], row["server"],
                                         f"{row['level']:.2f}", f"{row['score']:.2f}", row["rank"],
                                         datetime.now().strftime("%Y-%m-%d %H:%M:%S")])
                        file.flush()                                              # 중간에 멈춰도 남게
                        picked += 1
                        print(f"  {cls} / {label} {engraving} / {band_name} → {row['name']} ({row['level']:.2f}, {row['rank']}위)")
        context.close()

    print(f"완료: {picked}명 → {output_path}" + (f" (못 찾음 {len(missing)}개)" if missing else ""))
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
