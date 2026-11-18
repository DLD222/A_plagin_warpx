"""补齐 26.11 文档里有、目录里缺的参数（求解器调优 / AMReX 运行控制）。

类型与默认值取自 `_upstream/warpX-26.11/Docs/source/usage/parameters.rst`。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

# name: (type, default, 中文说明, 分类 id, 可选值)
NEW = {
    "warpx.self_fields_absolute_tolerance": ("float", "0.0",
        "自场泊松/静磁求解的**绝对**容差；0 表示只用相对容差（自场很大时相对容差可能过严，"
        "可给一个小的绝对值）", "solver", None),
    "warpx.self_fields_verbosity": ("int", "2",
        "多重网格求解器打印级别：0 静默、1 每步一行、2 每次求解的收敛信息；排查不收敛时设 2",
        "solver", None),
    "warpx.self_fields_agglomeration": ("int", "1",
        "是否对粗层做网格聚合（1 = 是，默认；显存/内存紧张或收敛异常时试 0）", "solver", None),
    "warpx.self_fields_consolidation": ("int", "1",
        "是否把粗层合并成大块（1 = 是，默认）；与聚合一起影响多重网格效率", "solver", None),
    "warpx.self_fields_max_coarsening_level": ("int", "30",
        "多重网格最多粗化多少层（默认 30，够用；很小的问题可调小以省时间）", "solver", None),
    "warpx.self_fields_num_final_sweeps": ("int", "8",
        "求解结束时额外做的平滑遍数（默认 8）；收敛曲线尾部抖动时可加大", "solver", None),
    "warpx.magnetostatic_solver_absolute_tolerance": ("float", None,
        "静磁求解器的绝对容差；不填则沿用 warpx.self_fields_absolute_tolerance", "solver", None),
    "warpx.magnetostatic_solver_required_precision": ("float", None,
        "静磁求解器的相对精度要求；不填则沿用 warpx.self_fields_required_precision", "solver", None),
    "warpx.magnetostatic_solver_verbosity": ("int", None,
        "静磁求解器打印级别；不填则沿用 warpx.self_fields_verbosity", "solver", None),
    "warpx.limit_verbose_step": ("int", "0",
        "是否限制每步的输出量（1 = 只在关键步打印，长时间运行时日志会小很多）", "control", None),
    "warpx.tiny_profiler_flush_interval": ("string", "-1",
        "tiny profiler 的落盘间隔（如 \"1.0\" 秒；-1 = 结束才写）", "control", None),
    "amrex.abort_on_unused_inputs": ("int", "0",
        "输入里出现 WarpX 不认识的参数时是否直接 abort（1 = 严格模式，能抓拼写错误；"
        "0 = 只警告，默认）", "control", None),
    "amrex.omp_threads": ("string", "nosmt",
        "\"system\" / \"nosmt\" / 正整数：AMReX 设置 OpenMP 线程数的方式；留空按环境变量 OMP_NUM_THREADS",
        "parallel", None),
    "amrex.async_out": ("int", "0",
        "是否异步写盘（1 = 后台线程写 openPMD，计算不阻塞；占内存更多）", "diag", None),
}


def main() -> int:
    cat_path = DATA / "catalog.json"
    cat = json.loads(cat_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in cat["params"]}
    added = 0
    for name, (kind, default, _doc, _cat, _opts) in NEW.items():
        if name not in names:
            cat["params"].append({"name": name, "type": kind, "default": default,
                                  "optional": True, "doc": "", "core": False, "options": None})
            added += 1
    cat["params"].sort(key=lambda p: p["name"])
    cat["counts"]["total"] = len(cat["params"])
    cat_path.write_text(json.dumps(cat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    by_id = {c["id"]: c for c in cats["categories"]}
    for name, (_t, _d, _doc, cat_id, _o) in NEW.items():
        c = by_id.get(cat_id)
        if c and name not in c["params"]:
            c["params"].append(name)
            c["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    for name, (_t, default, doc, _c, _o) in NEW.items():
        dflt = f"默认 {default}" if default not in (None, "") else "默认 见说明"
        zh["params"][name] = f"{doc}（取值：见说明；{dflt}）"
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"新增 {added} 个 26.11 参数")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
