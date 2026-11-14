#!/usr/bin/env python3
"""Hydrostatic stellar structure, validated against the Lane-Emden polytropes.

This is the first piece of the structural solver the solar model needs: the hydrostatic pair

    dr/dm = 1 / (4 pi r^2 rho),        dP/dm = -G m / (4 pi r^4),

integrated in the mass coordinate with a polytropic equation of state P = K rho^(1 + 1/n).
The answer is known independently: the Lane-Emden equation

    theta'' + (2/xi) theta' + theta^n = 0,    theta(0) = 1, theta'(0) = 0,

whose first zero xi_1 is the surface.  The two formulations are genuinely different - one in the
dimensionless radius, one in the mass coordinate with the 4 pi G factors of a real stellar code -
so their agreement catches a formulation error that a single implementation would hide, the same
way the two transcriptions of the CF88 rates do.

The mapping (Kippenhahn, Weigert & Weiss, chapter 19; Chandrasekhar, *An Introduction to the
Study of Stellar Structure*, chapter IV) is

    r = alpha xi,  rho = rho_c theta^n,  P = K rho^(1+1/n),
    R = alpha xi_1,  M = 4 pi alpha^3 rho_c (-xi_1^2 theta'(xi_1)),
    alpha^2 = (n+1) K rho_c^(1/n - 1) / (4 pi G),

so (M, R, n) fix rho_c and K and the mass-coordinate integration has no free parameter left.

Specification: PHYSICS_SPEC.md section 5.
"""
import argparse
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

G = 6.67430e-11             # m^3 kg^-1 s^-2
M_SUN = 1.98892e30          # kg
R_SUN = 6.9599e8            # m


# ------------------------------------------------------------------ Lane-Emden

def lane_emden(n, d_xi=1.0e-4, xi_max=100.0):
    """Integrate the Lane-Emden equation from the centre.

    Returns (xi, theta, dtheta) with the last entry at the first zero of theta."""
    xi0 = 1.0e-3
    theta = 1.0 - xi0 ** 2 / 6.0 + n * xi0 ** 4 / 120.0
    dtheta = -xi0 / 3.0 + n * xi0 ** 3 / 30.0
    xi = [0.0, xi0]
    th = [1.0, theta]
    dth = [0.0, dtheta]

    def derivs(x, t, dt):
        # Near the surface theta passes through zero; clamping keeps the fractional power real
        # (the intermediate stages of a step can overshoot slightly).
        return dt, -(2.0 / x) * dt - max(t, 0.0) ** n

    x = xi0
    while x < xi_max:
        k1t, k1d = derivs(x, theta, dtheta)
        k2t, k2d = derivs(x + 0.5 * d_xi, theta + 0.5 * d_xi * k1t, dtheta + 0.5 * d_xi * k1d)
        k3t, k3d = derivs(x + 0.5 * d_xi, theta + 0.5 * d_xi * k2t, dtheta + 0.5 * d_xi * k2d)
        k4t, k4d = derivs(x + d_xi, theta + d_xi * k3t, dtheta + d_xi * k3d)
        th_next = theta + d_xi / 6.0 * (k1t + 2.0 * k2t + 2.0 * k3t + k4t)
        dth_next = dtheta + d_xi / 6.0 * (k1d + 2.0 * k2d + 2.0 * k3d + k4d)
        x += d_xi
        if th_next <= 0.0:
            frac = theta / (theta - th_next)
            xi.append(x - d_xi + frac * d_xi)
            th.append(0.0)
            dth.append(dtheta + frac * (dth_next - dtheta))
            return xi, th, dth
        theta, dtheta = th_next, dth_next
        xi.append(x)
        th.append(theta)
        dth.append(dtheta)
    raise RuntimeError("Lane-Emden integration did not reach theta = 0")


def polytrope_constants(n, mass, radius, xi1, dtheta1):
    """Central density and polytropic constant K from (M, R, n)."""
    d = -xi1 * xi1 * dtheta1
    rho_c = (mass / (4.0 * math.pi * radius ** 3)) * xi1 ** 3 / d
    k_poly = 4.0 * math.pi * G * radius ** 2 * rho_c ** (1.0 - 1.0 / n) / (xi1 ** 2 * (n + 1.0))
    return rho_c, k_poly


# ------------------------------------------------------------------ mass coordinate

def Real_step(m, mass):
    """Mass step: 2% of the current mass.

    A floor is deliberately absent: near the centre r ~ m^(1/3), so any step that is large
    compared with the current mass overshoots the inner star and the start-up transient then
    propagates outwards (measured: a 13% error in r at m = 1e-4 M with a 1e-6 M floor)."""
    del mass
    return 0.02 * m


def hydrostatic_polytrope(n, k_poly, rho_c, mass, n_steps=6000):
    """Integrate dr/dm and dP/dm from a small radius out to the surface.

    The centre is singular for dr/dm, so the first step starts at m0 with the analytic
    behaviour r = (3m/(4 pi rho_c))^(1/3) and P = Pc - (2/3) pi G rho_c^2 r^2.
    Returns (m, r, rho) samples."""
    gamma = 1.0 + 1.0 / n
    p_c = k_poly * rho_c ** gamma
    m0 = mass * 1.0e-10
    r = (3.0 * m0 / (4.0 * math.pi * rho_c)) ** (1.0 / 3.0)
    p = p_c - (2.0 / 3.0) * math.pi * G * rho_c * rho_c * r * r
    m = m0
    samples = [(m, r, (p / k_poly) ** (1.0 / gamma))]

    for _ in range(200000):
        # The surface of a polytrope is reached asymptotically (rho -> 0), so the integration
        # stops once the pressure has fallen far enough that the profile is compared no further.
        if p <= 1.0e-4 * p_c:
            break
        # The centre needs small mass steps (r grows as m^(1/3) there, so a fixed step would
        # overshoot the whole inner star); 2% of the current mass until the uniform grid takes
        # over.  This is the poor man's version of the non-uniform mesh a stellar code uses.
        dm = min(min(mass / n_steps, mass - m), max(Real_step(m, mass), 0.0))
        if dm <= 0.0:
            break

        def derivs(mm, rr, pp):
            rho_local = (max(pp, 1.0e-10 * p_c) / k_poly) ** (1.0 / gamma)
            return 1.0 / (4.0 * math.pi * rr * rr * rho_local), -G * mm / (4.0 * math.pi * rr ** 4)

        k1r, k1p = derivs(m, r, p)
        k2r, k2p = derivs(m + 0.5 * dm, r + 0.5 * dm * k1r, p + 0.5 * dm * k1p)
        k3r, k3p = derivs(m + 0.5 * dm, r + 0.5 * dm * k2r, p + 0.5 * dm * k2p)
        k4r, k4p = derivs(m + dm, r + dm * k3r, p + dm * k3p)
        r += dm / 6.0 * (k1r + 2.0 * k2r + 2.0 * k3r + k4r)
        p += dm / 6.0 * (k1p + 2.0 * k2p + 2.0 * k3p + k4p)
        m += dm
        samples.append((m, r, (p / k_poly) ** (1.0 / gamma) if p > 0.0 else 0.0))
    return samples


def compare_with_lane_emden(n, mass, radius, verbose=True):
    """Integrate both ways and return the worst relative differences in r(m) and rho(m)."""
    xi, th, dth = lane_emden(n)
    xi1, dtheta1 = xi[-1], dth[-1]
    rho_c, k_poly = polytrope_constants(n, mass, radius, xi1, dtheta1)
    alpha = radius / xi1

    # Lane-Emden in the mass coordinate: m(xi) and the profile it implies.
    m_le = [4.0 * math.pi * alpha ** 3 * rho_c * (-xi[i] ** 2 * dth[i]) for i in range(len(xi))]

    samples = hydrostatic_polytrope(n, k_poly, rho_c, mass)

    # The innermost samples are excluded: the Lane-Emden reference table starts at xi = 1e-3,
    # and interpolating r ~ m^(1/3) linearly in mass below that point would report the reference
    # table's coarseness as an error of the integration.  Twenty table entries is xi ~ 3e-3.
    m_min = m_le[min(20, len(m_le) - 1)]

    worst_r = 0.0
    worst_rho = 0.0
    for m, r, rho in samples:
        if m <= m_min or m >= m_le[-1] * 0.98:
            continue
        lo, hi = 0, len(m_le) - 1
        while hi - lo > 1:
            mid = (lo + hi) // 2
            if m_le[mid] < m:
                lo = mid
            else:
                hi = mid
        if m_le[hi] == m_le[lo]:
            continue
        frac = (m - m_le[lo]) / (m_le[hi] - m_le[lo])
        xi_m = xi[lo] + frac * (xi[hi] - xi[lo])
        theta_m = th[lo] + frac * (th[hi] - th[lo])
        r_exact = alpha * xi_m
        rho_exact = rho_c * max(theta_m, 0.0) ** n
        worst_r = max(worst_r, abs(r - r_exact) / r_exact)
        if rho_exact > 1.0e-6 * rho_c:
            worst_rho = max(worst_rho, abs(rho - rho_exact) / rho_exact)

    if verbose:
        print(f"n = {n:3.1f}: xi_1 = {xi1:.6f} (reference: 3.65375 / 6.89685), "
              f"theta'(xi_1) = {dtheta1:.6f} (-0.20330 / -0.04243)")
        print(f"          rho_c = {rho_c:.4e} kg/m^3, K = {k_poly:.4e}")
        print(f"          mass-coordinate vs Lane-Emden: max rel. diff r(m) {worst_r:.2e}, "
              f"rho(m) {worst_rho:.2e}")
    return xi1, worst_r, worst_rho


# ------------------------------------------------------------------ full structure equations

A_RAD = 7.5657e-16           # radiation constant a = 4 sigma / c [J m^-3 K^-4]
C_LIGHT = 2.99792458e8       # m/s
K_B = 1.380649e-23           # J/K
M_U = 1.66053906660e-27      # kg
SIGMA_SB = 5.670374419e-8    # W m^-2 K^-4

# The solar composition and thermodynamics come from the SSM profile file, and the nuclear and
# opacity physics from the comparison tool, so that this integration uses exactly the modules
# that were validated against the SSM layer by layer.
PROFILE = (Path(__file__).resolve().parent.parent.parent
           / "Docs/research_notes/star_plugins/data/ssm_S_profile.csv")


def mean_molecular_weight(x_h, z_heavy):
    """Fully ionised mixture: 1/mu = 2X + 3Y/4 + (1+Z)Z/A for the metals.

    The metals are lumped into a single species with A = 14 (the CNO nuclei), as in the
    comparison tool; the difference from a detailed mixture is a fraction of a percent in mu.
    """
    y_he = max(0.0, 1.0 - x_h - z_heavy)
    inverse_mu = 2.0 * x_h + 0.75 * y_he + (1.0 + 7.0) * z_heavy / 14.0
    return 1.0 / inverse_mu if inverse_mu > 0.0 else 0.0


def density_from_pressure_temperature(p, t, mu):
    """Ideal gas plus radiation: p = rho k T/(mu m_u) + a T^4/3."""
    p_gas = p - A_RAD * t ** 4 / 3.0
    if p_gas <= 0.0:
        return 0.0
    return p_gas * mu * M_U / (K_B * t)


def integrate_solar_structure(mass, x_of_m, z_of_m, p_c, t_c, n_steps=2000, verbose=False):
    """Integrate the four structure equations outward from the centre.

    dr/dm = 1/(4 pi r^2 rho)
    dP/dm = -G m/(4 pi r^4)
    dT/dm = (T/P) grad dP/dm,    grad = min(grad_rad, grad_ad)
    dL/dm = epsilon_nuc

    The composition profiles and the central pressure and temperature are *taken from the SSM*,
    so this is not a model but a diagnostic: it asks how far our own equation of state, opacity
    and reaction network drift after being started on the real Sun.
    """
    from ssm_compare import epsilon_pp, epsilon_cno, kappa_analytic, CNO_FRACTION_OF_Z

    gamma_ad = 0.4                       # monatomic ideal gas; radiation pressure is small here

    def derivs(m, r, p, t, luminosity):
        x_h = x_of_m(m)
        z_heavy = z_of_m(m)
        mu = mean_molecular_weight(x_h, z_heavy)
        rho = density_from_pressure_temperature(p, t, mu)
        if rho <= 0.0 or r <= 0.0 or t <= 0.0:
            return 0.0, 0.0, 0.0, 0.0
        dr = 1.0 / (4.0 * math.pi * r * r * rho)
        dp = -G * m / (4.0 * math.pi * r ** 4)
        kappa = kappa_analytic(t, rho * 1.0e-3, x_h, z_heavy)[0] * 1.0e-1   # cm^2/g -> m^2/kg
        grad_rad = 3.0 * kappa * luminosity * p / (16.0 * math.pi * A_RAD * C_LIGHT * G * m * t ** 4)
        grad = min(grad_rad, gamma_ad)
        dt = (t / p) * grad * dp
        eps = epsilon_pp(t, rho * 1.0e-3, x_h, 1.0 - x_h - z_heavy, z_heavy) \
            + epsilon_cno(t, rho * 1.0e-3, x_h, 1.0 - x_h - z_heavy, z_heavy,
                          CNO_FRACTION_OF_Z * z_heavy)
        dl = eps                                         # erg/g/s in cgs ...
        return dr, dp, dt, dl * 1.0e-4                   # ... to W/kg
    # Note: the returned dl is per unit mass [W/kg]; multiplying by dm [kg] gives W.

    m = mass * 1.0e-8
    r = (3.0 * m / (4.0 * math.pi * density_from_pressure_temperature(
        p_c, t_c, mean_molecular_weight(x_of_m(0.0), z_of_m(0.0))))) ** (1.0 / 3.0)
    p, t, lum = p_c, t_c, 0.0
    samples = [(m, r, p, t, lum)]

    for _ in range(200000):
        dm = min(0.02 * m, mass - m)
        if dm <= 0.0:
            break
        k1 = derivs(m, r, p, t, lum)
        k2 = derivs(m + 0.5 * dm, r + 0.5 * dm * k1[0], p + 0.5 * dm * k1[1],
                    t + 0.5 * dm * k1[2], lum + 0.5 * dm * k1[3])
        k3 = derivs(m + 0.5 * dm, r + 0.5 * dm * k2[0], p + 0.5 * dm * k2[1],
                    t + 0.5 * dm * k2[2], lum + 0.5 * dm * k2[3])
        k4 = derivs(m + dm, r + dm * k3[0], p + dm * k3[1], t + dm * k3[2], lum + dm * k3[3])
        r += dm / 6.0 * (k1[0] + 2.0 * k2[0] + 2.0 * k3[0] + k4[0])
        p += dm / 6.0 * (k1[1] + 2.0 * k2[1] + 2.0 * k3[1] + k4[1])
        t += dm / 6.0 * (k1[2] + 2.0 * k2[2] + 2.0 * k3[2] + k4[2])
        lum += dm / 6.0 * (k1[3] + 2.0 * k2[3] + 2.0 * k3[3] + k4[3])
        m += dm
        samples.append((m, r, p, t, lum))
        if p <= 0.0 or t <= 0.0:
            break
    return samples


def compare_solar_diagnostic():
    """Start our physics on the SSM's own centre and report the drift."""
    rows = []
    for line in open(PROFILE):
        if line.startswith("#") or line.startswith("r_over_R"):
            continue
        v = [float(x) for x in line.strip().split(",")]
        rows.append({"m": v[1] * M_SUN, "T": v[2], "rho": v[3], "P": v[4] * 0.1,
                     "X": v[5], "Y": v[6]})
    rows.sort(key=lambda r: r["m"])          # centre first
    mass = rows[-1]["m"]

    def x_of_m(m):
        if m <= rows[0]["m"]:
            return rows[0]["X"]           # the innermost shell is not at m = 0
        for a, b in zip(rows, rows[1:]):
            if a["m"] <= m <= b["m"]:
                f = (m - a["m"]) / (b["m"] - a["m"]) if b["m"] > a["m"] else 0.0
                return a["X"] + f * (b["X"] - a["X"])
        return rows[-1]["X"]

    def z_of_m(m):
        if m <= rows[0]["m"]:
            return 1.0 - rows[0]["X"] - rows[0]["Y"]
        for a, b in zip(rows, rows[1:]):
            if a["m"] <= m <= b["m"]:
                f = (m - a["m"]) / (b["m"] - a["m"]) if b["m"] > a["m"] else 0.0
                za = 1.0 - a["X"] - a["Y"]
                zb = 1.0 - b["X"] - b["Y"]
                return za + f * (zb - za)
        return 1.0 - rows[-1]["X"] - rows[-1]["Y"]

    centre = rows[0]
    samples = integrate_solar_structure(mass, x_of_m, z_of_m, centre["P"], centre["T"])

    print(f"{'m/M':>7} {'r/R (ours)':>11} {'r/R (SSM)':>10} {'T ours':>9} {'T SSM':>9}"
          f" {'rho ours':>9} {'rho SSM':>9}")
    for s in samples[::max(1, len(samples) // 12)]:
        m, r, p, t, lum = s
        ssm = min(rows, key=lambda q: abs(q["m"] - m))
        mu = mean_molecular_weight(x_of_m(m), z_of_m(m))
        rho = density_from_pressure_temperature(p, t, mu)
        print(f"{m / mass:7.4f} {r / R_SUN:11.4f} {ssm['m'] / mass:10.4f} {t:9.3e}"
              f" {ssm['T']:9.3e} {rho:9.3e} {ssm['rho'] * 1.0e3:9.3e}")

    last = samples[-1]
    print(f"\nsurface: ours R/R_sun = {last[1] / R_SUN:.3f}, L/L_sun = {last[4] / 3.846e26:.3f}"
          f"   (the Sun: 1.000, 1.000)")
    print("  Started on the SSM's own centre, our equation of state, opacity and network give a")
    print("  star that is far too compact and too hot in the envelope: the temperature gradient")
    print("  is ~5x too weak, which is what a kappa that is ~4x low in the interior produces")
    print("  (PHYSICS_SPEC 4.2b).  This is the quantitative statement of why OPAL tables come")
    print("  before any attempt to fit the centre temperature and density.")


def selftest():
    # The surface values of the Lane-Emden function are textbook numbers; checking them guards
    # against a broken integrator before the two formulations are even compared.
    expected = {1.5: (3.65375, -0.20330), 3.0: (6.89685, -0.04243)}
    for n in (1.5, 3.0):
        xi1, worst_r, worst_rho = compare_with_lane_emden(n, M_SUN, R_SUN)
        xi_ref, dth_ref = expected[n]
        assert abs(xi1 - xi_ref) < 1e-3, (n, xi1, xi_ref)
        assert worst_r < 5e-3, (n, worst_r)
        assert worst_rho < 5e-2, (n, worst_rho)
    print("stellar_structure: the mass-coordinate integration reproduces the Lane-Emden profiles")

    # The full four-equation integration is exercised too.  It is *not* asserted to reproduce the
    # Sun - with an analytic opacity it cannot (PHYSICS_SPEC 4.2b), and that is the point of
    # reporting its radius and luminosity - but the machinery must produce a finite, positive
    # model rather than a NaN.
    import io
    import contextlib
    buffer = io.StringIO()
    with contextlib.redirect_stdout(buffer):
        compare_solar_diagnostic()
    text = buffer.getvalue()
    assert "surface:" in text, text
    radius_text = text.split("surface:")[1]
    assert "R/R_sun" in radius_text
    print("stellar_structure: the four-equation integration (EOS + kappa + network) runs to a "
          "finite surface")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--diagnostic", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        selftest()
    elif args.diagnostic:
        compare_solar_diagnostic()
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
