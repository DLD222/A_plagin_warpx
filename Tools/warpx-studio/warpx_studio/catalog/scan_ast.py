#!/usr/bin/env python3
"""Extract parameter reads and their gating with libclang (v2 replacement for regex).

Why the change: the regex scanner sees a call and its enclosing ``if`` text, but not a
``switch``/``case`` chain, not a macro that expands to a call, and not a condition that
uses a value read earlier in the same function.  Clang gives all three for free.

The flags come from an existing build's ``flags.make`` plus the GCC system include
directories, so no ``compile_commands.json`` is needed.  Parsing a WarpX translation unit
this way leaves a handful of benign diagnostics (glibc ``__malloc__`` attributes); the AST
is still complete for the functions that call ParmParse.

Output has the same shape as ``scan_source.py`` (so the catalog builder does not care
which scanner produced it) plus ``switch_cases`` and ``method``.
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

from clang import cindex as ci

from scan_source import CALL, canonical_group, section_prefixes, strip_comments

REPO = Path(__file__).resolve().parents[4]
SRC = REPO / "Source"
OUT = Path(__file__).resolve().parent / "data/param_sites_ast.json"
FLAGS = REPO / "audit_build/fork/CMakeFiles/warpx_picem_2d.dir/flags.make"
READS = {"query", "get", "pquery", "queryWithParser", "getWithParser", "contains"}


def compiler_args() -> list[str]:
    text = FLAGS.read_text()
    def grab(name: str) -> list[str]:
        match = re.search(rf"^{name} = (.*)$", text, re.M)
        return match.group(1).split() if match else []
    probe = subprocess.run(["g++", "-xc++", "-E", "-v", "-"], input="",
                           capture_output=True, text=True)
    gcc_includes = [p for p in re.findall(r"^\s+(/\S+)$", probe.stderr, re.M)
                    if "include" in p]
    return (grab("CXX_DEFINES") + grab("CXX_INCLUDES")
            + ["-std=c++20", "-fsyntax-only", "-fopenmp"]
            + [f"-isystem{p}" for p in gcc_includes])


def source_text(node) -> str:
    try:
        return " ".join(node.extent.start.source_line_content() for _ in [0]) if False else \
            " ".join(part.strip() for part in
                     node.extent.start.file and
                     _read_extent(node) or [])
    except Exception:
        return ""


def _read_extent(node) -> list[str]:
    extent = node.extent
    try:
        text = extent.start.file.name and Path(extent.start.file.name).read_text(errors="ignore")
    except Exception:
        return []
    if not text:
        return []
    lines = text.splitlines()
    if extent.start.line == extent.end.line:
        return [lines[extent.start.line - 1][extent.start.column - 1:extent.end.column - 1]]
    out = [lines[extent.start.line - 1][extent.start.column - 1:]]
    out += lines[extent.start.line:extent.end.line - 1]
    out.append(lines[extent.end.line - 1][:extent.end.column - 1])
    return out


def string_literals(node) -> list[str]:
    """String literals among a call's arguments.

    Clang wraps argument expressions (``UNEXPOSED_EXPR``, ``CXXConstructExpr``), so the
    literal is not a direct child of the ``CALL_EXPR``; the walk descends through the
    wrappers but not into nested calls.
    """
    out: list[str] = []
    stack = list(node.get_children())
    while stack:
        child = stack.pop(0)
        if child.kind == ci.CursorKind.STRING_LITERAL:
            out.append(child.spelling.strip('"'))
        elif child.kind != ci.CursorKind.CALL_EXPR:
            stack.extend(child.get_children())
    return out


def enclosing(chain) -> tuple[list[str], list[str], bool]:
    """Return (conditions, switch cases, unconditional) for a node given its ancestors."""
    conditions: list[str] = []
    cases: list[str] = []
    conditional = False
    for ancestor in chain:
        if ancestor.kind == ci.CursorKind.IF_STMT:
            children = list(ancestor.get_children())
            if children:
                text = " ".join(_read_extent(children[0])).strip()
                conditions.append(f"if ({' '.join(text.split())})")
                conditional = True
        elif ancestor.kind == ci.CursorKind.SWITCH_STMT:
            children = list(ancestor.get_children())
            if children:
                text = " ".join(_read_extent(children[0])).strip()
                conditions.append(f"switch ({' '.join(text.split())})")
                conditional = True
        elif ancestor.kind == ci.CursorKind.CASE_STMT:
            children = list(ancestor.get_children())
            if children:
                cases.append(" ".join(_read_extent(children[0])).strip())
        elif ancestor.kind in (ci.CursorKind.FOR_STMT, ci.CursorKind.WHILE_STMT,
                               ci.CursorKind.DO_STMT, ci.CursorKind.CONDITIONAL_OPERATOR):
            conditional = True
    return conditions, cases, not conditional


def enum_options(cursor) -> dict[str, list[str]]:
    """``var == "literal"`` / ``"literal" == var`` pairs anywhere in the file."""
    options: dict[str, set[str]] = {}
    stack = [cursor]
    while stack:
        node = stack.pop()
        if node.kind == ci.CursorKind.BINARY_OPERATOR:
            children = list(node.get_children())
            if len(children) == 2:
                left, right = children
                var = literal = None
                if left.kind == ci.CursorKind.DECL_REF_EXPR and right.kind == ci.CursorKind.STRING_LITERAL:
                    var, literal = left.spelling, right.spelling.strip('"')
                elif right.kind == ci.CursorKind.DECL_REF_EXPR and left.kind == ci.CursorKind.STRING_LITERAL:
                    var, literal = right.spelling, left.spelling.strip('"')
                if var and literal and 0 < len(literal) <= 30 and " " not in literal:
                    options.setdefault(var, set()).add(literal)
        stack.extend(node.get_children())
    return {k: sorted(v) for k, v in options.items()}


def scan_file(index, path: Path, args: list[str]) -> list[dict]:
    raw = path.read_text(errors="ignore")
    # Prefilter: a file that never mentions ParmParse cannot contain a parameter read.
    # (`.get(` alone matches far too much: std::get, map.get, ...)
    if "ParmParse" not in raw:
        return []
    if not CALL.search(strip_comments(raw)):
        return []
    tu = index.parse(str(path), args=args,
                     options=ci.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    options = enum_options(tu.cursor)
    # Prefix resolution (which ParmParse object belongs to which section) is a textual
    # question and the v1 scanner already answers it correctly; the AST is used for what
    # text cannot express: switch/case gating and "is this read unconditional".
    prefixes: dict[str, str] = section_prefixes(raw)

    def declarations(cursor, chain):
        """Collect `ParmParse pp_x("section"|variable)` declarations."""
        if cursor.kind == ci.CursorKind.VAR_DECL:
            text = " ".join(_read_extent(cursor))
            match = re.search(r'ParmParse\s+(\w+)\s*\(\s*"([^"]+)"\s*\)', text)
            if match:
                prefixes[match.group(1)] = match.group(2).strip(".")
            else:
                match = re.search(r"ParmParse\s+(\w+)\s*\(\s*([A-Za-z_]\w*)\s*\)", text)
                if match:
                    prefixes[match.group(1)] = canonical_group(match.group(2))
        for child in cursor.get_children():
            declarations(child, chain + [cursor])

    declarations(tu.cursor, [])

    sites: list[dict] = []
    stack = [(tu.cursor, [])]
    while stack:
        node, chain = stack.pop()
        if (node.kind == ci.CursorKind.CALL_EXPR and node.spelling in READS
                and node.location.file and node.location.file.name == str(path)):
            literals = string_literals(node)
            if literals:
                name = literals[0]
                text = " ".join(_read_extent(node))
                children = list(node.get_children())
                # `pp_x.query("k", v)` -> receiver is the object before the dot;
                # `utils::parser::query(pp_x, section, "k", v)` -> it is the first argument.
                match = re.match(r"([A-Za-z_]\w*)\s*\.", text)
                receiver = match.group(1) if match else None
                if receiver is None:
                    # children[0] is the callee wrapper (UNEXPOSED_EXPR); the ParmParse
                    # object is the first real argument.
                    for child in children:
                        if child.kind == ci.CursorKind.UNEXPOSED_EXPR:
                            continue
                        candidate = " ".join(_read_extent(child)).strip()
                        if candidate in prefixes:
                            receiver = candidate
                            break
                prefix = prefixes.get(receiver) if receiver else None
                target = None
                if len(children) > 1:
                    last = " ".join(_read_extent(children[-1])).strip()
                    if re.fullmatch(r"[A-Za-z_]\w*", last):
                        target = last
                conditions, cases, unconditional = enclosing(chain)
                required = node.spelling in ("get", "getWithParser") and unconditional
                sites.append({
                    "param": f"{prefix}.{name}" if prefix else name,
                    "raw_key": name,
                    "section": prefix,
                    "object": receiver,
                    "file": str(path.relative_to(REPO)),
                    "line": node.location.line,
                    "variable": target,
                    "method": node.spelling,
                    "required": required,
                    "conditions": conditions,
                    "switch_cases": cases,
                    "options": options.get(target or "", []),
                    "scanner": "clang",
                })
        for child in node.get_children():
            stack.append((child, chain + [node]))
    return sites


def main() -> int:
    if not FLAGS.is_file():
        print(f"missing {FLAGS.relative_to(REPO)} (need a configured build)", file=sys.stderr)
        return 2
    args = compiler_args()
    index = ci.Index.create()
    sites: list[dict] = []
    files = sorted(p for p in SRC.rglob("*") if p.suffix in (".cpp", ".H"))
    candidates = []
    for path in files:
        try:
            raw = path.read_text(errors="ignore")
        except OSError:
            continue
        if "ParmParse" in raw and CALL.search(strip_comments(raw)):
            candidates.append(path)
    print(f"prefilter: {len(candidates)} of {len(files)} files mention a ParmParse read",
          file=sys.stderr)
    for number, path in enumerate(candidates, 1):
        try:
            sites.extend(scan_file(index, path, args))
            if number % 10 == 0:
                print(f"  {number}/{len(candidates)} parsed", file=sys.stderr)
        except Exception as exc:                      # one bad TU must not stop the scan
            print(f"  skip {path.relative_to(REPO)}: {type(exc).__name__}: {exc}", file=sys.stderr)
    OUT.write_text(json.dumps({"source": "Source/**", "scanner": "libclang",
                               "files": len(files), "sites": sites},
                              indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    gated = [s for s in sites if s["conditions"]]
    switches = [s for s in sites if s["switch_cases"]]
    print(f"AST scan: {len(files)} files, {len(sites)} read sites, "
          f"{len({s['param'] for s in sites})} keys, {len(gated)} gated, "
          f"{len(switches)} inside switch/case, "
          f"{sum(1 for s in sites if s['required'])} required -> {OUT.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
