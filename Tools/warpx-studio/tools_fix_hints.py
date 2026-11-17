"""按参数类型生成"右侧取值提示"和灰色示例，替换掉一句通吃的敷衍提示。

原来的 hints 里 396 条都是"文本或数值，按参数说明填写"，还有 24 条把
"多个值用空格分隔，个数按维度给"套在物种名列表上 —— 与参数真实类型矛盾。

规则（只看 catalog 里声明的类型）：
  bool                      -> 0 = 关闭 / 1 = 开启（并做成下拉）
  表达式（名字带 (x,y,z) 等） -> 数学表达式，可用 x y z t 与 sin/cos/exp…
  list of strings           -> 多个名称，空格分隔（如 ions electrons）
  2/3 integers|floats        -> 几个数用空格分隔，个数按维度（2D 两个、3D 三个）
  float / int               -> 单个数值（可写 1.0e-6 这样的科学计数）
  string                    -> 文本
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

EXPR_RE = re.compile(r"\(([a-z,]*)\)\s*$")


EXPR_WORDS = ("eb_implicit_function", "function", "potential", "parser")


def is_expression(name: str) -> bool:
    if any(w in name for w in EXPR_WORDS) and "(" in name:
        return True
    if name.endswith(("eb_implicit_function", "ref_patch_function")):
        return True
    return bool(EXPR_RE.search(name)) and any(v in name for v in ("x", "y", "z", "t"))


def hint_for(name: str, ptype: str | None, options) -> str:
    t = (ptype or "").lower()
    if t in {"bool", "boolean"}:
        return "0 = 关闭，1 = 开启（下拉选择）"
    if options:
        joined = " / ".join(options[:6])
        return f"可选：{joined}" + (" …" if len(options) > 6 else "")
    if is_expression(name):
        return "数学表达式：可用 x y z（及 t）与 sin/cos/exp/sqrt 等"
    if "list of strings" in t or "strings" in t:
        return "多个名称，用空格分隔（例如 ions electrons）"
    if re.search(r"\b[23] (integers|floats)", t) or ("list of" in t and ("float" in t or "int" in t)):
        return "几个数用空格分隔，个数按维度（二维两个、三维三个）"
    if "float" in t or "real" in t:
        return "单个数值，可写科学计数法（如 1.0e-6）"
    if "int" in t:
        return "单个整数"
    if "string" in t:
        return "文本"
    return "按左侧说明填写"


def placeholder_for(name: str, ptype: str | None, options, ndims: int = 2) -> str:
    t = (ptype or "").lower()
    if t in {"bool", "boolean"}:
        return "请选择 0 或 1"
    if options:
        return "例如 " + options[0]
    if is_expression(name):
        return "例如 (1e-3)^2 - (x*x + z*z)"
    if "list of strings" in t or t.endswith("strings"):
        return "例如 ions electrons"
    if re.search(r"\b[23] (integers|floats)", t) or ("list of" in t and ("float" in t or "int" in t)):
        return "例如 " + " ".join(["64"] * ndims) if "int" in t else "例如 " + " ".join(["0.0"] * ndims)
    if "float" in t or "real" in t:
        return "例如 1.0e-6"
    if "int" in t:
        return "例如 10"
    return "按左侧说明填写"


def main() -> int:
    enums_path = DATA / "enums.json"
    doc = json.loads(enums_path.read_text(encoding="utf-8"))
    catalog = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
    hints = doc.get("hints") or {}
    changed = 0
    for p in catalog["params"]:
        name = p["name"]
        new = hint_for(name, p.get("type"), p.get("options"))
        if hints.get(name) != new:
            hints[name] = new
            changed += 1
    doc["hints"] = hints
    enums_path.write_text(json.dumps(doc, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"重写 {changed} 条取值提示")
    for probe in ("particles.deposit_on_main_grid", "particles.gather_from_main_grid",
                  "amr.n_cell", "warpx.do_pml_divb_cleaning", "warpx.eb_implicit_function",
                  "<species_name>.charge"):
        p = next(x for x in catalog["params"] if x["name"] == probe)
        print(f"  {probe}: {hint_for(probe, p.get('type'), p.get('options'))} | "
              f"{placeholder_for(probe, p.get('type'), p.get('options'))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
