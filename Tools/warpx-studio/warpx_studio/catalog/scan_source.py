#!/usr/bin/env python3
"""Scan this fork's ``Source/`` for input-parameter read sites and their gating.

Why the source and not the documentation: several inputs of this fork are only
read under a condition (a switch being on, a solver being selected, a list being
non-empty).  The documentation prose says so in English, the code says so in
C++, and the code is what the simulation obeys.

For every ``ParmParse`` read the scanner records

* the parameter name (the first string literal of the call),
* the variable it is read into (for a later dataflow pass),
* the enclosing ``if``/``else if``/``#if`` conditions (brace-aware, no compiler),
* the assertion messages that follow it, which often spell out the interaction
  with another switch ("can only be done if warpx.do_pml_in_domain = 1").

Known limitation (documented, not guessed): a condition that depends on a value
computed in another function, or hidden behind a macro, is reported as ``None``
rather than being invented.  The catalog keeps those parameters in the "all"
section.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
SRC = REPO / "Source"
OUT = Path(__file__).resolve().parent / "data/param_sites.json"

SECTION_OBJ = re.compile(
    r"ParmParse\s+([A-Za-z_]\w*)\s*\(\s*\"([^\"]+)\"\s*\)")
# A section built from a runtime value (`ParmParse pp_species_name(species_name)`) is a
# repeatable group: the key belongs to whichever species/collision/diagnostic the user
# declares.  Those become templates such as `<species_name>.charge`.
SECTION_VAR = re.compile(r"ParmParse\s+([A-Za-z_]\w*)\s*\(\s*([A-Za-z_]\w*)\s*\)")
CALL = re.compile(
    r"\b(?:queryWithParser|pquery|pp[A-Za-z_]*\.(?:query|get|contains)"
    r"|[A-Za-z_][\w:]*\.(?:query|get|contains))\s*\(")
STRING = re.compile(r'"([^"\\\n]{1,120})"')
ASSERT = re.compile(
    r"(?:WARPX_ALWAYS_ASSERT_WITH_MESSAGE|abort_with_message|AMREX_ALWAYS_ASSERT_WITH_MESSAGE)"
    r"\s*\(")
GATE = re.compile(r"^\s*(?:else\s+)?if\s*(?:constexpr\s*)?\((.*)\)\s*(?:const)?\s*$", re.S)
PREPROC = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")


def strip_comments(text: str) -> str:
    """Blank out comments, keeping every byte offset and line number intact."""
    out = list(text)
    i, n = 0, len(text)
    state = None  # None | 'line' | 'block' | 'str' | 'chr'
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state is None:
            if ch == "/" and nxt == "/":
                state = "line"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = "block"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if ch == '"':
                state = "str"
            elif ch == "'":
                state = "chr"
            i += 1
            continue
        if state == "line":
            if ch == "\n":
                state = None
            else:
                out[i] = " "
            i += 1
            continue
        if state == "block":
            if ch == "*" and nxt == "/":
                out[i] = out[i + 1] = " "
                state = None
                i += 2
                continue
            if ch != "\n":
                out[i] = " "
            i += 1
            continue
        # inside a string/char literal: keep the content, watch for the closing quote
        if ch == "\\":
            i += 2
            continue
        if (state == "str" and ch == '"') or (state == "chr" and ch == "'"):
            state = None
        i += 1
    return "".join(out)


def blocks(text: str) -> list[tuple[int, int, str, str]]:
    """Return [(start, end, kind, context)]; kind is ``brace`` or ``pp``."""
    found: list[tuple[int, int, str, str]] = []
    stack: list[tuple[int, str]] = []
    i, n = 0, len(text)
    while i < n:
        ch = text[i]
        if ch == "{":
            start = i - 1
            while start >= 0 and text[start] not in ";{}":
                start -= 1
            header = text[start + 1:i].strip()
            stack.append((i, header))
        elif ch == "}":
            if stack:
                open_pos, header = stack.pop()
                found.append((open_pos, i, "brace", header))
        i += 1
    # header guards look like `#ifndef NAME` + `#define NAME` and gate nothing
    guards = set(re.findall(r"^\s*#\s*ifndef\s+(\w+)\s*\n\s*#\s*define\s+\1\b",
                            text, re.M))

    # preprocessor regions, line based
    lines = text.split("\n")
    offset = 0
    pp_stack: list[tuple[int, str]] = []
    for line in lines:
        match = PREPROC.match(line)
        if match:
            kind, rest = match.group(1), match.group(2).strip()
            if kind == "ifndef" and rest in guards:
                offset += len(line) + 1
                continue
            if kind in ("if", "ifdef", "ifndef"):
                pp_stack.append((offset, f"#{kind} {rest}"))
            elif kind in ("endif",) and pp_stack:
                open_pos, header = pp_stack.pop()
                found.append((open_pos, offset + len(line), "pp", header))
        offset += len(line) + 1
    return found


def enclosing_conditions(found, pos: int) -> list[str]:
    """Conditions whose block contains ``pos``, outermost first."""
    chain = []
    for start, end, kind, header in found:
        if not (start < pos < end):
            continue
        if kind == "pp":
            # Only a real #if/#ifdef gates anything; #include and header guards are noise.
            if header.startswith(("#if ", "#ifdef ", "#ifndef ")):
                chain.append((start, header.strip()))
            continue
        match = GATE.match(header)
        if match:
            condition = re.sub(r"\s+", " ", match.group(1)).strip()
            if condition:
                chain.append((start, f"if ({condition})"))
    chain.sort(key=lambda item: item[0])
    return [text for _, text in chain]


def section_prefixes(text: str) -> dict[str, str]:
    """Map a ParmParse variable name to its section prefix ("pp_psatd" -> "psatd")."""
    mapping = {name: prefix.strip(".") for name, prefix in SECTION_OBJ.findall(text)}
    for name, variable in SECTION_VAR.findall(text):
        mapping.setdefault(name, canonical_group(variable))
    return mapping


def receiver(text: str, call_start: int, args_start: int) -> str | None:
    """Return the ParmParse object a read goes through ("pp_psatd" in a.b.query)."""
    direct = re.match(r"([A-Za-z_][\w:]*)\s*\.", text[call_start:])
    if direct:
        return direct.group(1)
    before = text[max(0, call_start - 80):call_start]
    match = re.search(r"([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)\s*\.\s*$", before)
    if match:
        return match.group(1)
    first = text[args_start + 1:args_start + 120].split(",", 1)[0].strip()
    return first if re.fullmatch(r"[A-Za-z_]\w*", first) else None


def canonical_group(variable: str) -> str:
    """Map a runtime section variable to the placeholder the documentation uses.

    ``ParmParse pp_species_name(species_name)``, ``pp_x(species)`` and
    ``pp_coll_name(coll_name)`` all describe the same logical group, so they get the
    same placeholder: the UI shows one card per species, not one card per C++ variable.
    """
    name = variable.lower()
    if "species" in name:
        return "<species_name>"
    if "coll" in name:
        return "<collision_name>"
    if "laser" in name:
        return "<laser_name>"
    if "reduced" in name or name in ("rd_name", "rd"):
        return "<reduced_diags_name>"
    if "diag" in name:
        return "<diag_name>"
    if "solver" in name or "prefix" in name or name in ("a_name", "m_name"):
        return "<solver_name>"
    return "<dynamic>"


def read_site(text: str, open_paren: int) -> tuple[str | None, str | None, bool]:
    """Return (name, target variable, is_suffix) for the call at ``open_paren``.

    ``is_suffix`` marks keys built by concatenation (``section + ".field"``), whose full
    name depends on a runtime value.
    """
    depth = 0
    i = open_paren
    args_end = len(text)
    while i < len(text):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                args_end = i
                break
        i += 1
    args = text[open_paren + 1:args_end]
    match = STRING.search(args)
    if not match:
        return None, None, False
    name = match.group(1)
    if not re.fullmatch(r"[A-Za-z0-9_./<>\-]+", name):
        return None, None, False
    is_suffix = name.startswith(".") or bool(re.search(r"\+\s*$", args[:match.start()]))
    tail = args[match.end():]
    parts = [part.strip() for part in tail.split(",") if part.strip()]
    target = parts[0] if parts else None
    if target and not re.fullmatch(r"[A-Za-z_][\w:\[\]\.>\-]*", target):
        target = None
    return name, target, is_suffix


def scan_file(path: Path) -> list[dict]:
    raw = path.read_text(errors="ignore")
    text = strip_comments(raw)
    found = blocks(text)
    prefixes = section_prefixes(text)
    sites = []
    for match in CALL.finditer(text):
        name, target, is_suffix = read_site(text, match.end() - 1)
        if not name:
            continue
        obj = receiver(text, match.start(), match.end() - 1)
        prefix = prefixes.get((obj or "").split(".")[0]) or None
        if is_suffix:
            # ``pp.contains(section_name + ".field")``: the section is a runtime value, so the
            # full key cannot be known statically.  Keep it out of the plain key space instead
            # of pretending it is a top-level parameter.
            name = name.lstrip(".")
            prefix = prefix or "<dynamic>"
            full = f"{prefix}.{name}"
        else:
            full = f"{prefix}.{name}" if prefix else name
        line = text.count("\n", 0, match.start()) + 1
        near = text[match.end():match.end() + 2000]
        asserts = [re.sub(r"\s+", " ", m.group(0)).strip()[:300]
                   for m in re.finditer(r'"([^"]{10,300})"', near)][:2]
        sites.append({
            "param": full,
            "raw_key": name,
            "dynamic_prefix": bool(is_suffix),
            "section": prefix,
            "object": obj,
            "file": str(path.relative_to(REPO)),
            "line": line,
            "variable": target,
            "conditions": enclosing_conditions(found, match.start()),
            "nearby_strings": asserts,
        })
    return sites


def main() -> int:
    if not SRC.is_dir():
        print(f"missing {SRC}", file=sys.stderr)
        return 2
    files = sorted(p for p in SRC.rglob("*") if p.suffix in (".cpp", ".H", ".hpp", ".h"))
    sites: list[dict] = []
    for path in files:
        try:
            sites.extend(scan_file(path))
        except OSError:
            continue
    gated = [s for s in sites if s["conditions"]]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(
        {"source": "Source/**", "files": len(files), "sites": sites},
        indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    params = {s["param"] for s in sites}
    print(f"scanned {len(files)} files: {len(sites)} read sites, {len(params)} distinct keys, "
          f"{len(gated)} sites under a condition -> {OUT.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
