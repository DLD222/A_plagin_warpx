import os
import sys
import numpy as np
import yt

yt.funcs.mylog.setLevel(50)
path = sys.argv[1]
root = os.path.dirname(path)

def energy(step):
    ds = yt.load(os.path.join(root, f"diag10000{step:02d}"))
    grid = ds.covering_grid(ds.max_level, ds.domain_left_edge, ds.domain_dimensions)
    total = 0.0
    for name in ("Ex", "Ey", "Ez", "Bx", "By", "Bz"):
        total += float(np.sum(np.array(grid[("boxlib", name)]) ** 2))
    return ds.max_level, total

level14, _ = energy(14)
level15, e15 = energy(15)
assert level14 == 0, f"level 1 before regrid: {level14}"
assert level15 == 1, f"level 1 after regrid: {level15}"
for step in range(15, 41):
    level, e = energy(step)
    assert level == 1, f"level changed at step {step}: {level}"
    rel = abs(e - e15) / max(e15, 1.0e-30)
    assert rel < 1.0e-12, f"field energy drift at step {step}: rel={rel}"
print(f"uniform B energy conserved: {e15}")
