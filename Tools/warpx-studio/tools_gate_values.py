"""按"某个参数选了哪个值"来显示对应的后续参数。

例：<species>.profile
    constant              -> 只需 <species>.density
    parse_density_function-> 需要 <species>.density_function(x,y,z)
    read_from_file        -> 需要 <species>.read_density_from_path（+ 网格名）

同一套规则也用于 injection_style / momentum_distribution_type / 外场初始化方式。
规则写入 catalog.json 的 gating_rules，op 用 equals / empty（未填时按默认值处理）。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"


def eq(param: str, value: str) -> dict:
    return {"param": param, "op": "equals", "value": value, "raw": "value gate"}


def empty(param: str) -> dict:
    return {"param": param, "op": "empty", "value": None, "raw": "value gate"}


# 依赖参数 -> [(选择器, 需要的取值)]
VALUE_GATES: dict[str, list[tuple[str, str | None]]] = {}


def gate(dependents, selector, value):
    for d in dependents:
        VALUE_GATES.setdefault(d, []).append((selector, value))


S = "<species_name>."
# ---- 密度剖面 ----
gate([S + "density", S + "density_max", S + "density_min"], S + "profile", "constant")
gate([S + "density_function(x,y,z)"], S + "profile", "parse_density_function")
gate([S + "read_density_from_path", S + "density_mesh_name", S + "read_density_distributed",
      S + "read_temperature_in_eV_from_path", S + "temperature_in_eV_mesh_name",
      S + "read_u_mean_from_path", S + "read_u_std_from_path"], S + "profile", "read_from_file")
# 未填 profile 时 WarpX 默认 constant，所以 density 也要露出来
gate([S + "density"], S + "profile", None)

# ---- 注入方式 ----
gate([S + "num_particles_per_cell_each_dim", S + "xmin", S + "xmax", S + "zmin", S + "zmax"],
     S + "injection_style", "NUniformPerCell")
gate([S + "num_particles_per_cell"], S + "injection_style", "NRandomPerCell")
# 面注入的全部配套参数都必须挂在 injection_style 上，否则会散落到列表末尾
gate([S + "flux_normal_axis", S + "flux_direction", S + "flux_tmin", S + "flux_tmax",
      S + "flux_profile", S + "flux_function(x,y,z,t)", S + "surface_flux_pos", S + "flux",
      S + "zinject_plane", S + "injection_sources", S + "do_continuous_injection",
      S + "npart_real", S + "flux_tmin", S + "flux_tmax"],
     S + "injection_style", "NFluxPerCell")
gate([S + "inject_from_embedded_boundary"], S + "injection_style", "NFluxPerCell")
gate([S + "injection_file"], S + "injection_style", "external_file")
gate([S + "single_particle_pos", S + "single_particle_u", S + "single_particle_weight"],
     S + "injection_style", "singleparticle")
gate([S + "multiple_particles_pos_x", S + "multiple_particles_pos_y",
      S + "multiple_particles_pos_z", S + "multiple_particles_ux",
      S + "multiple_particles_uy", S + "multiple_particles_uz",
      S + "multiple_particles_weight"], S + "injection_style", "multipleparticles")
gate([S + "q_tot", S + "npart", S + "x_m", S + "y_m", S + "z_m", S + "x_rms", S + "y_rms",
      S + "z_rms", S + "focal_distance", S + "do_gaussian_beam_rotation",
      S + "gaussian_beam_rotation_angle", S + "gaussian_beam_rotation_axis"],
     S + "injection_style", "gaussian_beam")
# 未填 injection_style 时不铺粒子，无需展开

# ---- 动量分布 ----
gate([S + "ux", S + "uy", S + "uz"], S + "momentum_distribution_type", "constant")
gate([S + "ux_m", S + "uy_m", S + "uz_m", S + "ux_th", S + "uy_th", S + "uz_th"],
     S + "momentum_distribution_type", "gaussian")
gate([S + "ux_th", S + "uy_th", S + "uz_th", S + "maxwellian_u_mean_distribution_type"],
     S + "momentum_distribution_type", "maxwellian")
gate([S + "momentum_function_ux(x,y,z)", S + "momentum_function_uy(x,y,z)",
      S + "momentum_function_uz(x,y,z)"], S + "momentum_distribution_type",
     "parse_momentum_function")

# ---- 外部场初始化方式 ----
gate(["warpx.Bx_external_grid_function(x,y,z)", "warpx.By_external_grid_function(x,y,z)",
      "warpx.Bz_external_grid_function(x,y,z)"], "warpx.B_ext_grid_init_style",
     "parse_B_ext_grid_function")
gate(["warpx.Ex_external_grid_function(x,y,z)", "warpx.Ey_external_grid_function(x,y,z)",
      "warpx.Ez_external_grid_function(x,y,z)"], "warpx.E_ext_grid_init_style",
     "parse_E_ext_grid_function")


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})
    added = 0
    for name, conds in VALUE_GATES.items():
        if name not in names:
            continue
        rs = [empty(sel) if val is None else eq(sel, val) for sel, val in conds]
        existing = rules.get(name, [])
        merged = existing + [r for r in rs if r not in existing]
        if merged != existing:
            rules[name] = merged
            added += 1
    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"新增/更新 {added} 条取值门控，累计 {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
