"""예전 로펙 측정 CSV(캐릭터 · 회차마다 한 파일)를 한 파일(lopec_all.csv)로 합친다.

    - 대상: Verify/lopec_results/로펙_<캐릭터>_<YYYY-MM-DD>_<HHMMSS>.csv
    - 줄마다 "측정" 열 = 파일 이름의 <YYYY-MM-DD>_<HHMMSS> (회차, 시간 순으로 정렬됨)
      시각이 없는 옛 파일(로펙_이름_날짜.csv, 로펙_이름_날짜_GEM.csv)은 <날짜>_000000(_꼬리표)
    - 이미 lopec_all.csv에 있는 (캐릭터, 측정) 회차는 다시 넣지 않음 (여러 번 돌려도 안전)
    - 합친 원본 파일은 지우지 않고 Verify/lopec_results/archive/로 옮김

사용법 (PowerShell):
    python tools/Merge-LopecResults.py            # 합치고 원본을 archive/로 옮김
    python tools/Merge-LopecResults.py --keep     # 합치기만 (원본은 그대로)
"""
import argparse
import csv
import glob
import os
import re
import shutil
import sys

RESULTS_DIR = "Verify/lopec_results"                                       # 측정 CSV 폴더 (gitignore)
ALL_FILE = "lopec_all.csv"                                                 # 합친 파일
ARCHIVE_DIR = "archive"                                                    # 원본을 옮길 하위 폴더
OLD_PATTERN = "로펙_*.csv"                                                 # 예전 파일 이름
MEASURE_COLUMN = "측정"                                                    # 회차 열
CHARACTER_COLUMN = "캐릭터"                                                # 캐릭터 열
NAME_PATTERN = re.compile(r"^로펙_(?P<name>.+?)_(?P<day>\d{4}-\d{2}-\d{2})(?:_(?P<tail>.+))?\.csv$")   # 이름 · 날짜 · 시각(또는 꼬리표)
TIME_PATTERN = re.compile(r"^\d{6}$")                                    # HHMMSS
NO_TIME = "000000"                                                         # 시각이 없는 옛 파일 (그날 가장 앞으로)


def main() -> int:
    parser = argparse.ArgumentParser(description="로펙 측정 CSV → lopec_all.csv 한 파일")
    parser.add_argument("--keep", action="store_true", help="원본 파일을 archive/로 옮기지 않음")
    args = parser.parse_args()

    sources = []                                                           # (측정, 경로)
    for path in glob.glob(os.path.join(RESULTS_DIR, OLD_PATTERN)):
        match = NAME_PATTERN.match(os.path.basename(path))
        if match is None:
            print(f"  [건너뜀] 이름 형식이 달라요: {os.path.basename(path)}")
            continue
        tail = match["tail"] or ""                                         # 시각 또는 꼬리표 (예: GEM)
        stamp = tail if TIME_PATTERN.match(tail) else (f"{NO_TIME}_{tail}" if tail else NO_TIME)   # 시각이 없으면 그날 맨 앞
        sources.append((f"{match['day']}_{stamp}", path))
    sources.sort()                                                         # 시간 순 (같은 캐릭터면 뒤가 최신)
    if not sources:
        print("합칠 파일이 없어요.")
        return 0

    all_path = os.path.join(RESULTS_DIR, ALL_FILE)
    header = None                                                          # 합친 파일 머리글
    existing = set()                                                       # 이미 있는 (캐릭터, 측정)
    if os.path.exists(all_path) and os.path.getsize(all_path) > 0:
        with open(all_path, encoding="utf-8-sig", newline="") as handle:
            reader = csv.DictReader(handle)
            header = reader.fieldnames
            existing = {(row[CHARACTER_COLUMN], row[MEASURE_COLUMN]) for row in reader}

    added_rows = 0
    merged = []                                                            # 옮길 원본
    is_new = header is None
    with open(all_path, "a", encoding="utf-8-sig" if is_new else "utf-8", newline="") as output:
        writer = None
        for measure, path in sources:
            with open(path, encoding="utf-8-sig", newline="") as handle:
                reader = csv.DictReader(handle)
                if header is None:                                         # 첫 파일 머리글 + 측정 열
                    header = list(reader.fieldnames) + [MEASURE_COLUMN]
                if writer is None:
                    writer = csv.DictWriter(output, fieldnames=header, extrasaction="ignore")
                    if is_new:
                        writer.writeheader()
                for row in reader:
                    if (row.get(CHARACTER_COLUMN, ""), measure) in existing:   # 이미 합친 회차
                        continue
                    row[MEASURE_COLUMN] = measure
                    writer.writerow(row)
                    added_rows += 1
            merged.append(path)

    print(f"합침: 파일 {len(merged)}개, {added_rows}줄 → {all_path}")
    if args.keep:
        return 0

    archive = os.path.join(RESULTS_DIR, ARCHIVE_DIR)
    os.makedirs(archive, exist_ok=True)
    for path in merged:                                                    # 지우지 않고 옮김
        shutil.move(path, os.path.join(archive, os.path.basename(path)))
    print(f"원본 {len(merged)}개 → {archive}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
