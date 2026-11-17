"""按"功能开关"隐藏依赖参数：开关本身永远可见，打开后才展开它下面的参数。

用户要求：没电离就看不到电离参数、没开静电求解器就看不到静电参数、没开 QED 就看不到
QED 参数……但**开关那一行必须在**，这样才有地方把它打开。

写进 catalog.json 的 gating_rules（界面用它判断是否渲染某一行）：
  {param, op, value}，op = truthy / falsy / equals / nonempty
同一个参数有多条规则时取"或"（满足任一即显示）。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"


def rule(param: str, op: str = "truthy", value=None) -> dict:
    return {"param": param, "op": op, "value": value, "raw": "feature gate"}


# 依赖参数 -> 触发显示的条件（开关本身不在列表里）
GATES: dict[str, list[dict]] = {}

def gate(names, rules):
    for n in names:
        GATES.setdefault(n, []).extend(rules)


# 电离：只有 do_field_ionization 打开才显示
gate(["<species_name>.ionization_initial_level", "<species_name>.ionization_product_species",
      "<species_name>.do_adk_correction"],
     [rule("<species_name>.do_field_ionization")])

# 静电 / 静磁求解器：只有 warpx.do_electrostatic 不是 none 才显示
gate(["<species_name>.initialize_self_fields", "<species_name>.self_fields_absolute_tolerance",
      "<species_name>.self_fields_max_iters", "<species_name>.self_fields_required_precision",
      "boundary.potential_lo_x", "boundary.potential_hi_x", "boundary.potential_lo_y",
      "boundary.potential_hi_y", "boundary.potential_lo_z", "boundary.potential_hi_z"],
     [rule("warpx.do_electrostatic", "equals", "none"), rule("warpx.do_electrostatic", "nonempty")])

# QED：各物种开关任一打开才显示对应参数
gate(["<species_name>.qed_breit_wheeler_ele_product_species",
      "<species_name>.qed_breit_wheeler_pos_product_species"],
     [rule("<species_name>.do_qed_breit_wheeler")])
gate(["<species_name>.qed_quantum_sync_phot_product_species"],
     [rule("<species_name>.do_qed_quantum_sync")])
gate(["<species>.qed_virtual_photons_do_beam_size_effect"],
     [rule("<species>.do_qed_virtual_photons")])
gate(["qed_qs.chi_min", "qed_qs.photon_creation_energy_threshold", "qed_qs.lookup_table_mode"],
     [rule("<species_name>.do_qed_quantum_sync"), rule("<species>.do_qed_virtual_photons")])
gate(["qed_bw.chi_min", "qed_bw.lookup_table_mode"],
     [rule("<species_name>.do_qed_breit_wheeler")])
gate(["qed_schwinger.xmin", "qed_schwinger.xmax", "qed_schwinger.ymin", "qed_schwinger.ymax",
      "qed_schwinger.zmin", "qed_schwinger.zmax", "qed_schwinger.y_size",
      "qed_schwinger.threshold_poisson_gaussian", "qed_schwinger.ele_product_species",
      "qed_schwinger.pos_product_species"],
     [rule("<species_name>.do_qed_schwinger"), rule("warpx.quantum_xi", "nonempty")])

# 碰撞：填了 collision_names 才显示其它碰撞参数
gate([n for n in ("collisions.beta_weight_exponent", "collisions.correct_energy_momentum",
                  "collisions.energy_correction_sort_by_weight", "collisions.energy_fraction",
                  "collisions.np_warning_threshold", "collisions.shuffling_method",
                  "collisions.split_momentum_push")],
     [rule("collisions.collision_names", "nonempty")])

# 激光：填了 lasers.names 才显示
gate(["lasers.deposit_on_main_grid"], [rule("lasers.names", "nonempty")])

# 缩减诊断：填了名字才显示
gate(["reduced_diags.extension", "reduced_diags.intervals", "reduced_diags.path",
      "reduced_diags.precision", "reduced_diags.separator"],
     [rule("warpx.reduced_diags_names", "nonempty")])

# 移动窗口：开关打开才显示其余
gate(["warpx.end_moving_window_step", "warpx.moving_window_dir", "warpx.moving_window_v",
      "warpx.start_moving_window_step", "warpx.num_mirrors", "warpx.mirror_z",
      "warpx.mirror_z_width", "warpx.mirror_z_npoints"],
     [rule("warpx.do_moving_window")])

# div 清理：各自开关打开才显示其数值参数
gate(["warpx.projection_div_cleaner.atol", "warpx.projection_div_cleaner.rtol"],
     [rule("warpx.do_divb_cleaning"), rule("warpx.do_dive_cleaning")])
gate(["warpx.do_pml_divb_cleaning", "warpx.do_pml_dive_cleaning", "warpx.do_pml_j_damping",
      "warpx.pml_has_particles", "warpx.pml_ncell", "warpx.do_similar_dm_pml",
      "warpx.do_pml_in_domain", "warpx.pml_delta"],
     [rule("warpx.do_pml_divb_cleaning"), rule("warpx.do_pml_dive_cleaning"),
      rule("boundary.field_lo", "contains", "pml"), rule("boundary.field_hi", "contains", "pml")])

# EB：给了几何才显示其余 EB 参数（几何本身与 geom_type 除外）
gate(["boundary.particle_eb", "warpx.eb_potential(x,y,z,t)", "eb2.stl_file"],
     [rule("warpx.eb_implicit_function", "nonempty"), rule("eb2.geom_type", "equals", "stl")])

# hybrid：只有选 hybrid 求解器才显示 hybrid_pic_model.*
gate([n for n in ("hybrid_pic_model.elec_temp", "hybrid_pic_model.n0_ref",
                  "hybrid_pic_model.gamma", "hybrid_pic_model.n_floor",
                  "hybrid_pic_model.substeps", "hybrid_pic_model.use_rkf45",
                  "hybrid_pic_model.max_substep_attempts", "hybrid_pic_model.substep_rtol",
                  "hybrid_pic_model.substep_atol", "hybrid_pic_model.substep_safety",
                  "hybrid_pic_model.substep_max_growth", "hybrid_pic_model.plasma_resistivity",
                  "hybrid_pic_model.plasma_hyper_resistivity(rho,B)",
                  "hybrid_pic_model.add_external_fields",
                  "hybrid_pic_model.include_joule_heating",
                  "hybrid_pic_model.solve_electron_energy_equation",
                  "hybrid_pic_model.holmstrom_vacuum_region",
                  "hybrid_pic_model.vacuum_seam_switch_mode")],
     [rule("algo.maxwell_solver", "equals", "hybrid")])

# PSATD：只有选 psatd 才显示 psatd.*
gate([n for n in ("psatd.periodic_single_box_fft", "psatd.current_correction",
                  "psatd.do_time_averaging", "psatd.JRhom", "psatd.update_with_rho",
                  "psatd.v_galilean", "psatd.v_comoving", "psatd.use_default_v_galilean",
                  "psatd.use_default_v_comoving", "psatd.nx_guard", "psatd.ny_guard",
                  "psatd.nz_guard")],
     [rule("algo.maxwell_solver", "equals", "psatd")])

# 宏观介质：只有选 macroscopic 才显示
gate([n for n in ("macroscopic.epsilon", "macroscopic.epsilon2", "macroscopic.sigma",
                  "macroscopic.mu", "algo.macroscopic_subpixel",
                  "algo.macroscopic_sigma_method")],
     [rule("algo.em_solver_medium", "equals", "macroscopic")])

# 外部场：只有初始化方式不是 none 才显示取值
gate(["warpx.B_external_grid"], [rule("warpx.B_ext_grid_init_style", "notequals", "none")])
gate(["warpx.E_external_grid"], [rule("warpx.E_ext_grid_init_style", "notequals", "none")])

# 粒子重采样/分裂/辐射反作用：各自开关打开才显示细节
gate(["<species_name>.resampling_algorithm", "<species_name>.resampling_trigger_intervals",
      "<species_name>.resampling_trigger_max_avg_ppc", "<species_name>.resampling_min_ppc"],
     [rule("<species_name>.do_resampling")])
gate(["<species_name>.split_type"], [rule("<species_name>.do_splitting")])


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})
    added = 0
    for name, rs in GATES.items():
        if name not in names:
            continue
        merged = list(rules.get(name, [])) + [r for r in rs if r not in rules.get(name, [])]
        if merged != rules.get(name):
            rules[name] = merged
            added += 1
    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"新增/更新 {added} 条功能门控规则，累计 {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
