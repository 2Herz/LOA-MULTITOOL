# -*- coding: utf-8 -*-
"""게임 패치 감지 (전투력 계수 재보정 필요 여부)

기준 캐릭터의 무기고를 API로 받아서, 장비 · 보석 · 각인 · 아크 패시브 · 아크 그리드 등이 그대로인데
API 전투력만 바뀌었으면 게임 밸런스 패치로 전투력 공식이 바뀐 것 → 계수 재보정이 필요하다고 알려 준다.

사용:
    set LOSTARK_API_KEY=...                       (API 키는 환경 변수로만 받음, 파일 · 로그에 남기지 않음)
    python tools/Check-CombatDrift.py --record 캐릭터1 캐릭터2   # 지금 상태를 기준으로 저장 (계수를 맞춘 직후)
    python tools/Check-CombatDrift.py                           # 저장된 기준과 비교

기준 파일: Verify/lopec_results/calibration_kit/drift_reference.json (gitignore, 캐릭터 이름이 들어 있음)
종료 코드: 0 = 변화 없음, 1 = 재보정 필요 의심, 2 = 실행 오류
"""
import argparse
import hashlib
import json
import os
import sys
import urllib.parse
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))                          # 저장소 루트
REFERENCE_PATH = os.path.join(ROOT, 'Verify', 'lopec_results', 'calibration_kit', 'drift_reference.json')   # 기준 (gitignore)
API_URL = 'https://developer-lostark.game.onstove.com/armories/characters/{}'               # 무기고 전체
TIMEOUT_SECONDS = 30                                                                        # 요청 제한 시간
CP_TOLERANCE_PERCENT = 0.05                                                                 # 이보다 작은 전투력 변화는 반올림 차이로 봄
EXIT_SAME, EXIT_DRIFT, EXIT_ERROR = 0, 1, 2                                                 # 종료 코드
# 전투력에 영향을 주는 무기고 묶음 (이 내용이 같으면 '장비는 그대로'로 판단)
FINGERPRINT_KEYS = ('ArmoryEquipment', 'ArmoryGem', 'ArmoryEngraving', 'ArmoryAvatars', 'ArkPassive', 'ArkGrid', 'ArmoryCard')


def fetch_armory(name: str, api_key: str) -> dict:
    url = API_URL.format(urllib.parse.quote(name))                                          # 이름은 URL 인코딩
    request = urllib.request.Request(url, headers={'authorization': f'bearer {api_key}', 'accept': 'application/json'})
    with urllib.request.urlopen(request, timeout=TIMEOUT_SECONDS) as response:              # 블로킹 요청
        return json.loads(response.read().decode('utf-8'))


def combat_power(armory: dict) -> float:
    text = str((armory.get('ArmoryProfile') or {}).get('CombatPower') or '0')                # "4,702.90" 형식
    return float(text.replace(',', ''))


def fingerprint(armory: dict) -> str:
    parts = {key: armory.get(key) for key in FINGERPRINT_KEYS}                              # 전투력 관련 묶음만
    canonical = json.dumps(parts, ensure_ascii=False, sort_keys=True)                       # 순서 고정
    return hashlib.sha256(canonical.encode('utf-8')).hexdigest()                            # 짧은 지문


def main() -> int:
    parser = argparse.ArgumentParser(description='게임 패치로 전투력 공식이 바뀌었는지 확인')
    parser.add_argument('--record', nargs='*', metavar='NAME', help='지금 상태를 기준으로 저장할 캐릭터 이름')
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')                                                # 한글 출력

    api_key = os.environ.get('LOSTARK_API_KEY', '').strip()                                 # 환경 변수에서만
    if not api_key:
        print('[ERROR] LOSTARK_API_KEY 환경 변수가 없어요.')
        return EXIT_ERROR

    if args.record:                                                                         # 기준 저장
        reference = {}
        for name in args.record:
            armory = fetch_armory(name, api_key)
            reference[name] = {'cp': combat_power(armory), 'fingerprint': fingerprint(armory)}
            print(f'[RECORD] {name}: 전투력 {reference[name]["cp"]:.2f}')
        os.makedirs(os.path.dirname(REFERENCE_PATH), exist_ok=True)
        json.dump(reference, open(REFERENCE_PATH, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
        return EXIT_SAME

    if not os.path.exists(REFERENCE_PATH):
        print('[ERROR] 기준이 없어요. 먼저 --record 캐릭터이름 으로 저장하세요.')
        return EXIT_ERROR

    reference = json.load(open(REFERENCE_PATH, encoding='utf-8'))
    drift = False
    for name, saved in reference.items():
        armory = fetch_armory(name, api_key)
        now_cp, same_gear = combat_power(armory), fingerprint(armory) == saved['fingerprint']
        change = (now_cp - saved['cp']) / saved['cp'] * 100 if saved['cp'] else 0.0       # 전투력 변화율 (%)
        if not same_gear:
            print(f'[SKIP] {name}: 장비 · 설정이 바뀌어서 비교 불가 (전투력 {change:+.3f}%) → 다시 --record')
        elif abs(change) > CP_TOLERANCE_PERCENT:
            drift = True
            print(f'[DRIFT] {name}: 장비는 그대로인데 전투력 {change:+.3f}% → 게임 패치로 공식 변경 의심, 계수 재보정 필요')
        else:
            print(f'[SAME] {name}: 전투력 {change:+.3f}%')
    return EXIT_DRIFT if drift else EXIT_SAME


if __name__ == '__main__':
    sys.exit(main())
