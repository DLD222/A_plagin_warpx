#!/usr/bin/env bash
# Copyright 2026 DLD222
#
# This file is part of WarpX.
#
# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
# F-03 verification: the 2D XZ normal component (Ez) is sampled at its real position.
#
# The 2D cavity deck oscillates as By = cos(kx x) cos(kz z) cos(omega t) with
# omega = c sqrt(kx^2 + kz^2)/sqrt(eps_r); Ez is the in-plane (normal) component, which is the
# path the half-cell offset used to corrupt.  A wrong Ez position shows up as a wrong omega, i.e. a
# wrong amplitude at t_end.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
run=${1:-/tmp/mac2d_v2}
build=${2:-$repo/build_2d}
mkdir -p "$run"
cp "$repo/Examples/Tests/macroscopic_solver/inputs_test_2d_macroscopic_solver" "$run/"
cd "$run"
"$build/bin/warpx.2d" inputs_test_2d_macroscopic_solver > run.log 2>&1
python3 "$repo/Tools/verify/check_macroscopic_cavity_mode_2d.py" diags/diag1000000 diags/diag1000100
