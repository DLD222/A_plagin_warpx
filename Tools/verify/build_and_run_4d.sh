#!/usr/bin/env bash
# Four-dimensional verification build: 1D/3D/RZ with openPMD and a parallel HDF5.
#
# Prerequisite: Tools/verify/build_parallel_hdf5.sh (defaults to /tmp/hdf5_mpi).
#
# Expected results with the code as of 2026-11-09 (see
# Docs/research_notes/S_SERIES_STATUS.md, section R5):
#   - 469 tests, ~444 pass
#   - failures: the *_boosted_opmd cases need -DWarpX_FFT=ON (psatd),
#     the load_external_field cases need the openPMD-example-datasets download,
#     and test_3d_particles_in_pml_mr is slow enough to time out under -j 12.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
HDF5_ROOT="${HDF5_ROOT:-/tmp/hdf5_mpi}"
BUILD="${BUILD:-$REPO/build_multi_hdf5}"
CONDA="${CONDA_PREFIX_PREFIX:-$HOME/miniconda3}"

cmake -S "$REPO" -B "$BUILD" \
    -DCMAKE_PREFIX_PATH="$CONDA" \
    -DHDF5_ROOT="$HDF5_ROOT" -DHDF5_DIR="$HDF5_ROOT" \
    -DWarpX_DIMS="1;3;RZ" -DWarpX_OPENPMD=ON -DWarpX_PYTHON=OFF \
    -DWarpX_COMPUTE=OMP -DWarpX_QED=ON \
    "${@:2}"
cmake --build "$BUILD" -j "$(nproc)"
ctest --test-dir "$BUILD" -j 8 --output-on-failure
