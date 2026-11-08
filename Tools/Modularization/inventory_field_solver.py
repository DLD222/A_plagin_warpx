#!/usr/bin/env python3
"""Inventory FieldSolver implementation files and map each to a solver module."""
from pathlib import Path
import re, sys
ROOT = Path(__file__).resolve().parents[2]
FS = ROOT / "Source/Plugins/PicEm/Modules/FieldSolver"
OUT = ROOT / "Docs/research_notes/FIELD_SOLVER_INVENTORY.md"

def classify(rel: Path):
    p = rel.as_posix()
    if p.startswith("FiniteDifference/Algorithms") or "FiniteDifference/FiniteDifferenceAlgorithms" in p:
        return "FiniteDifference", "FieldUpdateAlgorithm"
    if "HybridPIC" in p:
        return "FiniteDifference/HybridPIC", "HybridPICSolveElectricField"
    if "Macroscopic" in p:
        return "FiniteDifference/Macroscopic", "MacroscopicFieldUpdate"
    if "ApplySilverMuellerBoundary" in p:
        return "FiniteDifference/Boundaries", "ApplySilverMuellerBoundary"
    if "ComputeCurlA" in p:
        return "FiniteDifference/Operators", "ComputeCurlA"
    if "ComputeCurlB" in p:
        return "FiniteDifference/Operators", "ComputeCurlB"
    if "ComputeDivE" in p:
        return "FiniteDifference/Operators", "ComputeDivE"
    if "ComputeGradient" in p:
        return "FiniteDifference/Operators", "ComputeGradient"
    if "ComputeLaplacian" in p:
        return "FiniteDifference/Operators", "ComputeLaplacian"
    if rel.name == "FiniteDifferenceSolverFactory.cpp":
        return "FieldSolver/Factory", "FiniteDifferenceSolverFactory"
    if rel.name == "FiniteDifferenceSolver.cpp":
        return "FiniteDifference/Driver", "FiniteDifferenceFieldSolver"
    if p == "WarpXSolveFieldsES.cpp":
        return "Electrostatic/Driver", "SolveElectrostaticFields"
    if p == "WarpX_QED_Field_Pushers.cpp":
        return "QEDFieldPush", "QEDFieldPush"
    if p == "WarpXPushFieldsEM.cpp":
        return "Electromagnetic/Driver", "PushElectromagneticFields"
    if "EvolveF" in p:
        return "FiniteDifference/Units", "EvolveF"
    if "EvolveG" in p:
        return "FiniteDifference/Units", "EvolveG"
    if "EvolveE" in p or "EvolveB" in p:
        return "FiniteDifference/Units", "EvolveElectromagneticField"
    if "PML" in p:
        return "FiniteDifference/PML", "PerfectlyMatchedLayerUpdate"
    if p.startswith("Electrostatic"):
        return "Electrostatic", "ElectrostaticFieldSolve"
    if p.startswith("PSATD/SpectralAlgorithms"):
        return "PSATD/SpectralAlgorithms", "SpectralFieldUpdate"
    if p.startswith("PSATD/SpectralHankelTransform"):
        return "PSATD/Hankel", "HankelTransform"
    if p.startswith("PSATD"):
        return "PSATD", "SpectralFieldData"
    if p.startswith("Implicit"):
        return "Implicit", "ImplicitFieldSolve"
    if p.startswith("Magnetostatic"):
        return "Magnetostatic", "MagnetostaticFieldSolve"
    return "Shared", "FieldSolverShared"

def main():
    files = sorted(FS.rglob("*.cpp"))
    rows = []
    unclassified = []
    for f in files:
        rel = f.relative_to(FS)
        module, unit = classify(rel)
        text = f.read_text(errors="ignore")
        rows.append({
            "module": module, "unit": unit, "file": str(rel),
            "lines": len(text.splitlines()),
            "warp": text.count('#include "WarpX.H"'),
            "particles": text.count('#include "Particles/'),
        })
    if not files:
        print("no FieldSolver sources found", file=sys.stderr)
        return 1
    # A7.3.0 gate: every file must map to a named unit.
    for r in rows:
        if r["unit"] == "FieldSolverShared":
            unclassified.append(r["file"])
    lines = ["# FieldSolver 实现清单", "",
             f"文件总数：{len(rows)}", "",
             "| 模块 | 单元 | 文件 | 行数 | WarpX.H | Particles |",
             "|---|---|---|---:|---:|---:|"]
    for r in rows:
        lines.append(f"| {r['module']} | {r['unit']} | `{r['file']}` | {r['lines']} | {r['warp']} | {r['particles']} |")
    lines += ["", "## 汇总", ""]
    mods = {}
    for r in rows:
        mods.setdefault(r["module"], [0,0])[0] += 1
        mods[r["module"]][1] += r["lines"]
    for m, (n, l) in sorted(mods.items()):
        lines.append(f"- {m}: {n} 个 CPP, {l} 行")
    lines += ["", "## 未分类", ""]
    lines += [f"- `{x}`" for x in unclassified] or ["- 无"]
    OUT.write_text("\n".join(lines) + "\n")
    print(f"wrote {OUT} ({len(rows)} files)")
    if unclassified:
        print("unclassified files:")
        for x in unclassified: print(" ", x)
        return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
