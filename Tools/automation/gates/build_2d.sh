#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Build the 2D EB/MPI/OMP baseline used by the route gates.
set -euo pipefail
cd "$(dirname "$0")/../../.."

BUILD_DIR="${WARPX_BUILD_DIR:-build}"
JOBS="${WARPX_JOBS:-$(nproc)}"

if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    if [[ -z "${WARPX_AMREX_SRC:-}" ]]; then
        echo "WARPX_AMREX_SRC is not set and ${BUILD_DIR}/CMakeCache.txt is missing." >&2
        echo "Set WARPX_AMREX_SRC=/path/to/amrex-src (or pre-configure ${BUILD_DIR})." >&2
        exit 2
    fi
    cmake -S . -B "${BUILD_DIR}" \
        -DWarpX_DIMS=2 -DWarpX_EB=ON -DWarpX_MPI=ON -DWarpX_COMPUTE=OMP \
        -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=OFF -DWarpX_FFT=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX="${WARPX_AMREX_SRC}"
fi

cmake --build "${BUILD_DIR}" -j "${JOBS}"
