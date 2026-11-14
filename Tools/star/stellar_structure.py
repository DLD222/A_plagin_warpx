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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        selftest()
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
