#!/bin/bash
# Run one variant of the RZ laser acceleration case with both the official and
# the fork build, at t=0, and print where the rho modes differ.
# Usage: run_variant.sh VARNAME "extra input lines"
set -eu
ROOT=${WARPX_AUDIT_ROOT:-$(git rev-parse --show-toplevel)}
VAR=${1:?variant name}
EXTRA=${2:-}
BASE=$ROOT/audit_runs/round11/rz_laser
D=$BASE/var_$VAR
rm -rf "$D"
mkdir -p "$D/ref" "$D/new"
for side in ref new; do
    cp "$ROOT/Examples/Physics_applications/laser_acceleration/inputs_base_rz" "$D/$side/"
    cp "$ROOT/Examples/Physics_applications/laser_acceleration/inputs_test_rz_laser_acceleration" "$D/$side/"
    sed -i 's/^max_step = 10/max_step = 0/' "$D/$side/inputs_base_rz"
    printf 'max_step = 0\n%s\n' "$EXTRA" >> "$D/$side/inputs_test_rz_laser_acceleration"
done
run_one() {
    local side=$1 bin=$2
    ( cd "$D/$side" && OMP_NUM_THREADS=1 \
        "$ROOT/audit_build/cleanenv.sh" /usr/bin/mpiexec -np 1 --allow-run-as-root \
        "$bin" inputs_test_rz_laser_acceleration > run.log 2>&1 ) || {
        echo "FAILED: $side"; tail -20 "$D/$side/run.log"; return 1; }
}
run_one ref  "$ROOT/audit_build/official/bin/warpx.rz"
run_one new  "$ROOT/audit_build/fork/bin/warpx.rz"
python3 "$ROOT/Docs/audit_2026_11/scripts/rz_row_diff.py" \
    "$D/ref/diags/diag1000000" "$D/new/diags/diag1000000" \
    --vars rho_0_real,rho_1_real,rho_1_imag 2>&1 | grep -v '^yt :' | head -12
