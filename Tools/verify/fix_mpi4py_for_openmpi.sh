#!/usr/bin/env bash
#
# Rebuild mpi4py's extension modules against the system OpenMPI.
#
# Why this exists
# ---------------
# WarpX here is built against the system OpenMPI (libmpi.so.40).  The conda
# environment, however, carries an mpi4py wheel that is linked against conda's
# MPICH (libmpi.so.12) - installed as a side effect of installing conda's PETSc
# for the PETSc tests.  Launching such an mpi4py under /usr/bin/mpiexec aborts:
#
#     Fatal error in internal_Init_thread: Internal MPI error!
#     check_MPIR_CVAR_PMI_VERSION: Runtime environment uses unsupported PMI version PMIx
#
# and every PICMI test whose script does `from mpi4py import MPI` fails, while the
# PICMI tests that do not import it pass.  That is the signature of this problem.
#
# What it does
# ------------
# Builds mpi4py from source with the system mpicc, forcing the *link* step to use
# only the OpenMPI library directory - otherwise the conda prefix's own -L comes
# first and the extension ends up linked against MPICH again - and copies the
# resulting extension modules over the installed ones.  The pure-Python part of
# mpi4py is unchanged.
#
# Usage
# -----
#     Tools/verify/fix_mpi4py_for_openmpi.sh
#
# Afterwards `python3 -c "import mpi4py.MPI as m; print(m.get_vendor())"` must
# print Open MPI.  The MPICH extensions are saved next to the build as a backup.
set -euo pipefail

PYTHON=${PYTHON:-$HOME/miniconda3/bin/python3}
PIP=${PIP:-$HOME/miniconda3/bin/pip}
MPICC=${MPICC:-/usr/bin/mpicc}
OMPI_PREFIX=${OMPI_PREFIX:-/usr/lib/x86_64-linux-gnu/openmpi}
MPI4PY_VERSION=${MPI4PY_VERSION:-4.1.2}

WORK=$(mktemp -d /tmp/mpi4py-openmpi.XXXXXX)
echo "working in $WORK"

# The system tools must win over conda's, or the build links conda's MPICH.
export PATH=/usr/local/bin:/usr/bin:/bin
export CC="$MPICC" MPICC="$MPICC"
export CFLAGS="-I$OMPI_PREFIX/include"
export LDSHARED="$MPICC -shared -L$OMPI_PREFIX/lib -Wl,-rpath,$OMPI_PREFIX/lib"

"$PIP" download --no-binary mpi4py --no-deps --no-build-isolation \
    "mpi4py==$MPI4PY_VERSION" -d "$WORK" >/dev/null
tar -xf "$WORK"/mpi4py-*.tar.gz -C "$WORK"
cd "$WORK"/mpi4py-"$MPI4PY_VERSION"
"$PYTHON" setup.py build --mpicc="$MPICC" >"$WORK/build.log" 2>&1 || {
    echo "build failed, see $WORK/build.log" >&2
    exit 1
}

BUILT=$(ls build/lib.*/mpi4py/*.so)
SITE=$("$PYTHON" -c "import mpi4py, os; print(os.path.dirname(mpi4py.__file__))")
BACKUP="$SITE/.mpich-backup"
mkdir -p "$BACKUP"
cp "$SITE"/*.so "$BACKUP"/ 2>/dev/null || true
cp $BUILT "$SITE"/

echo "extension modules replaced; MPICH copies saved in $BACKUP"
"$PYTHON" -c "import mpi4py.MPI as m; v = m.get_vendor(); print('mpi4py now uses:', v[0], v[1]); \
               raise SystemExit(0 if 'Open MPI' in v[0] else 1)"
