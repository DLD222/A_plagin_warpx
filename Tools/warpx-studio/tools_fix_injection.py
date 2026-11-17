"""按文档把 injection_style 的每一个选项与它需要的参数对上（八种，逐一覆盖）。

来源：Docs/source/usage/parameters.rst「<species_name>.injection_style」
源码已核对（PlasmaInjector.cpp 会把取值转小写比较）：
  none / NUniformPerCell / NRandomPerCell / NFluxPerCell /
  SingleParticle / MultipleParticles / gaussian_beam / external_file
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
S = "<species_name>."
STYLE = S + "injection_style"

STYLES = ["none", "NUniformPerCell", "NRandomPerCell", "NFluxPerCell",
          "SingleParticle", "MultipleParticles", "gaussian_beam", "external_file"]

GLOSS = {
    "none": "不注入粒子（默认）",
    "NUniformPerCell": "每格均匀放固定个数（需要 num_particles_per_cell_each_dim，密度由 profile/density 决定）",
    "NRandomPerCell": "每格随机放固定个数（需要 num_particles_per_cell，密度由 profile/density 决定）",
    "NFluxPerCell": "从发射面持续注入通量（面注入；需要 flux 与发射面参数）",
    "SingleParticle": "只注入一个宏粒子（需要位置/动量/权重）",
    "MultipleParticles": "按列表注入多个宏粒子（位置/动量/权重各一个列表）",
    "gaussian_beam": "高斯束团（需要总电荷或真实粒子数、束心、束宽等）",
    "external_file": "从 openPMD 文件读粒子（需要 injection_file）",
}

# 每种注入方式需要的参数（不含开关本身）
REQUIRES: dict[str, list[str]] = {
    "NUniformPerCell": ["num_particles_per_cell_each_dim",
                        "xmin", "xmax", "zmin", "zmax", "ymin", "ymax"],
    "NRandomPerCell": ["num_particles_per_cell",
                       "xmin", "xmax", "zmin", "zmax", "ymin", "ymax"],
    "NFluxPerCell": ["flux_profile", "flux", "flux_normal_axis", "flux_direction",
                     "surface_flux_pos", "flux_tmin", "flux_tmax",
                     "inject_from_embedded_boundary", "num_particles_per_cell",
                     "flux_function(x,y,z,t)", "zinject_plane"],
    "SingleParticle": ["single_particle_pos", "single_particle_u", "single_particle_weight"],
    "MultipleParticles": ["multiple_particles_pos_x", "multiple_particles_pos_y",
                          "multiple_particles_pos_z", "multiple_particles_ux",
                          "multiple_particles_uy", "multiple_particles_uz",
                          "multiple_particles_weight"],
    "gaussian_beam": ["q_tot", "npart_real", "npart", "x_m", "y_m", "z_m",
                      "x_rms", "y_rms", "z_rms", "x_cut", "y_cut", "z_cut",
                      "do_symmetrize", "symmetrization_order", "focal_distance",
                      "do_gaussian_beam_rotation", "gaussian_beam_rotation_axis",
                      "gaussian_beam_rotation_angle", "do_gaussian_beam_rotation_momenta"],
    "external_file": ["injection_file", "charge", "mass", "z_shift", "impose_t_lab_from_file"],
}

NEW_PARAMS = {
    "x_cut": "float", "y_cut": "float", "z_cut": "float",
    "x_m": "float", "y_m": "float", "z_m": "float",
    "x_rms": "float", "y_rms": "float", "z_rms": "float",
}

DOCS = {
    "x_cut": "gaussian_beam：|x-x_m| > x_cut·x_rms 的粒子不注入",
    "y_cut": "gaussian_beam：|y-y_m| > y_cut·y_rms 的粒子不注入",
    "z_cut": "gaussian_beam：|z-z_m| > z_cut·z_rms 的粒子不注入",
    "x_m": "gaussian_beam 束心的 x 坐标（米）",
    "x_rms": "gaussian_beam 在 x 方向的均方根宽度（米）",
    "y_m": "gaussian_beam 束心的 y 坐标（米）；二维可忽略",
    "z_m": "gaussian_beam 束心的 z 坐标（米）",
    "y_rms": "gaussian_beam 在 y 方向的均方根宽度（米）",
    "z_rms": "gaussian_beam 在 z 方向的均方根宽度（米）",
}


def main() -> int:
    cat_path = DATA / "catalog.json"
    cat = json.loads(cat_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in cat["params"]}

    for short, t in NEW_PARAMS.items():
        full = S + short
        if full not in names:
            cat["params"].append({"name": full, "type": t, "default": None, "optional": True,
                                  "doc": DOCS[short], "core": False, "options": None})
            names.add(full)
    cat["params"].sort(key=lambda p: p["name"])
    cat["counts"]["total"] = len(cat["params"])

    rules = cat.setdefault("gating_rules", {})
    # 先清掉所有"挂在 injection_style 上"的旧规则，避免残留
    for name, rs in list(rules.items()):
        if not isinstance(rs, list):
            continue
        keep = []
        for r in rs:
            flat = r if isinstance(r, list) else [r]
            if any(isinstance(x, dict) and x.get("param") == STYLE for x in flat):
                continue
            keep.append(r)
        if keep:
            rules[name] = keep
        else:
            rules.pop(name, None)

    for style, params in REQUIRES.items():
        for short in params:
            full = S + short
            if full not in names:
                continue
            rules.setdefault(full, []).append(
                {"param": STYLE, "op": "equals", "value": style, "raw": "injection"})

    # do_symmetrize 打开才显示 symmetrization_order
    if S + "symmetrization_order" in names:
        rules.setdefault(S + "symmetrization_order", []).append(
            {"param": S + "do_symmetrize", "op": "truthy", "value": None, "raw": "injection"})
    # 旋转打开才显示旋转角度/轴
    for short in ("gaussian_beam_rotation_axis", "gaussian_beam_rotation_angle"):
        full = S + short
        if full in names:
            rules.setdefault(full, []).append(
                {"param": S + "do_gaussian_beam_rotation", "op": "truthy",
                 "value": None, "raw": "injection"})
    for p in cat["params"]:
        if p["name"] == STYLE:
            p["options"] = STYLES
    cat_path.write_text(json.dumps(cat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    enums_path = DATA / "enums.json"
    e = json.loads(enums_path.read_text(encoding="utf-8"))
    e["enums"][STYLE] = STYLES
    e.setdefault("glossary", {})[STYLE] = GLOSS
    enums_path.write_text(json.dumps(e, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for c in cats["categories"]:
        if c["id"] == "species":
            for short in NEW_PARAMS:
                full = S + short
                if full not in c["params"]:
                    c["params"].append(full)
            c["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"injection_style 八种取值与依赖已重建；规则总数 {len(rules)}；新增参数 {len(NEW_PARAMS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
