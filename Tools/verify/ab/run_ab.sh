#!/usr/bin/env bash
#
# Run one input through the upstream reference binary and ours, then compare.
#
# The comparison that backs UPSTREAM_PARITY.md used to live in a scratch directory;
# this is its checked-in form.  Point the two binaries at a case from Examples/:
#
#   export UP_BIN=/tmp/warpx_up2610/build_1d/bin/warpx.1d.MPI.OMP.DP.PDP.OPMD.QED
#   export MINE_BIN=$PWD/build_multi_hdf5_fft/bin/warpx.1d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED
#   Tools/verify/ab/run_ab.sh d1_langmuir warp:Examples/Tests/langmuir/inputs_test_1d_langmuir_multi
#   python3 Tools/verify/ab/compare_fields.py
#
# The binaries must be built with the same MPI as `mpiexec` (or set MPIEXEC); on this
# machine the upstream reference had to be built against the system OpenMPI - a
# conda-prefixed build links MPICH and OpenMPI together and aborts with
# "MPII_Comm_set_attr: Invalid communicator".
#
# Usage: run_ab.sh <case-name> <dims|warp:path> <input> [extra warpx args...]
#   The first argument is only kept for backwards compatibility; the input path is
#   what matters.  Everything after it is passed to both runs.
set -uo pipefail

REPO=$(cd "$(dirname "$0")/../../.." && pwd)
NAME=$1
INP=$2
INP=${INP#warp:}
shift 2
[ -f "$REPO/$INP" ] || INP=${INP}
UP=${UP_BIN:?set UP_BIN to the upstream reference executable}
MINE=${MINE_BIN:?set MINE_BIN to the executable under test}

for side in up mine; do
    rm -rf "$NAME/$side"; mkdir -p "$NAME/$side"
    (
        cd "$NAME/$side" || exit 1
        if [ "$side" = up ]; then EXE=$UP; else EXE=$MINE; fi
        AMREX_INPUTS_FILE_PREFIX="$REPO/$(dirname "$INP")" OMP_NUM_THREADS=1 \
            timeout 900 "${MPIEXEC:-mpiexec}" -n 1 "$EXE" "$(basename "$INP")" "$@" > run.log 2>&1
        echo "$side exit=$?"
    )
done
