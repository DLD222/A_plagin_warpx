"""把文档里列举了固定取值的参数补成下拉（select），而不是让用户手打。

WarpX 的 `parameters.rst` 用两种写法列出可选值：
    * ``Absorbing``: Particles that reach ... (列表项)
    Options are: ``a``, ``b`` or ``c``        （行内枚举）
这里把这两类都抽出来，写进 enums.json 与 catalog 的 options 字段，
界面上对应的输入框就会变成下拉框。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
RST = pathlib.Path(__file__).resolve().parents[2] / "Docs/source/usage/parameters.rst"

PARAM_RE = re.compile(r"^\.\. pp:param:: (\S+)\s*$")
BULLET_RE = re.compile(r"^\s*\*\s*``([^`]+)``")
INLINE_RE = re.compile(r"``([A-Za-z_][\w\-]*)``")


def parse_rst() -> dict[str, list[str]]:
    options: dict[str, list[str]] = {}
    current = None
    body: list[str] = []
    def flush():
        if current is None:
            return
        vals: list[str] = []
        for line in body:
            m = BULLET_RE.match(line)
            if m:
                vals.append(m.group(1).strip())
        # 只认列表项里的纯标识符；行内"Options are"那种会把参数名也抓进来，弃用
        vals = [v for v in vals if re.fullmatch(r"[A-Za-z][A-Za-z0-9_\-]{1,30}", v)]
        if len(vals) > 8:
            vals = []
        if len(vals) >= 2:
            options[current] = sorted(dict.fromkeys(vals))
    for line in RST.read_text(encoding="utf-8", errors="ignore").splitlines():
        m = PARAM_RE.match(line)
        if m:
            flush()
            current = m.group(1)
            body = []
            continue
        if current is not None:
            if line and not line.startswith((" ", "\t")) and not line.startswith(".."):
                flush()
                current, body = None, []
            else:
                body.append(line)
    flush()
    return options


# 文档里不是"列表项"写法、但确实是固定取值的参数，直接补上（源码/文档已核对）
CURATED: dict[str, list[str]] = {
    "eb2.geom_type": ["all_regular", "implicit_function", "stl"],
    "algo.em_solver_medium": ["vacuum", "constant", "macroscopic"],
    "<diag_name>.format": ["plotfile", "openpmd", "checkpoint", "ascent", "sensei"],
    "<diag_name>.diag_type": ["Full", "TimeAveraged", "BackTransformed"],
    "boundary.particle_eb": ["Absorbing", "Reflecting", "Thermal"],
    "geometry.dims": ["1", "2", "3", "RZ"],
}


def main() -> int:
    found = parse_rst()
    enums_path = DATA / "enums.json"
    enums_doc = json.loads(enums_path.read_text(encoding="utf-8"))
    enums = enums_doc["enums"]
    cat_path = DATA / "catalog.json"
    catalog = json.loads(cat_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}

    added = []
    for name, values in CURATED.items():
        if name in names:                  # 人工枚举是权威的，覆盖自动结果
            if enums.get(name) != values:
                enums[name] = values
                added.append((name, values))
    for name, values in found.items():
        if name not in names:
            continue
        clean = [v for v in values if len(v) <= 40 and " " not in v.strip()]
        if len(clean) < 2:
            continue
        if name in enums:
            continue        # 已有枚举不覆盖（避免把维度名之类混进来）
        enums[name] = clean
        added.append((name, clean))
    for p in catalog["params"]:
        if p["name"] in enums:
            p["options"] = enums[p["name"]]

    enums_path.write_text(json.dumps(enums_doc, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    cat_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"从文档补出 {len(added)} 个下拉；enums 现在 {len(enums)} 条")
    for n, v in added[:15]:
        print(f"  {n}: {v}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
