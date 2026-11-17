"""把 <collision_name>.* 参数补进目录（碰撞模块之前只有 collisions.collision_names 一个空壳）。

界面里填了 collisions.collision_names = c1 之后，应该能接着填 c1.CoulombLog / c1.species / www.profile …
这些参数在 parameters.rst 里以 `<collision_name>.X` 的形式定义，但生成目录时被整体丢掉。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
RST = pathlib.Path(__file__).resolve().parents[2] / "Docs/source/usage/parameters.rst"

TYPE_RE = re.compile(r":type:\s*``([^`]+)``")


def infer(name: str) -> str:
    tail = name.rsplit(".", 1)[-1]
    if tail.endswith(")") or "function" in tail:
        return "string"
    if tail.startswith(("do_", "is_", "parse_", "create_")):
        return "bool"
    if any(k in tail for k in ("name", "file", "profile", "species", "processes",
                               "type", "model", "log", "state")):
        return "string"
    return "float"


def main() -> int:
    lines = RST.read_text(encoding="utf-8", errors="ignore").splitlines()
    blocks, current, body = {}, None, []
    for line in lines:
        m = re.match(r"^\.\. pp:param:: (\S+)\s*$", line)
        if m:
            if current:
                blocks[current] = {"type": None, "doc": " ".join(body).strip()[:220]}
            current, body = m.group(1), []
            continue
        if current is not None:
            body.append(line.strip())
            tm = TYPE_RE.search(line)
            if tm:
                blocks[current] = {"type": tm.group(1).strip(), "doc": " ".join(body).strip()[:220]}
    if current:
        blocks[current] = {"type": None, "doc": " ".join(body).strip()[:220]}

    catalog_path = DATA / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    added = []
    for name, info in blocks.items():
        if not name.startswith("<collision_name>.") or name in names:
            continue
        catalog["params"].append({
            "name": name,
            "type": info.get("type") or infer(name),
            "default": None, "optional": True,
            "doc": info.get("doc") or "", "core": False, "options": None,
        })
        added.append(name)
    catalog["params"].sort(key=lambda p: p["name"])
    catalog["counts"]["total"] = len(catalog["params"])
    catalog_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for c in cats["categories"]:
        if c["id"] == "collision":
            for n in added:
                if n not in c["params"]:
                    c["params"].append(n)
            c["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"补进 {len(added)} 个碰撞参数，目录 {catalog['counts']['total']}")
    for n in added[:8]:
        print("  ", n)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
