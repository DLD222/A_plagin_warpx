#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Full local gate: compliance + audit + build + smoke.
set -euo pipefail
cd "$(dirname "$0")/../../.."
python3 Tools/automation/gates/compliance.py
python3 Tools/automation/gates/audit_check.py
bash Tools/automation/gates/build_2d.sh
bash Tools/automation/gates/smoke_2d.sh
echo "NOTE: A/B regression gate is added together with the first plugin (A5)."
