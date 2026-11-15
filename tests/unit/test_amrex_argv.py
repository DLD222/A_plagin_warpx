# Copyright 2026 The WarpX Community
#
# This file is part of WarpX.
#
# License: BSD-3-Clause-LBNL

"""`WarpX._amrex_argv` builds the AMReX command line from the script's argv."""

import pytest

from pywarpx.WarpX import WarpX


@pytest.mark.parametrize(
    "argv, expected_tail",
    [
        (["run.py", "my.in"], []),
        (["run.py", "-f", "kernel.json"], []),
        (["run.py", "max_step=10"], ["max_step=10"]),
        (["run.py", "max_step", "= 10"], ["max_step", "= 10"]),
        (["run.py", "--", "my.in", "max_step=5"], ["--", "my.in", "max_step=5"]),
        # a value that spans arguments can be a negative number, which must not be read as an
        # option of the script (this used to raise SystemExit)
        (["run.py", "geometry.prob_lo=", "-2.e-3", "-2.e-3"],
         ["geometry.prob_lo=", "-2.e-3", "-2.e-3"]),
    ],
)
def test_argv_is_split_into_script_options_and_overrides(
    monkeypatch, argv, expected_tail
):
    monkeypatch.setattr("sys.argv", argv)
    parsed = WarpX._amrex_argv("my.in")
    assert parsed[0]
    assert parsed[1] == 'FILE = "my.in"'
    assert parsed[2:] == expected_tail


def test_script_option_after_an_override_is_rejected(monkeypatch):
    """AMReX would read "--verbose" as a parameter name and glue it to the previous value."""
    monkeypatch.setattr("sys.argv", ["run.py", "max_step=10", "--verbose"])
    with pytest.raises(SystemExit):
        WarpX._amrex_argv("my.in")
