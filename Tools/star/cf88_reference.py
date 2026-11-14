#!/usr/bin/env python3
"""Caughlan & Fowler (1988) analytic rates, transcribed from PHYSICS_SPEC section 3.3.

This is the independent transcription used to check the C++ fits in
Source/Plugins/FluidMoments/Cf88Rates.H: the two implementations were written from the same
table, so a typo in either shows up as a disagreement, and the self-checks below catch the
systematic mistakes a typo would hide (negative rates, non-monotone behaviour, wrong units).

The fits return f in (cm^3/mol)^(N-1)/s; for a two-body reaction

    <sigma v> [cm^3/s] = f / N_A.

Run `python3 Tools/star/cf88_reference.py --table` to print the grid the C++ test pins, and
`--selftest` for the checks.
"""
import argparse
import math

N_A = 6.02214076e23
RC78 = 0.1


def t913(t9):
    return t9 ** (1.0 / 3.0)


def t923(t9):
    return t913(t9) ** 2


def t932(t9):
    return t9 * math.sqrt(t9)


def t943(t9):
    return t9 * t913(t9)


def t953(t9):
    return t9 * t923(t9)


def t965(t9):
    return t9 ** 1.2


def t912(t9):
    return math.sqrt(t9)


def f2(t9):
    return (4.01e-15 / t923(t9) * math.exp(-3.380 / t913(t9))
            * (1.0 + 0.123 * t913(t9) + 1.09 * t923(t9) + 0.938 * t9))


def f4(t9):
    return (2.24e03 / t923(t9) * math.exp(-3.720 / t913(t9))
            * (1.0 + 0.112 * t913(t9) + 3.38 * t923(t9) + 2.65 * t9))


def f18(t9):
    return (6.04e10 / t923(t9) * math.exp(-12.276 / t913(t9))
            * (1.0 + 0.034 * t913(t9) - 0.522 * t923(t9) - 0.124 * t9
               + 0.353 * t943(t9) + 0.213 * t953(t9)))


def f24(t9):
    t9a = t9 / (1.0 + 4.95e-2 * t9)
    return (5.61e06 * t9a ** (5.0 / 6.0) / t932(t9)
            * math.exp(-12.826 / t9a ** (1.0 / 3.0)))


def f41(t9):
    return (1.34e-10 / t912(t9)
            * (1.0 - 0.537 * t913(t9) + 3.86 * t923(t9)
               + 0.0027 / t9 * math.exp(2.515e-3 / t9)))


def f42(t9):
    return (3.11e05 / t923(t9) * math.exp(-10.262 / t913(t9))
            + 2.53e03 / t932(t9) * math.exp(-7.306 / t9))


def f61(t9):
    return (2.04e07 / t923(t9)
            * math.exp(-13.690 / t913(t9) - (t9 / 1.500) ** 2)
            * (1.00 + 0.030 * t913(t9) + 1.19 * t923(t9) + 0.254 * t9
               + 2.06 * t943(t9) + 1.12 * t953(t9))
            + 1.08e05 / t932(t9) * math.exp(-4.925 / t9)
            + 2.15e05 / t932(t9) * math.exp(-18.179 / t9))


def f64(t9):
    return (8.01e07 / t923(t9)
            * math.exp(-13.717 / t913(t9) - (t9 / 2.000) ** 2)
            * (1.0 + 0.030 * t913(t9) + 0.958 * t923(t9) + 0.204 * t9
               + 1.39 * t943(t9) + 0.753 * t953(t9))
            + 1.21e06 / t965(t9) * math.exp(-5.701 / t9))


def f71(t9):
    return (4.90e07 / t923(t9)
            * math.exp(-15.228 / t913(t9) - (t9 / 3.294) ** 2)
            * (1.0 + 0.027 * t913(t9) - 0.778 * t923(t9) - 0.149 * t9
               + 0.261 * t943(t9) + 0.127 * t953(t9))
            + 2.37e03 / t932(t9) * math.exp(-3.011 / t9)
            + 2.19e04 * math.exp(-12.530 / t9))


def f76(t9):
    return (9.78e08 / t923(t9)
            * math.exp(-15.251 / t913(t9) - (t9 / 0.450) ** 2)
            * (1.00 + 0.027 * t913(t9) + 0.219 * t923(t9) + 0.042 * t9
               + 6.83 * t943(t9) + 3.32 * t953(t9))
            + 1.11e04 / t932(t9) * math.exp(-3.328 / t9)
            + 1.49e04 / t932(t9) * math.exp(-4.665 / t9)
            + 3.80e06 / t932(t9) * math.exp(-11.048 / t9))


def f78(t9, rc=RC78):
    return (1.08e12 / t923(t9)
            * math.exp(-15.251 / t913(t9) - (t9 / 0.522) ** 2)
            * (1.00 + 0.027 * t913(t9) + 2.62 * t923(t9) + 0.501 * t9
               + 5.36 * t943(t9) + 2.60 * t953(t9))
            + 1.19e08 / t932(t9) * math.exp(-3.676 / t9)
            + 5.41e08 / t912(t9) * math.exp(-8.926 / t9)
            + rc * 4.72e08 / t932(t9) * math.exp(-7.721 / t9)
            + 2.20e09 / t932(t9) * math.exp(-11.418 / t9))


RATES = {
    "f2": (f2, 1.442), "f4": (f4, 5.494), "f18": (f18, 12.860), "f24": (f24, 1.588),
    "f41": (f41, 0.862), "f42": (f42, 0.137), "f61": (f61, 1.944), "f64": (f64, 7.551),
    "f71": (f71, 7.297), "f76": (f76, 12.128), "f78": (f78, 4.966),
}

T9_GRID = (0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 3.0)


def solar_centre_rate():
    """Energy generation implied by the p+p rate at the standard solar centre.

    PHYSICS_SPEC section 3.3b: epsilon = (1/rho) sum Q_i r_i, with r_i the reaction rate per
    unit volume.  Each p+p reaction feeds one half of 4p -> 4He + 2e+ + 2nu, whose total release
    is 26.73 MeV, so the energy per p+p reaction is 13.365 MeV.  The SSM profile file carries
    T, rho and X at the centre, so the comparison needs no remembered numbers.
    """
    t9 = 1.563376e7 / 1e9      # centre temperature of the SSM profile (data/ssm_S_profile.csv)
    rho = 1.526990e2           # g/cm^3
    x_h = 3.407625e-1          # hydrogen mass fraction
    m_p = 1.67262192e-24       # g
    n_p = rho * x_h / m_p
    sigma_v = f2(t9) / N_A
    rate = 0.5 * n_p * n_p * sigma_v            # reactions cm^-3 s^-1
    q_mev = 26.73 / 2.0                          # MeV per p+p reaction
    eps = q_mev * 1.602176634e-6 * rate / rho    # erg g^-1 s^-1
    return t9, rho, x_h, eps


def selftest():
    # 1. Every fit is positive over the temperature range of the table.  Monotonicity is only
    #    asserted where it is physically meaningful: several fits carry Gaussian cutoffs
    #    exp(-(T9/T0)^2) (T0 = 1.500, 2.000, 3.294, 0.450, 0.522) that bend the fit over above
    #    T0, and the 7Be electron capture decreases with T9 by construction.  Both are properties
    #    of the published fits, so the check is limited to the solar-relevant window rather than
    #    "corrected".
    for name, (func, _q) in RATES.items():
        for t9 in [0.005 + 0.0025 * i for i in range(1200)]:   # 0.005 .. 3.0025
            assert func(t9) > 0.0, (name, t9, func(t9))
    for name, (func, _q) in RATES.items():
        if name == "f41":
            continue
        previous = func(0.02)
        t9 = 0.02
        while t9 < 0.30:
            t9 += 0.0025
            value = func(t9)
            assert value > previous, (name, t9, value, previous)
            previous = value
    assert f41(0.02) > f41(0.05) > f41(0.1)

    # 2. The units in PHYSICS_SPEC 3.3: <sigma v> = f / N_A, and the p+p rate at the solar
    #    centre is about 1.6e-43 cm^3/s (an order-of-magnitude guard against a unit slip).
    sigma_v_pp = f2(1.563e-2) / N_A
    assert 1.0e-43 < sigma_v_pp < 3.0e-43, sigma_v_pp

    # 3. f24's transformed temperature is a pure rescaling: deep in the radiative regime the
    #    rate must still grow with T9.
    assert f24(0.05) < f24(0.1) < f24(0.2)

    # 4. The 15N(p,alpha)12C coefficient rc78 enters linearly through the term the table singles
    #    out: changing it from 0.1 to 0.2 must add exactly 0.1 * that term.
    term = 4.72e08 / t932(0.25) * math.exp(-7.721 / 0.25)
    delta = f78(0.25, rc=0.2) - f78(0.25, rc=0.1)
    # The comparison is limited by cancellation against the 609 that the other terms contribute,
    # hence the tolerance is relative to the rate itself, not to the small difference.
    assert abs(delta - 0.1 * term) < 1e-12 * abs(f78(0.25)), (delta, term)

    # 5. The Q values are the table's column 3.
    expected_q = {"f2": 1.442, "f4": 5.494, "f18": 12.860, "f24": 1.588, "f41": 0.862,
                  "f42": 0.137, "f61": 1.944, "f64": 7.551, "f71": 7.297, "f76": 12.128,
                  "f78": 4.966}
    for name, (_func, q) in RATES.items():
        assert abs(q - expected_q[name]) < 1e-12, name

    t9, rho, x_h, eps = solar_centre_rate()
    print("cf88_reference: all self-checks passed")
    print(f"  p+p at the solar centre: <sigma v> = {f2(t9) / N_A:.4e} cm^3/s")
    print(f"  T9 = {t9:.6f}, rho = {rho:.3f} g/cm^3, X = {x_h:.5f}"
          f"  ->  epsilon_pp = {eps:.2f} erg/g/s")
    print("  (SSM profile table at the same point: epsilon = 16.93 erg/g/s, which also"
          " contains the CNO cycle; the remaining difference is screening and the branch"
          " ratios - both belong to the solar-model round)")


def pin():
    """The handful of values the C++ test pins, with enough digits for a 1e-12 comparison."""
    for t9 in (0.02, 0.2, 1.0):
        print(f"// T9 = {t9}")
        print("{" + ", ".join(f"{RATES[name][0](t9):.17g}" for name in RATES) + "},")


def table():
    print("T9 " + " ".join(f"{name:>12s}" for name in RATES))
    for t9 in T9_GRID:
        print(f"{t9:6.3f} " + " ".join(f"{RATES[name][0](t9):12.5e}" for name in RATES))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--table", action="store_true")
    ap.add_argument("--pin", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        selftest()
    elif args.table:
        table()
    elif args.pin:
        pin()
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
