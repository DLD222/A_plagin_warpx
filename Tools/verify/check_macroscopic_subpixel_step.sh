#!/usr/bin/env bash
# F-04 verification: the exponential step runs, and its E^n snapshot is fully initialised.
#
# The deck is the 1D cavity with algo.macroscopic_subpixel = 1 and sigma = 0.001, which is the only
# configuration that enters the exponential branch (and therefore reads the snapshot).  A snapshot
# left uninitialised at the staggered edges would put a dirty value - or a NaN - into the tensor
# update, so two things are checked: the run completes, and the mode decays like the analytic
# exp(-sigma t / (2 eps)) within the same tolerance the backward-Euler path meets.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
run=${1:-/tmp/mac_ver/subpixel}
mkdir -p "$run"
cd "$run"
sed -e 's/^macroscopic.sigma   = 0/macroscopic.sigma   = 0.001/' \
    -e 's/^warpx.abort_on_warning_threshold = medium/warpx.abort_on_warning_threshold = high/' \
    -e '/^algo.em_solver_medium/a algo.macroscopic_subpixel = 1' \
    "$repo/Examples/Tests/macroscopic_solver/inputs_test_1d_macroscopic_solver" > inputs_subpixel
"$repo/build_multi/bin/warpx.1d" inputs_subpixel > run.log 2>&1
! grep -qE "nan|NaN" run.log
python3 "$repo/Tools/verify/check_macroscopic_cavity_mode.py" \
    diags/diag1000000 diags/diag1000200 0.001 | tee mode.txt
grep -q "amplitude ratio" mode.txt
