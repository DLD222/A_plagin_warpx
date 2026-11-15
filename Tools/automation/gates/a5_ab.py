# SPDX-License-Identifier: BSD-3-Clause
"""Deterministic handler A/B gate: same inputs with/without PluginHost handlers.

Runs every case twice (handlers enabled / disabled through
WARPX_DISABLE_PLUGIN_HANDLERS) and demands bit-identical fields, so any visible
change from the plugin dispatch shows up here.

The binary is located through WARPX_BIN, then $WARPX_BUILD_DIR/bin/warpx.2d,
then the decorated build names - a hard-coded "build/bin/warpx.2d.MPI..." only
worked for one build configuration (Tools_Tests/F28).  The scratch directories
are removed even when a run fails.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
import yt

ROOT = Path(__file__).resolve().parents[3]
CASES = [
    ("langmuir", [ROOT/"Examples/Tests/langmuir/inputs_base_2d",
                  ROOT/"Examples/Tests/langmuir/inputs_test_2d_langmuir_multi"],
     ["warpx.random_seed = 12345"], "diag1000080"),
    ("collision", [ROOT/"Examples/Tests/collision/inputs_test_2d_collision_xz"],
     ["warpx.random_seed = 2034958209"], "diag1000150"),
    ("eb_cube", [ROOT/"Examples/Tests/embedded_boundary_cube/inputs_test_2d_embedded_boundary_cube"],
     [], "diag1000114"),
]


def find_exe() -> Path:
    """Return the 2D executable to test."""
    explicit = os.environ.get("WARPX_BIN")
    if explicit:
        exe = Path(explicit)
        if not exe.exists():
            raise SystemExit(f"WARPX_BIN does not exist: {exe}")
        return exe
    build = Path(os.environ.get("WARPX_BUILD_DIR", ROOT / "build")) / "bin"
    for candidate in [build / "warpx.2d", *sorted(build.glob("warpx.2d.*"))]:
        if candidate.exists():
            return candidate
    raise SystemExit(f"no warpx.2d binary under {build}; set WARPX_BIN or WARPX_BUILD_DIR")


def run(exe: Path, inputs, extra, disable):
    """Run one deck; returns the directory that holds its diagnostics."""
    base = Path(tempfile.mkdtemp(prefix="ab_"))
    for src in inputs:
        shutil.copy(src, base / src.name)
    target = base / inputs[-1].name
    target.write_text(target.read_text() + "\n" + "\n".join(extra) + "\n")
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = "1"
    if disable:
        env["WARPX_DISABLE_PLUGIN_HANDLERS"] = "1"
    subprocess.run([str(exe), target.name], cwd=base, env=env, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return base


def fields(path):
    ds = yt.load(str(path))
    grid = ds.covering_grid(0, ds.domain_left_edge, ds.domain_dimensions)
    return {f: np.array(grid[("boxlib", f)]) for f in ["Ex", "Ey", "Ez", "Bx", "By", "Bz"]}


def main():
    exe = find_exe()
    wanted = sys.argv[1:]
    cases = [c for c in CASES if not wanted or c[0] in wanted]
    if not cases:
        raise SystemExit(f"no case matches {wanted}; known: {[c[0] for c in CASES]}")
    print(f"a5_ab: executable {exe}")
    for name, inputs, extra, plot in cases:
        on_dir = off_dir = None
        try:
            on_dir = run(exe, inputs, extra, False)
            off_dir = run(exe, inputs, extra, True)
            on = fields(on_dir / "diags" / plot)
            off = fields(off_dir / "diags" / plot)
        finally:
            for directory in (on_dir, off_dir):
                if directory is not None:
                    shutil.rmtree(directory, ignore_errors=True)
        for f in on:
            if not np.array_equal(on[f], off[f]):
                print(f"A/B FAIL {name} {f} maxdiff={np.max(np.abs(on[f]-off[f]))}")
                return 1
        print(f"A/B PASS {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
