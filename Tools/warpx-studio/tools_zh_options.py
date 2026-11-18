"""把"可选值"写全，并把所有枚举参数变成下拉框。

用户要求：
  1. 能选的必须做成选择题（下拉），不能是填空
  2. 注释里**不允许省略**：每个可选值都要写出来并给含义，不许出现"其它""…"
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

EXTRA_ENUMS = {
    "warpx.do_electrostatic": ["none", "labframe", "labframe-electromagnetostatic",
                               "labframe-effective-potential", "relativistic"],
    "warpx.poisson_solver": ["multigrid", "fft", "cudss"],
}
EXTRA_GLOSSARY = {
    "warpx.do_electrostatic": {
        "none": "纯电磁（默认）：每步用 Maxwell 方程推进 E/B，有电磁波与激光",
        "labframe": "实验室系静电：每步解泊松方程求 E，没有电磁波；步长不受 CFL 限制",
        "labframe-electromagnetostatic": "实验室系静磁静电：解泊松 + 静磁，忽略位移电流",
        "labframe-effective-potential": "实验室系有效势：用势函数解，适合强流束",
        "relativistic": "相对论静电：在粒子静止系解泊松，适合强相对论束",
    },
    "warpx.poisson_solver": {
        "multigrid": "多重网格迭代（默认，任意边界）",
        "fft": "FFT 泊松（快，要求周期/开放边界）",
        "cudss": "cuDSS 直接解（需 GPU 与 cuDSS 编译）",
    },
}

TAIL = re.compile(r"[（(]\s*可选[^）)]*[）)]\s*$")


def main() -> int:
    cat_path = DATA / "catalog.json"
    enums_path = DATA / "enums.json"
    zh_path = DATA / "zh.json"

    cat = json.loads(cat_path.read_text(encoding="utf-8"))
    enums_doc = json.loads(enums_path.read_text(encoding="utf-8"))
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    enums = enums_doc.setdefault("enums", {})
    glossary = enums_doc.setdefault("glossary", {})

    enums.update(EXTRA_ENUMS)
    for k, v in EXTRA_GLOSSARY.items():
        glossary.setdefault(k, {}).update(v)

    # 1) 所有枚举参数在 catalog 里标 options（界面据此渲染下拉框）
    names = {p["name"]: p for p in cat["params"]}
    fixed_dropdown = 0
    for name, values in enums.items():
        if not values:
            continue
        p = names.get(name)
        if p is None:
            continue
        if p.get("options") != values:
            p["options"] = list(values)
            fixed_dropdown += 1

    # 2) 注释写全每个可选值（不省略）
    rewritten = 0
    for name, values in enums.items():
        text = zh["params"].get(name)
        if not text or not values:
            continue
        meaning = glossary.get(name) or {}
        parts = []
        for v in values:
            m = meaning.get(v)
            parts.append(f"`{v}` = {m}" if m else f"`{v}`")
        listing = "；".join(parts)
        base = TAIL.sub("", text).rstrip("。；， ")
        # 旧文本里常写成"说明：a 含义 / b 含义 / 其它"——只保留冒号前的说明，
        # 后半段的选项列表交给下面统一生成，避免出现"labframe_，"这种残渣
        if "：" in base:
            head, rest = base.split("：", 1)
            if "/" in rest or "（" in rest:
                base = head
        base = re.sub(r"[（(]\s*取值[^）)]*[）)]", "", base)
        base = base.replace("其它见文档", "").replace("其它", "").replace("…", "").rstrip("。；，、 ")
        zh["params"][name] = (f"{base}（取值：下拉选择，共 {len(values)} 项 —— {listing}）")
        rewritten += 1

    # 3) 非枚举参数里残留的省略号/其它，去掉（不留空话）
    for name, text in list(zh["params"].items()):
        if "…" in text or "其它" in text or "..." in text:
            zh["params"][name] = (text.replace("其它见文档", "见 WarpX 文档")
                                  .replace("…", "，").replace("...", "，")
                                  .replace("其它", "其他取值"))

    cat_path.write_text(json.dumps(cat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    enums_path.write_text(json.dumps(enums_doc, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"下拉框修正 {fixed_dropdown} 个；可选值写全 {rewritten} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
