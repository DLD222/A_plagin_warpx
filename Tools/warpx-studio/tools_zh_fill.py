"""给缺少中文说明的参数补上汉语注释（不翻译物理量、取值与代码标识符）。

优先用人工词条 ZH_EXTRA；剩下的按参数名最后一段的中文词典 GLOSS 组句，
例如 `<diag_name>.format` → "诊断输出格式：plotfile / openpmd / checkpoint …"。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

ZH_EXTRA: dict[str, str] = {
    # 诊断
    "<diag_name>.format": "诊断输出格式：plotfile（AMReX 自带，ParaView 可直开）/ openpmd（HDF5，通用）/ checkpoint（续算用）",
    "<diag_name>.diag_type": "诊断类型：Full 全量输出 / TimeAveraged 时间平均 / BackTransformed 背变换（boosted frame）",
    "<diag_name>.species": "这个诊断要输出哪些粒子的数据（物种名列表，空格分隔）",
    "<diag_name>.write_species": "1 = 同时输出粒子数据，0 = 只输出场",
    "<diag_name>.file_min_digits": "文件名里迭代编号至少占几位（补零），例如 5 → 00042",
    "<diag_name>.openpmd_backend": "openPMD 后端：h5（HDF5，单机最常用）/ adios2",
    "<diag_name>.openpmd_encoding": "openPMD 写出方式：group-based（一份文件一帧）/ variable-based（每变量一文件）",
    "<diag_name>.coarsening_ratio": "诊断时对网格做粗化的倍数（>1 表示降采样输出）",
    "<diag_name>.diag_lo": "诊断输出区域的低端角点坐标（米）；留空 = 整个计算域",
    "<diag_name>.diag_hi": "诊断输出区域的高端角点坐标（米）；留空 = 整个计算域",
    "<diag_name>.particle_fields_to_plot": "要额外输出的粒子量（如 weight、charge、momentum）",
    "<diag_name>.particle_fields_species": "上面那些粒子量取哪些物种",
    "<species_name>.random_fraction": "每个粒子被随机保留的概率（0–1），做粒子稀疏化输出时用",
    "<species_name>.uniform_stride": "每隔多少个粒子输出一个（均匀抽样）",
    # 物种注入
    "<species_name>.density_max": "密度剖面的最大值（m^-3），配合 profile 使用",
    "<species_name>.density_min": "密度剖面的最小值（m^-3），配合 profile 使用",
    "<species_name>.profile": "空间密度剖面：constant 均匀 / parse_density_function 用函数 / 其它见文档",
    "<species_name>.flux_profile": "面注入的流量剖面：constant 常数 / parse_flux_function 用函数",
    "<species_name>.injection_sources": "注入源个数（NFluxPerCell 连续注入时可分多个源）",
    "<species_name>.do_continuous_injection": "1 = 随时间持续注入粒子（来流），0 = 只在初始化时铺一次",
    "<species_name>.species_type": "物种类型：plasma / photon（光子）/ …；一般用 plasma",
    "<species_name>.save_particles_at_xlo": "1 = 保存从低端 x 面离开的粒子（用于统计通量）",
    "<species_name>.save_particles_at_xhi": "1 = 保存从高端 x 面离开的粒子",
    "<species_name>.save_particles_at_ylo": "1 = 保存从低端 y 面离开的粒子",
    "<species_name>.save_particles_at_yhi": "1 = 保存从高端 y 面离开的粒子",
    "<species_name>.save_particles_at_zlo": "1 = 保存从低端 z 面离开的粒子",
    "<species_name>.save_particles_at_zhi": "1 = 保存从高端 z 面离开的粒子",
    "<species_name>.save_particles_at_eb": "1 = 保存撞到导体（EB）面上被吸收/反射的粒子",
    # 算法
    "algo.particle_pusher": "粒子推进器：boris（默认，适合普通等离子体）/ vay（大步长相对论更准）/ hybrid / kinetic_energy",
    "algo.field_gathering": "场收集方式：energy-conserving / momentum-conserving / direct",
    "algo.charge_deposition": "电荷沉积方式：standard / momentum-conserving",
    # 其它
    "amr.restart": "从哪个 checkpoint 续算：写文件名或 0/1（1 = 取最近一个）",
    "amr.ref_ratio_vect": "每个方向各自的加密比（比统一的 amr.ref_ratio 更灵活）",
    "collisions.correct_energy_momentum": "1 = 碰撞后强制修正能量与动量守恒",
    "collisions.energy_fraction": "碰撞能量分配比例（0–1）",
    "<diag_name>.adios2_engine.type": "ADIOS2 引擎类型（如 bp4 / bp5 / sst）",
    "<diag_name>.adios2_operator.type": "ADIOS2 压缩算子类型（如 blosc / zfp）",
    "<species_name>.split_type": "粒子分裂方式：proton-impact-ionization（质子碰撞电离）",
    "external_vector_potential.<field_name>.path": "外部矢势文件路径（从文件读取 A 时用）",
    "fluids.species_names": "参与流体模块的物种名列表",
    "lasers.names": "激光名称列表（每个名字对应一组 <名字>.* 激光参数）",
    "particle_thermalizer.species": "参与热化器（thermalizer）的物种名列表",
    "qed_bw.chi_min": "Breit-Wheeler 光子产生的最小量子参数 χ",
    "qed_qs.chi_min": "量子同步辐射的最小量子参数 χ",
    "reduced_diags.extension": "缩减诊断输出文件的扩展名（如 .txt / .csv）",
    "reduced_diags.path": "缩减诊断输出文件的路径",
    "warpx.do_subcycling": "1 = 不同网格层用不同时间步（子循环），0 = 全局同一步长",
    "warpx.max_dt": "时间步长上限（秒）；CFL 算出的 dt 超过它时会被截断",
    "warpx.max_omegac_dt": "回旋频率限制：保证 omega_c*dt 不超过该值",
    "warpx.max_omegap_dt": "等离子体频率限制：保证 omega_p*dt 不超过该值",
}

GLOSS: dict[str, str] = {
    "random_fraction": "随机抽样比例", "uniform_stride": "均匀抽样间隔",
    "type": "类型", "format": "格式", "species": "物种列表", "profile": "剖面",
    "path": "文件路径", "name": "名称", "names": "名称列表", "steps": "步数",
    "time": "时间", "min_iters": "最大迭代次数", "max_iters": "最大迭代次数",
    "split_type": "分裂方式", "do_splitting": "是否做粒子分裂：1 开 / 0 关",
    "do_resampling": "是否做粒子重采样：1 开 / 0 关",
    "do_field_ionization": "是否做场致电离：1 开 / 0 关",
    "do_temperature_deposition": "是否沉积温度：1 开 / 0 关",
    "ionization_initial_level": "电离初态能级",
    "ionization_product_species": "电离产生的物种名",
    "self_fields_max_iters": "自场泊松求解的最大迭代次数",
    "qed_quantum_sync_phot_product_species": "量子同步辐射产生的光子物种名",
    "qed_breit_wheeler_ele_product_species": "Breit-Wheeler 产生的电子物种名",
    "qed_breit_wheeler_pos_product_species": "Breit-Wheeler 产生的正电子物种名",
    "beta_weight_exponent": "权重因子指数（碰撞用）",
    "energy_correction_sort_by_weight": "能量修正时是否按权重排序：1 是 / 0 否",
    "do_average": "是否对该量做时间平均：1 是 / 0 否",
    "do_back_transformed_fields": "是否输出背变换（boosted frame）的场：1 是 / 0 否",
    "do_back_transformed_particles": "是否输出背变换的粒子：1 是 / 0 否",
    "num_snapshots_lab": "实验室系快照数量",
    "dt_snapshots_lab": "实验室系快照时间间隔（秒）",
    "average_period_steps": "时间平均的取样间隔（步）",
    "average_period_time": "时间平均的取样间隔（秒）",
}


def fill(name: str, current: str) -> str:
    if name in ZH_EXTRA:
        return ZH_EXTRA[name]
    tail = name.split(".")[-1]
    tail = re.sub(r"\(.*\)$", "", tail)
    if tail in GLOSS:
        return GLOSS[tail]
    if current.strip() and len(current.strip()) >= 4 and not re.search(r"[A-Za-z]{4,}", current):
        return current
    # 最后兜底：用参数名分段组成"分组：末段"
    parts = [p for p in re.split(r"[.]", name) if p and not p.startswith("<")]
    if parts:
        return f"{' · '.join(parts)}（参数含义见右侧取值与提示）"
    return "参数含义见右侧取值与提示"


def main() -> int:
    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    catalog = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
    params = zh["params"]
    filled = 0
    for p in catalog["params"]:
        name = p["name"]
        cur = params.get(name, "")
        new = fill(name, cur)
        if new != cur:
            params[name] = new
            filled += 1
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    remaining = [p["name"] for p in catalog["params"]
                 if len((params.get(p["name"]) or "").strip()) < 6]
    print(f"补充 {filled} 条；仍过短 {len(remaining)} 条")
    for n in remaining[:6]:
        print("  ", n, "->", params.get(n))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
