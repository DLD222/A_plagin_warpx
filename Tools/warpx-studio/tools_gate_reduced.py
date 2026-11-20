"""降维诊断：把 21 种 type 真正读取的参数补进目录并按 type 门控。

源码依据：MultiReducedDiags.cpp:63-102（21 种类型注册/分派）、
ReducedDiags.cpp:44-87（公共参数与全局 reduced_diags.*）、
以及各类型 .cpp 的 ReadParameters（ParticleHistogram/FieldProbe/...）。

旧 catalog 只有 <reduced_diags_name>.type，group 里虽有 27 个名字但 params 里没有，
表单永远看不到；这里补全 params/groups/categories 并写规则。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
R = "<reduced_diags_name>."
TYPES = ["BeamRelevant", "ChargeOnEB", "ColliderRelevant", "DifferentialLuminosity",
         "DifferentialLuminosity2D", "ParticleEnergy", "ParticleExtrema",
         "ParticleHistogram", "ParticleHistogram2D", "ParticleMomentum",
         "ParticleNumber", "FieldEnergy", "FieldMaximum", "FieldMomentum",
         "FieldPoyntingFlux", "FieldProbe", "FieldReduction", "LoadBalanceCosts",
         "LoadBalanceEfficiency", "RhoMaximum", "Timestep"]
SPECTRUM = ["DifferentialLuminosity", "DifferentialLuminosity2D",
            "ParticleHistogram", "ParticleHistogram2D"]


def cond(param, op, value=None):
    return {"param": param, "op": op, "value": value, "raw": "reduced diag gate"}


def eq(param, value):
    return cond(param, "equals", value)


def t_in(values):
    return cond(R + "type", "in", " ".join(values))


# suffix -> (type, doc, options)
PARAMS = {
    "type": ("string", "降维诊断类型（21 种，区分大小写）", TYPES),
    "intervals": ("string", "输出间隔；per-name 覆盖 reduced_diags.intervals", None),
    "path": ("string", "输出目录；per-name 覆盖 reduced_diags.path", None),
    "extension": ("string", "文件扩展名；per-name 覆盖 reduced_diags.extension", None),
    "separator": ("string", "列分隔符；per-name 覆盖 reduced_diags.separator", None),
    "precision": ("int", "输出精度；per-name 覆盖 reduced_diags.precision", None),
    "species": ("string", "该降维诊断作用的物种", None),
    "bin_number": ("int", "直方图/能谱分箱数", None),
    "bin_max": ("float", "分箱上限", None),
    "bin_min": ("float", "分箱下限", None),
    "bin_number_1": ("int", "二维能谱第一维分箱数", None),
    "bin_max_1": ("float", "二维能谱第一维上限", None),
    "bin_min_1": ("float", "二维能谱第一维下限", None),
    "bin_number_2": ("int", "二维能谱第二维分箱数", None),
    "bin_max_2": ("float", "二维能谱第二维上限", None),
    "bin_min_2": ("float", "二维能谱第二维下限", None),
    "bin_number_abs": ("int", "二维直方图纵轴分箱数", None),
    "bin_max_abs": ("float", "二维直方图纵轴上限", None),
    "bin_min_abs": ("float", "二维直方图纵轴下限", None),
    "bin_number_ord": ("int", "二维直方图横轴分箱数", None),
    "bin_max_ord": ("float", "二维直方图横轴上限", None),
    "bin_min_ord": ("float", "二维直方图横轴下限", None),
    "histogram_function(t,x,y,z,ux,uy,uz)": ("string", "直方图横轴函数（必填）", None),
    "histogram_function_abs(t,x,y,z,ux,uy,uz,w)": ("string", "二维直方图纵轴函数", None),
    "histogram_function_ord(t,x,y,z,ux,uy,uz,w)": ("string", "二维直方图横轴函数", None),
    "filter_function(t,x,y,z,ux,uy,uz)": ("string", "直方图粒子筛选函数", None),
    "filter_function(t,x,y,z,ux,uy,uz,w)": ("string", "二维直方图粒子筛选函数", None),
    "value_function(t,x,y,z,ux,uy,uz,w)": ("string", "二维直方图权重函数", None),
    "normalization": ("string", "直方图归一化方式",
                      ["default", "unity_particle_weight", "max_to_unity", "area_to_unity"]),
    "openpmd_backend": ("string", "openPMD 后端/文件后缀", ["default", "bp5", "bp4", "h5", "json"]),
    "file_min_digits": ("int", "文件名最小位数", None),
    "reduction_type": ("string", "FieldReduction 的归约方式",
                       ["Maximum", "Minimum", "Sum", "Integral"]),
    "reduced_function(x,y,z,Ex,Ey,Ez,Bx,By,Bz,jx,jy,jz)": ("string", "FieldReduction 归约函数", None),
    "weighting_function(x,y,z)": ("string", "ChargeOnEB 权重函数", None),
    "probe_geometry": ("string", "FieldProbe 几何：Point/Line/Plane",
                       ["Point", "Line", "Plane"]),
    "x_probe": ("float", "探针 x 坐标/线起点", None),
    "y_probe": ("float", "探针 y 坐标（3D）", None),
    "z_probe": ("float", "探针 z 坐标/线起点", None),
    "x1_probe": ("float", "线探针终点 x", None),
    "y1_probe": ("float", "线探针终点 y（3D）", None),
    "z1_probe": ("float", "线探针终点 z", None),
    "resolution": ("int", "线/面探针分辨率", None),
    "target_normal_x": ("float", "面探针法向 x", None),
    "target_normal_y": ("float", "面探针法向 y", None),
    "target_normal_z": ("float", "面探针法向 z", None),
    "target_up_x": ("float", "面探针 up 向量 x", None),
    "target_up_y": ("float", "面探针 up 向量 y", None),
    "target_up_z": ("float", "面探针 up 向量 z", None),
    "detector_radius": ("float", "面探针探测器半径", None),
    "integrate": ("bool", "FieldProbe 输出积分量（影响单位）", None),
    "interp_order": ("int", "FieldProbe 插值阶数", None),
    "do_moving_window_FP": ("bool", "FieldProbe 跟随移动窗口", None),
}

# suffix -> types that read it (common handled separately)
TYPE_SPECIFIC = {
    "species": ["BeamRelevant", "ColliderRelevant", "DifferentialLuminosity",
                "DifferentialLuminosity2D", "ParticleExtrema", "ParticleHistogram",
                "ParticleHistogram2D"],
    "bin_number": ["DifferentialLuminosity", "ParticleHistogram"],
    "bin_max": ["DifferentialLuminosity", "ParticleHistogram"],
    "bin_min": ["DifferentialLuminosity", "ParticleHistogram"],
    "bin_number_1": ["DifferentialLuminosity2D"], "bin_max_1": ["DifferentialLuminosity2D"],
    "bin_min_1": ["DifferentialLuminosity2D"], "bin_number_2": ["DifferentialLuminosity2D"],
    "bin_max_2": ["DifferentialLuminosity2D"], "bin_min_2": ["DifferentialLuminosity2D"],
    "bin_number_abs": ["ParticleHistogram2D"], "bin_max_abs": ["ParticleHistogram2D"],
    "bin_min_abs": ["ParticleHistogram2D"], "bin_number_ord": ["ParticleHistogram2D"],
    "bin_max_ord": ["ParticleHistogram2D"], "bin_min_ord": ["ParticleHistogram2D"],
    "histogram_function(t,x,y,z,ux,uy,uz)": ["ParticleHistogram"],
    "histogram_function_abs(t,x,y,z,ux,uy,uz,w)": ["ParticleHistogram2D"],
    "histogram_function_ord(t,x,y,z,ux,uy,uz,w)": ["ParticleHistogram2D"],
    "filter_function(t,x,y,z,ux,uy,uz)": ["ParticleHistogram"],
    "filter_function(t,x,y,z,ux,uy,uz,w)": ["ParticleHistogram2D"],
    "value_function(t,x,y,z,ux,uy,uz,w)": ["ParticleHistogram2D"],
    "normalization": ["ParticleHistogram"],
    "openpmd_backend": ["DifferentialLuminosity2D", "ParticleHistogram2D"],
    "file_min_digits": ["DifferentialLuminosity2D", "ParticleHistogram2D"],
    "reduction_type": ["FieldReduction"],
    "reduced_function(x,y,z,Ex,Ey,Ez,Bx,By,Bz,jx,jy,jz)": ["FieldReduction"],
    "weighting_function(x,y,z)": ["ChargeOnEB"],
    "probe_geometry": ["FieldProbe"], "x_probe": ["FieldProbe"], "y_probe": ["FieldProbe"],
    "z_probe": ["FieldProbe"], "x1_probe": ["FieldProbe"], "y1_probe": ["FieldProbe"],
    "z1_probe": ["FieldProbe"], "resolution": ["FieldProbe"],
    "target_normal_x": ["FieldProbe"], "target_normal_y": ["FieldProbe"],
    "target_normal_z": ["FieldProbe"], "target_up_x": ["FieldProbe"],
    "target_up_y": ["FieldProbe"], "target_up_z": ["FieldProbe"],
    "detector_radius": ["FieldProbe"], "integrate": ["FieldProbe"],
    "interp_order": ["FieldProbe"], "do_moving_window_FP": ["FieldProbe"],
}

# FieldProbe 子几何：Point/Line/Plane
PROBE_GEOM = {
    "x_probe": ["Point", "Line", "Plane"],
    "y_probe": ["Point", "Line", "Plane"],
    "z_probe": ["Point", "Line", "Plane"],
    "x1_probe": ["Line"], "y1_probe": ["Line"], "z1_probe": ["Line"],
    "resolution": ["Line", "Plane"],
    "target_normal_x": ["Plane"], "target_normal_y": ["Plane"],
    "target_normal_z": ["Plane"], "target_up_x": ["Plane"],
    "target_up_y": ["Plane"], "target_up_z": ["Plane"],
    "detector_radius": ["Plane"], "integrate": ["Point", "Line", "Plane"],
    "interp_order": ["Point", "Line", "Plane"], "do_moving_window_FP": ["Point", "Line", "Plane"],
}

# 维度限制：param -> (allowed geometry.dims, reason)
DIM_ALLOWED = {
    "x_probe": ["1", "2", "3", "RZ", "RCYLINDER", "RSPHERE"],
    "z_probe": ["1", "2", "3", "RZ"],
    "y_probe": ["3"],
    "x1_probe": ["1", "2", "3", "RZ", "RCYLINDER", "RSPHERE"],
    "z1_probe": ["1", "2", "3", "RZ"],
    "y1_probe": ["3"],
}


def add_param(catalog, suffix):
    name = R + suffix
    if any(p["name"] == name for p in catalog["params"]):
        return False
    kind, doc, options = PARAMS[suffix]
    catalog["params"].append({
        "name": name, "type": kind, "default": None, "optional": True,
        "doc": doc, "doc_section": None, "core": False,
        "options": options, "sites": [], "source_only": True,
    })
    catalog["groups"].setdefault("<reduced_diags_name>", []).append(name)
    catalog["groups"]["<reduced_diags_name>"] = sorted(catalog["groups"]["<reduced_diags_name>"])
    return True


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})
    added = [s for s in PARAMS if add_param(catalog, s)]

    # 类型选项/顺序
    for p in catalog["params"]:
        if p["name"] == R + "type":
            p["options"] = list(TYPES)

    # 清掉引用 reduced type 的旧规则
    removed = 0
    for name in list(rules):
        kept = []
        for rule in rules[name]:
            items = rule if isinstance(rule, list) else [rule]
            if any(isinstance(x, dict) and x.get("param") == R + "type" for x in items):
                removed += 1
                continue
            kept.append(rule)
        if kept:
            rules[name] = kept
        else:
            rules.pop(name, None)

    COMMON = {"type", "intervals", "path", "extension", "separator", "precision"}
    written = 0
    for suffix, types in TYPE_SPECIFIC.items():
        name = R + suffix
        if name not in {p["name"] for p in catalog["params"]}:
            continue
        rs = []
        if suffix in PROBE_GEOM:
            for geom in PROBE_GEOM[suffix]:
                conds = [t_in(["FieldProbe"]), eq(R + "probe_geometry", geom)]
                if suffix in DIM_ALLOWED:
                    conds.append(cond("geometry.dims", "in", " ".join(DIM_ALLOWED[suffix])))
                rs.append(conds)
        else:
            rs = [t_in(types)]
        rules[name] = rs
        written += 1
    for suffix in COMMON:
        rules.pop(R + suffix, None)

    catalog["counts"]["total"] = len(catalog["params"])
    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    # 分类：diag 页
    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for cat in cats["categories"]:
        if cat["id"] == "diag":
            for suffix in PARAMS:
                n = R + suffix
                if n in {p["name"] for p in catalog["params"]} and n not in cat["params"]:
                    cat["params"].append(n)
            cat["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n",
                         encoding="utf-8")
    print(f"降维诊断: 补 {len(added)} 个参数、{written} 个参数写门控，"
          f"gating_rules {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
