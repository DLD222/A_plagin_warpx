"""从已有 openPMD 输出派生质量密度（不改输入、不重跑）。

WarpX 原生没有 mass_density 诊断场，只有 rho_<species>（电荷密度）。
对电荷、质量恒定的物种：rho_m = (m/q) * rho_q。
当前项目：electron/positron，m_e、±q_e。
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

import h5py
import numpy as np

Q_E = 1.602176634e-19
M_E = 9.1093837015e-31


def species_props(inputs: Path) -> dict[str, tuple[float, float]]:
    text = inputs.read_text(errors="ignore") if inputs.is_file() else ""
    names = []
    m = re.search(r"^particles\.species_names\s*=\s*(.+)$", text, re.M)
    if m:
        names = m.group(1).replace('"', "").split()
    props = {}
    for name in names:
        mass = charge = None
        for line in text.splitlines():
            s = line.strip()
            if s.startswith(f"{name}.mass"):
                v = s.split("=", 1)[1].strip()
                mass = M_E if "m_e" in v else float(v)
            if s.startswith(f"{name}.charge"):
                v = s.split("=", 1)[1].strip()
                charge = -Q_E if v.startswith("-") else Q_E
            if s.startswith(f"{name}.species_type"):
                v = s.split("=", 1)[1].strip().strip('"')
                if v == "electron":
                    mass, charge = M_E, -Q_E
                elif v == "positron":
                    mass, charge = M_E, Q_E
        if mass is not None and charge not in (None, 0.0):
            props[name] = (mass, charge)
    return props


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir", type=Path)
    args = ap.parse_args()
    run = args.run_dir
    props = species_props(run / "inputs")
    files = sorted((run / "diags" / "diag1").glob("openpmd_*.h5"))
    if not files:
        files = sorted(run.rglob("openpmd_*.h5"))
    if not files or not props:
        print("no openPMD fields or no species with mass/charge")
        return 1
    out = run / "mass_density.h5"
    with h5py.File(out, "w") as f:
        f.attrs["description"] = "Derived mass density from rho_<species>; rho_m=(m/q) rho_q"
        f.attrs["units"] = "kg/m^3"
        for fn in files:
            it = int(fn.stem.split("_")[-1])
            with h5py.File(fn, "r") as h:
                g = h[f"data/{it}/fields"]
                total = None
                grp = f.create_group(f"data/{it}")
                for sp, (mass, charge) in props.items():
                    key = f"rho_{sp}"
                    if key not in g:
                        continue
                    rho_m = g[key][:] * (mass / charge)
                    grp.create_dataset(f"mass_density_{sp}", data=rho_m)
                    total = rho_m if total is None else total + rho_m
                if total is not None:
                    grp.create_dataset("mass_density", data=total)
    print(f"wrote {out} ({len(files)} iterations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
