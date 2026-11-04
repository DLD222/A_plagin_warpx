#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""Build the A0 header catalog (CSV + per-subsystem API + index + coverage).

Requires Tools/A0/parse_headers.py to have produced /tmp/header_catalog.json
(run it first). This script is intentionally simple: it is a snapshot generator,
not a C++ parser.
"""
from __future__ import annotations
import csv
import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
A0 = ROOT / "Docs/research_notes/A0"
API = A0 / "api"
BAD = re.compile(r"^(return|if|for|while|switch|else|AMREX_|#|\{|\})")


def clean(items):
    out, seen = [], set()
    for item in items:
        s = re.sub(r"\s+", " ", item).strip()
        if not s or BAD.match(s) or s in seen:
            continue
        seen.add(s)
        out.append(s)
    return out


def main() -> int:
    data = json.loads(Path("/tmp/header_catalog.json").read_text())
    API.mkdir(parents=True, exist_ok=True)
    for old in API.glob("*.md"):
        old.unlink()

    subs = defaultdict(list)
    for x in data:
        x["public_methods"] = clean(x["public_methods"])
        x["public_members"] = clean(x["public_members"])
        parts = x["file"].split("/")
        sub = parts[1] if len(parts) > 2 else "Root"
        subs[sub].append(x)

    for sub, items in sorted(subs.items()):
        with (API / f"{sub}.md").open("w") as f:
            f.write(f"# A0 API catalog — {sub}\n\n<!-- SPDX-License-Identifier: BSD-3-Clause -->\n\n")
            f.write(f"> Generated; {len(items)} classes/structs. Signatures are heuristic.\n\n")
            for x in sorted(items, key=lambda y: (y["file"], y["name"])):
                f.write(f"## `{x['name']}`\n\n- file: `{x['file']}`\n- kind: `{x['kind']}`")
                if x["bases"]:
                    f.write(f", bases: `{x['bases']}`")
                f.write(f"\n- public methods: {len(x['public_methods'])}; public members: {len(x['public_members'])}\n")
                if x["public_methods"]:
                    f.write("\n```cpp\n" + "\n".join(x["public_methods"]) + "\n```\n")
                if x["public_members"]:
                    f.write("\nPublic members:\n\n```cpp\n" + "\n".join(x["public_members"][:40]) + "\n```\n")
                f.write("\n")

    with (A0 / "classes.csv").open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["subsystem", "file", "kind", "class", "bases", "n_public_methods",
                    "n_public_members", "n_private_members", "is_template", "has_virtual", "has_pure"])
        for x in sorted(data, key=lambda y: (y["file"], y["name"])):
            parts = x["file"].split("/")
            sub = parts[1] if len(parts) > 2 else "Root"
            w.writerow([sub, x["file"], x["kind"], x["name"], x["bases"], len(x["public_methods"]),
                        len(x["public_members"]), x["n_private_members"], x["is_template"],
                        x["has_virtual"], x["has_pure"]])

    total_methods = sum(len(x["public_methods"]) for x in data)
    with (A0 / "HEADER_CATALOG.md").open("w") as f:
        f.write("# A0：头文件类目录与公共接口\n\n<!-- SPDX-License-Identifier: BSD-3-Clause -->\n\n")
        f.write("> 自动生成；签名启发式抽取，精确签名以头文件为准。"
                "机器可读：`classes.csv`；逐子系统：`api/`；覆盖：`COVERAGE.md`。\n\n")
        f.write("## 1. 总览\n\n| 子系统 | 类/结构体 | 公共方法 | 头文件 |\n|---|---:|---:|---:|\n")
        for sub, items in sorted(subs.items(), key=lambda kv: -sum(len(x["public_methods"]) for x in kv[1])):
            files = len({x["file"] for x in items})
            f.write(f"| [{sub}](api/{sub}.md) | {len(items)} | {sum(len(x['public_methods']) for x in items)} | {files} |\n")
        f.write(f"| **合计** | **{len(data)}** | **{total_methods}** | **{len({x['file'] for x in data})}** |\n\n")
        f.write("## 2. 类索引（按子系统）\n\n")
        for sub, items in sorted(subs.items()):
            f.write(f"### {sub}\n\n| 类/结构体 | 文件 | 公共方法 | 基类 |\n|---|---|---:|---|\n")
            for x in sorted(items, key=lambda y: -len(y["public_methods"])):
                f.write(f"| `{x['name']}` | `{x['file']}` | {len(x['public_methods'])} | {x['bases'][:80]} |\n")
            f.write("\n")

    headers = sorted(p for p in (ROOT / "Source").rglob("*.H"))
    parsed = {x["file"] for x in data}
    no = [p for p in headers if str(p.relative_to(ROOT)) not in parsed]
    with (A0 / "COVERAGE.md").open("w") as f:
        f.write("# A0：头文件覆盖表\n\n<!-- SPDX-License-Identifier: BSD-3-Clause -->\n\n")
        f.write(f"- 头文件总数：**{len(headers)}**\n- 解析出类：**{len(parsed)}**\n")
        f.write(f"- 无类头文件：**{len(no)}**（含 `_fwd.H` 与 free function/kernel）\n")
        f.write(f"- 类/结构体：**{len(data)}**；公共方法：**{total_methods}**\n\n")
        f.write("## 无 class/struct 的头文件\n\n")
        for p in no:
            f.write(f"- `{p.relative_to(ROOT)}`\n")
    print(f"headers={len(headers)} parsed={len(parsed)} classes={len(data)} methods={total_methods}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
