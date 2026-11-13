# -*- coding: utf-8 -*-
"""전투력 계수 회귀 테스트 (lopec 측정값 대비)

보정 키트(lopec_results/calibration_kit, gitignore)의 Python 미러로 지금까지 측정한 lopec 결과(아크 패시브 51명)를 다시 계산해서
data/CombatCoefficients.json을 바꾼 뒤 예측 오차가 허용치를 넘는 캐릭터가 생겼는지 확인한다.
 - 6랭크 캐릭터: ap_class_eval.py (단일 랭크 모델과 같음)
 - 랭크가 덜 찬 캐릭터: ap_tree_eval.py b (v1.5.4 트리별 랭크 모델)
 - lopec 쪽 특이점(레벨 슬라이더 오프셋 등)은 키트의 regression_exceptions.json에 적어 두고 제외

사용:
    python tools/Test-CombatRegression.py                  # 6랭크 0.1% · 덜 찬 랭크 0.8%
    python tools/Test-CombatRegression.py --full-rank 0.05 --partial-rank 0.8

종료 코드: 0 = 통과 (키트가 없는 PC면 건너뛰고 0), 1 = 허용치 초과, 2 = 평가 스크립트 실행 실패
측정 데이터(다른 플레이어 캐릭터 이름 포함)는 저장소에 올리지 않으므로 키트가 있는 개발 PC에서만 실제로 검사한다.
"""
import argparse
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))                        # 저장소 루트
KIT = os.path.join(ROOT, 'lopec_results', 'calibration_kit')                              # 보정 키트 (gitignore)
CLASS_SCRIPT = os.path.join(KIT, 'ap_class_eval.py')                                      # 6랭크 평가
TREE_SCRIPT = os.path.join(KIT, 'ap_tree_eval.py')                                        # 트리별 랭크 평가
CLASS_RESULT = os.path.join(KIT, 'analysis', 'class', 'ap_class_eval.json')               # 6랭크 평가 결과
ARMORY_FORMAT = os.path.join(KIT, 'analysis', 'class', 'armory_{}.json')                  # 캐릭터 무기고 (트리 랭크 확인)
EXCEPTIONS_PATH = os.path.join(KIT, 'regression_exceptions.json')                         # lopec 특이점 제외 목록 {"이름": ["항목", ...]}
DEFAULT_FULL_RANK_MAX = 0.1                                                               # 6랭크 허용 최대 오차 (%)
DEFAULT_PARTIAL_RANK_MAX = 0.8                                                            # 덜 찬 랭크 허용 최대 오차 (%) (랭크 0 부캐 약 0.77%)
FULL_RANKS = [6, 6, 6]                                                                    # 세 트리 모두 6랭크
EXIT_PASS, EXIT_FAIL, EXIT_ERROR = 0, 1, 2                                                # 종료 코드
STAT_MAX_INDEX = 1                                                                        # 결과 값 [평균, 최대 절댓값] 중 최대
TREE_LINE = re.compile(r'^(\S+)\s+\[[\d, ]+\]\s+\[[\d, ]+\]\s+(.*)$')                     # ap_tree_eval 출력 한 줄
TREE_ITEM = re.compile(r'(\S+) [+-][\d.]+/([\d.]+)')                                      # "항목 평균/최대"
TREE_KEY_PREFIX = {'enligh': 'enlightenment-point', 'rankCo': 'rankControl', 'levelC': 'levelControl'}   # 줄임 → 전체 이름


def tree_ranks(name: str):
    path = ARMORY_FORMAT.format(name)
    if not os.path.exists(path): return None                                              # 무기고가 없으면 판단 불가
    armory = json.load(open(path, encoding='utf-8'))
    ranks = []
    for point in armory['ArkPassive']['Points']:
        numbers = [int(x) for x in re.findall(r'\d+', point['Description'])]               # "N랭크 M레벨"
        ranks.append(numbers[0] if len(numbers) >= 2 else 0)                               # 빈 설명 = 랭크 0
    return ranks


def run_script(args):
    env = dict(os.environ, PYTHONIOENCODING='utf-8')                                      # 한글 출력
    return subprocess.run([sys.executable] + args, cwd=KIT, env=env, capture_output=True, text=True, encoding='utf-8')


def main() -> int:
    parser = argparse.ArgumentParser(description='lopec 측정값 대비 전투력 계수 회귀 테스트')
    parser.add_argument('--full-rank', type=float, default=DEFAULT_FULL_RANK_MAX, help='6랭크 허용 최대 오차 (%%)')
    parser.add_argument('--partial-rank', type=float, default=DEFAULT_PARTIAL_RANK_MAX, help='덜 찬 랭크 허용 최대 오차 (%%)')
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')

    if not (os.path.exists(CLASS_SCRIPT) and os.path.exists(TREE_SCRIPT)):                  # 키트가 없는 PC
        print('[SKIP] 보정 키트가 없어 회귀 테스트를 건너뜀:', KIT)
        return EXIT_PASS
    exceptions = json.load(open(EXCEPTIONS_PATH, encoding='utf-8')) if os.path.exists(EXCEPTIONS_PATH) else {}

    rows = []                                                                             # (이름, 항목, 최대 오차, 허용치)
    run = run_script([CLASS_SCRIPT])                                                      # 6랭크 평가
    if run.returncode != 0:
        print('[ERROR] ap_class_eval 실패\n', run.stderr[-2000:]); return EXIT_ERROR
    for row in json.load(open(CLASS_RESULT, encoding='utf-8')):
        if tree_ranks(row['name']) != FULL_RANKS: continue                                # 덜 찬 랭크는 아래에서
        for key, value in row.items():
            if key == 'cur' or not isinstance(value, list) or len(value) <= STAT_MAX_INDEX: continue
            rows.append((row['name'], key, abs(value[STAT_MAX_INDEX]), args.full_rank))

    run = run_script([TREE_SCRIPT, 'b'])                                                  # 트리별 랭크 평가
    if run.returncode != 0:
        print('[ERROR] ap_tree_eval 실패\n', run.stderr[-2000:]); return EXIT_ERROR
    for line in run.stdout.splitlines():
        match = TREE_LINE.match(line.strip())
        if not match: continue
        for short, worst in TREE_ITEM.findall(match.group(2)):
            rows.append((match.group(1), TREE_KEY_PREFIX.get(short, short), float(worst), args.partial_rank))

    failures = [r for r in rows if r[2] > r[3] and r[1] not in exceptions.get(r[0], [])]   # 허용치 초과 (제외 목록 빼고)
    skipped = sum(1 for r in rows if r[1] in exceptions.get(r[0], []))                     # 제외한 항목 수
    print(f'항목 {len(rows)}개 검사 (lopec 특이점 제외 {skipped}개), 허용치 6랭크 {args.full_rank}% · 덜 찬 랭크 {args.partial_rank}%')
    for name, key, worst, limit in sorted(failures, key=lambda f: -f[2]):
        print(f'[FAIL] {name} {key}: {worst:.3f}% (허용 {limit}%)')
    print('[PASS]' if not failures else f'[FAIL] {len(failures)}개 초과')
    return EXIT_PASS if not failures else EXIT_FAIL


if __name__ == '__main__':
    sys.exit(main())
