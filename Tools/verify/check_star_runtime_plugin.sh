#!/usr/bin/env bash
# Copyright 2026 DLD222
#
# This file is part of WarpX.
#
# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
# Load the star runtime plugin into a real WarpX run and check that it drives NewtonGravity.
#
# The plugin is a MODULE built by the plugin's own CMakeLists (target star_runtime_plugin_3d) and
# loaded through WARPX_PLUGIN_PATH, so the core never names it (CODE_REVIEW section 15.4).
#
# A run that does not exit 0 is a failure: this script used to accept a non-zero status (the deck
# aborts at shutdown with a heap corruption in a deque destructor when the plugin is loaded) and
# still print success, which turned "loading the plugin crashes the process" into a "NOTE".
# Pass --allow-shutdown-abort to tolerate exactly that known defect, explicitly.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)

allow_shutdown_abort=0
case "${1:-}" in
    --allow-shutdown-abort) allow_shutdown_abort=1; shift ;;
esac
run=${1:-/tmp/star_bus}
mkdir -p "$run"
cp "$repo/Examples/Tests/boundaries/inputs_test_3d_particle_boundaries" "$run/"
sed -i 's/^max_step.*/max_step = 3/' "$run/inputs_test_3d_particle_boundaries"
cd "$run"

set +e
WARPX_PLUGIN_PATH="$repo/build_multi/lib/star_runtime_plugin_3d.so" \
  "$repo/build_multi/bin/warpx.3d" inputs_test_3d_particle_boundaries > star_bus.log 2>&1
status=$?
set -e

grep -q "solved Poisson" star_bus.log || {
    echo "FAIL: the plugin did not solve the potential" >&2
    tail -20 star_bus.log >&2
    exit 1
}
grep -q "phi_min = -" star_bus.log || {
    echo "FAIL: no potential minimum in the log" >&2
    exit 1
}
if [ "$status" -ne 0 ]; then
    if [ "$allow_shutdown_abort" -eq 1 ]; then
        echo "KNOWN DEFECT (explicitly allowed): the run exited $status; see STAR_PLUGIN_BUS.md"
    else
        echo "FAIL: the run exited $status (heap corruption at shutdown with the plugin loaded?)" >&2
        tail -20 star_bus.log >&2
        exit 1
    fi
fi
echo "star runtime plugin: NewtonGravity solved the potential from a loaded plugin"
grep "\[star\]" star_bus.log | head -4
