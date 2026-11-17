"""把参数行里的"废话标签"换成有用的说明。

界面上每行显示的是 zh.params[name]。之前有 113 条是自动生成的占位串，例如
`网格与几何 · 低端·高端（prob_lo/hi）`、`嵌入边界（EB） · 粒子·嵌入边界（particle_eb）（类型 string）`，
既没说清方向，也没说清取值的含义。这里用人工撰写的说明替换掉，未覆盖的至少去掉
"分类 · " 前缀和 "（类型 X）" 后缀。

每一条的写法固定为："是什么 —— 方向/单位；可取什么值、每个值是什么意思"。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

CURATED: dict[str, str] = {
    # ---------------- 网格与几何 ----------------
    "geometry.dims":
        "模拟维度：1 / 2 / 3 / RZ。二维是 (x,z) 平面，三维是 (x,y,z)，RZ 是轴对称 (r,z)",
    "geometry.prob_lo/hi":
        "计算域的两个角点坐标（米）：prob_lo 下界、prob_hi 上界。二维按 (x, z) 顺序写两个数，"
        "例如 `-2e-3 -4e-3` = x∈[-2mm]、z∈[-4mm]。它只决定盒子多大，不是边界条件；"
        "格距 dx=(prob_hi-prob_lo)/n_cell",
    "amr.n_cell":
        "每个方向的网格数：二维写 `nx nz`（对应 x、z），与 geometry.prob_lo/hi 一起定出格距 dx、dz",
    "amr.max_grid_size":
        "单个网格块（box）的最大格数；只影响并行划分与性能，不改变物理",
    "amr.blocking_factor":
        "网格块边长必须是它的整数倍；影响负载均衡，不改变物理",
    "amr.max_level":
        "自适应加密（AMR）层数：0 = 单层均匀网格（本仓库多数算例用 0）",
    "amr.ref_ratio":
        "相邻 AMR 层的加密比（每层一格分成 n 格），必须 ≥ 2",
    "warpx.regrid_int":
        "每隔多少步重做一次 AMR 网格重划分；0 = 不重划分",

    # ---------------- 边界条件（场与粒子是两套） ----------------
    "boundary.field_lo/hi":
        "**场** 在低端（_lo）/ 高端（_hi）的边界条件，按方向分别给出，二维顺序是 x、z。"
        "可选 periodic 周期 / pec 理想导体壁（切向 E=0，本仓库用它做导体）/ pml 吸收层 / "
        "open 开放（需 FFT 泊松求解）/ absorbing 吸收 / neumann 零梯度 / dirichlet 固定值。"
        "例：`pec periodic` = x 方向导体壁、z 方向周期。这是场的边界，与粒子边界互不影响",
    "boundary.particle_lo":
        "**粒子** 在低端边界的条件，按方向分别给出（二维顺序 x、z）。"
        "可选 periodic / absorbing 吸收（飞出即删除）/ reflecting 反射 / thermal 热化发射。"
        "与 boundary.field_lo 是两套独立设置：场周期不等于粒子周期",
    "boundary.particle_lo/hi":
        "**粒子** 低端与高端的边界条件，按方向分别给出（二维顺序 x、z）："
        "periodic / absorbing / reflecting / thermal",
    "boundary.particle_eb":
        "粒子到达导体（EB）表面时的处理方式，文本取值（不是 0/1）："
        "Absorbing = 删除粒子（默认）/ Reflecting = 镜面反射 / Thermal = 热化再发射"
        "（需 boundary.<物种>.u_th）。做固体障碍物绕流要用 Reflecting 才能把粒子挡回；"
        "默认 Absorbing 是撞上就消失",
    "boundary.reflect_all_velocities":
        "粒子撞边界反射时是否三个速度分量都反向：1 = 全反向，0 = 只反向法向分量",
    "boundary.verboncoeur_axis_correction":
        "RZ 几何轴线处的伪源项修正：1 = 开启（默认），一般不用改",
    "boundary.potential_lo_x":
        "静电场求解器下低端 x 面的固定电势（伏特）：0 = 接地导体壁；仅 warpx.do_electrostatic 开启时有效",
    "boundary.potential_hi_x":
        "静电场求解器下高端 x 面的固定电势（伏特）：0 = 接地导体壁",
    "boundary.potential_lo_y":
        "静电场求解器下低端 y 面的固定电势（伏特）；二维没有 y，忽略",
    "boundary.potential_hi_y":
        "静电场求解器下高端 y 面的固定电势（伏特）；二维没有 y，忽略",
    "boundary.potential_lo_z":
        "静电场求解器下低端 z 面的固定电势（伏特）：0 = 接地导体壁",
    "boundary.potential_hi_z":
        "静电场求解器下高端 z 面的固定电势（伏特）：0 = 接地导体壁",

    # ---------------- 嵌入边界（EB） ----------------
    "warpx.eb_implicit_function":
        "导体/介质的几何：隐函数 F(x,y,z)，**F > 0 是实体内部**，F < 0 是流体（可计算）区域，"
        "F = 0 是表面（WarpX 官方约定）。例如 `(1e-3)^2 - (x*x + z*z)` 在圆柱内为正 → "
        "半径 1 mm 的实心圆柱；二维里 y 恒为 0。实体内部按理想导体（PEC）处理",
    "warpx.eb_potential(x,y,z,t)":
        "给 EB 导体施加的静电势（伏特）；0 = 接地。只在静电场求解器下生效",
    "eb2.geom_type":
        "EB 几何描述方式：all_regular 无实体 / implicit_function 用隐函数（本仓库用这个）/ stl 读 STL 文件",

    # ---------------- 求解器与时间步 ----------------
    "algo.maxwell_solver":
        "麦克斯韦求解器：yee 标准 FDTD（电磁 PIC 默认）/ ckc Yee 的色散修正 / psatd 谱方法 / "
        "ect 电荷守恒 Yee / hybrid 动理学离子 + 流体电子（适合 MHD 尺度）",
    "algo.current_deposition":
        "电流沉积方法：esirkepov 电荷守恒（推荐）/ villasenor 较老、近似守恒 / direct 最快但不守恒",
    "algo.particle_shape":
        "粒子形状函数阶数：1 线性 / 2 二次 / 3 三次。阶数越高噪声越低、计算越贵",
    "warpx.const_dt":
        "固定时间步长（秒）。一旦设置就覆盖 CFL 自动步长；与 warpx.cfl 二选一",
    "warpx.cfl":
        "CFL 系数：dt = cfl × min(dx,dz)/c；只有不设 warpx.const_dt 时才生效",
    "max_step":
        "总共跑的步数（整数）；与 stop_time 谁先到就停",
    "stop_time":
        "物理时间上限（秒）；与 max_step 谁先到就停",
    "warpx.B_ext_grid_init_style":
        "外部磁场的初始化方式：constant 固定值（配 warpx.B_external_grid）/ parse_B_ext_grid_function 用函数",
    "warpx.E/B_external_grid":
        "外部电磁场的固定值（国际单位），按方向给 3 个数。磁化等离子体垂直 B 漂移时还必须同时设 "
        "E = -v×B，否则初始态不是平衡态，流会被洛伦兹力偏转",
    "warpx.pml_ncell":
        "PML 吸收层厚度（格数）：通常 10–40；只在对应方向设成 pml 时生效",
    "warpx.do_pml_in_domain":
        "PML 放域内还是域外：1 = 内部（占用格点），0 = 外部（默认）",

    # ---------------- 物种 ----------------
    "particles.species_names":
        "参与的粒子种类名列表（空格分隔，如 `ions electrons`）；每种对应一组 `<名字>.charge`、`<名字>.mass` 等",
    "<species_name>.charge":
        "该粒子电荷（库仑）：电子写 `-q_e`，铝离子 Z=5 写 `5*q_e`",
    "<species_name>.mass":
        "该粒子质量（千克）：电子 `m_e`，质子 `m_p`，铝 `27*m_p`",
    "<species_name>.density":
        "该粒子的数密度（m^-3），与 injection_style 一起决定每格放多少粒子",
    "<species_name>.injection_style":
        "粒子注入方式：none 不放（默认）/ NUniformPerCell 每格均匀放固定个数（最常用）/ "
        "NFluxPerCell 从面持续注入通量（做来流用这个）",
    "<species_name>.num_particles_per_cell_each_dim":
        "每格每方向放几个粒子：二维写 `2 2` = 每格 4 个。越高噪声越低、越慢",
    "<species_name>.momentum_distribution_type":
        "初始动量分布：constant 单能（配 ux/uy/uz）/ gaussian 高斯（配 ux_m 与 ux_th 等热速度）",
    "<species_name>.ux_th":
        "x 方向热动量展宽（无量纲 u = γv/c）；冷束写 0，温度 T 对应 u_th = sqrt(2T/mc²)",

    # ---------------- hybrid ----------------
    "hybrid_pic_model.elec_temp":
        "hybrid 模型的电子温度（eV）；决定电子压强与声速，必填",
    "hybrid_pic_model.n0_ref":
        "hybrid 的参考密度（m^-3），用于电子压强归一化；不填则取初始最大密度",
    "hybrid_pic_model.n_floor":
        "hybrid 的密度下限（m^-3，默认 1）：广义 Ohm 定律含 1/n 项，无粒子区域用这个密度兜底防止除零",
    "hybrid_pic_model.substeps":
        "每个时间步内的磁场子步数：whistler 波的稳定性靠它，越大越稳也越慢",
    "hybrid_pic_model.use_rkf45":
        "1 = 自适应 RKF45 子步（更准，但本仓库 EB cut cell 处误差估计会退化导致中止）；"
        "0 = 固定 RK4 子步（配 substeps）。带 EB 时建议 0",
    "hybrid_pic_model.plasma_resistivity":
        "等离子体电阻率（Ω·m）：0 = 无耗散（激波很薄），小量如 5e-6 可提供耗散、压低网格噪声",
}


def strip_junk(label: str, name: str) -> str:
    """去掉自动生成的 `分类 · 词（name）（类型 T）` 外壳。"""
    text = label or ""
    text = re.sub(r"（[^）]*）（类型 [^）]*）\s*$", "", text).strip()
    text = re.sub(r"（类型 [^）]*）\s*$", "", text).strip()
    text = re.sub(r"^.*?·\s*", "", text).strip()          # 去掉"分类 · "前缀（带不带空格都去掉）
    text = re.sub(r"^参数「.*」$", "", text).strip()        # "参数「potential_lo_x」"这种占位串
    return text or ""


def main() -> int:
    cat = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
    names = {p["name"] for p in cat["params"]}
    docs = {p["name"]: (p.get("doc") or "").strip() for p in cat["params"]}
    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    params = zh["params"]

    curated = {k: v for k, v in CURATED.items() if k in names}
    changed = 0
    for name in names:
        if name in curated:
            if params.get(name) != curated[name]:
                params[name] = curated[name]
                changed += 1
            continue
        old = params.get(name, "")
        new = strip_junk(old, name)
        if new != old:
            params[name] = new
            changed += 1
        # 没有人工说明的条目：用英文原文的第一句兜底，至少说明它做什么
        if name not in curated and docs.get(name) and len(params.get(name, "")) < 24:
            first = re.split(r"(?<=[.!?])\s", docs[name])[0][:160]
            if first and first not in params[name]:
                params[name] = (f"{params[name]}。" if params.get(name) else "") + first
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"更新 {changed} 条；人工撰写 {len(curated)} 条")
    for probe in ("geometry.prob_lo/hi", "boundary.field_lo/hi", "boundary.particle_lo",
                  "boundary.particle_eb", "warpx.eb_implicit_function"):
        print(f"  {probe}: {params[probe][:110]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
