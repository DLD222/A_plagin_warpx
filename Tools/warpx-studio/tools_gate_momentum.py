"""动量注入的两级联动：momentum_distribution_type → 子类型 → 具体参数。

WarpX 的链条（源码 TemperatureProperties.cpp 已核对）：
  momentum_distribution_type = constant            -> ux / uy / uz
                             = gaussian            -> ux_m/uy_m/uz_m, ux_th/uy_th/uz_th
                             = maxwellian          -> u_mean / u_std / temperature 三选一，各自还有子类型
                             = maxwell_juttner     -> 同上（相对论版本）
                             = parse_momentum_function -> momentum_function_ux/uy/uz(x,y,z)
子类型取值：constant / parser / read_from_file
  u_mean:  constant -> ux_mean,uy_mean,uz_mean
           parser   -> ux_mean_function(x,y,z),...
           read_from_file -> read_u_mean_from_path
  u_std:   constant -> ux_std,uy_std,uz_std
           parser   -> ux_std_function(x,y,z),...
           read_from_file -> read_u_std_from_path
  T(eV):   constant -> temperature_in_eV
           parser   -> temperature_in_eV_function(x,y,z)
           read_from_file -> read_temperature_in_eV_from_path
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
S = "<species_name>."


def eq(param: str, value: str) -> dict:
    return {"param": param, "op": "equals", "value": value, "raw": "momentum gate"}


def empty(param: str) -> dict:
    return {"param": param, "op": "empty", "value": None, "raw": "momentum gate"}


def or_empty(param: str, value: str) -> list:
    """未填时按默认（constant）处理。"""
    return [[eq(param, value)], [empty(param)]]


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})

    def add(targets, conds):
        for t in targets:
            if t not in names:
                continue
            rules.setdefault(t, [])
            for c in conds:
                if c not in rules[t]:
                    rules[t].append(c)

    # ---- constant / parse_momentum_function ----
    add([S + "ux", S + "uy", S + "uz"], or_empty(S + "momentum_distribution_type", "constant"))
    add([S + "momentum_function_ux(x,y,z)", S + "momentum_function_uy(x,y,z)",
         S + "momentum_function_uz(x,y,z)"],
        [[eq(S + "momentum_distribution_type", "parse_momentum_function")]])

    # ---- 三个子类型选择器本身：只在 maxwellian / maxwell_juttner 时出现 ----
    subs = [S + "maxwellian_u_mean_distribution_type", S + "maxwellian_u_std_distribution_type",
            S + "maxwellian_temperature_in_eV_distribution_type",
            S + "maxwell_juttner_u_mean_distribution_type",
            S + "maxwell_juttner_temperature_in_eV_distribution_type"]
    add(subs, [[eq(S + "momentum_distribution_type", "maxwellian")],
               [eq(S + "momentum_distribution_type", "maxwell_juttner")]])

    # ---- maxwellian 的三个分支 ----
    add([S + "ux_mean", S + "uy_mean", S + "uz_mean"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_u_mean_distribution_type", "constant")],
         [eq(S + "momentum_distribution_type", "maxwellian"),
          empty(S + "maxwellian_u_mean_distribution_type")]])
    add([S + "ux_mean_function(x,y,z)", S + "uy_mean_function(x,y,z)", S + "uz_mean_function(x,y,z)"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_u_mean_distribution_type", "parser")]])
    add([S + "read_u_mean_from_path"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_u_mean_distribution_type", "read_from_file")],
         [eq(S + "momentum_distribution_type", "maxwell_juttner"),
          eq(S + "maxwell_juttner_u_mean_distribution_type", "read_from_file")]])

    add([S + "ux_std", S + "uy_std", S + "uz_std"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_u_std_distribution_type", "constant")],
         [eq(S + "momentum_distribution_type", "maxwellian"),
          empty(S + "maxwellian_u_std_distribution_type")]])
    add([S + "ux_std_function(x,y,z)", S + "uy_std_function(x,y,z)", S + "uz_std_function(x,y,z)"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_u_std_distribution_type", "parser")]])
    add([S + "read_u_std_from_path"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_u_std_distribution_type", "read_from_file")],
         [eq(S + "momentum_distribution_type", "maxwell_juttner"),
          eq(S + "maxwell_juttner_temperature_in_eV_distribution_type", "read_from_file")]])

    add([S + "temperature_in_eV"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_temperature_in_eV_distribution_type", "constant")],
         [eq(S + "momentum_distribution_type", "maxwell_juttner"),
          eq(S + "maxwell_juttner_temperature_in_eV_distribution_type", "constant")]])
    add([S + "temperature_in_eV_function(x,y,z)"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_temperature_in_eV_distribution_type", "parser")],
         [eq(S + "momentum_distribution_type", "maxwell_juttner"),
          eq(S + "maxwell_juttner_temperature_in_eV_distribution_type", "parser")]])
    add([S + "read_temperature_in_eV_from_path", S + "temperature_in_eV_mesh_name"],
        [[eq(S + "momentum_distribution_type", "maxwellian"),
          eq(S + "maxwellian_temperature_in_eV_distribution_type", "read_from_file")],
         [eq(S + "momentum_distribution_type", "maxwell_juttner"),
          eq(S + "maxwell_juttner_temperature_in_eV_distribution_type", "read_from_file")]])

    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"动量两级联动规则已写入，gating_rules 共 {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
