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

exit $status
