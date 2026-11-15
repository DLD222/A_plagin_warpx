#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Fast physics smoke test: 2D Langmuir run + analysis.
set -euo pipefail
cd "$(dirname "$0")/../../.."
BUILD_DIR="${WARPX_BUILD_DIR:-build}"
# --no-tests=error: ctest exits 0 when -R matches nothing, which would make this
# gate pass after a test rename/move (see Tools_Tests/F25 in Docs/audit_2026_11).
ctest --test-dir "${BUILD_DIR}" --no-tests=error \
      -R '^test_2d_langmuir_multi\.(run|analysis)$|^runtime_(snapshot|lb)_' \
      --output-on-failure
