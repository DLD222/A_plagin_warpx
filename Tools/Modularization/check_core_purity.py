#!/usr/bin/env python3
"""Keep concrete WarpX particle types out of the core (D1-a ratchet).

The core must stay physics-agnostic: no concrete particle container, no
concrete tile iterator, no WarpX singleton.  The remaining references are
listed in core_concrete_allowlist.txt with the reason; this checker fails when
a new one appears, so the number can only go down.

A reference inside a comment does not count.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
CORE = ROOT / "Source" / "Core"
ALLOWLIST = ROOT / "Tools" / "Modularization" / "core_concrete_allowlist.txt"

TOKENS = [
    "WarpXParticleContainer",
    "MultiParticleContainer",
    "PhysicalParticleContainer",
    "LaserParticleContainer",
    "WarpXParIter",
    "WarpX::",
]

def strip_comments_and_strings(text):
    """Blank out // and /* */ comments and string/char literals, keeping line structure.

    The previous version only skipped lines whose *first* non-blank character was a comment
    marker: a trailing ``// see WarpX::Evolve`` was reported as a violation, and a block-comment
    continuation line that did not start with ``*`` was not skipped at all.
    """
    out = []
    in_block = False
    i = 0
    n = len(text)
    while i < n:
        two = text[i:i + 2]
        if in_block:
            if two == "*/":
                in_block = False
                out.append("  ")
                i += 2
            else:
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            continue
        if two == "/*":
            in_block = True
            out.append("  ")
            i += 2
            continue
        if two == "//":
            while i < n and text[i] != "\n":
                out.append(" ")
                i += 1
            continue
        if text[i] in "\"'":
            quote = text[i]
            out.append(" ")
            i += 1
            while i < n and text[i] != quote:
                out.append("\n" if text[i] == "\n" else " ")
                if text[i] == "\\":
                    i += 1
                    if i < n:
                        out.append(" ")
                i += 1
            if i < n:
                out.append(" ")
                i += 1
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


ALIAS_PATTERNS = [
    # namespace W = WarpX;  -> every later W:: is a WarpX:: reference
    re.compile(r"\bnamespace\s+(\w+)\s*=\s*WarpX\s*;"),
    # using PC = PhysicalParticleContainer; -> later PC is a concrete type
    re.compile(r"\busing\s+(\w+)\s*=\s*(\w+)\s*;"),
]

allowed = set()
if ALLOWLIST.exists():
    for line in ALLOWLIST.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        allowed.add(line)

errors = []
stale = []
found = set()
for path in sorted(CORE.rglob("*")):
    if path.suffix not in (".H", ".h", ".cpp", ".cxx"):
        continue
    rel = str(path.relative_to(ROOT))
    text = strip_comments_and_strings(path.read_text(errors="ignore"))
    for lineno, line in enumerate(text.splitlines(), 1):
        tokens = list(TOKENS)
        # A namespace alias of the singleton is the same reference as the literal token, and a
        # `using X = <concrete type>` makes every later X a concrete-type reference; neither
        # contained the literal token, so both used to pass silently.
        for m in ALIAS_PATTERNS[0].finditer(text):
            tokens.append(m.group(1) + "::")
        for m in ALIAS_PATTERNS[1].finditer(text):
            if m.group(2) in TOKENS:
                tokens.append(m.group(1))
        for token in tokens:
            pattern = (re.escape(token) if token.endswith("::")
                       else r"\b" + re.escape(token) + r"\b")
            if re.search(pattern, line):
                entry = f"{rel}|{token}"
                found.add(entry)
                if entry not in allowed:
                    errors.append(f"{entry} (line {lineno})")

for entry in sorted(allowed - found):
    stale.append(entry)

if stale:
    print("note: allowlist entries no longer present (remove them):")
    for entry in stale:
        print("  " + entry)

if errors:
    print("concrete particle references in Core without an allowlist entry:")
    for err in errors:
        print("  " + err)
    sys.exit(1)

if not CORE.is_dir():
    print(f"core purity: {CORE} does not exist - the check would pass vacuously", file=sys.stderr)
    sys.exit(2)

print(f"core purity OK ({len(found)} known references, {len(errors)} new)")
