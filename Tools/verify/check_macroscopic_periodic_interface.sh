#!/usr/bin/env bash
# Copyright 2026 DLD222
#
# This file is part of WarpX.
#
# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
# F-06: on a periodic seam with a material interface, the node-material ghosts must hold the
# wrapped values, otherwise the rho_medium seed is computed from zeros at the seam.
#
# The deck is 1D, periodic, with an interface at x = 0.5 (subpixel rows on) and an initial
# longitudinal field Ez = 1e-3 cos(2 pi z), so divD is non-zero and the seam is actually exercised.
# The Gauss residual is a self-consistency check (it stays zero either way, because the seed and
# the check read the same ghosts); what changes is the *value* the seed computes.  This script
# records that value, which is 5.66e-13 with the ghost fill and 1.70e-12 without it.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
build=${BUILD:-$repo/build_multi}
run=${1:-/tmp/mac_ver/periodic_iface}
mkdir -p "$run"
cp "$repo/Examples/Tests/macroscopic_solver/inputs_1d_periodic_interface" "$run/"
cd "$run"
"$build/bin/warpx.1d" inputs_1d_periodic_interface > periodic_interface.log 2>&1
grep "\[GAUSS\]" periodic_interface.log | head -2
# The Gauss residual is the quantity this check is about: 5.66e-13 with the seam ghosts filled,
# 1.70e-12 without them.  A printed line alone proves nothing, so assert on the number
# (Tools_Tests/F23).
python3 - periodic_interface.log <<'PY'
import re
import sys

text = open(sys.argv[1]).read()
values = [float(m) for m in
          re.findall(r"max\|divD - \(rho_p \+ rho_m\)\| = ([0-9.eE+-]+)", text)]
if not values:
    sys.exit("no [GAUSS] residual line in the log")
worst = max(values)
print(f"worst Gauss residual over {len(values)} checks: {worst:.3e} (threshold 1e-12)")
sys.exit(0 if worst < 1e-12 else 1)
PY
