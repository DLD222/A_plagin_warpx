"""把"合并行"参数拆成 WarpX 真正认识的独立参数。

catalog 里有 18 个用 `/` 把多个参数并成一行的条目，例如 `geometry.prob_lo/hi`。
它们只是文档里并排定义的产物：在界面上填一个值，写盘时会生成
`geometry.prob_lo/hi = ...` —— WarpX 根本没有这个参数，整行是无效输入。

这里把每个合并项展开成真实参数（`geometry.prob_lo`、`geometry.prob_hi` …），
并让 validation 拒绝任何含 `/` 的键，防止再写进输入文件。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

# 合并项 -> 真实参数
SPLIT: dict[str, list[str]] = {
    "geometry.prob_lo/hi": ["geometry.prob_lo", "geometry.prob_hi"],
    "boundary.field_lo/hi": ["boundary.field_lo", "boundary.field_hi"],
    "boundary.particle_lo/hi": ["boundary.particle_lo", "boundary.particle_hi"],
    "boundary.potential_lo/hi_x/y/z": [
        "boundary.potential_lo_x", "boundary.potential_hi_x",
        "boundary.potential_lo_y", "boundary.potential_hi_y",
        "boundary.potential_lo_z", "boundary.potential_hi_z"],
    "macroscopic.sigma/epsilon/mu": [
        "macroscopic.sigma", "macroscopic.epsilon", "macroscopic.mu"],
    "macroscopic.sigma/epsilon/mu_function(x,y,z)": [
        "macroscopic.sigma_function(x,y,z)", "macroscopic.epsilon_function(x,y,z)",
        "macroscopic.mu_function(x,y,z)"],
    "particles.E/B_ext_particle_init_style": [
        "particles.E_ext_particle_init_style", "particles.B_ext_particle_init_style"],
    "psatd.nox/noy/noz": ["psatd.nox", "psatd.noy", "psatd.noz"],
    "psatd.nx/ny/nz_guard": ["psatd.nx_guard", "psatd.ny_guard", "psatd.nz_guard"],
    "qed_schwinger.xmin/ymin/zmin/xmax/ymax/zmax": [
        "qed_schwinger.xmin", "qed_schwinger.ymin", "qed_schwinger.zmin",
        "qed_schwinger.xmax", "qed_schwinger.ymax", "qed_schwinger.zmax"],
    "warpx.E/B_external_grid": ["warpx.E_external_grid", "warpx.B_external_grid"],
    "warpx.current_centering_nox/noy/noz": [
        "warpx.current_centering_nox", "warpx.current_centering_noy",
        "warpx.current_centering_noz"],
    "warpx.field/particle_io_nfiles": [
        "warpx.field_io_nfiles", "warpx.particle_io_nfiles"],
    "warpx.field_centering_nox/noy/noz": [
        "warpx.field_centering_nox", "warpx.field_centering_noy",
        "warpx.field_centering_noz"],
    "warpx.fine_tag_lo/hi": ["warpx.fine_tag_lo", "warpx.fine_tag_hi"],
    "<species_name>.xmin/ymin/zmin/xmax/ymax/zmax": [
        "<species_name>.xmin", "<species_name>.ymin", "<species_name>.zmin",
        "<species_name>.xmax", "<species_name>.ymax", "<species_name>.zmax"],
    "<species_name>.save_particles_at_xlo/ylo/zlo/xhi/yhi/zhi/eb": [
        "<species_name>.save_particles_at_xlo", "<species_name>.save_particles_at_ylo",
        "<species_name>.save_particles_at_zlo", "<species_name>.save_particles_at_xhi",
        "<species_name>.save_particles_at_yhi", "<species_name>.save_particles_at_zhi",
        "<species_name>.save_particles_at_eb"],
    "hybrid_pic_model.J[x/y/z]_external_grid_function(x,y,z,t)": [
        "hybrid_pic_model.Jx_external_grid_function(x,y,z,t)",
        "hybrid_pic_model.Jy_external_grid_function(x,y,z,t)",
        "hybrid_pic_model.Jz_external_grid_function(x,y,z,t)"],
}

# 拆分后每个成员的说明（比合并项的说明更具体）
MEMBER_DOC = {
    "geometry.prob_lo": "计算域下界角点坐标（米）：二维写 2 个数 `x z`，三维写 3 个 `x y z`",
    "geometry.prob_hi": "计算域上界角点坐标（米）：二维写 2 个数 `x z`，三维写 3 个 `x y z`",
    "boundary.field_lo": "场的低端边界条件（按方向，二维 x z）",
    "boundary.field_hi": "场的高端边界条件（按方向，二维 x z）",
    "boundary.particle_lo": "粒子的低端边界条件（按方向，二维 x z）",
    "boundary.particle_hi": "粒子的高端边界条件（按方向，二维 x z）",
    "warpx.E_external_grid": "外部电场固定值（V/m），按方向给 2 个(2D)/3 个(3D)数",
    "warpx.B_external_grid": "外部磁场固定值（T），按方向给 2 个(2D)/3 个(3D)数",
}


def main() -> int:
    cat_path = DATA / "catalog.json"
    catalog = json.loads(cat_path.read_text(encoding="utf-8"))
    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))

    existing = {p["name"] for p in catalog["params"]}
    added, removed = [], []
    for composite, members in SPLIT.items():
        src = next((p for p in catalog["params"] if p["name"] == composite), None)
        if src is None:
            continue
        catalog["params"].remove(src)
        removed.append(composite)
        for member in members:
            if member in existing:
                continue
            entry = dict(src)
            entry["name"] = member
            entry["doc"] = MEMBER_DOC.get(member, src.get("doc") or "")
            entry["options"] = None
            entry.pop("options", None)
            catalog["params"].append(entry)
            existing.add(member)
            added.append(member)
        # 分类里把合并项换成真实成员
        for category in cats["categories"]:
            if composite in category["params"]:
                idx = category["params"].index(composite)
                category["params"][idx:idx + 1] = members

    catalog["params"].sort(key=lambda p: p["name"])
    catalog["counts"]["total"] = len(catalog["params"])
    cat_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"拆分 {len(removed)} 个合并项 → 新增 {len(added)} 个真实参数")
    print(f"参数总数: {catalog['counts']['total']}")
    left = [p["name"] for p in catalog["params"] if "/" in p["name"]]
    print(f"仍含 '/' 的条目: {left or '无'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
