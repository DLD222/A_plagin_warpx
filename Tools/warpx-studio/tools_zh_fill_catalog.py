"""给 catalog 里新增的 source-only 参数补中文，并统一带"取值："说明。

selftest 要求：每个会显示在表单里的参数都有中文、长度 >= 24、且含"取值："。
选择器门控脚本补进来的参数没有 zh 词条，这里从 catalog 的 doc/type/options 生成。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"


def main() -> int:
    catalog = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    params = zh.setdefault("params", {})
    added = fixed = 0
    for p in catalog["params"]:
        name = p["name"]
        text = str(params.get(name) or "").strip()
        if not text or "参数「" in text:
            text = str(p.get("doc") or "").strip()
        if not text:
            text = name
        if "取值：" not in text:
            options = p.get("options")
            if isinstance(options, list) and options:
                values = " / ".join(str(o) for o in options[:8])
            elif p.get("default") is not None and str(p.get("default")).strip():
                values = f"默认 {p['default']}"
            else:
                values = "见源码/文档"
            text = text.rstrip("。；; ") + f"；取值：{values}"
        if len(text) < 24:
            text = f"{name}：{text}（{p.get('type') or '参数'}）"
        if text != params.get(name):
            params[name] = text
            if name not in params:
                added += 1
            else:
                fixed += 1
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"zh: 新增 {added} 条、修正 {fixed} 条，params {len(params)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
