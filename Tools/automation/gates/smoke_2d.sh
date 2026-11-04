#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Fast physics smoke test: 2D Langmuir run + analysis.
set -euo pipefail
cd "$(dirname "$0")/../../.."
BUILD_DIR="${WARPX_BUILD_DIR:-build}"
ctest --test-dir "${BUILD_DIR}" -R '^test_2d_langmuir_multi\.(run|analysis)$' --output-on-failure
