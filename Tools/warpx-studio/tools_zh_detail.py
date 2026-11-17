"""给每条参数说明补上"取值怎么填"，让注释足够详细而不只是一句定义。

格式：<中文说明>（取值：…；默认：…）
取值提示按类型生成（bool / 枚举 / 表达式 / 数组 / 数值 / 文本），
默认值取自 catalog 的 default 字段；没有默认值就写"不填 = 用 WarpX 默认值"。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"


def value_hint(p: dict) -> str:
    t = (p.get("type") or "").lower()
    opts = p.get("options") or []
    name = p["name"]
    if opts:
        return "可选 " + " / ".join(opts[:6]) + (" …" if len(opts) > 6 else "")
    if t == "bool":
        return "0 = 关闭，1 = 开启"
    if ("(" in name and any(v in name for v in ("x", "y", "z", "t"))) \
            or name.endswith(("_function", "implicit_function", "ref_patch_function")):
        return "写成含 x y z（必要时 t）的表达式，可用 sin/cos/exp/sqrt 等"
    if "list of strings" in t:
        return "多个名字，用空格分隔（如 ions electrons）"
    if re.search(r"\b[23] (integers|floats)", t) or ("list of" in t and ("int" in t or "float" in t)):
        return "若干个数用空格分隔，个数按维度（二维 2 个、三维 3 个）"
    if "float" in t or "real" in t:
        return "单个数值，可写 1.0e-6 这样的科学计数法"
    if "int" in t:
        return "单个整数"
    if "string" in t:
        return "文本"
    return ""


def when_needed(catalog: dict, name: str, zh_params: dict) -> str:
    """这条参数在什么时候才生效（由 gating_rules 反推，用中文说出来）。"""
    rules = (catalog.get("gating_rules") or {}).get(name) or []
    sels = []
    for r in rules:
        sel = r.get("param")
        if not sel:
            continue
        label = sel.split(".", 1)[-1] if sel.startswith("<") else sel
        if r.get("op") == "equals":
            sels.append(f"{label} = {r.get('value')}")
        elif r.get("op") == "truthy":
            sels.append(f"{label} 打开")
        elif r.get("op") == "nonempty":
            sels.append(f"{label} 填了之后")
    if not sels:
        return ""
    return "仅在 " + " 或 ".join(dict.fromkeys(sels[:2])) + " 时生效"


def main() -> int:
    catalog = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    params = zh["params"]
    updated = 0
    for p in catalog["params"]:
        name = p["name"]
        text = (params.get(name) or "").strip()
        if not text:
            continue
        if "取值：" in text:
            continue
        hint = value_hint(p)
        # 说明里已经列过可选值时，不再重复
        if hint.startswith("可选") and all(o in text for o in (p.get("options") or [])[:4]):
            hint = ""
        default = p.get("default")
        default_txt = (f"默认 {default}" if default not in (None, "")
                       else "不填就用 WarpX 默认值")
        detail = "；".join(x for x in (hint, default_txt) if x)
        needed = when_needed(catalog, name, params)   # 每条都写清"何时生效"
        if detail:
            tail = f"取值：{detail}"
            if needed:
                tail += f"；{needed}"
            params[name] = f"{text}（{tail}）"
            updated += 1
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"补充取值说明 {updated} 条")
    for n in ("<species_name>.ux", "<species_name>.profile", "warpx.eb_implicit_function"):
        print(f"  {n}: {params.get(n)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
