#!/usr/bin/env python3
"""Group ctest failures by test case and by cause.

Reads a `ctest --output-on-failure` log and answers the question the parity
report needs to answer: of the failing entries, which are environment
(no openPMD, no FFT, missing data), which are platform checksums, and which
are candidate real defects?

Each test case produces up to three ctest entries (`.run`, `.analysis`,
`.checksum`); the classifier collapses them onto the case name and looks for
the distinguishing error text in the captured output.

Usage:
    python3 Tools/verify/classify_ctest_log.py build_2d_full.log
    python3 Tools/verify/classify_ctest_log.py --failures-only log.txt
"""

import argparse
import collections
import re
import sys

OPENPMD_MARKERS = (
    "USE_OPENPMD",
    "openPMD support",
    "OpenPMD support",
    "openPMD-api",
    "Could not detect format",
)
FFT_MARKERS = (
    "built without spectral solvers",
    "need to compile with USE_FFT",
)
DATA_MARKERS = (
    "No such file",
    "datasets not found",
    "Could not detect format",
)
PYTHON_MARKERS = ("No module named 'pywarpx'",)


RESULT_RE = re.compile(
    r"^(?:\s*)(\d+)/(\d+) Test\s+#(\d+): (\S+?)\s+\.*\s*"
    r"(\*\*\*Failed|Passed|Not Run|\*\*\*Timeout|\*\*\*Exception|"
    r"Subprocess terminated)")
START_RE = re.compile(r"^\s*Start\s+\d+:\s+(\S+)\s*$")


def parse(path):
    """Return ({entry: status}, total, {entry: [output lines]}).

    ctest --output-on-failure prints a test's output *after* its result line,
    so the output of an entry is the run of lines that follows the result line
    up to the next result/start line.  Attributing output this way survives
    the interleaving that -jN produces (the Start line does not).
    """
    result, total, blocks = {}, 0, collections.defaultdict(list)
    current = None
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.rstrip("\n")
        m = RESULT_RE.match(line)
        if m:
            result[m.group(4)] = m.group(5).strip("*")
            total = max(total, int(m.group(2)))
            current = m.group(4)
            continue
        if START_RE.match(line):
            current = None
            continue
        if current is not None:
            blocks[current].append(line)
    return result, total, blocks


def classify(name, status, blocks):
    text = "\n".join(blocks.get(name, []))
    case = re.sub(r"\.(run|analysis|checksum)$", "", name)

    if "Timeout" in status:
        return case, "timeout (machine contention)"
    if "Subprocess terminated" in status:
        return case, "killed / hung (needs a manual rerun)"
    if any(m in text for m in PYTHON_MARKERS):
        return case, "environment (python bindings)"
    if any(m in text for m in FFT_MARKERS):
        return case, "environment (FFT)"
    if any(m in text for m in OPENPMD_MARKERS):
        return case, "environment (openPMD)"
    if any(m in text for m in DATA_MARKERS):
        return case, "missing data / I/O"
    if name.endswith(".checksum"):
        return case, "platform checksum"
    return case, "candidate real defect"


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log")
    ap.add_argument("--failures-only", action="store_true",
                    help="only print the failing cases")
    args = ap.parse_args()

    result, total, blocks = parse(args.log)
    if not result:
        print(f"{args.log}: no ctest result lines found", file=sys.stderr)
        return 2

    passed = sum(1 for v in result.values() if v == "Passed")
    print(f"{args.log}: {len(result)}/{total} entries executed, "
          f"{passed} passed, {len(result) - passed} failed")

    cases = collections.defaultdict(set)
    for name, status in result.items():
        if status == "Passed":
            continue
        case, why = classify(name, status, blocks)
        cases[case].add(why)

    # A case that also has a timeout / environment / data explanation does not
    # need a "real defect" guess: the explanation is already there.
    explained = {"timeout (machine contention)", "environment (openPMD)",
                 "environment (FFT)", "environment (python bindings)",
                 "missing data / I/O", "killed / hung (needs a manual rerun)"}
    for case, why in cases.items():
        if why & explained:
            why.discard("candidate real defect")

    print(f"distinct failing cases: {len(cases)}")
    for case in sorted(cases):
        print(f"  {case}: {', '.join(sorted(cases[case]))}")

    if not args.failures_only:
        print("\ncases with a candidate real defect need the serial rerun "
              "(`ctest -j1 -R <case> --timeout 2400`) before they are believed:")
        bad = [c for c, why in cases.items() if "candidate real defect" in why]
        print("  " + ("\n  ".join(bad) if bad else "(none)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
