"""裁剪 Studio 的参数目录：只保留"有文档、非内部、且功能没被别处覆盖"的参数。

背景：catalog.json 有 802 条参数，其中 415 条只出现在源码里、没有任何文档，属于内部实现；
另有 amrex./ablastr. 前缀的 AMReX 内部开关，以及 warpx.max_step 这类与 max_step 完全等价的别名。
界面上把它们全列出来正是"输入参数太多"的来源。

规则（可复核、可重复执行）：
R1 只保留"有文档，或被仓库示例算例实际使用"的参数（两者都没有 = 内部实现）
R2 去掉 AMReX 内部前缀（amrex./ablastr.）
R3 去掉与 Studio 自身控件重复的参数（MPI/OMP、输出文件名）
R4 去掉别名（warpx.max_step / warpx.stop_time / ... 与裸名等价）
R5 去掉求解器内部迭代旋钮（self_fields_* / magnetostatic_solver_* 等）
R6 去掉未被编辑器使用的模板组（激光/缩减诊断/流体物种/...）
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

KEEP_TEMPLATES = ("<species>", "<species_name>", "<diag_name>")

DROP_EXACT = {
    # R3 Studio 自己控制的东西
    "amrex.omp_threads", "warpx.used_inputs_file", "amrex.abort_on_unused_inputs",
    # R4 别名
    "warpx.max_step", "warpx.stop_time", "warpx.verbose", "warpx.authors", "authors",
    "verbose",   # 裸名 max_step / stop_time / warpx.cfl / warpx.const_dt 是唯一的正式写法，保留
    # R5 内部旋钮
    "warpx.poisson_solver", "algo.evolve_scheme", "restart_length", "time_chunk_size",
    "warpx.boost_direction", "warpx.gamma_boost", "warpx.do_multi_J",
    "warpx.safe_guard_cells", "warpx.build_eb_data_per_level",
    "warpx.limit_verbose_step", "warpx.always_warn_immediately",
    "warpx.compute_max_step_from_btd", "warpx.zmax_plasma_to_compute_max_step",
    "warpx.serialize_ics", "warpx.serialize_initial_conditions",
    "warpx.break_signals", "warpx.checkpoint_signals", "warpx.use_kspace_filter",
    "warpx.write_diagnostics_on_restart", "warpx.abort_on_warning_threshold",
    "warpx.random_seed",
}

DROP_RE = re.compile(
    r"^(amrex|ablastr)\."                       # R2
    r"|^warpx\.self_fields_"                    # R5
    r"|^warpx\.magnetostatic_solver_"           # R5
    r"|^warpx\.usesingle"                       # R5
    r"|^warpx\.usesingle"                       # 拼写兼容
    r"|^warpx\.restart$"                        # 由 Studio 的续算按钮管理
)


def example_used_names() -> set[str]:
    """参数名集合：出现在 Examples/ 下任何 inputs 文件里的名字。

    文档提取对少数参数（例如 warpx.eb_implicit_function）会得到空串，但它们是最常用的
    用户参数；用"示例里用过"来兜底，避免把关键参数裁掉。
    """
    root = pathlib.Path(__file__).resolve().parents[2] / "Examples"
    used: set[str] = set()
    pat = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_.()\-]*)\s*=")
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix in {".py", ".rst", ".md", ".json", ".txt"}:
            continue
        if "inputs" not in path.name:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for line in text.splitlines():
            if line.lstrip().startswith("#"):
                continue
            m = pat.match(line)
            if m:
                used.add(m.group(1))
    return used


def keep(name: str, doc: str | None, used: set[str] | None = None) -> bool:
    if not (doc or "").strip() and name not in (used or set()):
        return False                                   # R1
    if name.startswith("<"):
        return any(name.startswith(t) for t in KEEP_TEMPLATES)   # R6
    if DROP_RE.search(name):
        return False
    return name not in DROP_EXACT


def main() -> int:
    catalog_path = DATA / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    before = len(catalog["params"])
    used = example_used_names()
    catalog["params"] = [p for p in catalog["params"] if keep(p["name"], p.get("doc"), used)]
    kept = {p["name"] for p in catalog["params"]}

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for category in cats["categories"]:
        category["params"] = [n for n in category["params"] if n in kept]
    cats["categories"] = [c for c in cats["categories"] if c["params"]]

    catalog["counts"]["total"] = len(catalog["params"])
    catalog["counts"]["documented"] = len(catalog["params"])
    catalog["counts"]["source_only"] = 0
    catalog_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"参数 {before} → {len(catalog['params'])}（-{before - len(catalog['params'])}）")
    for c in sorted(cats["categories"], key=lambda x: x["order"]):
        print(f"  {c['name']}: {len(c['params'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
