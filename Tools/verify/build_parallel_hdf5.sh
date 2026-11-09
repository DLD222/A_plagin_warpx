#!/usr/bin/env bash
# Build an MPI-enabled HDF5 for openPMD, using the system OpenMPI.
#
# Why this exists: the openPMD dependency of this repository needs a *parallel*
# HDF5 to enable its HDF5 backend.  The HDF5 shipped with the local conda
# environment is serial, and openPMD silently disables the backend in that case
# (its CMake warns "only serial version of HDF5"), which makes every openPMD
# test fail with "openPMD-api built without support for backend HDF5".
#
# Usage:  Tools/verify/build_parallel_hdf5.sh [install-prefix]
# Result: $PREFIX/lib/libhdf5.so and $PREFIX/include/hdf5.h
set -euo pipefail

PREFIX="${1:-/tmp/hdf5_mpi}"
WORK="${WORK:-/tmp/hdf5_build}"
VERSION="1.14.6"

mkdir -p "$WORK"
cd "$WORK"
if [ ! -d "hdf5-${VERSION}" ]; then
    curl -sL -o hdf5.tar.gz \
        "https://github.com/HDFGroup/hdf5/releases/download/hdf5_${VERSION}/hdf5-${VERSION}.tar.gz"
    tar xzf hdf5.tar.gz
fi

cd "hdf5-${VERSION}"
CC=/usr/bin/mpicc CXX=/usr/bin/mpicxx cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DHDF5_ENABLE_PARALLEL=ON \
    -DHDF5_BUILD_CPP_LIB=OFF -DHDF5_BUILD_EXAMPLES=OFF \
    -DHDF5_BUILD_TOOLS=OFF -DBUILD_TESTING=OFF \
    -DHDF5_ENABLE_Z_LIB_SUPPORT=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$(nproc)"
cmake --install build
echo "installed parallel HDF5 into $PREFIX"
