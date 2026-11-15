#!/usr/bin/env sh
# M1 unit tests: compile against AMReX headers and run.
#
# The MomentsSolver sources moved to Source/Plugins/PicEm/Modules/MomentsSolver when the
# project was split into Core and Plugins (commit ff2e9dc); this script still pointed at the old
# Source/MomentsSolver, so it failed on the first compile.  Paths are now derived from the
# repository root, and the AMReX include directory can be overridden with AMREX_INC (it is
# normally .../audit_build/fork/_deps/localamrex-src or a system AMReX install).
set -e
TESTS_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$TESTS_DIR/../../../../../.." && pwd)   # -> repository root (6 levels up)
cd "$ROOT"

if [ -z "${AMREX_INC:-}" ]; then
    for cand in "$ROOT"/audit_build/*/_deps/localamrex-src "$ROOT"/audit_build/deps/amrex-*/; do
        if [ -d "$cand/Src/Core" ] || [ -d "$cand/Src/Base" ]; then AMREX_INC="$cand"; break; fi
    done
fi
if [ -z "${AMREX_INC:-}" ]; then
    echo "run_tests.sh: set AMREX_INC to an AMReX source tree (it needs Src/Base)" >&2
    exit 2
fi

MS="Source/Plugins/PicEm/Modules/MomentsSolver"
INC="-I$MS -ISource/Plugins/PicEm/Modules -I$AMREX_INC/Src/Base -I$AMREX_INC/Src/Base/Parser -I$AMREX_INC/Src/Base/AMReX_Config"
CXX="${CXX:-g++} -std=c++17 -O2"

fails=0
for t in euler_sod ten_moment maxwell langmuir em_dispersion ten_moment_k0 ten_moment_wave friction eos fv_nd coupling_nd yee_deposition gauss_conservation; do
  echo "=== $t ==="
  if $CXX $INC "$MS/Tests/test_${t}.cpp" -o "/tmp/test_${t}"; then
      if "/tmp/test_${t}"; then :; else fails=$((fails+1)); fi
  else
      fails=$((fails+1))
  fi
done
if [ "$fails" -ne 0 ]; then
  echo "run_tests.sh: $fails test(s) failed" >&2
  exit 1
fi
