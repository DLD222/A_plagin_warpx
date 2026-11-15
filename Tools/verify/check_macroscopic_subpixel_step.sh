#!/usr/bin/env bash
# Copyright 2026 DLD222
#
# This file is part of WarpX.
#
# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
# F-04 verification: the exponential step runs, and its E^n snapshot is fully initialised.
#
# The deck is the 1D cavity with algo.macroscopic_subpixel = 1 and sigma = 0.001, which is the only
# configuration that enters the exponential branch (and therefore reads the snapshot).  A snapshot
# left uninitialised at the staggered edges would put a dirty value - or a NaN - into the tensor
# update, so two things are checked: the run completes, and the mode decays like the analytic
# exp(-sigma t / (2 eps)) within the same tolerance the backward-Euler path meets.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
build=${BUILD:-$repo/build_multi}
run=${1:-/tmp/mac_ver/subpixel}
mkdir -p "$run"
cd "$run"
sed -e 's/^macroscopic.sigma   = 0/macroscopic.sigma   = 0.001/' \
    -e 's/^warpx.abort_on_warning_threshold = medium/warpx.abort_on_warning_threshold = high/' \
    -e '/^algo.em_solver_medium/a algo.macroscopic_subpixel = 1' \
    "$repo/Examples/Tests/macroscopic_solver/inputs_test_1d_macroscopic_solver" > inputs_subpixel
# `sed` does not fail when a pattern does not match, so a renamed/realigned input line would
# silently run the *linear* branch and still pass.  Assert that both edits landed.
grep -q '^macroscopic.sigma   = 0.001' inputs_subpixel || {
    echo "sed did not set macroscopic.sigma (deck changed?)" >&2; exit 1; }
grep -q '^algo.macroscopic_subpixel = 1' inputs_subpixel || {
    echo "sed did not add algo.macroscopic_subpixel (deck changed?)" >&2; exit 1; }
"$build/bin/warpx.1d" inputs_subpixel > run.log 2>&1
! grep -qE "nan|NaN" run.log
# The check itself now compares measured and expected (and exits non-zero on a mismatch); the
# previous version only grepped this output for the words "amplitude ratio", so any decay at all
# counted as a pass.
python3 "$repo/Tools/verify/check_macroscopic_cavity_mode.py" \
    diags/diag1000000 diags/diag1000200 0.001 | tee mode.txt
