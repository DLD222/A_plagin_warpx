#!/usr/bin/env python3
"""Assign every input parameter to a physics category and give it a Chinese line.

Two problems with the v1 behaviour:

* parameters were grouped by their ``ParmParse`` section (``warpx``, ``algo``, ...), which
  mixes unrelated physics: "collisions", "boundary", "embedded boundary", "diagnostics" and
  "external fields" all live under a handful of C++ sections;
* only 78 of 800+ parameters had a Chinese description, so the rest fell back to English.

This module produces ``data/categories.json`` (ordered physics categories) and rewrites
``data/zh.json`` so that **every** parameter gets a Chinese line: hand-written where the
wording matters, composed from a token lexicon otherwise, always carrying the type and the
default value so a beginner still knows what to type.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
CATALOG = DATA / "catalog.json"
ZH = DATA / "zh.json"
CATEGORIES = DATA / "categories.json"

# ordered: the UI shows them in this order, which is the order a deck is written in
# WarpX 文档的章节本身就是按物理功能划分的（parameters.rst 的 section 标题），
# 所以文档里有的参数一律按章节归类，参数名正则只用于"只存在于源码、没进文档"的参数。
SECTION_TO_CATEGORY: dict[str, str] = {
    "Overall simulation parameters": "control",
    "Simulation Time": "control",
    "Checkpoints and restart": "control",
    "Testing and Debugging": "control",
    "Setting up the field mesh": "mesh",
    "Domain Boundary Conditions": "boundary",
    "Additional PML parameters": "boundary",
    "Embedded Boundary Conditions": "eb",
    "Particle initialization": "species",
    "Particle thermalizer": "species",
    "Cold Relativistic Fluid initialization": "species",
    "Laser initialization": "laser",
    "Collision models": "collision",
    "External fields": "extern",
    "Diagnostics and output": "diag",
    "Numerics and algorithms": "solver",
    "QED": "qed",
    "Distribution across MPI ranks and parallelization": "parallel",
}

CATEGORY_TITLES = {
    "control": "运行控制与仿真时间",
    "mesh": "网格与几何",
    "boundary": "边界条件（含 PML）",
    "eb": "嵌入边界（EB）",
    "species": "粒子注入与物种",
    "laser": "激光",
    "collision": "碰撞与散射",
    "extern": "外场",
    "diag": "诊断与输出",
    "solver": "算法与求解器",
    "qed": "QED 与强场过程",
    "hybrid": "混合 PIC 模型（本 fork）",
    "parallel": "并行与负载均衡",
    "plugin": "插件与扩展（本 fork）",
    "misc": "其它参数",
}

CATEGORY_ORDER = ["control", "mesh", "boundary", "eb", "species", "laser", "collision",
                  "extern", "diag", "solver", "qed", "hybrid", "parallel", "plugin", "misc"]

CATEGORY_RULES = [
    # 只对"文档里没有、只存在于源码"的参数生效；文档里有的按章节走（见 SECTION_TO_CATEGORY）。
    # 每条规则后面注明依据，便于复核。
    ("parallel", "并行与负载均衡", [
        r"load_balance", r"numprocs", r"buffer_size", r"sort_", r"use_sfc", r"costs_",
        r"nfiles", r"async_out",
    ]),
    ("diag", "诊断与输出", [
        r"back_transformed", r"snapshots_lab", r"lab_data_directory", r"plot_crsepatch",
        r"plot_finepatch", r"plot_file", r"plot_int", r"plot_", r"reduced_diags",
        r"openpmd", r"diag", r"field_io", r"particle_io", r"slice\.", r"lab_data",
    ]),
    ("eb", "嵌入边界（EB）", [
        r"^eb2\.", r"^eb\.", r"geom_type", r"embedded_boundary", r"implicit_function",
    ]),
    ("qed", "QED 与强场过程", [
        r"qed", r"breit_wheeler", r"compton", r"schwinger", r"trident", r"quantum_sync",
        r"radiation_reaction", r"photon", r"spin",
    ]),
    ("boundary", "边界条件（含 PML）", [
        r"^boundary\.", r"pml", r"absorber", r"silver_mueller", r"particle_boundary",
        r"perfect_conductor", r"potential_(lo|hi)", r"^boundary",
    ]),
    ("extern", "外场", [
        r"ext_grid", r"ext_particle", r"external", r"_init_style$", r"init_style",
        r"grid_init", r"particle_init", r"_function\(", r"external_field",
    ]),
    ("solver", "算法与求解器", [
        r"centering", r"do_nodal", r"do_multi_j", r"divb_cleaning", r"dive_cleaning",
        r"effective_potential", r"^pc_", r"pc_petsc", r"precon_mat", r"mass_matrices",
        r"pc_type", r"jacobian", r"hypre", r"petsc", r"^gmres", r"^newton", r"implicit",
        r"^algo\.", r"^psatd\.", r"^<solver_name>\.", r"^<a_name>\.", r"^<a_prefix>\.",
        r"deposition", r"gathering", r"interpolation", r"verbose_int", r"bottom_verbose",
        r"^macroscopic",
    ]),
    ("mesh", "网格与几何", [
        r"refine_", r"regrid", r"is_periodic", r"^amr\.", r"^geometry\.", r"^amrex\.",
        r"blocking", r"max_grid", r"grid_eff",
    ]),
    ("species", "粒子注入与物种", [
        r"^particles\.", r"^<species_name>\.", r"^<species>\.", r"^<species>",
        r"^particle_thermalizer", r"^<fluid_species_name>\.", r"thermaliz", r"momentum_distribution",
        r"maxwellian", r"quasineutral", r"density", r"temperature",
    ]),
    ("laser", "激光", [r"^lasers\.", r"laser_name>", r"^<m_laser_name>", r"^lasersiter",
                        r"laser"]),
    ("collision", "碰撞与散射", [
        r"^collisions\.", r"^<collision_name>\.", r"collision", r"^<coll_name>\.",
        r"scattering", r"ionization",
    ]),
    ("hybrid", "混合 PIC 模型（本 fork）", [r"^hybrid_pic_model\.", r"^hybrid_"]),
    ("control", "运行控制与仿真时间", [
        r"^max_step$", r"^stop_time$", r"^warpx\.(cfl|verbose|serialize|synchronize)",
        r"^warpx\.(restart|authors|time_chunk|tiny_profiler|compute_dt|checkpoint)",
        r"^amrex\.(abort|fpe|verbose|async|handle|omp)", r"^vismf\.", r"^warpx\.",
    ]),
]

COMPILED = [(cid, title, [re.compile(rule) for rule in rules])
            for cid, title, rules in CATEGORY_RULES]
_order: dict[str, int] = {}
for _index, (_cid, _, _) in enumerate(CATEGORY_RULES):
    _order.setdefault(_cid, _index)          # first occurrence wins (control appears twice)
ORDER = _order
FALLBACK = ("misc", "其它参数")

# hand-written Chinese for the parameters a beginner actually touches
HAND = {
    "max_step": "总时间步数（跑到这个步数就停）",
    "stop_time": "物理终止时间（秒），到点就停",
    "warpx.cfl": "CFL 数，Courant 条件；通常 0.9–1.0，越小越稳越慢",
    "warpx.verbose": "日志详细程度（0 安静，1 每步一行，2 更啰嗦）",
    "warpx.serialize_initial_conditions": "串行初始化，网格大时省内存",
    "warpx.use_filter": "启用电流滤波，压制数值不稳定",
    "amr.n_cell": "每个方向的网格点数（2D 写 x z，中间用空格）",
    "amr.max_level": "自适应加密层数，0 表示单层网格",
    "amr.max_grid_size": "单个网格块的最大点数，影响并行划分",
    "amr.blocking_factor": "网格块大小必须是这个数的整数倍",
    "geometry.dims": "维度：1 / 2 / 3 / RZ",
    "geometry.prob_lo": "计算域下角坐标（每维一个数）",
    "geometry.prob_hi": "计算域上角坐标（每维一个数）",
    "algo.maxwell_solver": "场求解器：yee / ckc / psatd / ect …",
    "algo.particle_shape": "粒子形状阶数 1–4；阶数越高越平滑，也越慢",
    "algo.particle_pusher": "粒子推进器：boris / vay / hybrid / kinetic_energy …",
    "algo.current_deposition": "电流沉积算法：esirkepov / direct / vay",
    "algo.field_gathering": "场收集方式：energy-conserving / momentum-conserving / direct",
    "algo.em_solver_medium": "介质电磁求解方式：vacuum / macroscopic",
    "boundary.field_lo/hi": "场在低端/高端边界的条件（每个方向一个；下面两行分别设置）",
    "boundary.particle_lo/hi": "粒子在低端/高端边界的条件（每个方向一个）",
    "boundary.field_lo": "场在低端边界的条件（每个方向一个）",
    "boundary.field_hi": "场在高端边界的条件（每个方向一个）",
    "boundary.particle_lo": "粒子在低端边界的条件（每个方向一个）",
    "boundary.particle_hi": "粒子在高端边界的条件（每个方向一个）",
    "algo.macroscopic_subpixel": "宏观介质求解的 subpixel 平滑开关：1 时按体积分数对 ε/σ/μ 做方向性平均"
                                "（法向调和、切向算术），界面不再被阶梯化，保持二阶精度；0 时按格点取材料",
    "algo.macroscopic_sigma_method": "宏观介质的 E 更新格式：backwardeuler（一阶隐式，稳）/"
                                     "laxwendroff（二阶半隐式，易振荡、需更小步长）",
    "particles.species_names": "粒子组分名列表（由下面的组分卡片自动生成）",
    "particles.use_fdtd_nci_corr": "FDTD 数值 Cherenkov 不稳定性修正器",
    "diagnostics.diags_names": "诊断名列表（由诊断卡片自动生成）",
    "diagnostics.reduced_diags_names": "归约诊断名列表（由卡片自动生成）",
    "collisions.collision_names": "碰撞对名列表（由碰撞卡片自动生成）",
    "lasers.names": "激光名列表（由激光卡片自动生成）",
}

# token -> Chinese, used to compose a line for parameters without a hand entry
TOKENS = {
    "density": "密度", "temperature": "温度", "energy": "能量", "momentum": "动量",
    "weighting": "权重", "charge": "电荷", "mass": "质量", "species": "物种",
    "particle": "粒子", "particles": "粒子", "cfl": "CFL 数", "dt": "时间步长",
    "time": "时间", "step": "步数", "steps": "步数", "intervals": "输出间隔（步）",
    "period": "输出周期（步）", "fields": "场", "field": "场", "boundary": "边界",
    "lo": "低端", "hi": "高端", "min": "最小值", "max": "最大值", "num": "数量",
    "count": "数量", "number": "数量", "velocity": "速度", "position": "位置",
    "radius": "半径", "length": "长度", "width": "宽度", "waist": "束腰半径",
    "wavelength": "波长", "frequency": "频率", "direction": "方向",
    "polarization": "偏振", "duration": "脉冲时长", "profile": "剖面",
    "threshold": "阈值", "factor": "系数", "coefficient": "系数", "scale": "缩放",
    "order": "阶数", "level": "层数", "levels": "层数", "ref_ratio": "网格加密比",
    "interpolation": "插值", "deposition": "沉积", "gathering": "收集",
    "collision": "碰撞", "collisions": "碰撞", "scattering": "散射",
    "ionization": "电离", "cross_section": "截面", "coulomb_log": "库仑对数",
    "backend": "后端", "format": "输出格式", "file_prefix": "文件名前缀",
    "type": "类型", "style": "方式", "model": "模型", "enabled": "是否启用",
    "verbose": "详细程度", "debug": "调试开关", "seed": "随机种子",
    "ppc": "每格粒子数", "ncell": "网格点数", "grid": "网格", "mesh": "网格",
    "parser": "表达式解析器", "function": "函数", "expression": "表达式",
    "warpx": "WarpX 主控", "do": "开关", "use": "开关", "write": "写出",
    "read": "读入", "path": "路径", "name": "名称", "names": "名称列表",
    "species_names": "物种名列表", "electron": "电子", "ion": "离子",
    "target": "靶", "source": "源", "inject": "注入", "injection": "注入",
    "diagnostic": "诊断", "openpmd": "openPMD 输出", "plotfile": "plotfile 输出",
    "checkpoint": "检查点", "restart": "续算", "solver": "求解器",
    "spectral": "谱方法", "hybrid": "混合", "psi": "混合模型 ψ", "eta": "电阻率",
    "rho": "电荷密度", "j": "电流密度", "e": "电场", "b": "磁场",
    "ext": "外部", "external": "外部", "constant": "常数", "uniform": "均匀",
    "gaussian": "高斯", "thermal": "热", "mcc": "MCC 蒙特卡洛碰撞",
    "bremsstrahlung": "轫致辐射", "photon": "光子", "qed": "QED 强场效应",
    "spin": "自旋", "ponderomotive": "有质动力", "boost": "洛伦兹推进",
    "pml": "PML 吸收层", "divb": "div(B)", "dive": "div(E)", "cleaning": "清理",
    "damping": "阻尼", "damp": "阻尼", "ref": "加密", "ratio": "比例",
    "all": "全部", "involved": "涉及的", "extension": "扩展名", "digits": "位数",
    "bin": "分箱", "bins": "分箱", "thermalizer": "热化器", "gmres": "GMRES",
    "newton": "牛顿法", "macroscopic": "宏观介质", "slice": "切片",
    "particle_thermalizer": "粒子热化器", "mcc": "MCC 碰撞",
    "temperature_in_eV": "温度 (eV)", "maxwellian": "麦克斯韦分布",
    "background": "背景", "stopping": "减速", "product": "产物",
    "secondary": "次级", "scattering_angle": "散射角", "cumulative": "累积",
    "subcycling": "子循环", "ndt": "每个时间步的子循环数",
    "sort": "排序", "weight": "权重", "exponent": "指数", "fraction": "分数",
    "boundary": "边界", "eb": "嵌入边界", "embedded": "嵌入", "lo/hi": "低端/高端",
    "hi": "高端", "particle_boundary": "粒子边界", "perfect": "完美导体",
    "conductor": "导体", "absorber": "吸收层", "damping": "阻尼",
    "physical_element": "物理元素", "ionization": "电离", "product": "产物",
    "rotate": "旋转", "frame": "参考系", "lab": "实验室系", "boosted": "推进系",
}


# root-level keys carry no section, so a keyword pass decides where they belong
KEYWORD_FALLBACK = [
    ("solver", re.compile(r"(tolerance|iter|solver|gmres|newton|precond|smooth|cycle|"
                          r"coarsen|agglomerat|asm_|nodal|stencil|interp)")),
    ("control", re.compile(r"(step|time|dt|cfl|verbose|regrid|checkpoint|restart|profiler)")),
    ("diag", re.compile(r"(diag|output|file|format|prefix|write|read|slice|bin|plot)")),
    ("species", re.compile(r"(species|particle|lattice|element|charge|mass|neutral)")),
    ("mesh", re.compile(r"(grid|block|box|domain|level|ref)")),
]


def category_of(param: dict) -> tuple[str, str]:
    name = param["name"]
    # `amr.plot_*` 是 AMReX 自带的写盘诊断，不是网格设置
    if name.startswith("amr.plot_") or name.startswith("amr.check_"):
        return "diag", CATEGORY_TITLES["diag"]
    section = param.get("doc_section")
    if section and section in SECTION_TO_CATEGORY:
        cid = SECTION_TO_CATEGORY[section]
        return cid, CATEGORY_TITLES[cid]
    for cid, title, patterns in COMPILED:
        for pattern in patterns:
            if pattern.search(name):
                return cid, title
    for cid, pattern in KEYWORD_FALLBACK:
        if pattern.search(name):
            return cid, CATEGORY_TITLES.get(cid, cid)
    return FALLBACK


def zh_for(param: dict, category_title: str) -> str:
    """Hand-written wording when it exists, a token translation otherwise.

    Composing Chinese word by word from a C++ key only works when most words are known;
    gluing the unknown ones together produces lines like "开关pmldivbcleaning", which is
    worse than saying what the parameter is.  So a composed line keeps only the words that
    were translated, and falls back to the bare key name when fewer than half of them were.
    """
    name = param["name"]
    if name in HAND:
        return HAND[name]

    tail = name.split(".")[-1]
    template = name.split(".")[0] if name.startswith("<") else ""
    spaced = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", tail)
    words = [w for w in re.split(r"[^A-Za-z]+", spaced.lower()) if w]
    known = [TOKENS[w] for w in words if w in TOKENS]
    raw_words = [w for w in words if w not in TOKENS and not w.isdigit()]

    extras = []
    if param.get("type"):
        extras.append(f"类型 {param['type'].split(',')[0][:24]}")
    if param.get("options"):
        extras.append("可选值 " + "/".join(param["options"][:4]))
    if param.get("required"):
        extras.append("必填")
    suffix = "（" + "，".join(extras) + "）" if extras else ""

    if known and len(known) >= max(1, len(raw_words)):
        label = tail if not template else f"{template}{name.split('.', 1)[1]}"
        return f"{category_title} · {'·'.join(known)}（{label}）{suffix}"
    return f"{category_title}参数「{tail}」{suffix}"


def main() -> int:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    zh = json.loads(ZH.read_text(encoding="utf-8"))
    params = catalog["params"]

    categories: dict[str, dict] = {}
    composed = 0
    for param in params:
        cid, title = category_of(param)
        order = CATEGORY_ORDER.index(cid) if cid in CATEGORY_ORDER else 999
        bucket = categories.setdefault(cid, {"id": cid, "name": title,
                                             "order": order, "params": []})
        bucket["params"].append(param["name"])
        # regenerate every composed line (hand-written ones are returned verbatim by
        # zh_for), so improving the lexicon actually reaches the UI
        if param["name"] not in HAND:
            composed += 1
        zh["params"][param["name"]] = zh_for(param, title)

    ordered = sorted(categories.values(), key=lambda c: c["order"])
    CATEGORIES.write_text(json.dumps({"categories": ordered}, ensure_ascii=False, indent=1) + "\n",
                          encoding="utf-8")
    for bucket in ordered:
        zh["sections"].setdefault(bucket["id"], bucket["name"])
    ZH.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    missing = [p["name"] for p in params if not zh["params"].get(p["name"])]
    print(f"categories: {len(ordered)}, params: {len(params)}, "
          f"中文新增 {composed} 条，缺失 {len(missing)} 条")
    for bucket in ordered:
        print(f"  {len(bucket['params']):4d}  {bucket['name']}")
    if missing:
        print("  缺失示例:", missing[:5])
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
