#!/usr/bin/env python3
"""Run every unit test in its own process.

A WarpX simulation can be initialized only once per process: a second one either
aborts with "ParmParse::Initialize(): already initialized" or, after a teardown,
with "MPI_Comm_dup() was called after MPI_FINALIZE".  The unit tests each build a
simulation, so they cannot share an interpreter.

The node ids are collected here, at *test* time, rather than by CMake at configure
time: on a clean build pywarpx does not exist yet while CMake is configuring, and
collecting then silently registered no tests at all.

Usage: run_tests.py <test-dir> [extra pytest args...]
Exit code is 0 only when every test passed.
"""
import os
import subprocess
import sys


def project_root(directory):
    """The directory whose relative paths the node ids are expressed against."""
    current = os.path.abspath(directory)
    while True:
        if os.path.exists(os.path.join(current, "pyproject.toml")):
            return current
        parent = os.path.dirname(current)
        if parent == current:
            return os.path.abspath(directory)
        current = parent


def collect(directory):
    result = subprocess.run(
        [sys.executable, "-m", "pytest", "--collect-only", "-q", directory],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        raise SystemExit(
            "could not collect tests - is pywarpx built and importable? "
            "(cmake --build <build> --target pip_install)"
        )
    nodes = []
    for line in result.stdout.splitlines():
        line = line.strip()
        if "::" in line and not line.startswith("<"):
            nodes.append(line.split(" - ")[0])
    return nodes


def main(argv):
    if len(argv) < 2:
        raise SystemExit(__doc__)
    directory, extra = argv[1], argv[2:]
    root = project_root(directory)
    nodes = collect(directory)
    if not nodes:
        raise SystemExit(f"no tests collected from {directory}")

    failed = []
    for node in nodes:
        result = subprocess.run(
            [sys.executable, "-m", "pytest", "-q", "-p", "no:cacheprovider", *extra, node],
            cwd=root,
        )
        if result.returncode != 0:
            failed.append(node)
        sys.stdout.flush()

    print(f"\n{len(nodes) - len(failed)}/{len(nodes)} unit tests passed")
    for node in failed:
        print(f"  FAILED {node}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
