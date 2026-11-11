#!/usr/bin/env bash
# Four-dimensional verification build: 1D/3D/RZ with openPMD and a parallel HDF5.
#
# Prerequisite: Tools/verify/build_parallel_hdf5.sh (defaults to /tmp/hdf5_mpi).
#
# Expected results with the code as of 2026-11-10 (see
# Docs/research_notes/S_SERIES_STATUS.md, section R5):
#   - 469 tests without FFT; 573 with -DWarpX_FFT=ON, ~570 pass
#   - the only remaining failures are platform-dependent checksums
#
# Prerequisites beyond HDF5:
#   - openPMD-example-datasets cloned next to the repository (upstream CI does
#     the same); the load_external_field inputs resolve ../../../../ from
#     build/bin/<test>, which is the repository's parent directory.
#   - blaspp/lapackpp for RZ + FFT (conda-forge, see Tools/verify/README.md).
#   - MPLBACKEND=Agg, exported below: seven analysis scripts call plt.show() to
#     draw a diagnostic figure.  On a headless CI that is a no-op, but on a
#     machine with DISPLAY set matplotlib picks TkAgg and plt.show() blocks in
#     tkinter.mainloop() forever -- that is what made
#     test_3d_linear_compton_bunch_laser.analysis "hang" for 25 minutes while
#     the same script finishes in 0.7 s (and passes) with the Agg backend.
set -euo pipefail

export MPLBACKEND=Agg

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
HDF5_ROOT="${HDF5_ROOT:-/tmp/hdf5_mpi}"
BUILD="${BUILD:-$REPO/build_multi_hdf5}"
CONDA="${CONDA_PREFIX_PREFIX:-$HOME/miniconda3}"

# Runtime libraries: the parallel HDF5, FFTW and blaspp/lapackpp live outside
# the default loader path.
export LD_LIBRARY_PATH="${HDF5_ROOT}/lib:${HOME}/third_party/fftw/lib:${HOME}/third_party/fft-deps/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Some analysis scripts (e.g. ionization_dsmc) import pywarpx, which lives in the
# site-packages of a Python build, not in this one.  Without it they fail with
# ModuleNotFoundError -- an environment gap, not a physics failure.
for _d in "$REPO"/build_py_4d "$REPO"/build_py_opmd "$REPO"/build_py; do
    if [ -d "$_d/lib/site-packages/pywarpx" ]; then
        export PYTHONPATH="$_d/lib/site-packages${PYTHONPATH:+:$PYTHONPATH}"
        break
    fi
done

cmake -S "$REPO" -B "$BUILD" \
    -DCMAKE_PREFIX_PATH="$CONDA" \
    -DHDF5_ROOT="$HDF5_ROOT" -DHDF5_DIR="$HDF5_ROOT" \
    -DWarpX_DIMS="1;3;RZ" -DWarpX_OPENPMD=ON -DWarpX_PYTHON=OFF \
    -DWarpX_COMPUTE=OMP -DWarpX_QED=ON \
    "${@:2}"
cmake --build "$BUILD" -j "$(nproc)"

# Analyses that import pywarpx need the bindings.  ctest bakes PYTHONPATH at
# configure time from $ENV{PYTHONPATH} plus <build>/lib/site-packages, so the
# cheapest fix for a PYTHON=OFF verification build is to point the build's
# site-packages at bindings that already exist (a PYTHON=ON build installs them
# there itself).  Without this, e.g. test_3d_ionization_electron_dsmc.analysis
# dies with ModuleNotFoundError.
if [ ! -d "$BUILD/lib/site-packages/pywarpx" ]; then
    for _d in "$REPO"/build_py_4d "$REPO"/build_py_opmd "$REPO"/build_py; do
        if [ -d "$_d/lib/site-packages/pywarpx" ]; then
            mkdir -p "$BUILD/lib/site-packages"
            ln -sfn "$_d/lib/site-packages/pywarpx" "$BUILD/lib/site-packages/pywarpx"
            ln -sfn "$_d/lib/site-packages/amrex"  "$BUILD/lib/site-packages/amrex"
            echo "pywarpx: linked $_d/lib/site-packages into $BUILD/lib/site-packages"
            break
        fi
    done
fi

ctest --test-dir "$BUILD" -j 8 --output-on-failure
