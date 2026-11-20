#!/usr/bin/env python3
"""Dropdown values for parameters that only accept a fixed set of words.

Three sources, in order of authority:

1. the **source scan** (``catalog.json`` → ``options``): values the C++ compares against,
   e.g. ``<collision_name>.type``;
2. a **curated table** below, for switches whose values are compared through helper
   functions or defined in the documentation bullet list.  Each entry names where it came
   from so it can be re-checked instead of trusted blindly;
3. the **type**: ``bool`` / "0 or 1" becomes 0/1, and small integer ranges (particle shape)
   become their own enumeration.

The result is ``data/enums.json``: ``{param: [value, ...]}``.  The UI turns every parameter
with an entry into a dropdown, so nobody has to type "TimeAveraged" from memory.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
CATALOG = DATA / "catalog.json"
OUT = DATA / "enums.json"

# name -> (values, where the values come from)
CURATED: dict[str, tuple[list[str], str]] = {
    "algo.maxwell_solver": (["yee", "ckc", "psatd", "ect"], "Source: ElectromagneticSolverAlgo"),
    "algo.particle_pusher": (["boris", "vay", "hybrid", "kinetic_energy", "adaptive_boris"],
                             "docs: Numerics and algorithms"),
    "algo.current_deposition": (["esirkepov", "direct", "vay"], "docs: current deposition"),
    "algo.field_gathering": (["energy-conserving", "momentum-conserving", "direct"],
                             "docs: field gathering"),
    "algo.em_solver_medium": (["vacuum", "macroscopic"], "docs: EM solver medium"),
    "algo.particle_shape": (["1", "2", "3", "4"], "docs: 1 linear … 4 quartic"),
    "<diag_name>.diag_type": (["Full", "TimeAveraged", "BackTransformed",
                               "BoundaryScraping"],
                              "Source: Diagnostics (string compare)"),
    "<diag_name>.format": (["plotfile", "checkpoint", "openpmd", "ascent", "sensei",
                            "catalyst"],
                           "Source: Diagnostics/FlushFormats"),
    "<diag_name>.openpmd_backend": (["h5", "bp", "json"], "docs: openPMD backend"),
    "<species_name>.injection_style": (
        ["NUniformPerCell", "NRandomPerCell", "NFluxPerCell", "gaussian_beam",
         "external_file", "singleparticle", "multipleparticles", "none"],
        "Source: PlasmaInjector.cpp string compares"),
    "<species_name>.profile": (["constant", "parse_density_function", "read_from_file"],
                               "Source: SpeciesUtils.cpp parseDensity"),
    "<species_name>.momentum_distribution_type": (
        ["at_rest", "constant", "uniform", "gaussian", "gaussianflux", "maxwellian",
         "maxwell_juttner", "parse_momentum_function"], "Source: SpeciesUtils.cpp"),
    "<collision_name>.type": (
        ["pairwisecoulomb", "dsmc", "background_mcc", "background_stopping",
         "nuclearfusion", "bremsstrahlung", "inverse_bremsstrahlung",
         "linear_breit_wheeler", "linear_compton", "pulsed_decay",
         "hybrid_resistive_drag"], "Source: CollisionHandler.cpp dispatch"),
    "<laser_name>.profile": (["gaussian", "parse_field_function", "from_file"],
                             "Source: LaserParticleContainer.cpp"),
    "boundary.field_lo": (["periodic", "pml", "absorbing_silver_mueller", "damped", "pec",
                           "pmc", "pec_insulator", "neumann", "open", "none"],
                          "docs: boundary.field_lo/hi bullets"),
    "boundary.field_hi": (["periodic", "pml", "absorbing_silver_mueller", "damped", "pec",
                           "pmc", "pec_insulator", "neumann", "open", "none"],
                          "docs: boundary.field_lo/hi bullets"),
    "boundary.particle_lo": (["periodic", "absorbing", "reflecting", "thermal", "none"],
                             "docs: boundary.particle_lo/hi bullets"),
    "boundary.particle_hi": (["periodic", "absorbing", "reflecting", "thermal", "none"],
                             "docs: boundary.particle_lo/hi bullets"),
    "geometry.dims": (["1", "2", "3", "RZ"], "docs: dimensionality"),
    "algo.macroscopic_subpixel": (["0", "1"], "docs: algo.macroscopic_subpixel"),
    "algo.macroscopic_sigma_method": (["backwardeuler", "laxwendroff"],
                                      "docs: algo.macroscopic_sigma_method"),
    "algo.em_solver_medium": (["vacuum", "macroscopic"], "docs: EM solver medium"),
    "warpx.E_ext_grid_init_style": (["default", "constant", "parse_e_ext_grid_function",
                                     "read_from_file", "load_from_python"],
                                    "Source: Initialization/ExternalField.cpp"),
    "warpx.B_ext_grid_init_style": (["default", "constant", "parse_b_ext_grid_function",
                                     "read_from_file", "load_from_python", "none"],
                                    "Source: Initialization/ExternalField.cpp"),
}

BOOL_LIKE = re.compile(r"\b(bool|boolean|0 or 1|1 or 0)\b", re.I)

# 每个取值到底是什么意思：下拉框里显示为选项的悬浮说明，选中后再显示一行提示。
# 只写能确定的内容；不确定的不写，避免编造。
GLOSSARY: dict[str, dict[str, str]] = {
    "algo.maxwell_solver": {
        "yee": "标准 FDTD（二阶精度，最常用，支持 PML/EB）",
        "ckc": "Cole-Karkkainen 低色散 FDTD（省网格，稳定性稍差）",
        "psatd": "伪谱方法（需 FFT 构建，色散极小，适合长距离传播）",
        "ect": "Enlarged Cell Technique，共形/嵌入式边界的替代方案",
    },
    "algo.particle_pusher": {
        "boris": "经典 Boris 推进器，默认、最稳",
        "vay": "Vay 推进器，强场/大加速度下更准",
        "hybrid": "Boris 与 Vay 的混合（按 |E|/|B| 切换）",
        "kinetic_energy": "能量守恒推进器，长时间积分不漂移",
        "adaptive_boris": "自适应 Boris，按局部磁化程度调整",
    },
    "algo.current_deposition": {
        "esirkepov": "Esirkepov 格式，严格电荷守恒（默认推荐）",
        "direct": "直接沉积，最快但不守恒",
        "vay": "Vay 格式，低噪声",
    },
    "algo.field_gathering": {
        "energy-conserving": "能量守恒收集（默认，噪声低）",
        "momentum-conserving": "动量守恒收集",
        "direct": "直接插值，最快",
    },
    "algo.macroscopic_subpixel": {
        "0": "关闭：按格点取材料，斜/曲面界面被阶梯化（一阶几何）",
        "1": "开启：按体积分数做方向性平均（法向调和/切向算术），保持二阶",
    },
    "algo.macroscopic_sigma_method": {
        "backwardeuler": "一阶隐式，稳健（默认）",
        "laxwendroff": "二阶半隐式，易振荡、需更小时间步",
    },
    "algo.em_solver_medium": {
        "vacuum": "真空（标准 Maxwell）",
        "macroscopic": "宏观介质（含介电张量/电导率/磁化）",
    },
    "algo.particle_shape": {
        "1": "线性样条（最快，噪声最大）",
        "2": "二次样条",
        "3": "三次样条（常用折中）",
        "4": "四次样条（最平滑，最慢）",
    },
    "<diag_name>.diag_type": {
        "Full": "整场快照：每个输出时刻写全场",
        "TimeAveraged": "时间平均场：按窗口平均后输出",
        "BackTransformed": "背变换/运动窗口：适合激光尾场",
        "BoundaryScraping": "边界刮削：统计穿过/打在指定边界的粒子（常配 openPMD）",
    },
    "<collision_name>.type": {
        "pairwisecoulomb": "两体库仑（ee/ei/ii）；用 CoulombLog，可选能量/动量修正",
        "dsmc": "DSMC：按 scattering_processes 抽样弹性/激发/电离/电荷交换",
        "background_mcc": "背景气体 MCC：电子/离子与中性背景碰撞，可含电离",
        "background_stopping": "背景阻止：带电粒子在背景中的能量损失",
        "nuclearfusion": "核聚变：D-T / D-D / D-He3 / p-B，生成产物粒子",
        "bremsstrahlung": "韧致辐射：电子-离子碰撞产生光子",
        "inverse_bremsstrahlung": "逆韧致辐射：光子被吸收/电子被加热",
        "linear_breit_wheeler": "线性 Breit-Wheeler：γ+γ → e+ + e−",
        "linear_compton": "线性 Compton：γ+e → γ+e",
        "pulsed_decay": "脉冲衰变：不稳定粒子按 decay_rate 衰变",
        "hybrid_resistive_drag": "混合 PIC 电阻拖曳（无专属输入参数）",
    },
    "<laser_name>.profile": {
        "gaussian": "高斯时空包络：profile_waist/duration/t_peak 等",
        "parse_field_function": "用 field_function(X,Y,t) 给激光场",
        "from_file": "从 binary/lasy 文件读激光场",
    },
    "<diag_name>.format": {
        "plotfile": "AMReX 原生 plotfile（最快）",
        "checkpoint": "检查点，可用来续算",
        "openpmd": "openPMD（HDF5/BP），ParaView 可读，实时可视化用这个",
        "ascent": "Ascent 原位可视化",
        "sensei": "Sensei 原位可视化",
        "catalyst": "Catalyst 原位可视化（需带 Catalyst 的 ParaView）",
    },
    "<diag_name>.openpmd_backend": {
        "h5": "HDF5（单文件，ParaView 直接读）",
        "bp": "ADIOS2 BP（并行 I/O 更快）",
        "json": "JSON（只适合小数据/调试）",
    },
    "<species_name>.injection_style": {
        "NUniformPerCell": "每格固定宏粒子数，密度由 profile 决定（最常用）",
        "NRandomPerCell": "每格粒子数随机（泊松涨落更真实）",
        "NFluxPerCell": "按通量注入（连续源/束流）",
        "gaussian_beam": "高斯束团（给定发射度/束腰）",
        "external_file": "从文件读粒子",
        "singleparticle": "单个粒子（测试用）",
        "multipleparticles": "若干指定粒子（测试用）",
        "none": "不注入（由别处产生）",
    },
    "<species_name>.profile": {
        "constant": "常数密度（配合 density 使用）",
        "parse_density_function": "用表达式给密度（density_function(x,y,z)）",
        "read_from_file": "从文件读密度场",
    },
    "<species_name>.momentum_distribution_type": {
        "constant": "常数动量（冷等离子体，速度由 ux/uy/uz 给）",
        "maxwellian": "麦克斯韦分布（给温度）",
        "maxwell_juttner": "相对论麦克斯韦-于特纳分布（高温）",
        "uniform": "均匀分布（ux_min/ux_max 等）",
        "gaussian": "高斯分布（ux_m/ux_th 等）",
        "gaussianflux": "高斯通量（仅 NFluxPerCell）",
        "at_rest": "静止（动量为 0）",
        "parse_momentum_function": "用表达式给动量（momentum_function_ux…）",
    },
    "boundary.field_lo": {
        "periodic": "周期（对应的另一端也必须是周期）",
        "pml": "完美匹配吸收层（电磁波无反射，网格外/内加层）",
        "absorbing_silver_mueller": "Silver-Müller 吸收边界（仅 Yee 求解器）",
        "damped": "阻尼层（简易吸收）",
        "pec": "完美电导体（金属壁，反射）",
        "pmc": "完美磁导体",
        "pec_insulator": "PEC 上加绝缘层",
        "neumann": "Neumann 外推边界",
        "open": "开放边界（场自由流出）",
        "none": "不设边界条件（高级用法）",
    },
    "boundary.particle_lo": {
        "periodic": "粒子周期穿越",
        "absorbing": "粒子打到边界被吸收",
        "reflecting": "粒子镜面反射",
        "thermal": "热化边界（按壁温重新发射）",
        "none": "不做粒子边界处理",
    },
}


# 语义重复/派生的参数：只保留覆盖面最广的那一套，其余默认折叠并说明被谁覆盖。
# 判据都来自代码：`warpx.do_pml` 在本 fork 里会直接 abort；`*_names` 由可重复组自动生成；
# `boundary.*_lo/hi` 是文档把 lo 与 hi 合成一行的写法，真实键是 lo 与 hi 两个。
DERIVED: dict[str, str] = {
    "warpx.do_pml": "已废弃：本 fork 会报 \"do_pml is not supported anymore\"，请改用 "
                    "boundary.field_lo / boundary.field_hi = pml",
    "boundary.field_lo/hi": "文档别名：真实输入是 boundary.field_lo 与 boundary.field_hi 两行，"
                            "上面已分别提供",
    "boundary.particle_lo/hi": "文档别名：真实输入是 boundary.particle_lo 与 boundary.particle_hi 两行",
    "particles.species_names": "由「粒子与物种」卡片自动生成：加一张卡片就自动加一个名字",
    "diagnostics.diags_names": "由「诊断与输出」卡片自动生成",
    "diagnostics.reduced_diags_names": "由「归约诊断」卡片自动生成",
    "collisions.collision_names": "由「碰撞与散射」卡片自动生成",
    "lasers.names": "由「激光」卡片自动生成",
}

# 扫描时把 "前缀 + 字面量" 的拼接键当成参数收进来了，这些片段不是真正的输入项：
# 它们是某个运行期字符串的后半截（例如 Bfield_fp、dBdx），去掉。
FRAGMENTS = [
    "Bfield_fp", "Efield_fp", "phi_fp", "beta", "consolidation", "dBdx", "dEdx", "delay",
    "density_min", "density_max", "direction", "electron", "element", "energy", "eta",
    "extension", "field", "file", "format", "index", "ion", "iter", "length", "level",
    "mass", "name", "number", "order", "particle", "path", "photon", "position", "prefix",
    "rho", "scale", "species", "spin", "step", "temperature", "time", "type", "value",
    "velocity", "weight", "width",
]


# 这些键的"取值含义"是结构性的（每维一个数/时间/步数），文档里也只是一句话
STRUCT_HINT = {
    "amr.n_cell": "每个方向一个整数，个数 = 维度（2D 是 x z，如 64 64）",
    "geometry.prob_lo": "每个方向一个实数，个数 = 维度",
    "geometry.prob_hi": "每个方向一个实数，个数 = 维度",
    "amr.max_grid_size": "一个整数（每块最大点数，越大越省并行开销）",
    "amr.blocking_factor": "一个整数，块大小必须是它的倍数",
    "amr.max_level": "整数：0 = 不加密",
    "max_step": "整数：跑到第几步停",
    "stop_time": "实数（秒）：跑到这个物理时间停",
    "warpx.cfl": "0–1 之间的实数；越小越稳越慢（常用 0.9–0.99）；"
        "电磁显式 dt=cfl·min(dx,dz)/c；const_dt 优先",
    "warpx.const_dt": "固定时间步长 dt（秒）；总时长≈max_step×dt；覆盖 cfl；示例 1.0e-15",
    "warpx.verbose": "整数：0 安静 / 1 每步一行 / 2 更详细",
    "boundary.field_lo/hi": "文档把 lo/hi 合成一行；实际要给 boundary.field_lo 和 "
                            "boundary.field_hi 各一行，每维一个：periodic / pml / pec / "
                            "absorbing_silver_mueller / damped / neumann / open / none",
    "boundary.particle_lo/hi": "每维一个：periodic / absorbing / reflecting / thermal / none",
    "boundary.field_lo": "每维一个：periodic / pml / pec / absorbing_silver_mueller / "
                         "damped / neumann / open / none",
    "boundary.field_hi": "每维一个（与 field_lo 对应；pml 需两端都是 pml）",
    "boundary.particle_lo": "每维一个：periodic / absorbing / reflecting / thermal / none",
    "boundary.particle_hi": "每维一个：periodic / absorbing / reflecting / thermal / none",
    "<species_name>.ux_mean": "归一化平均动量 ⟨u_x⟩=⟨γv_x/c⟩，无量纲；0.1≈0.1c；"
        "不是 kg·m/s/m/s；仅 maxwellian/MJ 的 constant 均值子类型",
    "<species_name>.uy_mean": "归一化平均动量 ⟨u_y⟩=⟨γv_y/c⟩，无量纲；0.1≈0.1c",
    "<species_name>.uz_mean": "归一化平均动量 ⟨u_z⟩=⟨γv_z/c⟩，无量纲；0.1≈0.1c",
    "<species_name>.ux_std": "归一化动量标准差 σ_ux（热展宽），无量纲；0.01≈1%c；"
        "不是温度，温度分支用 temperature_in_eV",
    "<species_name>.uy_std": "归一化动量标准差 σ_uy（热展宽），无量纲；0.01≈1%c",
    "<species_name>.uz_std": "归一化动量标准差 σ_uz（热展宽），无量纲；0.01≈1%c",
    "<species_name>.ux_m": "gaussian 分布中心/平均归一化动量 ux_m，无量纲；0.1≈0.1c；"
        "与 maxwellian 的 ux_mean 不同",
    "<species_name>.uy_m": "gaussian 分布中心/平均归一化动量 uy_m，无量纲；0.1≈0.1c",
    "<species_name>.uz_m": "gaussian 分布中心/平均归一化动量 uz_m，无量纲；0.1≈0.1c",
    "<species_name>.flux": "物理粒子通量 Γ=n·v（m^-2 s^-1）；num_particles_per_cell "
        "只影响宏粒子权重/噪声，不改变物理通量",
    "<species_name>.num_particles_per_cell": "每格抽样宏粒子数；NFluxPerCell 时可为小数，"
        "只影响权重/噪声；NRandomPerCell 时是每格物理粒子数",
    "<species_name>.do_classical_radiation_reaction": "1 = 开启（仅 species_type=electron/positron "
        "且 Boris 推进器；离子/中子/未指定类型会 abort）/ 0 = 关闭",
}

# 模板类参数的取值含义（按子键名匹配，对所有实例生效）
TEMPLATE_HINT = {
    "charge": "电荷：-q_e（电子）/ q_e（质子）或数值，单位 C",
    "mass": "质量：m_e / m_p 或数值，单位 kg",
    "density": "数密度，单位 m^-3（如 1.e24）",
    "density_max": "密度峰值，单位 m^-3",
    "num_particles_per_cell_each_dim": "每方向每格宏粒子数，个数 = 维度（2D 写 1 1）",
    "intervals": "整数：每多少步输出/执行一次（0 表示关闭）",
    "period": "整数：BackTransformed 诊断的变换周期（步）",
    "file_prefix": "输出文件名/目录前缀（不含扩展名）",
    "fields_to_plot": "要输出的场分量，空格分隔：Ex Ey Ez Bx By Bz rho jx jy jz",
    "write_species": "1 = 同时写粒子数据，0 = 只写场",
    "xmin": "注入区下界（米）",
    "xmax": "注入区上界（米）",
    "ymin": "注入区下界（米）",
    "ymax": "注入区上界（米）",
    "zmin": "注入区下界（米）",
    "zmax": "注入区上界（米）",
}


def value_hint(param: dict, values: list[str] | None = None,
               glossary: dict[str, str] | None = None) -> str:
    """一个参数"填什么值代表什么"——每个参数都要有，不只是枚举。

    * 有取值列表：列出值 + 含义（有词典就用词典）；
    * 布尔量：0 关闭 / 1 开启；
    * 列表型（每维一个）：说明要按维度给几个数；
    * 数值/字符串：说明类型与默认值。
    """
    name = param["name"]
    if name in STRUCT_HINT:
        return STRUCT_HINT[name]
    if name.startswith("<"):
        sub = name.split(".", 1)[1] if "." in name else ""
        if sub in TEMPLATE_HINT:
            return TEMPLATE_HINT[sub]
    typ = str(param.get("type") or "").lower()
    default = param.get("default")
    suffix = f"；默认 {default}" if default not in (None, "") else ""
    options = values if values else (param.get("options") or [])
    if options == ["0", "1"] or BOOL_LIKE.search(typ):
        return "0 = 关闭，1 = 开启" + suffix
    if options:
        gloss = glossary or {}
        shown = [f"{value}={gloss[value]}" if gloss.get(value) else value
                 for value in options[:5]]
        more = " 等" if len(options) > 5 else ""
        return "可选：" + "，".join(shown) + more
    if "strings" in typ or "list" in typ or " per " in typ:
        return "多个值用空格分隔，个数按维度给" + suffix
    if "int" in typ:
        return "整数" + suffix
    if "float" in typ or "real" in typ:
        return "数值（可写表达式，如 1.0e-6 或 0.1+2.3）" + suffix
    if "string" in typ:
        return "文本" + suffix
    return ("见说明" if param.get("doc") else "文本或数值，按参数说明填写") + suffix


def main() -> int:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    enums: dict[str, list[str]] = {}
    origin: dict[str, str] = {}

    for name, (values, where) in CURATED.items():
        enums[name] = values
        origin[name] = where

    for param in catalog["params"]:
        name = param["name"]
        if param.get("options") and name not in enums:
            enums[name] = param["options"]
            origin[name] = "source scan (string compares)"
        elif BOOL_LIKE.search(str(param.get("type") or "")) and name not in enums:
            enums[name] = ["0", "1"]
            origin[name] = "type bool"

    # particle-level external fields mirror the grid ones
    for field in ("E", "B"):
        for name in (f"<species_name>.{field}_ext_particle_init_style",
                     f"<species_name>.{field}_ext_init_style"):
            if name in {p["name"] for p in catalog["params"]}:
                enums[name] = ["none", "constant", f"parse_{field.lower()}_ext_particle_function",
                               "read_from_file"]
                origin[name] = "Source: Initialization"

    known0 = {p["name"] for p in catalog["params"]}
    derived = {name: why for name, why in DERIVED.items() if name in known0}
    for frag in FRAGMENTS:
        if frag in known0:
            derived[frag] = ("扫描时从 \"前缀 + 字面量\" 拼接键里截到的片段，"
                             "不是独立输入项（真正的键请用完整名）")
    hints = {p["name"]: value_hint(p, enums.get(p["name"]), GLOSSARY.get(p["name"]))
             for p in catalog["params"]}
    hints = {k: v for k, v in hints.items() if v}
    OUT.write_text(json.dumps({"_origin": origin, "enums": enums, "hints": hints,
                               "glossary": {k: v for k, v in GLOSSARY.items() if k in enums},
                               "derived": derived},
                              ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    known = {p["name"] for p in catalog["params"]}
    # the documentation folds the lo/hi boundary pair into one entry
    for alias in ("boundary.field_lo/hi", "boundary.particle_lo/hi"):
        base = alias.replace("_lo/hi", "_lo")
        if base in enums and alias in known:
            enums[alias] = enums[base]
            origin[alias] = origin.get(base, "")
    dropped = [n for n in enums if n not in known]
    for name in dropped:
        enums.pop(name, None)
        origin.pop(name, None)
    print(f"取值说明: {len(hints)}/{len(catalog['params'])} 个参数有中文取值提示")
    print(f"enums: {len(enums)} 个参数有下拉（{len([n for n in enums if n not in CURATED])} 个来自源码扫描），"
          f"{len(GLOSSARY)} 组取值带含义说明，{len(DERIVED)} 条衍生/重复参数")
    for name in list(enums)[:6]:
        print(f"  {name}: {enums[name][:5]}")
    if dropped:
        print("  目录里不存在的键（已跳过）:", dropped[:4])
    return 0


if __name__ == "__main__":
    sys.exit(main())
