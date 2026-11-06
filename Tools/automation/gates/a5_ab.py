# SPDX-License-Identifier: BSD-3-Clause
"""Deterministic handler A/B gate: same inputs with/without PluginHost handlers."""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path
import numpy as np
import yt

ROOT = Path(__file__).resolve().parents[3]
EXE = ROOT / "build/bin/warpx.2d.MPI.OMP.DP.PDP.EB.QED"
CASES = [
    ("langmuir", [ROOT/"Examples/Tests/langmuir/inputs_base_2d",
                  ROOT/"Examples/Tests/langmuir/inputs_test_2d_langmuir_multi"],
     ["warpx.random_seed = 12345"], "diag1000080"),
    ("collision", [ROOT/"Examples/Tests/collision/inputs_test_2d_collision_xz"],
     ["warpx.random_seed = 2034958209"], "diag1000150"),
    ("eb_cube", [ROOT/"Examples/Tests/embedded_boundary_cube/inputs_test_2d_embedded_boundary_cube"],
     [], "diag1000114"),
]

def run(name, inputs, extra, disable):
    base = Path(tempfile.mkdtemp(prefix=f"ab_{name}_"))
    for src in inputs:
        shutil.copy(src, base / src.name)
    target = base / inputs[-1].name
    target.write_text(target.read_text() + "\n" + "\n".join(extra) + "\n")
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = "1"
    if disable:
        env["WARPX_DISABLE_PLUGIN_HANDLERS"] = "1"
    subprocess.run([str(EXE), target.name], cwd=base, env=env, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return base

def fields(path):
    ds = yt.load(str(path))
    grid = ds.covering_grid(0, ds.domain_left_edge, ds.domain_dimensions)
    return {f: np.array(grid[("boxlib", f)]) for f in ["Ex","Ey","Ez","Bx","By","Bz"]}

def main():
    for name, inputs, extra, plot in CASES:
        on = fields(run(name, inputs, extra, False) / "diags" / plot)
        off = fields(run(name, inputs, extra, True) / "diags" / plot)
        for f in on:
            if not np.array_equal(on[f], off[f]):
                print(f"A/B FAIL {name} {f} maxdiff={np.max(np.abs(on[f]-off[f]))}")
                return 1
        print(f"A/B PASS {name}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
