#!/usr/bin/env python3
"""Parse ``Docs/source/usage/parameters.rst`` into a list of input parameters.

The fork documents its inputs with the custom Sphinx directive ``pp:param``::

    .. pp:param:: warpx.do_pml_in_domain
        :type: ``int``
        :default: 0

        Whether to create the PML inside the simulation area ...

Only the documented subset is reachable this way; parameters that exist only in
the source (this fork adds several, e.g. under ``Source/Plugins``) are collected
by :mod:`scan_source`.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
DOCS = REPO / "Docs/source/usage/parameters.rst"
OUT = Path(__file__).resolve().parent / "data/params_docs.json"

DIRECTIVE = re.compile(r"^\.\. pp:param::\s*(\S+)\s*$")
OPTION = re.compile(r"^:([a-z_]+):\s*(.*)$")
SECTION = re.compile(r"^([A-Za-z].*)\n-{3,}$", re.M)
HEADING = re.compile(r"^(-{3,}|={3,}|\.\. _[^\n]*)$")


def _clean(text: str) -> str:
    """Collapse the reStructuredText roles/escapes that would leak into the UI."""
    text = re.sub(r":pp:param:`([^`<]*?)\s*<([^`]+)>`", r"\1 (\2)", text)
    text = re.sub(r":pp:param:`([^`]+)`", r"\1", text)
    text = re.sub(r":ref:`([^`<]*?)\s*<([^`]+)>`", r"\1", text)
    text = re.sub(r"``([^`]+)``", r"\1", text)
    text = re.sub(r":math:`([^`]+)`", r"\1", text)
    text = re.sub(r"\s+", " ", text)
    return text.strip()


def parse_sections(text: str) -> list[tuple[int, str]]:
    """Return [(line_index, section_title)] for the ``-----`` headings."""
    out = []
    lines = text.split("\n")
    for i in range(1, len(lines)):
        if re.fullmatch(r"-{3,}", lines[i]) and lines[i - 1].strip():
            out.append((i, lines[i - 1].strip()))
    return out


def parse(text: str) -> list[dict]:
    lines = text.split("\n")
    sections = parse_sections(text)
    params: list[dict] = []
    i = 0
    while i < len(lines):
        match = DIRECTIVE.match(lines[i])
        if not match:
            i += 1
            continue
        name = match.group(1)
        entry = {"name": name, "type": None, "default": None,
                 "optional": False, "doc": "", "section": None, "docs_line": i + 1}
        section = [title for (li, title) in sections if li < i]
        entry["section"] = section[-1] if section else None
        i += 1
        body: list[str] = []
        while i < len(lines):
            line = lines[i]
            if DIRECTIVE.match(line):
                break
            if HEADING.match(line) or line.startswith(".. _"):
                break
            stripped = line.strip()
            if not stripped:
                body.append("")
                i += 1
                continue
            if not line.startswith(" "):
                break  # dedented text belongs to the section, not the parameter
            option = OPTION.match(stripped)
            if option and not body:
                key, value = option.group(1), option.group(2)
                if key == "type":
                    entry["type"] = _clean(value)
                elif key == "default":
                    entry["default"] = _clean(value)
                elif key == "optional":
                    entry["optional"] = True
            elif option and body:
                # malformed: option after the description started
                body.append(stripped)
            else:
                body.append(stripped)
            i += 1
        entry["doc"] = _clean(" ".join(part for part in body if part))
        params.append(entry)
    return params


def main() -> int:
    if not DOCS.exists():
        print(f"missing {DOCS}", file=sys.stderr)
        return 2
    params = parse(DOCS.read_text(encoding="utf-8"))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(
        {"source": str(DOCS.relative_to(REPO)), "count": len(params), "params": params},
        indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    by_section: dict[str, int] = {}
    for p in params:
        by_section[p["section"] or "(none)"] = by_section.get(p["section"] or "(none)", 0) + 1
    print(f"parsed {len(params)} documented parameters -> {OUT.relative_to(REPO)}")
    for title, count in sorted(by_section.items(), key=lambda kv: -kv[1])[:8]:
        print(f"  {count:4d}  {title}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
