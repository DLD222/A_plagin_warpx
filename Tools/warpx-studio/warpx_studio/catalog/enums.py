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
    "<diag_name>.diag_type": (["Full", "TimeAveraged", "BackTransformed"],
                              "Source: Diagnostics (string compare)"),
    "<diag_name>.format": (["plotfile", "checkpoint", "openpmd", "ascent", "sensei"],
                           "Source: Diagnostics/FlushFormats"),
    "<diag_name>.openpmd_backend": (["h5", "bp", "json"], "docs: openPMD backend"),
    "<species_name>.injection_style": (
        ["NUniformPerCell", "NRandomPerCell", "NFluxPerCell", "gaussian_beam",
         "external_file", "singleparticle", "multipleparticles", "none"],
        "Source: PlasmaInjector.cpp string compares"),
    "<species_name>.profile": (["constant", "parse_density_function", "read_from_file"],
                               "Source: SpeciesUtils.cpp parseDensity"),
    "<species_name>.momentum_distribution_type": (
        ["constant", "maxwellian", "maxwell_juttner", "parse_momentum_function",
         "read_from_file"], "Source: SpeciesUtils.cpp"),
    "<species_name>.position_initialization": (["standard", "random", "parse_position_function"],
                                                "docs: particle initialization"),
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
    "warpx.E_ext_grid_init_style": (["none", "constant", "parse_e_ext_grid_function",
                                     "read_from_file"], "Source: Initialization"),
    "warpx.B_ext_grid_init_style": (["none", "constant", "parse_b_ext_grid_function",
                                     "read_from_file"], "Source: Initialization"),
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
        "BackTransformed": "运动窗口/实验室系变换（适合激光尾场）",
    },
    "<diag_name>.format": {
        "plotfile": "AMReX 原生 plotfile（最快）",
        "checkpoint": "检查点，可用来续算",
        "openpmd": "openPMD（HDF5/BP），ParaView 可读，实时可视化用这个",
        "ascent": "Ascent 原位可视化",
        "sensei": "Sensei 原位可视化",
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
        "parse_momentum_function": "用表达式给动量（momentum_function_ux…）",
        "read_from_file": "从文件读动量分布",
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
    OUT.write_text(json.dumps({"_origin": origin, "enums": enums,
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
    print(f"enums: {len(enums)} 个参数有下拉（{len([n for n in enums if n not in CURATED])} 个来自源码扫描），"
          f"{len(GLOSSARY)} 组取值带含义说明，{len(DERIVED)} 条衍生/重复参数")
    for name in list(enums)[:6]:
        print(f"  {name}: {enums[name][:5]}")
    if dropped:
        print("  目录里不存在的键（已跳过）:", dropped[:4])
    return 0


if __name__ == "__main__":
    sys.exit(main())
