"""补齐文档里有、目录里缺的常用参数（并写中文说明、放对步骤）。

来源：Docs/source/usage/parameters.rst，类型/默认值已用脚本核对过。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

# name: (type, default, 中文说明, 归类 id)
NEW = {
    "warpx.poisson_solver": ("string", "multigrid",
        "静电/静磁泊松方程用哪个求解器（下拉）：multigrid = 多重网格（默认，支持任意边界）；"
        "fft = 快速傅里叶（快，要求周期或开放边界）；cudss = GPU 直接解（需专门编译）",
        "solver"),
    "algo.evolve_scheme": ("string", "explicit",
        "时间推进方式（下拉）：explicit = 显式（默认，标准 PIC）；"
        "有的构建还支持隐式方案，填错会直接报错", "solver"),
    "warpx.random_seed": ("int", None,
        "随机数种子（整数）：固定它可以让每次运行得到完全相同的粒子采样，便于复现；"
        "不填则每次不同", "control"),
    "warpx.gamma_boost": ("float", "1.",
        "洛伦兹助推因子 γ：>1 时在助推参考系里算，再变换回实验室系（用于强相对论束，"
        "可从瞬态里提取稳态）。不填 = 1（不助推）", "solver"),
    "warpx.boost_direction": ("string", None,
        "洛伦兹助推方向（下拉 x/y/z）：只有 gamma_boost > 1 时才需要", "solver"),
    "warpx.self_fields_max_iters": ("int", "200",
        "自场（泊松/静磁）迭代求解的最大迭代次数：不收敛时可加大（更慢但更准），"
        "默认 200", "solver"),
    "warpx.self_fields_required_precision": ("float", "1.e-11",
        "自场迭代求解的相对精度要求：越小越准越慢，默认 1e-11", "solver"),
    "warpx.magnetostatic_solver_max_iters": ("int", None,
        "静磁求解器的最大迭代次数（仅 hybrid/静磁模式用）；不填则沿用 self_fields_max_iters",
        "solver"),
    "warpx.safe_guard_cells": ("int", "0",
        "是否给进程边界留出保护层（0 = 否，1 = 是）：并行分区边上出现异常时开它可以规避，"
        "代价是略慢", "control"),
    "warpx.abort_on_warning_threshold": ("string", None,
        "遇到警告时的处置（下拉）：low/medium/high = 达到该级别就中止；"
        "不填 = 警告只打印不中止", "control"),
    "warpx.serialize_initial_conditions": ("int", "0",
        "初始化阶段是否串行（0 = 并行初始化，1 = 先生成再分发）："
        "粒子数极大或初始化报错时开 1 更稳", "control"),
    "warpx.write_diagnostics_on_restart": ("int", "0",
        "续算后是否立刻写一次诊断（0 = 不写，1 = 写）：想看续算起始状态时开 1", "diag"),
    "warpx.used_inputs_file": ("string", "warpx_used_inputs",
        "把本次用到的所有输入参数另存哪个文件（含默认值），方便核对；默认 warpx_used_inputs",
        "control"),
}

ZH_PREFIX = ""


def main() -> int:
    cat_path = DATA / "catalog.json"
    cat = json.loads(cat_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in cat["params"]}
    added = 0
    for name, (kind, default, doc, cat_id) in NEW.items():
        if name not in names:
            cat["params"].append({"name": name, "type": kind, "default": default,
                                  "optional": True, "doc": "", "core": False,
                                  "options": None})
            added += 1
    cat["params"].sort(key=lambda p: p["name"])
    cat["counts"]["total"] = len(cat["params"])
    cat_path.write_text(json.dumps(cat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    by_id = {c["id"]: c for c in cats["categories"]}
    for name, (_, _, _, cat_id) in NEW.items():
        c = by_id.get(cat_id)
        if c and name not in c["params"]:
            c["params"].append(name)
            c["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    for name, (_, default, doc, _) in NEW.items():
        dflt = f"默认 {default}" if default not in (None, "") else "默认 不填"
        zh["params"][name] = f"{doc}（取值：见说明；{dflt}）"
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"新增 {added} 个参数")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
