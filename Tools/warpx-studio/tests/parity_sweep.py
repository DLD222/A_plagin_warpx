"""按维度批量跑官方算例的输入/输出一致性（验收项 5）。

挑选规则：只取 `Examples/Tests` 下**自包含**（无 FILE 指令也能通过内联处理）、
有 `max_step` 和 `geometry.dims`、且网格小/步数少的算例，按代价排序取前 N 个。

用法:
  python tests/parity_sweep.py --dim 1d --limit 10
"""
from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[3]


def candidates(dim: str, cells_max: int, steps_max: int) -> list[pathlib.Path]:
    want = "RZ" if dim.lower() in ("rz", "r-z") else (dim[:-1] if dim.endswith("d") else dim)
    out = []
    for f in sorted((REPO / "Examples/Tests").rglob("inputs*")):
        if not f.is_file() or f.suffix == ".py":
            continue
        text = f.read_text(errors="ignore")
        m = re.search(r"max_step\s*=\s*(\d+)", text)
        d = re.search(r"geometry\.dims\s*=\s*(\d|RZ|rz)", text)
        if not m or not d or d.group(1).upper() != str(want).upper():
            continue
        # 需要外部数据文件的算例无法自包含地跑（两边都会读失败），跳过
        if re.search(r"load_density|read_from_file|injection_file|read_[a-z_]*_from_path|"
                     r"\.h5\b|openpmd_\*", text):
            continue
        step = int(m.group(1))
        if step > steps_max:
            continue
        ncell = re.search(r"amr\.n_cell\s*=\s*([0-9 ]+)", text)
        cells = 1
        if ncell:
            for part in ncell.group(1).split():
                try:
                    cells *= int(part)
                except ValueError:
                    pass
        if cells > cells_max:
            continue
        out.append((step, cells, f))
    out.sort()
    return [f for _, _, f in out]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dim", default="2d")
    ap.add_argument("--limit", type=int, default=8)
    ap.add_argument("--cells-max", type=int, default=20000)
    ap.add_argument("--steps-max", type=int, default=200)
    ap.add_argument("--tolerance", type=float, default=1e-10)
    args = ap.parse_args()

    files = candidates(args.dim, args.cells_max, args.steps_max)[: args.limit]
    if not files:
        print("没有符合条件的算例")
        return 2
    passed, failed = [], []
    for f in files:
        rel = f.relative_to(REPO)
        r = subprocess.run([sys.executable, str(REPO / "Tools/warpx-studio/tests/parity_run.py"),
                            "--examples", str(rel), "--dim", args.dim,
                            "--tolerance", str(args.tolerance)],
                           capture_output=True, text=True)
        ok = r.returncode == 0
        (passed if ok else failed).append(str(rel))
        print(("PASS " if ok else "FAIL ") + str(rel))
        if not ok:
            for line in (r.stdout or "").splitlines():
                if line.strip().startswith(("B/", "E/", "j/", "缺少", "rho")):
                    print("     ", line.strip())
    print(f"\n{args.dim}: {len(passed)}/{len(files)} 通过")
    return 0 if not failed else 1


if __name__ == "__main__":
    raise SystemExit(main())
