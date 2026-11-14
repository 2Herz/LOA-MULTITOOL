# -*- coding: utf-8 -*-
"""회귀 테스트용 무기고 받기 (아크 패시브 트리 랭크 · 레벨)

lopec 측정 CSV(Verify/lopec_results/로펙_*.csv)에 나오는 캐릭터마다 로스트아크 API 무기고를 받아서
Verify/lopec_results/regression/armory/<이름>.json으로 저장한다. (받은 시각 FetchedAt 추가)
앱의 자체 검사(LOA-MULTITOOL.exe --self-test)가 이 파일을 앱의 무기고 해석기로 읽고, CSV의 lopec 값과 실제 C++ 엔진 예측을 비교한다.

API 키
    - 기본: 앱이 저장한 키 (%LOCALAPPDATA%\\LostArkMultiTool\\APIKey.dat, Windows DPAPI · 같은 Windows 계정에서만 풀림)
    - 대신: LOSTARK_API_KEY 환경 변수
    키는 화면 · 파일 · 로그 어디에도 남기지 않는다.

사용:
    python tools/Fetch-RegressionArmory.py              # 아직 안 받은 캐릭터만
    python tools/Fetch-RegressionArmory.py --refresh    # 모두 다시 받기
종료 코드: 0 = 성공, 1 = 일부 실패, 2 = 실행 오류 (키 없음 등)
"""
import argparse
import csv
import ctypes
import ctypes.wintypes
import glob
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))                          # 저장소 루트
RESULTS_DIR = os.path.join(ROOT, 'Verify', 'lopec_results')                                 # lopec 측정 CSV (gitignore)
ARMORY_DIR = os.path.join(RESULTS_DIR, 'regression', 'armory')                              # 저장 위치 (gitignore)
CONSTANTS_PATH = os.path.join(ROOT, 'APP_Constants.ixx')                                    # DPAPI 보조값 위치
KEY_PATH = os.path.join(os.environ.get('LOCALAPPDATA', ''), 'LostArkMultiTool', 'APIKey.dat')   # 앱이 저장한 키
API_URL = 'https://developer-lostark.game.onstove.com/armories/characters/{}'               # 무기고 전체
TIMEOUT_SECONDS = 30                                                                        # 요청 제한 시간
REQUEST_INTERVAL_SECONDS = 0.7                                                              # 요청 간격 (분당 100회 제한 아래로)
PASSIVE_GROUPS = ('ARK_PASSIVE', 'COMBO_ARK_PASSIVE')                                       # 아크 패시브 측정 줄
CRYPTPROTECT_UI_FORBIDDEN = 0x1                                                             # 대화 상자 금지
ENTROPY_RE = re.compile(r'KEY_ENTROPY_TEXT\{\s*"([^"]*)"\s*\}')                             # 보조값 상수
EXIT_OK, EXIT_PARTIAL, EXIT_ERROR = 0, 1, 2                                                 # 종료 코드


class DataBlob(ctypes.Structure):                                                           # DPAPI DATA_BLOB
    _fields_ = [('cbData', ctypes.wintypes.DWORD), ('pbData', ctypes.POINTER(ctypes.c_char))]


def make_blob(data: bytes) -> DataBlob:
    buffer = ctypes.create_string_buffer(data, len(data))                                   # 입력 버퍼
    blob = DataBlob(len(data), ctypes.cast(buffer, ctypes.POINTER(ctypes.c_char)))
    blob._buffer = buffer                                                                   # 버퍼 수명 유지
    return blob


def load_stored_key() -> str:
    if not os.path.exists(KEY_PATH): return ''                                              # 저장된 키 없음
    match = ENTROPY_RE.search(open(CONSTANTS_PATH, encoding='utf-8-sig').read())
    if not match: return ''                                                                 # 보조값을 못 찾음
    source, entropy, output = make_blob(open(KEY_PATH, 'rb').read()), make_blob(match.group(1).encode('utf-8')), DataBlob()
    if not ctypes.windll.crypt32.CryptUnprotectData(ctypes.byref(source), None, ctypes.byref(entropy), None, None,
                                                    CRYPTPROTECT_UI_FORBIDDEN, ctypes.byref(output)):
        return ''                                                                           # 다른 계정 · 손상
    try:
        return ctypes.string_at(output.pbData, output.cbData).decode('utf-8').strip()
    finally:
        ctypes.windll.kernel32.LocalFree(output.pbData)                                     # DPAPI 출력 해제


def passive_characters() -> list[str]:
    names = set()
    for path in glob.glob(os.path.join(RESULTS_DIR, '로펙_*.csv')):
        for row in csv.DictReader(open(path, encoding='utf-8-sig')):
            if row.get('계통') in PASSIVE_GROUPS: names.add(row['캐릭터'])                   # 아크 패시브를 측정한 캐릭터
    return sorted(names)


def fetch_armory(name: str, api_key: str) -> dict:
    request = urllib.request.Request(API_URL.format(urllib.parse.quote(name)),
                                     headers={'authorization': f'bearer {api_key}', 'accept': 'application/json'})
    with urllib.request.urlopen(request, timeout=TIMEOUT_SECONDS) as response:
        return json.loads(response.read().decode('utf-8') or 'null') or {}


def main() -> int:
    parser = argparse.ArgumentParser(description='회귀 테스트용 무기고 받기')
    parser.add_argument('--refresh', action='store_true', help='이미 받은 캐릭터도 다시 받기')
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')

    api_key = load_stored_key() or os.environ.get('LOSTARK_API_KEY', '').strip()           # 저장된 키 우선
    if not api_key:
        print('[ERROR] API 키를 찾지 못했어요. (앱에서 키를 등록하거나 LOSTARK_API_KEY 설정)')
        return EXIT_ERROR

    os.makedirs(ARMORY_DIR, exist_ok=True)
    failed = 0
    for name in passive_characters():
        path = os.path.join(ARMORY_DIR, f'{name}.json')
        if os.path.exists(path) and not args.refresh: continue                              # 이미 받음
        try:
            armory = fetch_armory(name, api_key)
            if not armory.get('ArkPassive'): raise ValueError('ArkPassive 없음 (캐릭터 이름 변경 · 삭제?)')
            armory['FetchedAt'] = time.strftime('%Y-%m-%d %H:%M:%S')                        # 받은 시각
            json.dump(armory, open(path, 'w', encoding='utf-8'), ensure_ascii=False)       # 앱 해석기가 그대로 읽는 전체 응답
            print(f'[OK] {name}')
        except Exception as error:                                                          # 이 캐릭터만 실패 처리
            failed += 1
            print(f'[FAIL] {name}: {error}')
        time.sleep(REQUEST_INTERVAL_SECONDS)
    print(f'완료 · 실패 {failed}개 · 저장 위치 {ARMORY_DIR}')
    return EXIT_OK if failed == 0 else EXIT_PARTIAL


if __name__ == '__main__':
    sys.exit(main())
