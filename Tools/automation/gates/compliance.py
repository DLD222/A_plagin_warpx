#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""Public-release compliance gate (fast, no build).

Checks:
  - upstream license/notice files are present;
  - no third-party PDFs or generated logs are tracked;
  - no local absolute paths in tracked text files;
  - no obvious credentials;
  - fork-added docs/code carry an SPDX identifier (warn only for generated data).
Exit code 0 on success, 1 on failure.
"""
from __future__ import annotations
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
REQUIRED = ["LICENSE", "LICENSE.txt", "NOTICE.txt", "LEGAL.txt", "FORK_NOTICE.md"]
FORBIDDEN_SUFFIX = (".pdf", ".log")
SECRET = re.compile(
    r"(BEGIN (RSA|OPENSSH|EC|PGP) PRIVATE KEY|AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{36}|"
    r"github_pat_[A-Za-z0-9_]{20,}|xox[baprs]-[A-Za-z0-9-]+)"
)
SPDX = "SPDX-License-Identifier: BSD-3-Clause"
SPDX_REQUIRED = [
    "Source/Runtime/Runtime.H",
    "Source/Runtime/Runtime.cpp",
    "Docs/research_notes/最终的技术路线.md",
    "Docs/research_notes/SOURCE_AUDIT.md",
    "Docs/research_notes/A0/README.md",
]


def tracked() -> list[str]:
    out = subprocess.run(["git", "ls-files"], cwd=ROOT, capture_output=True, text=True, check=True)
    return [line for line in out.stdout.splitlines() if line]


def is_text(path: Path) -> bool:
    try:
        chunk = path.read_bytes()[:4096]
    except OSError:
        return False
    return b"\0" not in chunk


def main() -> int:
    errors: list[str] = []
    warnings: list[str] = []
    files = tracked()

    for name in REQUIRED:
        if not (ROOT / name).exists():
            errors.append(f"missing required notice: {name}")

    for f in files:
        p = ROOT / f
        if f.lower().endswith(FORBIDDEN_SUFFIX):
            errors.append(f"tracked third-party/generated artifact: {f}")
        if not p.exists() or not is_text(p):
            continue
        text = p.read_text(errors="ignore")
        if "/home/douliangduo" in text or "\\wsl.localhost" in text:
            errors.append(f"personal absolute path in {f}")
        if SECRET.search(text):
            errors.append(f"possible credential in {f}")

    for f in SPDX_REQUIRED:
        p = ROOT / f
        if p.exists() and SPDX not in p.read_text(errors="ignore"):
            errors.append(f"missing SPDX header: {f}")

    big = [f for f in files if (ROOT / f).exists() and (ROOT / f).stat().st_size > 5_000_000]
    for f in big:
        warnings.append(f"large tracked file (>5 MB): {f}")

    print(f"tracked files: {len(files)}")
    if warnings:
        print("WARNINGS:")
        for w in warnings:
            print("  -", w)
    if errors:
        print("FAIL:")
        for e in errors:
            print("  -", e)
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
