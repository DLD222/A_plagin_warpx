"""把文档里有、但生成目录时漏掉的 <species_name>.* 参数补进 catalog。

原生成器只认 `.. pp:param::` 形式的定义，"constant 需要 <species_name>.density" 这类
写在列表项里的参数全部丢失，导致界面上选了 profile=constant 也找不到 density 可填。
这里从 parameters.rst 里把所有 `<species_name>.X` 捞出来，缺的补上：
类型优先取 pp:param 块里的 :type:，否则按名字推断；
说明取 pp:param 块正文或所在列表项的那句话。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
RST = pathlib.Path(__file__).resolve().parents[2] / "Docs/source/usage/parameters.rst"

NAME_RE = re.compile(r"<species_name>\.([A-Za-z0-9_(),]+)")
TYPE_RE = re.compile(r":type:\s*``([^`]+)``")


def infer_type(name: str) -> str:
    tail = name.rsplit(".", 1)[-1]
    if tail.endswith(")") or "function" in tail:
        return "string"
    if tail.startswith(("do_", "is_", "inject_from_", "impose_")) or tail.endswith("_rotation"):
        return "bool"
    if any(k in tail for k in ("file", "path", "name", "species", "type", "style",
                               "axis", "direction", "algorithm", "element")):
        return "string"
    if any(k in tail for k in ("npart", "num", "level", "iter", "step", "stride", "order")):
        return "int"
    return "float"


def main() -> int:
    rst = RST.read_text(encoding="utf-8", errors="ignore").splitlines()

    # 1) pp:param 块的类型与说明
    blocks: dict[str, dict] = {}
    current = None
    body: list[str] = []
    for line in rst:
        m = re.match(r"^\.\. pp:param:: (\S+)\s*$", line)
        if m:
            if current:
                blocks[current] = {"type": None, "doc": " ".join(body).strip()}
                for b in body:
                    tm = TYPE_RE.search(b)
                    if tm:
                        blocks[current]["type"] = tm.group(1).strip()
            current, body = m.group(1), []
            continue
        if current is not None:
            body.append(line.strip())

    # 2) 列表项里提到的参数 + 那句话
    items: dict[str, str] = {}
    for i, line in enumerate(rst):
        m = re.match(r"^\s*\*\s*``<species_name>\.([A-Za-z0-9_(),]+)``", line)
        if m:
            name = f"<species_name>.{m.group(1)}"
            text = line.strip().lstrip("* ").strip()
            items.setdefault(name, text[:160])
        for m2 in NAME_RE.finditer(line):
            name = f"<species_name>.{m2.group(1)}"
            if name not in items:
                text = line.strip()
                items[name] = text[:160] if len(text) > 8 else ""

    catalog_path = DATA / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    added = []
    for name in sorted(set(items) | set(blocks)):
        if not name.startswith("<species_name>.") or name in names:
            continue
        tail = name.rsplit(".", 1)[-1]
        if tail in {".", ""}:
            continue
        info = blocks.get(name, {})
        catalog["params"].append({
            "name": name,
            "type": info.get("type") or infer_type(name),
            "default": None,
            "optional": True,
            "doc": info.get("doc") or items.get(name, ""),
            "core": False,
            "options": None,
        })
        added.append(name)

    catalog["params"].sort(key=lambda p: p["name"])
    catalog["counts"]["total"] = len(catalog["params"])
    catalog_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for c in cats["categories"]:
        if c["id"] == "species":
            for n in added:
                if n not in c["params"]:
                    c["params"].append(n)
            c["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"补进 {len(added)} 个物种参数，目录总数 {catalog['counts']['total']}")
    for n in added[:12]:
        print("  ", n)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
