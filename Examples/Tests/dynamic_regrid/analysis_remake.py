import sys
import numpy as np
import yt

yt.funcs.mylog.setLevel(50)
ds = yt.load(sys.argv[1])
assert ds.max_level == 1, f"level 1 missing: max_level={ds.max_level}"
grid = ds.covering_grid(0, ds.domain_left_edge, ds.domain_dimensions)
ex = np.array(grid[("boxlib", "Ex")])
assert np.all(np.isfinite(ex)), "non-finite Ex after RemakeLevel"
print("dynamic regrid RemakeLevel fields finite")
