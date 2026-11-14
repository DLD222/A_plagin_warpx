#!/bin/bash
# Run every registered *.run test of one build tree, in parallel, and record
# a JUnit XML + full log.  Usage: run_suite.sh BUILD_DIR OUT_DIR [JOBS]
set -u
BUILD=${1:?build dir}
OUT=${2:?out dir}
J=${3:-8}
mkdir -p "$OUT"
ctest --test-dir "$BUILD" -R '\.run$' -j "$J" --timeout 3600 \
      --output-on-failure --output-junit "$OUT/junit.xml" -O "$OUT/ctest.log"
