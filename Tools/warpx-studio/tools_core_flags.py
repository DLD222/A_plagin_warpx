"""给参数打"常用"标记：界面默认只显示这些，其余折叠起来。

用户反复说"参数太多、又臭又长"。全部展开是 351 项，但一个正常的算例只需要十几项。
这里按"搭一个能跑的电磁/混合 PIC 算例需要改哪些"列出常用集，其余收进
「展开其余 N 项」，能力不减、默认界面变短。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

CORE = {
    # 网格与几何
    "geometry.dims", "amr.n_cell", "geometry.prob_lo", "geometry.prob_hi",
    "amr.max_grid_size", "amr.max_level",
    # 时间
    "max_step", "stop_time", "warpx.const_dt", "warpx.cfl",
    # 边界
    "boundary.field_lo", "boundary.field_hi", "boundary.particle_lo", "boundary.particle_hi",
    # 导体/EB
    "warpx.eb_implicit_function", "boundary.particle_eb", "eb2.geom_type",
    # 外场
    "warpx.B_ext_grid_init_style", "warpx.B_external_grid",
    "warpx.E_ext_grid_init_style", "warpx.E_external_grid",
    # 求解器
    "algo.maxwell_solver", "algo.current_deposition", "algo.particle_shape",
    # 物种
    "particles.species_names",
    "<species_name>.charge", "<species_name>.mass", "<species_name>.density",
    "<species_name>.injection_style", "<species_name>.momentum_distribution_type",
    "<species_name>.num_particles_per_cell_each_dim",
    "<species_name>.ux", "<species_name>.ux_th",
    "<species_name>.xmin", "<species_name>.xmax", "<species_name>.zmin", "<species_name>.zmax",
    "<species_name>.do_not_deposit",
    # 诊断
    "<diag_name>.intervals", "<diag_name>.fields_to_plot", "<diag_name>.format",
    "<diag_name>.diag_type", "<diag_name>.file_prefix", "<diag_name>.write_species",
    # hybrid
    "hybrid_pic_model.elec_temp", "hybrid_pic_model.n0_ref",
    "hybrid_pic_model.substeps", "hybrid_pic_model.use_rkf45",
    "hybrid_pic_model.n_floor", "hybrid_pic_model.plasma_resistivity",
}


def main() -> int:
    cat_path = DATA / "catalog.json"
    catalog = json.loads(cat_path.read_text(encoding="utf-8"))
    # 所有门控规则里的"选择器"都必须是常用项：开关一旦被折叠，用户就没法打开对应功能
    selectors = {r["param"] for rs in (catalog.get("gating_rules") or {}).values()
                 for r in rs if r.get("param")}
    core = 0
    for p in catalog["params"]:
        p["core"] = p["name"] in CORE or p["name"] in selectors
        core += int(p["core"])
    cat_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"常用参数 {core} / {len(catalog['params'])}；其余默认折叠")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
