# -*- coding: utf-8 -*-
"""Visual.html(멀티툴 구조도) 다시 만들기

Visual.html은 고정된 화면 틀(HTML · CSS · JS)과 모듈 데이터 배열 `const MODULES = [...]`로 되어 있다.
이 스크립트는 소스(.ixx · .cpp · .vcxproj.filters)를 읽어 MODULES 배열과 머리말의 버전 · 기준 날짜만 다시 채운다.
(예전 생성기 make_visual.py는 보정 키트와 함께 삭제되어 저장소 안으로 옮김)

모듈 한 개의 값
    file        : 인터페이스 파일 이름 (.ixx)
    module      : export module 이름
    imports     : .ixx가 import하는 모듈 (std 제외)
    cppImports  : 같은 이름 .cpp가 import하는 모듈 (std 제외)
    lines       : .ixx 줄 수,  cppLines : .cpp 줄 수 (없으면 0),  hasCpp : .cpp 존재 여부
    filter      : Visual Studio 솔루션 탐색기 필터 (.vcxproj.filters)
    desc        : .ixx의 첫 번째 // 주석 (모듈 설명)
    types       : .ixx에 선언된 class · struct 이름 (최대 8개)

사용:
    python tools/Make-Visual.py
"""
import datetime
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent                                   # 저장소 루트
VISUAL_PATH = ROOT / 'Visual.html'                                               # 구조도
FILTERS_PATH = ROOT / '로스트아크 멀티툴.vcxproj.filters'                         # 솔루션 탐색기 필터
CONSTANTS_PATH = ROOT / 'APP_Constants.ixx'                                      # APP_VERSION 위치
MODULES_PREFIX = 'const MODULES = '                                              # 데이터 배열 시작
LF = '\n'                                                                        # 줄바꿈 (유닉스)
CRLF = '\r\n'                                                                    # 줄바꿈 (윈도우)
MAX_TYPES = 8                                                                    # 모듈당 표시할 타입 수
STD_MODULE = 'std'                                                               # 표준 라이브러리 모듈 (그래프에서 제외)
EXPORT_RE = re.compile(r'^\s*export\s+module\s+([\w.]+)\s*;', re.M)             # export module X;
IMPORT_RE = re.compile(r'^\s*(?:export\s+)?import\s+([\w.]+)\s*;', re.M)         # import X;
COMMENT_RE = re.compile(r'^\s*//\s?(.+?)\s*$', re.M)                             # 첫 // 주석
TYPE_RE = re.compile(r'^\s*(?:export\s+)?(?:template\s*<[^>]*>\s*)?(?:class|struct)\s+(\w+)(?![^\n]*;\s*$)', re.M)   # 선언 (전방 선언 제외)
FILTER_RE = re.compile(r'<ClCompile Include="([^"]+)">\s*<Filter>([^<]+)</Filter>')   # 파일 → 필터
VERSION_RE = re.compile(r'APP_VERSION\{\s*"([^"]+)"\s*\}')                       # 앱 버전
SUB_VERSION_RE = re.compile(r'(C\+\+23 모듈 · )v[\d.]+( · .*? · 기준 )\d{4}-\d{2}-\d{2}')   # 머리말 버전 · 날짜


def read_text(path: Path) -> str:
    return path.read_text(encoding='utf-8-sig')                                  # BOM 유무 상관없이


def imports_of(text: str, own_module: str) -> list[str]:
    found = [name for name in IMPORT_RE.findall(text) if name not in (STD_MODULE, own_module)]   # std · 자기 자신 제외
    return sorted(dict.fromkeys(found))                                          # 중복 제거 + 정렬


def build_modules() -> list[dict]:
    filters = dict(FILTER_RE.findall(FILTERS_PATH.read_text(encoding='utf-8-sig')))   # 파일 → 필터
    modules = []
    for ixx in sorted(ROOT.glob('*.ixx'), key=lambda p: p.name.lower()):          # 루트의 모든 인터페이스
        text = read_text(ixx)
        exported = EXPORT_RE.search(text)
        if not exported: continue                                                # 모듈 인터페이스가 아님
        name = exported.group(1)
        cpp = ixx.with_suffix('.cpp')
        cpp_text = read_text(cpp) if cpp.exists() else ''
        comment = COMMENT_RE.search(text)
        modules.append({
            'file': ixx.name,
            'module': name,
            'imports': imports_of(text, name),
            'cppImports': [m for m in imports_of(cpp_text, name) if m not in imports_of(text, name)],   # .ixx에 없는 것만
            'lines': text.count('\n') + 1,
            'cppLines': cpp_text.count('\n') + 1 if cpp_text else 0,
            'hasCpp': cpp.exists(),
            'filter': filters.get(ixx.name, ''),
            'desc': comment.group(1) if comment else '',
            'types': list(dict.fromkeys(TYPE_RE.findall(text)))[:MAX_TYPES],
        })
    return modules


def main() -> int:
    sys.stdout.reconfigure(encoding='utf-8')
    html = VISUAL_PATH.read_bytes().decode('utf-8')                              # 줄바꿈 그대로 (CRLF 유지)
    newline = CRLF if CRLF in html else LF                                      # 파일의 줄바꿈 방식
    start = html.index(MODULES_PREFIX) + len(MODULES_PREFIX)                    # 배열 시작 '['
    _, length = json.JSONDecoder().raw_decode(html[start:])                     # 기존 배열 길이 (괄호 짝 맞춤)
    modules = build_modules()                                                   # 소스에서 다시 계산
    array = json.dumps(modules, ensure_ascii=False, indent=1).replace(LF, newline)   # 파일 줄바꿈에 맞춤
    html = html[:start] + array + html[start + length:]                         # 배열만 교체

    version = VERSION_RE.search(read_text(CONSTANTS_PATH)).group(1)            # 현재 버전
    today = datetime.date.today().isoformat()                                    # 기준 날짜
    html = SUB_VERSION_RE.sub(lambda m: f'{m.group(1)}v{version}{m.group(2)}{today}', html, count=1)

    VISUAL_PATH.write_bytes(html.encode('utf-8'))                               # 줄바꿈 그대로 저장
    print(f'{len(modules)} modules · v{version} · {today}')
    print('Visual.html updated')
    return 0


if __name__ == '__main__':
    sys.exit(main())
