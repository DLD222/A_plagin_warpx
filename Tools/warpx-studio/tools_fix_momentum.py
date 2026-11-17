"""按 WarpX 文档重建 momentum_distribution_type 的全部依赖（九种分布）。

来源：Docs/source/usage/parameters.rst「<species_name>.momentum_distribution_type」

  at_rest                  -> 无参数
  constant                 -> ux, uy, uz
  uniform                  -> ux_min/ux_max, uy_min/uy_max, uz_min/uz_max
  gaussian                 -> ux_m/uy_m/uz_m, ux_th/uy_th/uz_th
  gaussianflux             -> 同上 6 个 + flux_normal_axis, flux_direction（且必须 NFluxPerCell）
  maxwellian（非相对论）    -> u_mean 子类型 + (u_std 子类型 或 temperature 子类型)
  maxwell_juttner（相对论） -> u_mean 子类型 + temperature 子类型（没有 u_std！）
  parse_momentum_function  -> momentum_function_ux/uy/uz(x,y,z)

子类型取值统一为 constant / parser / read_from_file。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
S = "<species_name>."
MOM = S + "momentum_distribution_type"


def eq(v):
    return {"param": MOM, "op": "equals", "value": v, "raw": "momentum"}


def eqp(param, v):
    return {"param": param, "op": "equals", "value": v, "raw": "momentum"}


def empty(param):
    return {"param": param, "op": "empty", "value": None, "raw": "momentum"}


MEAN = (S + "ux_mean", S + "uy_mean", S + "uz_mean")
MEAN_F = (S + "ux_mean_function(x,y,z)", S + "uy_mean_function(x,y,z)", S + "uz_mean_function(x,y,z)")
STD = (S + "ux_std", S + "uy_std", S + "uz_std")
STD_F = (S + "ux_std_function(x,y,z)", S + "uy_std_function(x,y,z)", S + "uz_std_function(x,y,z)")
TEMP = (S + "temperature_in_eV",)
TEMP_F = (S + "temperature_in_eV_function(x,y,z)",)
TEMP_FILE = (S + "read_temperature_in_eV_from_path", S + "temperature_in_eV_mesh_name")
GAUSS6 = (S + "ux_m", S + "uy_m", S + "uz_m", S + "ux_th", S + "uy_th", S + "uz_th")
UNIFORM6 = (S + "ux_min", S + "ux_max", S + "uy_min", S + "uy_max", S + "uz_min", S + "uz_max")

NEW_PARAMS = {
    S + "ux_min": "float", S + "ux_max": "float",
    S + "uy_min": "float", S + "uy_max": "float",
}

GATES: dict[str, list] = {}


def gate(names, conds):
    for n in names:
        GATES.setdefault(n, []).extend(conds)


gate((S + "ux", S + "uy", S + "uz"), [eq("constant")])
gate(UNIFORM6, [eq("uniform")])
gate(GAUSS6, [eq("gaussian")])
gate(GAUSS6, [eq("gaussianflux")])
# flux_normal_axis / flux_direction 属于"面注入平面"的参数（由 tools_gate_values 挂在
# NFluxPerCell 上），不能挂到 gaussianflux 名下，否则 NFluxPerCell 的算例会缺发射面法向
gate((S + "momentum_function_ux(x,y,z)", S + "momentum_function_uy(x,y,z)",
      S + "momentum_function_uz(x,y,z)"), [eq("parse_momentum_function")])

# --- 子类型选择器 ---
gate((S + "maxwellian_u_mean_distribution_type", S + "maxwellian_u_std_distribution_type",
      S + "maxwellian_temperature_in_eV_distribution_type"), [eq("maxwellian")])
gate((S + "maxwell_juttner_u_mean_distribution_type",
      S + "maxwell_juttner_temperature_in_eV_distribution_type"), [eq("maxwell_juttner")])

# --- maxwellian 分支 ---
gate(MEAN, [[eq("maxwellian"), eqp(S + "maxwellian_u_mean_distribution_type", "constant")],
            [eq("maxwellian"), empty(S + "maxwellian_u_mean_distribution_type")],
            [eq("maxwell_juttner"), eqp(S + "maxwell_juttner_u_mean_distribution_type", "constant")],
            [eq("maxwell_juttner"), empty(S + "maxwell_juttner_u_mean_distribution_type")]])
gate(MEAN_F, [[eq("maxwellian"), eqp(S + "maxwellian_u_mean_distribution_type", "parser")],
              [eq("maxwell_juttner"), eqp(S + "maxwell_juttner_u_mean_distribution_type", "parser")]])
gate((S + "read_u_mean_from_path",),
     [[eq("maxwellian"), eqp(S + "maxwellian_u_mean_distribution_type", "read_from_file")],
      [eq("maxwell_juttner"), eqp(S + "maxwell_juttner_u_mean_distribution_type", "read_from_file")]])

gate(STD, [[eq("maxwellian"), eqp(S + "maxwellian_u_std_distribution_type", "constant")],
           [eq("maxwellian"), empty(S + "maxwellian_u_std_distribution_type")]])
gate(STD_F, [[eq("maxwellian"), eqp(S + "maxwellian_u_std_distribution_type", "parser")]])
gate((S + "read_u_std_from_path",),
     [[eq("maxwellian"), eqp(S + "maxwellian_u_std_distribution_type", "read_from_file")]])

# maxwellian 的"温度"与"u_std"互斥，默认走 u_std；只有显式选了温度才显示温度那一支
gate(TEMP, [[eq("maxwellian"), eqp(S + "maxwellian_temperature_in_eV_distribution_type", "constant")],
            [eq("maxwell_juttner"), eqp(S + "maxwell_juttner_temperature_in_eV_distribution_type", "constant")],
            [eq("maxwell_juttner"), empty(S + "maxwell_juttner_temperature_in_eV_distribution_type")]])
gate(TEMP_F, [[eq("maxwellian"), eqp(S + "maxwellian_temperature_in_eV_distribution_type", "parser")],
              [eq("maxwell_juttner"), eqp(S + "maxwell_juttner_temperature_in_eV_distribution_type", "parser")]])
gate(TEMP_FILE, [[eq("maxwellian"), eqp(S + "maxwellian_temperature_in_eV_distribution_type", "read_from_file")],
                 [eq("maxwell_juttner"), eqp(S + "maxwell_juttner_temperature_in_eV_distribution_type", "read_from_file")]])

# --- 旧的（错误的）规则要先清掉 ---
CLEAR = {S + "ux", S + "uy", S + "uz", *GAUSS6, *STD, *STD_F, *MEAN, *MEAN_F, *TEMP, *TEMP_F,
         *TEMP_FILE, *UNIFORM6, S + "momentum_function_ux(x,y,z)",
         S + "momentum_function_uy(x,y,z)", S + "momentum_function_uz(x,y,z)",
         S + "read_u_mean_from_path"}

ENUM_VALUES = ["at_rest", "constant", "uniform", "gaussian", "gaussianflux",
               "maxwellian", "maxwell_juttner", "parse_momentum_function"]

GLOSSARY = {
    "at_rest": "静止（动量为 0）",
    "constant": "常数值（ux/uy/uz 给三个方向的分量）",
    "uniform": "均匀分布（用 ux_min/ux_max 等给出每个方向的上下限）",
    "gaussian": "高斯分布（ux_m/uy_m/uz_m 给中心，ux_th/uy_th/uz_th 给热展宽）",
    "gaussianflux": "高斯通量分布（只在 injection_style=NFluxPerCell 时可用，需 flux_normal_axis/flux_direction）",
    "maxwellian": "非相对论麦克斯韦分布（u_mean + u_std 或 temperature，三者各有 constant/parser/read_from_file 子类型）",
    "maxwell_juttner": "相对论 Maxwell-Jüttner 分布（u_mean + temperature，没有 u_std）",
    "parse_momentum_function": "用表达式给动量（momentum_function_ux/uy/uz(x,y,z)）",
}


def main() -> int:
    cat_path = DATA / "catalog.json"
    cat = json.loads(cat_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in cat["params"]}

    for n, t in NEW_PARAMS.items():
        if n not in names:
            cat["params"].append({"name": n, "type": t, "default": None, "optional": True,
                                  "doc": "uniform 分布在该方向的上下限", "core": False, "options": None})
            names.add(n)
    cat["params"].sort(key=lambda p: p["name"])
    cat["counts"]["total"] = len(cat["params"])

    rules = cat.setdefault("gating_rules", {})
    for name in CLEAR:
        rules.pop(name, None)
    for name, conds in GATES.items():
        if name in names:
            rules[name] = conds

    for p in cat["params"]:
        if p["name"] == MOM:
            p["options"] = ENUM_VALUES
    cat_path.write_text(json.dumps(cat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    enums_path = DATA / "enums.json"
    e = json.loads(enums_path.read_text(encoding="utf-8"))
    e["enums"][MOM] = ENUM_VALUES
    gl = e.setdefault("glossary", {})
    gl[MOM] = GLOSSARY
    for n in ("maxwellian_u_mean_distribution_type", "maxwellian_u_std_distribution_type",
              "maxwellian_temperature_in_eV_distribution_type",
              "maxwell_juttner_u_mean_distribution_type",
              "maxwell_juttner_temperature_in_eV_distribution_type"):
        e["enums"][S + n] = ["constant", "parser", "read_from_file"]
    enums_path.write_text(json.dumps(e, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for c in cats["categories"]:
        if c["id"] == "species":
            for n in NEW_PARAMS:
                if n not in c["params"]:
                    c["params"].append(n)
            c["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"动量分布重建：{len(GATES)} 个参数挂上规则，新增 {len(NEW_PARAMS)} 个 uniform 参数，枚举 {len(ENUM_VALUES)} 项")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
