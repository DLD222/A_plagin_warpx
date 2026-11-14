#!/usr/bin/env bash
# Copyright 2026 DLD222
#
# This file is part of WarpX.
#
# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
# Verify the two independent star runtime plugins on a real deck.
#
# Both are MODULE libraries built from an OBJECT library and linking no host code, so the process
# keeps a single copy of AMReX (an earlier version linked the archive into the .so and died with
# "free(): invalid pointer" at exit).  They know nothing about each other: the coupling is four
# field names on the data bus.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
build=${1:-$repo/build_star}
run=${2:-/tmp/star_bus}
mkdir -p "$run"
cp "$repo/Examples/Tests/boundaries/inputs_test_3d_particle_boundaries" "$run/"
sed -i 's/^max_step.*/max_step = 3/' "$run/inputs_test_3d_particle_boundaries"
cd "$run"
for one in star_gravity_plugin star_fluid_plugin; do
  WARPX_PLUGIN_PATH="$build/lib/$one.so" "$build/bin/warpx.3d" \
    inputs_test_3d_particle_boundaries > "$one.log" 2>&1
  echo "$one: exit $?"
done
WARPX_PLUGIN_PATH="$build/lib/star_gravity_plugin.so:$build/lib/star_fluid_plugin.so" \
  "$build/bin/warpx.3d" inputs_test_3d_particle_boundaries > both.log 2>&1
echo "both: exit $?"
grep -q "\[gravity\] step 1" both.log
grep -q "g from bus: yes" both.log
grep "\[gravity\]\|\[fluid\]" both.log | head -4
