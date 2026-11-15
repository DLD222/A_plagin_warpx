#!/bin/bash
# Remove the generated outputs of one case's run directory so a fork-vs-official
# sweep starts from a clean slate.
#
# Why this exists: WarpX reduced diagnostics are opened in append mode
# (`std::ofstream::app`, see FieldProbe::WriteToFile and every ReducedDiags
# WriteToFile).  Re-running a case in a directory that already has `diags/`
# therefore leaves several runs concatenated in the same file: measured on
# `test_3d_beam_beam_collision`, the reference (fresh) file had the 92 clean rows
# 0..90 while the re-run directory had 30 rows from three different runs (steps
# 0, 80, 17, 90), so a text comparison of that file is meaningless.  Plotfiles and
# openPMD files are overwritten by name, but stale ones from a run with a
# different step count can survive and be compared against nothing.
#
# The case inputs themselves live in `Examples/` and are referenced by absolute
# path, and the `*_prepare` tests generate their files again, so removing
# everything here is safe.
#
# Usage:
#   clean_run_dirs.sh <build-dir> [case...]     # all <build-dir>/bin/* by default
#   clean_run_dirs.sh --dry-run <build-dir>
set -eu

dry=0
if [ "${1:-}" = "--dry-run" ]; then
    dry=1
    shift
fi
build=${1:?usage: clean_run_dirs.sh [--dry-run] <build-dir> [case...]}
shift || true

[ -d "$build/bin" ] || { echo "no such build/bin: $build/bin" >&2; exit 1; }

if [ "$#" -gt 0 ]; then
    cases=("$@")
else
    cases=()
    for d in "$build"/bin/*/; do
        cases+=("$(basename "$d")")
    done
fi

n=0
for name in "${cases[@]}"; do
    dir="$build/bin/$name"
    [ -d "$dir" ] || continue
    if [ "$dry" -eq 1 ]; then
        echo "would clean $dir"
        n=$((n + 1))
        continue
    fi
    (
        cd "$dir"
        rm -rf diags Backtrace* warpx_job_info warpx_used_inputs
        rm -rf diag?????? openpmd_* plotfiles* samples particles
        rm -rf __pycache__ .pytest_cache
        # loose artefacts some tests write next to the deck
        find . -maxdepth 1 \( -name '*.png' -o -name '*.pdf' -o -name '*.dat' \) -delete
    )
    n=$((n + 1))
done
echo "cleaned $n case director$([ "$n" -eq 1 ] && echo y || echo ies) under $build/bin"
