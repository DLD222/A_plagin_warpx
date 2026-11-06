import os
import sys
import numpy as np
import yt

yt.funcs.mylog.setLevel(50)
path = sys.argv[1]
ds = yt.load(path)
assert ds.max_level == 1, f"dynamic regrid did not create level 1: max_level={ds.max_level}"

before_path = os.path.join(os.path.dirname(path), "diag1000014")
ds_before = yt.load(before_path)
assert ds_before.max_level == 0, "level 1 existed before regrid step"

def level0_field(dataset, name):
    grid = dataset.covering_grid(0, dataset.domain_left_edge, dataset.domain_dimensions)
    return np.array(grid[("boxlib", name)])

for name in ("Ex", "Ey", "Ez"):
    before = level0_field(ds_before, name)
    after = level0_field(ds, name)
    denom = max(float(np.max(np.abs(before))), 1.0e-30)
    rel = float(np.max(np.abs(after - before))) / denom
    assert rel < 5.0e-2, f"{name} changed across regrid: rel={rel}"
    assert np.all(np.isfinite(after)), f"non-finite {name} after dynamic regrid"

energy_file = os.path.join(os.path.dirname(path), "reduced_files", "field_energy.txt")
if os.path.exists(energy_file):
    rows = [line.split() for line in open(energy_file) if line.strip() and not line.startswith("#")]
    by_step = {int(r[0]): float(r[1]) for r in rows}
    e14 = by_step.get(14)
    e15 = by_step.get(15)
    if e14 and e15:
        rel_e = abs(e15 - e14) / max(abs(e14), 1.0e-30)
        assert rel_e < 1.0e-1, f"field energy jumped at regrid: rel={rel_e}"
print("dynamic regrid level1 created; level0 fields continuous")
