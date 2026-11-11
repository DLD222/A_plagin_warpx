#!/usr/bin/env bash
#
# Run the whole A/B comparison set from UPSTREAM_PARITY.md, then print the table.
#
# Each case names its input; the binaries are selected per dimensionality, so export
# the ones you have (see Tools/verify/ab/README.md for how the reference is built):
#
#   export UP_BIN_1D=...    MINE_BIN_1D=...
#   export UP_BIN_2D=...    MINE_BIN_2D=...
#   export UP_BIN_3D=...    MINE_BIN_3D=...
#   export UP_BIN_RZ=...    MINE_BIN_RZ=...
#
#   Tools/verify/ab/run_all.sh            # every case
#   Tools/verify/ab/run_all.sh d1 d2      # only cases whose name starts with these prefixes
#
# The scripts are run from the current directory, so each case leaves its output in
# ./<case>/{up,mine}.  Missing binary variables skip that case with a message rather
# than failing, so a partial setup still produces a partial table.
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../../.." && pwd)
FILTER=("$@")

# case name | dimensionality | input
CASES=(
    "d1_langmuir|1D|Examples/Tests/langmuir/inputs_test_1d_langmuir_multi"
    "d1_collision|1D|Examples/Tests/collision/inputs_test_1d_collision_z"
    "d1_implicit|1D|Examples/Tests/implicit/inputs_test_1d_theta_implicit_planar_pinch"
    "d2_langmuir|2D|Examples/Tests/langmuir/inputs_test_2d_langmuir_multi"
    "d2_plasma_mirror|2D|Examples/Physics_applications/plasma_mirror/inputs_test_2d_plasma_mirror"
    "d2_collision|2D|Examples/Tests/collision/inputs_test_2d_collision_xz"
    "d3_langmuir|3D|Examples/Tests/langmuir/inputs_test_3d_langmuir_multi"
    "d3_implicit|3D|Examples/Tests/implicit/inputs_test_3d_theta_implicit_jfnk_direct"
    "d3_laser|3D|Examples/Physics_applications/laser_acceleration/inputs_test_3d_laser_acceleration"
    "d3_eb|3D|Examples/Tests/embedded_boundary_cube/inputs_test_3d_embedded_boundary_cube"
    "d3_collision|3D|Examples/Tests/collision/inputs_test_3d_collision_iso"
    "d3_bw_two|3D|Examples/Tests/linear_breit_wheeler/inputs_test_3d_linear_breit_wheeler_two_photons"
    "d3_bw_many|3D|Examples/Tests/linear_breit_wheeler/inputs_test_3d_linear_breit_wheeler_many_photons"
    "rz_collision|RZ|Examples/Tests/collision/inputs_test_rz_collision"
    "rz_es|RZ|Examples/Tests/electrostatic_sphere/inputs_test_rz_electrostatic_sphere"
    "rz_langmuir|RZ|Examples/Tests/langmuir/inputs_test_rz_langmuir_multi"
    "rz_laser|RZ|Examples/Physics_applications/laser_acceleration/inputs_test_rz_laser_acceleration"
)

short_dim() {
    case "$1" in
        1D) echo 1D ;;
        2D) echo 2D ;;
        3D) echo 3D ;;
        RZ) echo RZ ;;
    esac
}

ran=0
skipped=0
for entry in "${CASES[@]}"; do
    IFS='|' read -r name dim input <<<"$entry"
    if [ ${#FILTER[@]} -gt 0 ]; then
        match=0
        for prefix in "${FILTER[@]}"; do
            case "$name" in "$prefix"*) match=1 ;; esac
        done
        [ $match -eq 1 ] || continue
    fi

    up_var="UP_BIN_$dim"
    mine_var="MINE_BIN_$dim"
    up=${!up_var:-}
    mine=${!mine_var:-}
    if [ -z "$up" ] || [ -z "$mine" ]; then
        echo "skip $name: set $up_var and $mine_var to compare it"
        skipped=$((skipped + 1))
        continue
    fi
    if [ ! -f "$REPO/$input" ]; then
        echo "skip $name: $input not found under $REPO"
        skipped=$((skipped + 1))
        continue
    fi

    echo "== $name ($dim)"
    UP_BIN="$up" MINE_BIN="$mine" bash "$HERE/run_ab.sh" "$name" "warp:$input"
    ran=$((ran + 1))
done

echo
echo "ran $ran case(s), skipped $skipped"
[ $ran -gt 0 ] && python3 "$HERE/compare_fields.py"
exit 0
