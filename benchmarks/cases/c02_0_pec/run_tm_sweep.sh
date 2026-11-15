#!/usr/bin/env bash
# A: TM normal-incidence PEC reflection, resolution sweep. Expect r -> -1.
set -e
cd "$(dirname "$0")"
# Absolute default: the script cds into its own directory, so a relative path here
# resolved to benchmarks/cases/c02_0_pec/build_2d/... and never existed.
REPO=$(cd "$(dirname "$0")/../../../.." && pwd)
BIN=${WARPX_BIN:-$REPO/build_2d/bin/warpx.2d.MPI.OMP.DP.PDP.EB}
for n in 384 768 1152; do
  sed -e "s/amr.n_cell = 384 384/amr.n_cell = $n $n/" -e "s/max_step = 800/max_step = 20000/" inputs_2d_tm_flat > "inputs_tm_${n}"
  echo "=== n = $n ==="
  OMP_NUM_THREADS=8 "$BIN" "inputs_tm_${n}" > /tmp/tm_${n}.log 2>&1
  cp diags/reducedfiles/fp_inc.txt "probe_tm_${n}.txt"
  python3 analyze_tm.py "probe_tm_${n}.txt"
  echo
done
