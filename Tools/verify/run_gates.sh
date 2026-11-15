#!/usr/bin/env bash
#
# Run exactly what the route-gates CI job runs, before pushing.
#
# .github/workflows/route-gates.yml has three steps; all of them are fast and local, so
# there is no reason to discover a failure in the CI notification instead of here:
#
#   Tools/verify/run_gates.sh
#
# Exit code is non-zero as soon as one gate fails.
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT" || exit 1

status=0
run_gate() {
    local name=$1; shift
    printf '== %s\n' "$name"
    if "$@"; then
        printf '   OK\n'
    else
        printf '   FAILED\n'
        status=1
    fi
}

run_gate "public-release compliance" python3 Tools/automation/gates/compliance.py
run_gate "A0 audit consistency"      python3 Tools/automation/gates/audit_check.py
run_gate "route task graph"          python3 Tools/automation/route_tasks.py next

# Modularization invariants.  These existed but were not wired into any automatic gate (CI,
# pre-commit or this script), so a regression in the plugin boundaries could not fail anything;
# and some of them silently skipped missing directories (see Docs/audit_2026_11).  They are fast
# (well under a second each) and now actually fail when the invariant breaks.
run_gate "plugin boundaries"         python3 Tools/Modularization/check_plugin_boundaries.py
run_gate "core purity"               python3 Tools/Modularization/check_core_purity.py
run_gate "kernel coupling"           python3 Tools/Modularization/check_kernel_coupling.py
run_gate "build boundaries"          python3 Tools/Modularization/check_build_boundaries.py
run_gate "legacy coverage"           python3 Tools/Modularization/check_legacy_coverage.py
run_gate "field solver extraction"   python3 Tools/Modularization/check_field_solver_extraction.py

exit $status
