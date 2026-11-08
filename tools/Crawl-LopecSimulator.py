"""로펙(lopec.kr) 시뮬레이터 크롤러 → CSV.

동작 원리 (2026-11-07 확인):
    - 로펙 시뮬레이터는 계산을 전부 브라우저 안에서 한다 (값을 바꿔도 서버 요청이 없음).
      → 페이지를 캐릭터마다 한 번만 열고, 화면의 선택 상자 · 숫자 칸 · 옵션 선택 창을 바꾸면서
        왼쪽 [시뮬레이팅] 칸의 로펙 점수 · 예상 전투력 · 아이템 레벨을 읽는다. 사이트 서버에 부담이 없다.
    - 예상 전투력은 로아랩 실측과 거의 같다 (무기 품질 100→50: 둘 다 4160.26 / 목걸이 스탯 17857: 4711.12 vs 4710.99).
      단, 목걸이 적주피 2% → 추피 2.6% 교체처럼 예상 전투력이 움직이지 않는 경우도 있어서 로펙 점수도 같이 기록한다.

사용법 (PowerShell):
    python tools/Crawl-LopecSimulator.py                           # 기준 + 검증 캐릭터 3명, 전체
    python tools/Crawl-LopecSimulator.py --name 핢머니
    python tools/Crawl-LopecSimulator.py --only ACCESSORY,COMBO_DEAL
    python tools/Crawl-LopecSimulator.py --gear-samples 250 --deal-samples 250
    python tools/Crawl-LopecSimulator.py --list                    # 실험 수만 세고 끝냄

실험 종류:
    단일 변경 : 선택 상자 · 숫자 칸 · 아바타 · 옵션 선택 창을 하나씩 모든 값으로 바꾸고 원래 값으로 되돌림
    상급 재련 : 장비 등급을 T4 고대로 바꾼 뒤 상급 재련 X0 ~ X40 (전율은 상급 재련이 없어서)
    COMBO_GEAR: 6부위 재련 + 품질을 동시에 무작위 (기본 250)
    COMBO_DEAL: 악세 연마 · 주스탯 + 팔찌 효과 + 스톤 각인을 동시에 무작위 (기본 250)

결과: lopec_results/로펙_<캐릭터>_<날짜>.csv (utf-8-sig)
"""
import argparse
import csv
import os
import random
import sys
import time
import urllib.parse
from datetime import date, datetime

from playwright.sync_api import sync_playwright

# ===== 접속 · 출력 =====
SIMULATOR_URL = "https://lopec.kr/character/simulator/{name}"   # 캐릭터 시뮬레이터 주소
DEFAULT_CHARACTERS = ["핢머니", "리퍼는지켜줘야해", "필례"]   # 기준 + 검증 캐릭터
OUTPUT_DIR = "lopec_results"                                      # CSV 저장 폴더
PROFILE_DIR = "lopec_browser_profile"                             # 브라우저 프로필 (쿠키 유지)
AD_MARKER = "google_vignette"                                     # 전면 광고가 뜨면 주소에 붙는 표시
NO_THROTTLE_ARGS = ["--disable-background-timer-throttling",      # 가려진 창의 타이머 억제 끄기
                    "--disable-renderer-backgrounding",
                    "--disable-backgrounding-occluded-windows"]

# ===== 대기 =====
PAGE_TIMEOUT_MS = 60000          # 페이지 로드 최대 대기
READY_TIMEOUT = 60.0             # 시뮬레이터 칸이 그려질 때까지 최대 대기 (초)
POLL = 0.15                      # 값 확인 간격 (초)
SETTLE_MIN = 0.3                 # 바꾼 뒤 최소 대기 (초)
SETTLE_MAX = 4.0                 # 값이 안정될 때까지 최대 대기 (초)
CHARACTER_INTERVAL = 5.0         # 캐릭터 사이 쉬는 시간 (초)
DRIFT_TOLERANCE = 0.005          # 원래대로 되돌렸을 때 허용 오차 (이보다 다르면 페이지 새로 열기)
VALUE_DIGITS = 2                 # 화면 값 자릿수
RATIO_DIGITS = 6                 # 배율 자릿수

# ===== 숫자 칸 측정 값 =====
QUALITY_VALUES = range(0, 101, 5)            # 품질
ACCESSORY_STAT_STEPS = 10                    # 악세 주스탯: 최소 ~ 최대를 10등분
BRACELET_STAT_VALUES = range(0, 201, 20)     # 팔찌 스탯 (치명 · 특화 · 신속 …)
ARK_PASSIVE_LEVELS = range(1, 31)            # 아크패시브 레벨
ADVANCED_TIER = "T4 고대"                    # 상급 재련이 열리는 장비 등급
ADVANCED_LEVELS = range(0, 41)               # 상급 재련 X0 ~ X40
ARMOR_PARTS = ["helmet", "shoulder", "armor", "pants", "gloves", "weapon"]   # 장비 부위 id

# ===== 무작위 조합 =====
COMBO_GEAR_COUNT = 250          # 장비 조합 수
COMBO_DEAL_COUNT = 250          # 악세 + 팔찌 + 스톤 조합 수
OUTPUT_TIME_FORMAT = "%H%M%S"   # 결과 파일 이름 뒤에 붙이는 시각 (덮어쓰기 방지)
COMBO_ACC_COUNT = 0            # 악세 전용 조합 수 (기본은 안 돌림, --acc-samples로 지정)
COMBO_SEED = 20261107
COMBO_DEAL_SEED_OFFSET = 1      # 딜 조합 난수 시드 = COMBO_SEED + 1 (예전 결과와 같은 조합)
COMBO_ACC_SEED_OFFSET = 2       # 악세 전용 조합 난수 시드 = COMBO_SEED + 2
COMBO_PROGRESS_INTERVAL = 25    # 진행 상황을 찍는 간격 (조합 수)

# ===== 아크그리드 조합 (COMBO_GRID) =====
COMBO_GRID_COUNT = 0            # 아크그리드 조합 수 (기본은 안 돌림, --grid-samples로 지정)
COMBO_GRID_SEED_OFFSET = 3      # 아크그리드 조합 난수 시드 = COMBO_SEED + 3
ARK_CORE_SLOT_COUNT = 6         # 코어 칸 수 (질서 해·달·별 + 혼돈 해·달·별)
ARK_GEMS_PER_CORE = 4           # 코어 하나에 끼우는 젬 수
ARK_GEM_OPTION_SLOTS = (1, 2)   # 젬 하나의 옵션 칸 번호 (option-1, option-2)
ARK_GEM_DEAL_OPTIONS = ("공격력", "추가 피해", "보스 피해")   # 딜러 전투력에 들어가는 젬 옵션만 사용
ARK_GEM_LEVELS = range(1, 6)    # 젬 옵션 레벨 Lv.1 ~ Lv.5
ARK_GEM_POINTS = range(1, 6)    # 젬 하나의 포인트 1 ~ 5
ARK_CORE_POINT_STEPS = (10, 14, 17, 18, 19, 20)   # 코어 효과가 바뀌는 포인트 (이 값들을 자주 뽑음)
ARK_CORE_POINT_STEP_CHANCE = 0.5                  # 목표 포인트를 위 계단값에서 뽑을 확률 (나머지는 아무 값)
COMBO_MIN_REFORGE = 10
COMBO_MIN_QUALITY = 50
ACCESSORY_PICKER_PARTS = ["necklace", "earing1", "earing2", "ring1", "ring2"]   # 연마 옵션 선택 창 id 앞부분
BRACELET_PICKER_PREFIX = "bangle"
STONE_PICKER_PREFIX = "stone"
NONE_OPTION = "없음"

# ===== 단일 변경에서 건너뛰는 컨트롤 (결과에 영향이 없거나, 일괄 변경 · 이름만 바꾸는 것) =====
SKIP_PREFIXES = ("gem-skill", "ark-blade", "all-normal-setting", "all-advanced-setting",
                 "ark-gem-level-all", "stone-option-health")
SKIP_SUFFIXES = ("-willpower",)

CSV_HEADER = ["번호", "캐릭터", "계통", "컨트롤", "항목", "변경 전", "변경 후",
              "기준 로펙", "기준 전투력", "로펙 점수", "예상 전투력", "아이템 레벨",
              "전투력 증감", "전투력 배율", "로펙 증감", "메모"]

# ===== 페이지 안 도우미 (한 번 심어 두고 호출) =====
HELPER_JS = r"""
() => {
  const sleep = ms => new Promise(r => setTimeout(r, ms));
  const numberAfter = (text, label) => {
    const m = text.match(new RegExp(label + '\\s*\\n\\s*([\\d,]+(?:\\.\\d+)?)'));
    return m ? parseFloat(m[1].replace(/,/g, '')) : null;
  };
  // 숫자 칸 이름 붙이기: 품질은 id, 나머지는 순서대로 (악세 5 → 팔찌 → 아크패시브 레벨)
  const numberInputs = () => {
    const list = [];
    let acc = 0, bangle = 0, ark = 0;
    for (const el of document.querySelectorAll('input[type=number]')) {
      let key;
      if (el.id) key = el.id;
      else if ((el.className || '').includes('SimGroupArk')) key = 'ark-passive-level-' + (ark++);
      else if (el.getAttribute('aria-label') === '스텟값 입력' && el.max) key = 'acc-stat-' + (acc++);
      else if (el.getAttribute('aria-label') === '스텟값 입력') key = 'bangle-stat-' + (bangle++);
      else continue;
      list.push({ key, el });
    }
    return list;
  };
  const pickerTrigger = id => document.getElementById(id);
  const pickerValue = id => (document.querySelector(`input[type=hidden][name="${id}"]`) || {}).value;
  const waitDialog = async (open) => {
    for (let i = 0; i < 40; i++) {
      const d = document.querySelector('[role=dialog]');
      if (open ? d : !d) return d;
      await sleep(50);
    }
    return null;
  };
  // 점수 칸 구조: [class*=resultBox] 안에 h3(이름) + [class*=_value](값) → 페이지 전체 글자를 읽지 않음
  const RESULT_NAMES = { '로펙 점수': 'lopec', '예상 전투력': 'combat', '아이템 레벨': 'level' };
  const readPanel = () => {
    const out = {};
    for (const box of document.querySelectorAll('[class*="SimGroupResult_resultBox"]')) {
      const name = RESULT_NAMES[(box.querySelector('h3') || {}).textContent?.trim()];
      const value = box.querySelector('[class*="SimGroupResult_value"]');
      if (name && value) out[name] = parseFloat(value.textContent.replace(/,/g, ''));
    }
    return out;
  };
  window.__lp = {
    read() {
      const fast = readPanel();
      if (fast.lopec != null && fast.combat != null && fast.level != null) return fast;
      // 클래스 이름이 바뀌었을 때 대비: 페이지 전체 글자에서 찾기
      const t = document.body.innerText;
      return { lopec: numberAfter(t, '로펙 점수'), combat: numberAfter(t, '예상 전투력'), level: numberAfter(t, '아이템 레벨') };
    },
    // 값이 두 번 연속 같아질 때까지 페이지 안에서 기다렸다가 돌려줌 (Python과 왕복을 줄임)
    async settle(minMs, pollMs, maxMs) {
      const start = performance.now();
      await sleep(minMs);
      let last = JSON.stringify(this.read());
      while (performance.now() - start < maxMs) {
        await sleep(pollMs);
        const now = JSON.stringify(this.read());
        if (now === last) break;
        last = now;
      }
      return JSON.parse(last);
    },
    controls() {
      const selects = [...document.querySelectorAll('select')].filter(s => s.id).map(s => ({
        kind: 'select', key: s.id, label: s.getAttribute('aria-label') || s.id, disabled: s.disabled,
        value: s.value, text: s.options[s.selectedIndex]?.text || '',
        options: [...s.options].map(o => [o.value, o.text])
      }));
      const numbers = numberInputs().map(({ key, el }) => ({
        kind: 'number', key, label: el.getAttribute('aria-label') || key, value: el.value,
        min: el.min === '' ? null : Number(el.min), max: el.max === '' ? null : Number(el.max)
      }));
      const slots = {};
      for (const r of document.querySelectorAll('input[type=radio]')) {
        const [grade, slot] = (r.value || '').split(':');
        if (!slot) continue;
        (slots[slot] = slots[slot] || { kind: 'radio', key: 'avatar-' + slot, label: '아바타 ' + slot, options: [] }).options.push([r.value, grade]);
        if (r.checked) slots[slot].value = r.value;
      }
      const pickers = [...document.querySelectorAll('button[aria-haspopup=dialog][id*="-option-select-"]')].map(b => ({
        kind: 'picker', key: b.id, label: (b.getAttribute('aria-label') || b.id).split(':')[0], value: pickerValue(b.id)
      }));
      return [...selects, ...numbers, ...Object.values(slots), ...pickers];
    },
    // 화면 전체 상태 (선택 상자 · 숫자 칸 · 아바타 · 옵션 선택 창)
    snapshot() {
      const selects = {};
      for (const s of document.querySelectorAll('select')) if (s.id) selects[s.id] = s.value;
      const numbers = {};
      for (const { key, el } of numberInputs()) numbers[key] = el.value;
      const radios = [...document.querySelectorAll('input[type=radio]:checked')].map(r => r.value);
      const pickers = {};
      for (const b of document.querySelectorAll('button[aria-haspopup=dialog][id*="-option-select-"]')) pickers[b.id] = pickerValue(b.id);
      return { selects, numbers, radios, pickers };
    },
    // 저장한 상태로 되돌림. 하나를 바꾸면 다른 칸이 초기화되는 경우(보석 종류 → 레벨 등)가 있어서 여러 번 반복
    async restore(snap) {
      let changed = 0;
      for (let pass = 0; pass < 3; pass++) {
        let passChanged = 0;
        for (const [id, value] of Object.entries(snap.selects)) {
          const s = document.getElementById(id);
          if (s && !s.disabled && s.value !== value) { this.setSelect(id, value); passChanged++; }
        }
        const current = Object.fromEntries(numberInputs().map(({ key, el }) => [key, el.value]));
        for (const [key, value] of Object.entries(snap.numbers)) {
          if (key in current && current[key] !== value) { this.setNumber(key, value); passChanged++; }
        }
        for (const value of snap.radios) {
          const r = document.querySelector(`input[type=radio][value="${value}"]`);
          if (r && !r.checked) { r.click(); passChanged++; }
        }
        for (const [id, value] of Object.entries(snap.pickers)) {
          if (value && pickerValue(id) !== value) { await this.pick(id, value); passChanged++; }
        }
        changed += passChanged;
        if (!passChanged) break;
        await sleep(100);
      }
      return changed;
    },
    setSelect(id, value) {
      const s = document.getElementById(id);
      if (!s || s.disabled) return false;
      Object.getOwnPropertyDescriptor(HTMLSelectElement.prototype, 'value').set.call(s, value);
      s.dispatchEvent(new Event('change', { bubbles: true }));
      return s.value === value;
    },
    setNumber(key, value) {
      const found = numberInputs().find(x => x.key === key);
      if (!found) return false;
      const el = found.el;
      el.focus();
      Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value').set.call(el, String(value));
      el.dispatchEvent(new Event('input', { bubbles: true }));
      el.dispatchEvent(new Event('change', { bubbles: true }));
      el.blur();
      return true;
    },
    setRadio(value) {
      const r = document.querySelector(`input[type=radio][value="${value}"]`);
      if (!r) return false;
      r.click();
      return true;
    },
    async pickerOptions(id) {
      const trigger = pickerTrigger(id);
      if (!trigger) return [];
      trigger.click();
      const dialog = await waitDialog(true);
      if (!dialog) return [];
      const options = [...dialog.querySelectorAll('button[value]')].map(b => ({
        value: b.value, group: b.dataset.optionGroup || '', disabled: b.disabled || b.getAttribute('aria-disabled') === 'true'
      }));
      const close = dialog.querySelector('button[aria-label="옵션 선택 닫기"]');
      if (close) close.click();
      await waitDialog(false);
      return options;
    },
    async pick(id, value) {
      if (pickerValue(id) === value) return true;
      const trigger = pickerTrigger(id);
      if (!trigger) return false;
      trigger.click();
      const dialog = await waitDialog(true);
      if (!dialog) return false;
      const button = [...dialog.querySelectorAll('button[value]')].find(b => b.value === value);
      if (!button || button.disabled) {
        const close = dialog.querySelector('button[aria-label="옵션 선택 닫기"]');
        if (close) close.click();
        await waitDialog(false);
        return false;
      }
      button.click();
      await waitDialog(false);
      return true;
    }
  };
  return true;
}
"""


# ---------------------------------------------------------------------------
# 페이지 조작
# ---------------------------------------------------------------------------
def open_simulator(page, name):
    """캐릭터 시뮬레이터 열기 → 도우미 심기 → 왼쪽 점수 칸이 그려질 때까지 기다림"""
    url = SIMULATOR_URL.format(name=urllib.parse.quote(name))
    for _ in range(3):                                   # 전면 광고로 주소가 바뀌면 다시 열기
        page.goto(url, wait_until="domcontentloaded", timeout=PAGE_TIMEOUT_MS)
        deadline = time.time() + READY_TIMEOUT
        while time.time() < deadline:
            if AD_MARKER in page.url:
                break
            page.evaluate(HELPER_JS)
            scores = page.evaluate("() => window.__lp.read()")
            if scores["combat"] and page.evaluate("() => document.querySelectorAll('select').length") > 0:
                time.sleep(SETTLE_MAX / 2)              # 처음 그려진 직후 값이 한 번 더 바뀔 수 있음
                return stable_read(page)
            time.sleep(POLL * 2)
    raise TimeoutError(f"{name}: 시뮬레이터 칸이 나오지 않았어요")


def stable_read(page):
    """점수 칸 값이 두 번 연속 같을 때까지 페이지 안에서 기다렸다가 읽음 (왕복 1번)"""
    return page.evaluate("([a, b, c]) => window.__lp.settle(a, b, c)",
                         [int(SETTLE_MIN * 1000), int(POLL * 1000), int(SETTLE_MAX * 1000)])


def apply(page, control, value):
    """컨트롤 하나를 값으로 바꿈 (성공 여부)"""
    kind, key = control["kind"], control["key"]
    if kind == "select":
        return page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [key, value])
    if kind == "number":
        return page.evaluate("([k, v]) => window.__lp.setNumber(k, v)", [key, value])
    if kind == "radio":
        return page.evaluate("v => window.__lp.setRadio(v)", value)
    return page.evaluate("([k, v]) => window.__lp.pick(k, v)", [key, value])


def same_scores(a, b):
    """두 점수가 허용 오차 안에서 같은지"""
    return all(a[k] is not None and b[k] is not None and abs(a[k] - b[k]) <= DRIFT_TOLERANCE
               for k in ("lopec", "combat", "level"))


# ---------------------------------------------------------------------------
# 실험 목록
# ---------------------------------------------------------------------------
def group_of(key):
    """컨트롤 id → 계통 이름 (--only 로 고를 때 씀)"""
    if key.startswith("gem-"):
        return "GEM"
    if key.endswith("-quality"):
        return "QUALITY"
    if key.startswith("armory-"):
        return "ARMORY"
    if key.startswith(("necklace", "earing", "ring", "acc-stat")):
        return "ACCESSORY"
    if key.startswith(("bangle",)):
        return "BRACELET"
    if key.startswith("stone"):
        return "STONE"
    if key.startswith("engraving"):
        return "ENGRAVING"
    if key.startswith("avatar"):
        return "AVATAR"
    if key.startswith(("ark-core", "ark-point")):
        return "ARK_CORE"
    if key.startswith("ark-gem"):
        return "ARK_GEM"
    return "ARK_PASSIVE"                                # enlightenment-point · levelControl · rankControl · 레벨 칸


def number_values(control):
    """숫자 칸마다 측정할 값 목록"""
    key = control["key"]
    if key.endswith("-quality"):
        return list(QUALITY_VALUES)
    if key.startswith("acc-stat") and control["min"] is not None and control["max"] is not None:
        low, high = control["min"], control["max"]
        return sorted({round(low + (high - low) * step / ACCESSORY_STAT_STEPS) for step in range(ACCESSORY_STAT_STEPS + 1)})
    if key.startswith("bangle-stat"):
        return list(BRACELET_STAT_VALUES)
    return list(ARK_PASSIVE_LEVELS)


def sweep_values(page, control):
    """단일 변경 실험에서 넣어 볼 값 목록 [(값, 표시 이름)] (지금 값은 빼고)"""
    if control["kind"] in ("select", "radio"):
        return [(value, text) for value, text in control["options"] if value != control["value"]]
    if control["kind"] == "number":
        return [(v, str(v)) for v in number_values(control) if str(v) != str(control["value"])]
    options = page.evaluate("id => window.__lp.pickerOptions(id)", control["key"])
    return [(o["value"], o["value"]) for o in options if o["value"] != control["value"] and not o["disabled"]]


def is_skipped(control):
    """단일 변경에서 건너뛸 컨트롤인지"""
    key = control["key"]
    return (key.startswith(SKIP_PREFIXES) or key.endswith(SKIP_SUFFIXES) or control.get("disabled")
            or (control["kind"] == "select" and len(control["options"]) < 2))


# ---------------------------------------------------------------------------
# 결과 기록
# ---------------------------------------------------------------------------
class ResultWriter:
    """CSV 한 파일 (줄마다 바로 저장해서 중간에 멈춰도 결과가 남음)"""

    def __init__(self, path, name, baseline):
        self.m_handle = open(path, "w", newline="", encoding="utf-8-sig")
        self.m_writer = csv.writer(self.m_handle)
        self.m_name = name
        self.m_baseline = baseline
        self.m_count = 0
        self.m_writer.writerow(CSV_HEADER)
        self.row("BASELINE", "", "기준 (변경 없음)", "", "", baseline, "")

    def row(self, group, key, label, before, after, scores, memo):
        """실험 한 줄"""
        base = self.m_baseline
        combat, lopec = scores.get("combat"), scores.get("lopec")
        ok = combat is not None and base["combat"]
        self.m_writer.writerow([
            self.m_count, self.m_name, group, key, label, before, after, base["lopec"], base["combat"],
            lopec, combat, scores.get("level"),
            round(combat - base["combat"], VALUE_DIGITS) if ok else "",
            round(combat / base["combat"], RATIO_DIGITS) if ok else "",
            round(lopec - base["lopec"], VALUE_DIGITS) if lopec is not None else "", memo])
        self.m_handle.flush()
        self.m_count += 1

    def close(self):
        self.m_handle.close()


# ---------------------------------------------------------------------------
# 실험 실행
# ---------------------------------------------------------------------------
def restore_state(page, snapshot):
    """저장한 화면 상태로 되돌림 (바뀐 칸만, 반환: 되돌린 칸 수)"""
    return page.evaluate("snap => window.__lp.restore(snap)", snapshot)


def restore_or_reload(page, name, control, snapshot, baseline):
    """저장한 상태로 되돌리고, 점수가 기준과 다르면 페이지를 새로 열어 기준 상태로 돌아감
    (반환: 새 페이지의 상태 저장본 또는 그대로)"""
    restore_state(page, snapshot)
    if same_scores(stable_read(page), baseline):
        return snapshot
    print(f"   [복구] {control['key']} 되돌린 뒤 점수가 달라서 페이지를 새로 엽니다.")
    open_simulator(page, name)
    return page.evaluate("() => window.__lp.snapshot()")


def run_single(page, name, writer, baseline, only):
    """단일 변경: 컨트롤마다 모든 값 → 읽기. 값을 바꿀 때마다 먼저 기준 상태로 되돌려서
    한 칸을 바꾸면 다른 칸이 초기화되는 경우(보석 종류 → 레벨 등)에도 '한 가지만 바꾼' 측정이 되게 함"""
    snapshot = page.evaluate("() => window.__lp.snapshot()")
    controls = [c for c in page.evaluate("() => window.__lp.controls()") if not is_skipped(c)]
    for control in controls:
        group = group_of(control["key"])
        if only and group not in only:
            continue
        values = sweep_values(page, control)
        before = control.get("text") or control["value"]
        print(f"   {group:<12} {control['label']} ({len(values)}개)")
        for value, text in values:
            restore_state(page, snapshot)
            if not apply(page, control, value):
                writer.row(group, control["key"], control["label"], before, text, {}, "적용 실패")
                continue
            writer.row(group, control["key"], control["label"], before, text, stable_read(page), "")
        snapshot = restore_or_reload(page, name, control, snapshot, baseline)


def run_advanced(page, name, writer, baseline):
    """상급 재련: 장비 등급을 T4 고대로 바꾸고 X0 ~ X40 (끝나면 페이지를 새로 열어 원래 상태로)"""
    for part in ARMOR_PARTS:
        tier, advanced = f"armory-{part}-tier", f"armory-{part}-advanced"
        if not page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [tier, ADVANCED_TIER]):
            print(f"   [건너뜀] {part}: 장비 등급을 {ADVANCED_TIER}로 바꾸지 못함")
            continue
        stable_read(page)
        print(f"   ADVANCED     {part} ({ADVANCED_TIER}) X0 ~ X40")
        for level in ADVANCED_LEVELS:
            ok = page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [advanced, str(level)])
            writer.row("ADVANCED", advanced, f"{part} 상급 재련 ({ADVANCED_TIER})", "X0", f"X{level}",
                       stable_read(page) if ok else {}, "" if ok else "적용 실패")
        open_simulator(page, name)


def picker_option_lists(page, prefixes):
    """옵션 선택 창 id → 고를 수 있는 옵션 목록 (조합용)"""
    lists = {}
    for control in page.evaluate("() => window.__lp.controls()"):
        if control["kind"] == "picker" and control["key"].startswith(prefixes):
            lists[control["key"]] = [o for o in page.evaluate("id => window.__lp.pickerOptions(id)", control["key"])
                                     if not o["disabled"]]
    return lists


def run_combo_gear(page, name, writer, count):
    """장비 조합: 6부위 재련 + 품질 동시에 무작위 (끝나면 페이지를 새로 엶)"""
    rng = random.Random(COMBO_SEED)
    for number in range(1, count + 1):
        parts = []
        for part in ARMOR_PARTS:
            reforge = rng.randint(COMBO_MIN_REFORGE, 25)
            quality = rng.randint(COMBO_MIN_QUALITY, 100)
            page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [f"armory-{part}-normal", str(reforge)])
            page.evaluate("([k, v]) => window.__lp.setNumber(k, v)", [f"armory-{part}-quality", quality])
            parts.append(f"{part}+{reforge}/q{quality}")
        writer.row("COMBO_GEAR", "", f"장비 조합 #{number}", "현재", " ".join(parts), stable_read(page), "")
        if number % 50 == 0:
            print(f"   COMBO_GEAR {number}/{count}")
    open_simulator(page, name)


def run_combo_deal(page, name, writer, count, accessory_only=False):
    """악세 + 팔찌 + 스톤 조합: 연마 옵션 · 주스탯 · 팔찌 효과 · 스톤 각인을 동시에 무작위 (끝나면 페이지를 새로 엶)
    accessory_only=True → 악세(연마 옵션 · 주스탯)만 바꾸는 COMBO_ACC (팔찌 · 스톤은 기준 그대로)"""
    rng = random.Random(COMBO_SEED + (COMBO_ACC_SEED_OFFSET if accessory_only else COMBO_DEAL_SEED_OFFSET))
    combo_group = "COMBO_ACC" if accessory_only else "COMBO_DEAL"
    combo_label = "악세 조합" if accessory_only else "딜 조합"
    extra_prefixes = () if accessory_only else (BRACELET_PICKER_PREFIX, STONE_PICKER_PREFIX)
    lists = picker_option_lists(page, tuple(ACCESSORY_PICKER_PARTS) + extra_prefixes)
    stats = [c for c in page.evaluate("() => window.__lp.controls()") if c["key"].startswith("acc-stat")]
    for number in range(1, count + 1):
        labels = []
        for part in ACCESSORY_PICKER_PARTS:
            # 한 부위 안에서는 같은 옵션 종류가 겹치지 않게 (없음은 여러 번 가능)
            used = set()
            for slot in range(3):
                key = f"{part}-option-select-{slot}"
                choices = [o for o in lists.get(key, []) if o["group"] not in used or o["value"] == NONE_OPTION]
                if not choices:
                    continue
                choice = rng.choice(choices)
                if choice["value"] != NONE_OPTION:
                    used.add(choice["group"])
                if page.evaluate("([k, v]) => window.__lp.pick(k, v)", [key, choice["value"]]):
                    labels.append(f"{part}:{choice['value']}")
        for stat in stats:
            value = rng.randint(int(stat["min"]), int(stat["max"]))
            page.evaluate("([k, v]) => window.__lp.setNumber(k, v)", [stat["key"], value])
            labels.append(f"{stat['key']}={value}")
        for key, options in lists.items():
            # 팔찌 · 스톤 (악세 전용 조합이면 extra_prefixes가 비어서 건너뜀)
            if extra_prefixes and key.startswith(extra_prefixes) and options:
                choice = rng.choice(options)
                if page.evaluate("([k, v]) => window.__lp.pick(k, v)", [key, choice["value"]]):
                    labels.append(f"{key}:{choice['value']}")
        writer.row(combo_group, "", f"{combo_label} #{number}", "현재", " | ".join(labels), stable_read(page), "")
        if number % COMBO_PROGRESS_INTERVAL == 0:
            print(f"   {combo_group} {number}/{count}")
    open_simulator(page, name)


def split_core_point(rng, target):
    """코어 포인트 목표값을 젬 4개의 포인트(각 1 ~ 5)로 나눔 (반환: 포인트 목록)"""
    low, high = min(ARK_GEM_POINTS), max(ARK_GEM_POINTS)
    target = max(low * ARK_GEMS_PER_CORE, min(high * ARK_GEMS_PER_CORE, target))
    points = [low] * ARK_GEMS_PER_CORE
    remain = target - low * ARK_GEMS_PER_CORE
    while remain > 0:
        # 아직 최대가 아닌 젬 하나에 1씩 더함
        index = rng.choice([i for i, p in enumerate(points) if p < high])
        points[index] += 1
        remain -= 1
    return points


def run_combo_grid(page, name, writer, count):
    """아크그리드 조합: 코어 6칸의 등급 · 코어 종류 · 젬 4개(포인트 · 딜 옵션 2개 · 레벨)를 동시에 무작위
    (코어 포인트 = 젬 포인트 합, 끝나면 페이지를 새로 엶)"""
    rng = random.Random(COMBO_SEED + COMBO_GRID_SEED_OFFSET)
    controls = {c["key"]: c for c in page.evaluate("() => window.__lp.controls()")}
    point_low = min(ARK_GEM_POINTS) * ARK_GEMS_PER_CORE
    point_high = max(ARK_GEM_POINTS) * ARK_GEMS_PER_CORE
    for number in range(1, count + 1):
        labels = []
        for core in range(ARK_CORE_SLOT_COUNT):
            grade_key, blade_key = f"ark-core-{core}", f"ark-blade-{core}"
            # 1) 코어 등급 (등급을 먼저 바꿔야 아래 칸이 초기화돼도 다시 채워짐)
            grade = rng.choice([value for value, _ in controls[grade_key]["options"]])
            page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [grade_key, grade])
            # 2) 코어 종류 (없음 제외: 질서 3종 · 혼돈 6종)
            blades = [value for value, _ in controls[blade_key]["options"] if value != NONE_OPTION]
            blade = rng.choice(blades)
            page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [blade_key, blade])
            # 3) 코어 포인트 목표 → 젬 4개 포인트로 나눔
            if rng.random() < ARK_CORE_POINT_STEP_CHANCE:
                target = rng.choice(ARK_CORE_POINT_STEPS)
            else:
                target = rng.randint(point_low, point_high)
            points = split_core_point(rng, target)
            gem_labels = []
            for gem, point in enumerate(points):
                prefix = f"ark-gem-{core}-{gem}"
                page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [f"{prefix}-point", str(point)])
                # 옵션 2칸은 서로 다른 딜 옵션
                options = rng.sample(ARK_GEM_DEAL_OPTIONS, len(ARK_GEM_OPTION_SLOTS))
                parts = [f"P{point}"]
                for slot, option in zip(ARK_GEM_OPTION_SLOTS, options):
                    level = rng.choice(ARK_GEM_LEVELS)
                    page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [f"{prefix}-option-{slot}", option])
                    page.evaluate("([k, v]) => window.__lp.setSelect(k, v)", [f"{prefix}-level-{slot}", str(level)])
                    parts.append(f"{option}{level}")
                gem_labels.append("/".join(parts))
            labels.append(f"{core}:{grade}:{blade}:{sum(points)}P[{' '.join(gem_labels)}]")
        writer.row("COMBO_GRID", "", f"아크그리드 조합 #{number}", "현재", " | ".join(labels), stable_read(page), "")
        if number % COMBO_PROGRESS_INTERVAL == 0:
            print(f"   COMBO_GRID {number}/{count}")
    open_simulator(page, name)


def random_value(page, control, rng):
    """컨트롤 하나에 넣을 무작위 값 (반환: (값, 표시 이름) 또는 None)"""
    kind = control["kind"]
    if kind in ("select", "radio"):
        options = control.get("options") or []
        return rng.choice(options) if options else None
    if kind == "number":
        values = number_values(control)
        if not values:
            return None
        value = rng.choice(values)
        return value, str(value)
    options = [o for o in page.evaluate("id => window.__lp.pickerOptions(id)", control["key"]) if not o["disabled"]]
    if not options:
        return None
    choice = rng.choice(options)
    return choice["value"], choice["value"]


def run_combo_groups(page, name, writer, groups, count):
    """범용 조합: 지정한 계통(AVATAR · ARK_PASSIVE · ENGRAVING · STONE · BRACELET · ARMORY …)의 컨트롤을
    동시에 무작위로 바꿈 (단일 변경에서 건너뛰는 컨트롤은 제외, 끝나면 페이지를 새로 엶)"""
    group_name = "COMBO_" + "+".join(sorted(groups))
    # 계통 이름으로 시드를 정해서 같은 계통은 다시 돌려도 같은 조합
    rng = random.Random(COMBO_SEED + sum(ord(ch) for ch in group_name))
    controls = [c for c in page.evaluate("() => window.__lp.controls()")
                if not is_skipped(c) and group_of(c["key"]) in groups]
    print(f"   {group_name}: 컨트롤 {len(controls)}개 × 조합 {count}개")
    for number in range(1, count + 1):
        labels = []
        for control in controls:
            picked = random_value(page, control, rng)
            if picked is None:
                continue
            value, text = picked
            if apply(page, control, value):
                labels.append(f"{control['key']}={text}")
        writer.row(group_name, "", f"{group_name} #{number}", "현재", " | ".join(labels), stable_read(page), "")
        if number % COMBO_PROGRESS_INTERVAL == 0:
            print(f"   {group_name} {number}/{count}")
    open_simulator(page, name)


def crawl_character(page, name, only, gear_count, deal_count, list_only, acc_count=COMBO_ACC_COUNT,
                    grid_count=COMBO_GRID_COUNT, combo_groups=(), combo_count=0, skip_single=False):
    """캐릭터 하나 전체 (반환: CSV 경로 또는 None)"""
    print(f"\n===== {name} =====")
    try:
        baseline = open_simulator(page, name)
    except Exception as error:
        print(f"[건너뜀] {error}")
        return None
    print(f"기준: 로펙 {baseline['lopec']} · 예상 전투력 {baseline['combat']} · 아이템 레벨 {baseline['level']}")
    controls = [c for c in page.evaluate("() => window.__lp.controls()") if not is_skipped(c)]
    if list_only:
        counts = {}
        for control in controls:
            group = group_of(control["key"])
            size = len(control.get("options", [])) - 1 if control["kind"] in ("select", "radio") else \
                len(number_values(control)) if control["kind"] == "number" else 0
            counts[group] = counts.get(group, 0) + max(size, 0)
        print("   단일 변경(옵션 선택 창 제외):", counts, f"· COMBO_GEAR {gear_count} · COMBO_DEAL {deal_count}")
        return None

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    # 같은 날 여러 번 돌려도 덮어쓰지 않게 시각(시분초)까지 붙임
    stamp = datetime.now().strftime(OUTPUT_TIME_FORMAT)
    path = os.path.join(OUTPUT_DIR, f"로펙_{name}_{date.today().isoformat()}_{stamp}.csv")
    writer = ResultWriter(path, name, baseline)
    try:
        # 교차 검증 캐릭터처럼 조합만 필요하면 --skip-single로 단일 변경을 건너뜀
        if not skip_single:
            run_single(page, name, writer, baseline, only)
        if not skip_single and (not only or "ADVANCED" in only):
            run_advanced(page, name, writer, baseline)
        if (not only or "COMBO_GEAR" in only) and gear_count:
            run_combo_gear(page, name, writer, gear_count)
        if (not only or "COMBO_DEAL" in only) and deal_count:
            run_combo_deal(page, name, writer, deal_count)
        # 악세 전용 조합은 --acc-samples로 수를 줬고, --only가 없거나 COMBO_ACC를 포함할 때만
        if (not only or "COMBO_ACC" in only) and acc_count:
            run_combo_deal(page, name, writer, acc_count, accessory_only=True)
        # 아크그리드 조합은 --grid-samples로 수를 줬고, --only가 없거나 COMBO_GRID를 포함할 때만
        if (not only or "COMBO_GRID" in only) and grid_count:
            run_combo_grid(page, name, writer, grid_count)
        # 범용 조합 (--combo 계통 목록 + --combo-samples 개수)
        if combo_groups and combo_count:
            run_combo_groups(page, name, writer, set(combo_groups), combo_count)
    finally:
        writer.close()
    print(f"저장: {path} ({writer.m_count}줄)")
    return path


def run():
    """브라우저 실행 → 캐릭터마다 crawl_character"""
    parser = argparse.ArgumentParser(description="로펙 시뮬레이터 크롤러")
    parser.add_argument("--name", default=",".join(DEFAULT_CHARACTERS), help="캐릭터 이름 (쉼표로 여러 명)")
    parser.add_argument("--only", default="", help="계통만: GEM,ARMORY,QUALITY,ADVANCED,ACCESSORY,BRACELET,STONE,"
                                                   "ENGRAVING,AVATAR,ARK_CORE,ARK_GEM,ARK_PASSIVE,COMBO_GEAR,COMBO_DEAL")
    parser.add_argument("--gear-samples", type=int, default=COMBO_GEAR_COUNT, help="장비 무작위 조합 수")
    parser.add_argument("--deal-samples", type=int, default=COMBO_DEAL_COUNT, help="악세 + 팔찌 + 스톤 무작위 조합 수")
    parser.add_argument("--acc-samples", type=int, default=COMBO_ACC_COUNT, help="악세 전용 무작위 조합 수 (COMBO_ACC)")
    parser.add_argument("--grid-samples", type=int, default=COMBO_GRID_COUNT, help="아크그리드 무작위 조합 수 (COMBO_GRID)")
    parser.add_argument("--combo", default="", help="범용 조합 계통 (쉼표로 여러 개, 예: AVATAR 또는 ENGRAVING,STONE)")
    parser.add_argument("--combo-samples", type=int, default=0, help="범용 조합 수")
    parser.add_argument("--skip-single", action="store_true", help="단일 변경 · 상급 재련을 건너뛰고 조합만")
    parser.add_argument("--profile", default=PROFILE_DIR, help="브라우저 프로필 폴더 (동시에 여러 개 돌릴 때 서로 다르게)")
    parser.add_argument("--list", action="store_true", help="실험 수만 세고 끝냄")
    parser.add_argument("--first", type=int, default=0, help="캐릭터 목록 앞에서 N명만 (한글 입력 없이 시험할 때)")
    args = parser.parse_args()
    only = {item.strip().upper() for item in args.only.split(",") if item.strip()}
    names = [item.strip() for item in args.name.split(",") if item.strip()]
    if args.first > 0:
        names = names[:args.first]

    results = []
    with sync_playwright() as playwright:
        # 화면이 보이는 브라우저 (진행 상황을 눈으로 확인할 수 있게)
        # 창이 다른 창에 가려져도 페이지가 느려지지 않게 (백그라운드 타이머 · 렌더링 억제 끄기)
        context = playwright.chromium.launch_persistent_context(
            # 여러 크롤러를 동시에 돌릴 때는 --profile로 프로필 폴더를 따로 써야 함 (폴더 하나는 한 브라우저만 열 수 있음)
            args.profile, headless=False, viewport={"width": 1440, "height": 960}, args=NO_THROTTLE_ARGS)
        page = context.pages[0] if context.pages else context.new_page()
        for index, name in enumerate(names):
            if index:
                time.sleep(CHARACTER_INTERVAL)
            path = crawl_character(page, name, only, args.gear_samples, args.deal_samples, args.list, args.acc_samples,
                                   args.grid_samples,
                                   [g.strip().upper() for g in args.combo.split(",") if g.strip()],
                                   args.combo_samples, args.skip_single)
            if path:
                results.append(path)
        context.close()

    print("\n완료:")
    for path in results:
        print(f"   {path}")
    return 0


if __name__ == "__main__":
    sys.exit(run())
