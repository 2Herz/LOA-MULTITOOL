"""
graphify 지식 그래프(graphify-out/graph.json)에 C++23 모듈 의존 엣지를 추가하는 도구.

graphify의 C++ 분석기는 `export module X;` / `module X;` / `import X;` 문법을 이해하지 못해서
.ixx 파일이 노드로는 들어가도 모듈 사이의 의존 관계가 그래프에 나타나지 않는다.
이 스크립트는 소스를 직접 읽어서 다음을 그래프에 넣는다.

    모듈 노드        : 모듈 이름 하나당 노드 하나 (예: LostArkMultiTool.Core.Input)
    exports_module   : 인터페이스 파일(.ixx)  → 모듈 노드
    implements_module: 구현 단위 파일(.cpp)   → 모듈 노드
    imports_module   : import 하는 파일        → import 대상 모듈 노드
    depends_on_module: 모듈 노드              → import 대상 모듈 노드 (모듈끼리 경로 추적용)

여러 번 실행해도 결과가 같다(이전 실행에서 넣은 노드·엣지를 먼저 지우고 다시 넣음).
`/graphify --update` 를 실행한 뒤에 이어서 실행하면 된다.

사용법:
    python tools/Graphify-ModuleEdges.py            # 그래프 갱신 + graph.html 다시 생성
    python tools/Graphify-ModuleEdges.py --no-html  # graph.json 만 갱신
"""

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

# ──────────────────────────── 상수 ────────────────────────────

# 프로젝트 루트 (이 스크립트는 tools/ 안에 있음)
PROJECT_ROOT = Path(__file__).resolve().parent.parent
# graphify 출력 폴더와 그래프 파일
GRAPH_DIR = PROJECT_ROOT / "graphify-out"
GRAPH_PATH = GRAPH_DIR / "graph.json"
# 모듈 문법을 찾을 소스 확장자
SOURCE_EXTENSIONS = (".ixx", ".cppm", ".cpp")
# 인터페이스 단위 확장자 (export module 이 들어 있는 파일)
INTERFACE_EXTENSIONS = (".ixx", ".cppm")
# 스캔에서 제외할 폴더 이름 (빌드 산출물, 그래프 출력)
EXCLUDED_DIRS = {"x64", "Debug", "Release", "obj", "bin", "graphify-out", ".git", ".vs"}
# 이 스크립트가 만든 노드·엣지를 구분하는 표시값 (재실행 시 이것만 지움)
ORIGIN_TAG = "module_scan"
# 그래프에 넣을 엣지 관계 이름
RELATION_EXPORTS = "exports_module"
RELATION_IMPLEMENTS = "implements_module"
RELATION_IMPORTS = "imports_module"
RELATION_DEPENDS = "depends_on_module"
# 모듈 노드 id 접두사
MODULE_ID_PREFIX = "module_"
# 표준 라이브러리 모듈 (프로젝트 의존성이 아니므로 제외)
STANDARD_MODULES = {"std", "std.compat"}
# 소스에서 직접 읽은 관계이므로 확신도는 최대
EXTRACTED_CONFIDENCE = 1.0
# 엣지 기본 가중치
DEFAULT_WEIGHT = 1.0
# 파일 노드를 못 찾았을 때 다시 시도하지 않고 건너뛴 개수를 보여 줄 최대 예시 수
MAX_MISSING_EXAMPLES = 5

# 블록 주석 /* ... */ 와 줄 주석 // ... 제거용 패턴
BLOCK_COMMENT_PATTERN = re.compile(r"/\*.*?\*/", re.S)
LINE_COMMENT_PATTERN = re.compile(r"//[^\n]*")
# `export module A.B;` 또는 `export module A.B:Part;` (인터페이스 / 파티션 인터페이스)
EXPORT_MODULE_PATTERN = re.compile(r"^\s*export\s+module\s+([\w.]+(?::[\w.]+)?)\s*;", re.M)
# `module A.B;` 또는 `module A.B:Part;` (구현 단위). `module;` (전역 모듈 조각)은 이름이 없어서 제외됨
MODULE_PATTERN = re.compile(r"^\s*module\s+([\w.]+(?::[\w.]+)?)\s*;", re.M)
# `import A.B;` / `export import A.B;` / `import :Part;` (헤더 유닛 `import <x>;` 는 제외)
IMPORT_PATTERN = re.compile(r"^\s*(?:export\s+)?import\s+([\w.]*(?::[\w.]+)?)\s*;", re.M)


# ──────────────────────────── 소스 분석 ────────────────────────────

def strip_comments(text: str) -> str:
    """주석 안의 module/import 문을 잘못 읽지 않도록 주석을 지운다."""
    # 블록 주석 먼저, 그다음 줄 주석
    return LINE_COMMENT_PATTERN.sub("", BLOCK_COMMENT_PATTERN.sub("", text))


def primary_name(module_name: str) -> str:
    """파티션 이름(A.B:Part)에서 주 모듈 이름(A.B)만 돌려준다."""
    return module_name.split(":", 1)[0]


def resolve_import(imported: str, own_module: str | None) -> str | None:
    """import 대상 이름을 완전한 모듈 이름으로 바꾼다. 해석할 수 없으면 None."""
    # `import :Part;` 는 자기 모듈의 파티션
    if imported.startswith(":"):
        return f"{primary_name(own_module)}{imported}" if own_module else None
    # 빈 이름은 무시
    return imported or None


def iter_sources(root: Path):
    """제외 폴더를 건너뛰며 모듈 문법을 찾을 소스 파일을 나열한다."""
    for path in sorted(root.rglob("*")):
        # 파일이 아니거나 확장자가 대상이 아니면 건너뜀
        if not path.is_file() or path.suffix.lower() not in SOURCE_EXTENSIONS:
            continue
        # 제외 폴더 아래에 있으면 건너뜀
        if any(part in EXCLUDED_DIRS for part in path.relative_to(root).parts):
            continue
        yield path


def scan_sources(root: Path) -> list[dict]:
    """각 소스 파일의 모듈 선언과 import 목록을 읽는다."""
    units = []
    for path in iter_sources(root):
        # BOM 이 있어도 읽히도록 utf-8-sig, 깨진 바이트는 대체
        text = strip_comments(path.read_text(encoding="utf-8-sig", errors="replace"))
        exported = EXPORT_MODULE_PATTERN.search(text)
        implemented = None if exported else MODULE_PATTERN.search(text)
        own_module = (exported or implemented).group(1) if (exported or implemented) else None
        # import 대상 해석 (표준 모듈 제외, 중복 제거, 순서 유지)
        imports = []
        for match in IMPORT_PATTERN.finditer(text):
            target = resolve_import(match.group(1), own_module)
            if target and target not in STANDARD_MODULES and target not in imports:
                imports.append(target)
        units.append({
            # 그래프의 source_file 과 같은 형식 (루트 기준 상대 경로, / 구분자)
            "file": path.relative_to(root).as_posix(),
            "exports": exported.group(1) if exported else None,
            "implements": implemented.group(1) if implemented else None,
            "imports": imports,
        })
    return units


# ──────────────────────────── 그래프 갱신 ────────────────────────────

def module_node_id(module_name: str) -> str:
    """모듈 이름을 graphify 노드 id 규칙(소문자, 단어 문자만)에 맞게 바꾼다."""
    return MODULE_ID_PREFIX + re.sub(r"\W", "_", module_name.lower())


def find_file_nodes(nodes: list[dict]) -> dict[str, dict]:
    """source_file → 파일 노드 (라벨이 파일 이름인 노드) 대응표를 만든다."""
    file_nodes = {}
    for node in nodes:
        source = str(node.get("source_file") or "").replace("\\", "/")
        # 파일 노드는 라벨이 파일 이름과 같다
        if source and node.get("label") == Path(source).name:
            file_nodes[source] = node
    return file_nodes


def make_edge(source: str, target: str, relation: str, source_file: str) -> dict:
    """graphify 엣지 형식에 맞는 엣지 하나를 만든다."""
    return {
        "source": source,
        "target": target,
        "relation": relation,
        "_origin": ORIGIN_TAG,
        "confidence": "EXTRACTED",
        "confidence_score": EXTRACTED_CONFIDENCE,
        "source_file": source_file,
        "source_location": None,
        "weight": DEFAULT_WEIGHT,
    }


def apply_module_edges(graph: dict, units: list[dict]) -> dict:
    """그래프에서 이전 결과를 지우고 모듈 노드·엣지를 다시 넣는다. 통계를 돌려준다."""
    # 이전 실행 결과 제거 (재실행해도 같은 결과)
    graph["nodes"] = [n for n in graph["nodes"] if n.get("_origin") != ORIGIN_TAG]
    graph["links"] = [e for e in graph["links"] if e.get("_origin") != ORIGIN_TAG]

    file_nodes = find_file_nodes(graph["nodes"])
    # 모듈 이름 → 그 모듈을 export 하는 파일
    interface_of = {u["exports"]: u["file"] for u in units if u["exports"]}
    # 모듈 노드가 필요한 모든 이름 (export, 구현, import 대상)
    all_modules = set(interface_of)
    for unit in units:
        all_modules.update(filter(None, [unit["implements"], *unit["imports"]]))

    # 모듈 노드 추가 (커뮤니티는 인터페이스 파일 노드의 것을 따름)
    for name in sorted(all_modules):
        owner = file_nodes.get(interface_of.get(name, ""), {})
        graph["nodes"].append({
            "id": module_node_id(name),
            "label": name,
            "file_type": "code",
            "norm_label": name.lower(),
            "source_file": interface_of.get(name),
            "source_location": "L1" if name in interface_of else None,
            "community": owner.get("community"),
            "community_name": owner.get("community_name"),
            "_origin": ORIGIN_TAG,
        })

    stats = Counter()
    missing = []
    for unit in units:
        file_node = file_nodes.get(unit["file"])
        # 그래프에 파일 노드가 없으면 (graphify 가 제외한 파일) 건너뜀
        if file_node is None:
            if unit["exports"] or unit["implements"] or unit["imports"]:
                missing.append(unit["file"])
            continue
        source_id = file_node["id"]
        # 인터페이스 → 모듈
        if unit["exports"]:
            graph["links"].append(make_edge(source_id, module_node_id(unit["exports"]), RELATION_EXPORTS, unit["file"]))
            stats[RELATION_EXPORTS] += 1
        # 구현 단위 → 모듈
        if unit["implements"]:
            graph["links"].append(make_edge(source_id, module_node_id(unit["implements"]), RELATION_IMPLEMENTS, unit["file"]))
            stats[RELATION_IMPLEMENTS] += 1
        # import 하는 파일 → 대상 모듈
        for target in unit["imports"]:
            graph["links"].append(make_edge(source_id, module_node_id(target), RELATION_IMPORTS, unit["file"]))
            stats[RELATION_IMPORTS] += 1

    # 모듈 → 모듈 의존 엣지 (`graphify path 모듈A 모듈B` 가 방향대로 따라갈 수 있게)
    # 인터페이스와 구현 단위의 import 를 합쳐서, 같은 쌍은 한 번만 넣는다
    dependency_pairs = set()
    for unit in units:
        own_module = unit["exports"] or unit["implements"]
        if not own_module:
            continue
        for target in unit["imports"]:
            # 자기 자신의 파티션을 import 하는 경우는 의존이 아님
            if primary_name(target) != primary_name(own_module):
                dependency_pairs.add((own_module, target, unit["file"]))
    seen_pairs = set()
    for own_module, target, source_file in sorted(dependency_pairs):
        if (own_module, target) in seen_pairs:
            continue
        seen_pairs.add((own_module, target))
        graph["links"].append(make_edge(module_node_id(own_module), module_node_id(target), RELATION_DEPENDS, source_file))
        stats[RELATION_DEPENDS] += 1

    stats["modules"] = len(all_modules)
    stats["external_modules"] = len(all_modules - set(interface_of))
    return {"stats": stats, "missing": missing}


# ──────────────────────────── 실행 ────────────────────────────

def run() -> int:
    """명령행 인자를 읽고 그래프를 갱신한다."""
    parser = argparse.ArgumentParser(description="graphify 그래프에 C++23 모듈 의존 엣지를 추가")
    parser.add_argument("--no-html", action="store_true", help="graph.html 을 다시 만들지 않음")
    args = parser.parse_args()

    # 그래프가 없으면 먼저 /graphify 를 돌려야 함
    if not GRAPH_PATH.exists():
        print(f"그래프가 없어: {GRAPH_PATH} (먼저 /graphify . 실행)")
        return 1

    graph = json.loads(GRAPH_PATH.read_text(encoding="utf-8"))
    units = scan_sources(PROJECT_ROOT)
    result = apply_module_edges(graph, units)
    GRAPH_PATH.write_text(json.dumps(graph, ensure_ascii=False, indent=2), encoding="utf-8")

    stats = result["stats"]
    print(f"모듈 {stats['modules']}개 (외부 {stats['external_modules']}개) | "
          f"export {stats[RELATION_EXPORTS]} · 구현 {stats[RELATION_IMPLEMENTS]} · import {stats[RELATION_IMPORTS]} · "
          f"모듈 의존 {stats[RELATION_DEPENDS]}")
    # 그래프에 없는 파일은 엣지를 못 붙였으므로 알려 줌
    if result["missing"]:
        print(f"그래프에 파일 노드가 없어 건너뜀 {len(result['missing'])}개: {result['missing'][:MAX_MISSING_EXAMPLES]}")

    # 시각화 다시 생성 (graphify CLI 가 graph.json 을 읽음)
    if not args.no_html:
        subprocess.run(["graphify", "export", "html"], cwd=PROJECT_ROOT, check=False)
    return 0


if __name__ == "__main__":
    sys.exit(run())
