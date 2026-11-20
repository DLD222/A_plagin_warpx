"""按源码重建"选择器取值 -> 它真正读取的参数集合"的门控规则。

这个脚本取代原先零散写在 tools_gate_values.py / tools_gate_momentum.py 里的
species 级规则。旧规则有几个结构性问题：

  * 扫描器把 if / else-if 分支的所有条件都记下来，再被当成"或"，于是
    `ux_th` 在 maxwellian 下也会出现（源码只在 gaussian/gaussianflux 读它）；
  * `profile`、`injection_style`、`momentum_distribution_type` 是**联合**条件：
    `density_function` 只有"按体积注入 + profile=parse_density_function"才读，
    旧规则拆成两条"或"，任一条成立就显示；
  * `read_temperature_in_eV_from_path` 被错误地挂到 profile=read_from_file 上，
    它其实只取决于温度子类型。

做法：对每个受管选择器，先删掉所有引用它的旧规则（不管来自扫描器还是手工脚本），
再按源码写入精确规则。规则形状与前端一致：
  - 一个 dict = 一个条件；
  - 一个 list[dict] = 这些条件"与"；
  - 多个规则之间"或"。

规则里的 `in` / `member` / `falsy` 由前端 ruleSatisfied() 和
warpx_studio/deck/validate.py:_rule_ok() 共同实现。

源码依据（行号会随重构漂移，注释里保留函数名，便于复查）：
  - SpeciesUtils.cpp: parseMomentum / parseDensity
  - PlasmaInjector.cpp: setupNFluxPerCell / setupNuniformPerCell /
    setupNRandomPerCell / setupSingleParticle / setupMultipleParticles /
    setupGaussianBeam / setupExternalFile / parseFlux
  - VelocityProperties.cpp: ParseVelocityVector
  - TemperatureProperties.cpp: parse_temperature_in_eV 与 maxwellian 分支
  - PhysicalParticleContainer.cpp: injection_sources / do_continuous_injection /
    initialize_self_fields
  - MultiParticleContainer.cpp: particles.rigid_injected_species 选择
    RigidInjectedParticleContainer -> zinject_plane / rigid_advance
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
S = "<species_name>."

# 会读取 profile / momentum 的注入方式（parseDensity / parseMomentum 的调用点）
VOLUME = "NUniformPerCell NRandomPerCell"                       # parseDensity
MOMENTUM = "NUniformPerCell NRandomPerCell NFluxPerCell gaussian_beam"  # parseMomentum
# gaussianflux 只允许 NFluxPerCell（SpeciesUtils.cpp 里 assert style == nfluxpercell）
FLUX = f"{S}injection_style"
MOM = f"{S}momentum_distribution_type"
PROFILE = f"{S}profile"
U_MEAN_MW = f"{S}maxwellian_u_mean_distribution_type"
U_STD_MW = f"{S}maxwellian_u_std_distribution_type"
T_MW = f"{S}maxwellian_temperature_in_eV_distribution_type"
U_MEAN_MJ = f"{S}maxwell_juttner_u_mean_distribution_type"
T_MJ = f"{S}maxwell_juttner_temperature_in_eV_distribution_type"


def cond(param: str, op: str = "equals", value=None) -> dict:
    return {"param": param, "op": op, "value": value, "raw": "selector gate"}


def eq(param: str, value: str) -> dict:
    return cond(param, "equals", value)


def empty(param: str) -> dict:
    return cond(param, "empty")


def truthy(param: str) -> dict:
    return cond(param, "truthy")


def falsy(param: str) -> dict:
    return cond(param, "falsy")


def in_(param: str, values: str) -> dict:
    return cond(param, "in", values)


def member(param: str) -> dict:
    return cond(param, "member")


def style(value: str) -> dict:
    return eq(FLUX, value)


def style_in(values: str) -> dict:
    return in_(FLUX, values)


# 依赖参数 -> 规则列表（规则之间"或"；list 内部"与"）
RULES: dict[str, list] = {}


def add(targets, *rules):
    for target in targets:
        RULES.setdefault(target, []).extend(rules)


# ---------------------------------------------------------------------------
# 1) 注入方式 injection_style
# ---------------------------------------------------------------------------
# 选择器本身永远可见（用户先选它，其它集合才出现），所以不给它加规则。
add([S + "profile"], style_in(VOLUME))
add([S + "momentum_distribution_type"], style_in(MOMENTUM))

# 体积注入：坐标范围、密度上下限、每格粒子数
for box in ("xmin", "xmax", "ymin", "ymax", "zmin", "zmax",
            "density_min", "density_max"):
    add([S + box], style_in(VOLUME))
add([S + "num_particles_per_cell_each_dim"], style("NUniformPerCell"))
add([S + "num_particles_per_cell"],
    style("NRandomPerCell"), style("NFluxPerCell"))

# 面注入
add([S + "flux_tmin", S + "flux_tmax", S + "inject_from_embedded_boundary",
     S + "flux_profile"], style("NFluxPerCell"))
# 从 EB 注入时不做平面参数：PlasmaInjector 在 else 分支才读这三个
add([S + "surface_flux_pos", S + "flux_normal_axis", S + "flux_direction"],
    [style("NFluxPerCell"), falsy(S + "inject_from_embedded_boundary")])
# 面通量本身由 flux_profile 再分一支：constant -> flux；parse_flux_function -> 函数
add([S + "flux"], [style("NFluxPerCell"), eq(S + "flux_profile", "constant")])
add([S + "flux_function(x,y,z,t)"],
    [style("NFluxPerCell"), eq(S + "flux_profile", "parse_flux_function")])

# 单粒子 / 多粒子
for p in ("single_particle_pos", "single_particle_u", "single_particle_weight"):
    add([S + p], style("SingleParticle"))
for p in ("multiple_particles_pos_x", "multiple_particles_pos_y",
          "multiple_particles_pos_z", "multiple_particles_ux",
          "multiple_particles_uy", "multiple_particles_uz",
          "multiple_particles_weight"):
    add([S + p], style("MultipleParticles"))

# 高斯束（setupGaussianBeam）
for p in ("x_m", "y_m", "z_m", "x_rms", "y_rms", "z_rms",
          "x_cut", "y_cut", "z_cut", "npart", "npart_real",
          "do_symmetrize", "symmetrization_order", "focal_distance",
          "do_gaussian_beam_rotation", "do_gaussian_beam_rotation_momenta"):
    add([S + p], style("gaussian_beam"))
# 旋转参数只有 do_gaussian_beam_rotation 打开才读
add([S + "gaussian_beam_rotation_angle", S + "gaussian_beam_rotation_axis"],
    [style("gaussian_beam"), truthy(S + "do_gaussian_beam_rotation")])

# 外部分布文件（setupExternalFile）
for p in ("injection_file", "z_shift", "impose_t_lab_from_file"):
    add([S + p], style("external_file"))
# q_tot 在 gaussian_beam 与 external_file 两个分支都会读
add([S + "q_tot"], style("gaussian_beam"), style("external_file"))

# 刚性注入由顶层 particles.rigid_injected_species 选择，而不是 injection_style
add([S + "zinject_plane", S + "rigid_advance"],
    member("particles.rigid_injected_species"))

# ---------------------------------------------------------------------------
# 2) 密度 profile（SpeciesUtils::parseDensity）
# ---------------------------------------------------------------------------
# density 只在 profile=constant（未填默认 constant）时读；
# density_min/max 是体积注入的密度上下限，与 profile 无关（上面已按 VOLUME 门控）。
add([S + "density"],
    [style_in(VOLUME), eq(PROFILE, "constant")],
    [style_in(VOLUME), empty(PROFILE)])                 # 未填默认 constant
add([S + "density_function(x,y,z)"],
    [style_in(VOLUME), eq(PROFILE, "parse_density_function")])
for p in ("read_density_from_path", "density_mesh_name", "read_density_distributed"):
    add([S + p], [style_in(VOLUME), eq(PROFILE, "read_from_file")])

# ---------------------------------------------------------------------------
# 3) 动量 momentum_distribution_type（SpeciesUtils::parseMomentum）
# ---------------------------------------------------------------------------
add([S + "ux", S + "uy", S + "uz"],
    [style_in(MOMENTUM), eq(MOM, "constant")])
add([S + "ux_m", S + "uy_m", S + "uz_m",
     S + "ux_th", S + "uy_th", S + "uz_th"],
    [style_in(MOMENTUM), eq(MOM, "gaussian")],
    # gaussianflux 在源码里只允许 NFluxPerCell
    [eq(FLUX, "NFluxPerCell"), eq(MOM, "gaussianflux")])
add([S + "ux_min", S + "ux_max", S + "uy_min", S + "uy_max",
     S + "uz_min", S + "uz_max"],
    [style_in(MOMENTUM), eq(MOM, "uniform")])
for p in ("momentum_function_ux(x,y,z)", "momentum_function_uy(x,y,z)",
          "momentum_function_uz(x,y,z)"):
    add([S + p], [style_in(MOMENTUM), eq(MOM, "parse_momentum_function")])

# maxwellian / maxwell_juttner 的子类型选择器本身
add([U_MEAN_MW, U_STD_MW, T_MW],
    [style_in(MOMENTUM), eq(MOM, "maxwellian")])
add([U_MEAN_MJ, T_MJ],
    [style_in(MOMENTUM), eq(MOM, "maxwell_juttner")])

# u_mean：constant（含未填默认）-> ux_mean/...；parser -> 函数；read_from_file -> 路径
for sub, mean, func, path in (
        (U_MEAN_MW, "ux_mean", "ux_mean_function(x,y,z)", "read_u_mean_from_path"),
        (U_MEAN_MJ, "ux_mean", "ux_mean_function(x,y,z)", "read_u_mean_from_path")):
    sub_mom = "maxwellian" if "maxwellian_" in sub else "maxwell_juttner"
    add([S + mean, S + "uy_mean", S + "uz_mean"],
        [style_in(MOMENTUM), eq(MOM, sub_mom), eq(sub, "constant")],
        [style_in(MOMENTUM), eq(MOM, sub_mom), empty(sub)])
    add([S + func, S + func.replace("ux", "uy"), S + func.replace("ux", "uz")],
        [style_in(MOMENTUM), eq(MOM, sub_mom), eq(sub, "parser")])
    add([S + path],
        [style_in(MOMENTUM), eq(MOM, sub_mom), eq(sub, "read_from_file")])

# maxwellian 的温度/热展宽：先看温度子类型；没填温度才用 u_std（默认 constant）
add([S + "ux_std", S + "uy_std", S + "uz_std"],
    [style_in(MOMENTUM), eq(MOM, "maxwellian"), eq(U_STD_MW, "constant"),
     empty(T_MW)],
    [style_in(MOMENTUM), eq(MOM, "maxwellian"), empty(U_STD_MW), empty(T_MW)])
for c in ("ux", "uy", "uz"):
    add([S + f"{c}_std_function(x,y,z)"],
        [style_in(MOMENTUM), eq(MOM, "maxwellian"),
         eq(U_STD_MW, "parser"), empty(T_MW)])
add([S + "read_u_std_from_path"],
    [style_in(MOMENTUM), eq(MOM, "maxwellian"),
     eq(U_STD_MW, "read_from_file"), empty(T_MW)])

# temperature_in_eV：maxwellian 只有显式选温度才读；maxwell_juttner 默认就是温度
add([S + "temperature_in_eV"],
    [style_in(MOMENTUM), eq(MOM, "maxwellian"), eq(T_MW, "constant")],
    [style_in(MOMENTUM), eq(MOM, "maxwell_juttner"), eq(T_MJ, "constant")],
    [style_in(MOMENTUM), eq(MOM, "maxwell_juttner"), empty(T_MJ)])
add([S + "temperature_in_eV_function(x,y,z)"],
    [style_in(MOMENTUM), eq(MOM, "maxwellian"), eq(T_MW, "parser")],
    [style_in(MOMENTUM), eq(MOM, "maxwell_juttner"), eq(T_MJ, "parser")])
add([S + "read_temperature_in_eV_from_path", S + "temperature_in_eV_mesh_name"],
    [style_in(MOMENTUM), eq(MOM, "maxwellian"), eq(T_MW, "read_from_file")],
    [style_in(MOMENTUM), eq(MOM, "maxwell_juttner"), eq(T_MJ, "read_from_file")])

# ---------------------------------------------------------------------------
# 4) 物种自场参数：由 initialize_self_fields 控制，而不是由 do_electrostatic
# ---------------------------------------------------------------------------
add([S + "self_fields_required_precision", S + "self_fields_absolute_tolerance",
     S + "self_fields_max_iters", S + "self_fields_verbosity"],
    truthy(S + "initialize_self_fields"))
# initialize_self_fields 本身是物种级开关，任何电磁/静电运行都可能用它；
# 它被旧 feature-gate 错误地挂在 warpx.do_electrostatic 上，这里清掉。
RULES.setdefault(S + "initialize_self_fields", [])

# 静电边界电势 / 全局自场容差：只有 do_electrostatic 真的选了一个非 none 的
# 求解器才相关。nonempty 在前端把 "" 和 "none" 都当空。
add(["boundary.potential_lo_x", "boundary.potential_hi_x",
     "boundary.potential_lo_y", "boundary.potential_hi_y",
     "boundary.potential_lo_z", "boundary.potential_hi_z"],
    cond("warpx.do_electrostatic", "nonempty"))
for p in ("warpx.self_fields_required_precision", "warpx.self_fields_absolute_tolerance",
          "warpx.self_fields_max_iters", "warpx.self_fields_verbosity",
          "warpx.magnetostatic_solver_required_precision",
          "warpx.magnetostatic_solver_absolute_tolerance",
          "warpx.magnetostatic_solver_max_iters",
          "warpx.magnetostatic_solver_verbosity"):
    add([p], cond("warpx.do_electrostatic", "nonempty"))

# 需要"先清掉旧规则"的选择器：任何引用这些选择器的旧规则都不可信。
OWNED_SELECTORS = {
    FLUX, PROFILE, MOM, U_MEAN_MW, U_STD_MW, T_MW, U_MEAN_MJ, T_MJ,
    S + "flux_profile", S + "do_gaussian_beam_rotation",
    "particles.rigid_injected_species", "warpx.do_electrostatic",
}


def references_owned(rule) -> bool:
    items = rule if isinstance(rule, list) else [rule]
    return any(isinstance(r, dict) and r.get("param") in OWNED_SELECTORS for r in items)


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})

    missing = sorted(set(RULES) - names)
    if missing:
        print("warning: rules reference unknown parameters:", ", ".join(missing[:10]))

    # 1) 删除所有引用受管选择器的旧规则（扫描器/旧 value gate/旧 momentum gate）
    removed = 0
    for name in list(rules):
        kept = [r for r in rules[name] if not references_owned(r)]
        if len(kept) != len(rules[name]):
            removed += len(rules[name]) - len(kept)
            if kept:
                rules[name] = kept
            else:
                del rules[name]

    # 2) 写入精确规则；空列表表示"永远可见"，直接删掉规则
    added = 0
    for name, rs in RULES.items():
        if name not in names:
            continue
        if rs:
            rules[name] = rs
            added += 1
        else:
            rules.pop(name, None)

    # 统一计数/去重（多个 gate 脚本都会改 params/rules）
    params = catalog["params"]
    counts = catalog.setdefault("counts", {})
    counts.update({
        "total": len(params),
        "documented": sum(1 for p in params if not p.get("source_only")),
        "source_only": sum(1 for p in params if p.get("source_only")),
        "with_runtime_gating": len(catalog.get("gating", {})),
        "with_gating_rules": len(rules),
        "with_options": sum(1 for p in params if p.get("options")),
        "required": sum(1 for p in params if p.get("required")),
    })
    for g in list(catalog.get("groups", {})):
        catalog["groups"][g] = sorted(set(catalog["groups"][g]))

    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"species 选择器门控: 删除旧规则 {removed} 条，写入 {added} 个参数，"
          f"gating_rules 累计 {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
