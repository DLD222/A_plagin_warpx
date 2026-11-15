#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Full local gate: compliance + audit + build + smoke.
set -euo pipefail
cd "$(dirname "$0")/../../.."
python3 Tools/automation/gates/compliance.py
python3 Tools/automation/gates/audit_check.py
bash Tools/automation/gates/build_2d.sh
bash Tools/automation/gates/smoke_2d.sh
# The A/B handler gate exists since A5 and is declared in route_tasks.json's "full" set;
# it was never wired in (Tools_Tests/F28).  build_2d.sh above produced the binary it needs.
python3 Tools/automation/gates/a5_ab.py
