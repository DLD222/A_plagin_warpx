#!/usr/bin/env bash
# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
# Does the macroscopic (tensor-medium) E-update reduce to Yee in vacuum?
#
# The strongest check available for the material solver: with epsilon = epsilon0,
# mu = mu0 and sigma = 0 the macroscopic update must reproduce the Yee one
# exactly.  Run one step from the same initial condition with both solvers and
# compare max|Ex|.
#
# Usage: Tools/verify/check_macroscopic_vacuum_limit.sh [build-dir]
#
# Status 2026-11-10: PASSES.  yee max|Ex| = macro max|Ex| = 3 678 360.615965,
# ratio = 1.000000, exit code 0.
#
# History: before 7a96278 the ratio was exactly 2.000000 (macro 7 356 721.232):
# the node permittivity took its neighbour along a hard-coded axis, which in 1D
# read past the row and produced epsilon/2.  See
# Docs/research_notes/UPSTREAM_PARITY.md sections 10.1.1-10.1.5.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${1:-$REPO/build_multi_hdf5_fft}"
INPUTS="$REPO/Examples/Tests/macroscopic_solver/inputs_test_1d_macroscopic_solver"
EXE="$BUILD/bin/warpx.1d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED"
WORK="${WORK:-/tmp/macro_vacuum_limit}"
PY="${PYTHON:-$HOME/miniconda3/bin/python3}"

run () { # $1 = tag, $2 = extra parameter
    rm -rf "$WORK/$1"; mkdir -p "$WORK/$1"; cd "$WORK/$1"
    AMREX_INPUTS_FILE_PREFIX="$INPUTS_DIR" OMP_NUM_THREADS=1 mpiexec -n 1 "$EXE" \
        "$(basename "$INPUTS")" "warpx.cfl = 0.5" "max_step = 1" "diag1.intervals = 1" "$2" \
        > run.log 2>&1
}
INPUTS_DIR="$(dirname "$INPUTS")"
run yee   "algo.em_solver_medium = vacuum"
run macro "macroscopic.epsilon = epsilon0"

"$PY" - <<'PYEOF'
import glob, numpy as np, yt
yt.funcs.mylog.setLevel(0)
def last_ex(tag):
    files = sorted(glob.glob(f'/tmp/macro_vacuum_limit/{tag}/diags/diag1*'))
    ds = yt.load(files[-1])
    d = ds.covering_grid(level=0, left_edge=ds.domain_left_edge, dims=ds.domain_dimensions)
    return float(np.max(np.abs(np.array(d['Ex']).squeeze())))
yee, macro = last_ex('yee'), last_ex('macro')
print(f'yee   max|Ex| = {yee:.6f}')
print(f'macro max|Ex| = {macro:.6f}')
print(f'ratio         = {macro/yee:.6f}   (expected 1.000000)')
raise SystemExit(0 if abs(macro/yee - 1.0) < 1e-6 else 1)
PYEOF
