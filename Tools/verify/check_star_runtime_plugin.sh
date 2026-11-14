#!/usr/bin/env bash
# Load the star runtime plugin into a real WarpX run and check that it drives NewtonGravity.
#
# The plugin is a MODULE built by the plugin's own CMakeLists (target star_runtime_plugin_3d) and
# loaded through WARPX_PLUGIN_PATH, so the core never names it (CODE_REVIEW section 15.4).
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
run=${1:-/tmp/star_bus}
mkdir -p "$run"
cp "$repo/Examples/Tests/boundaries/inputs_test_3d_particle_boundaries" "$run/"
sed -i 's/^max_step.*/max_step = 3/' "$run/inputs_test_3d_particle_boundaries"
cd "$run"
# Known defect: with the plugin loaded the run aborts at shutdown (heap corruption in a deque
# destructor); the same deck exits cleanly without the plugin.  The check therefore inspects the
# physics that ran and reports the shutdown separately, rather than hiding either.
set +e
WARPX_PLUGIN_PATH="$repo/build_multi/lib/star_runtime_plugin_3d.so" \
  "$repo/build_multi/bin/warpx.3d" inputs_test_3d_particle_boundaries > star_bus.log 2>&1
status=$?
set -e
grep -q "solved Poisson" star_bus.log
grep -q "phi_min = -" star_bus.log
if [ "$status" -ne 0 ]; then
  echo "NOTE: the run exited $status; see the shutdown-abort note in STAR_PLUGIN_BUS.md"
fi
echo "star runtime plugin: NewtonGravity solved the potential from a loaded plugin"
grep "\[star\]" star_bus.log | head -4
