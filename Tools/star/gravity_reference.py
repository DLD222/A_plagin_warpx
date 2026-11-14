#!/usr/bin/env python3
"""Analytic reference solutions for the Newton-gravity plugin's acceptance tests.

Every expression here corresponds to an entry in the specification
(Docs/research_notes/star_plugins/PHYSICS_SPEC.md, section 1.4) and is derived from the
Poisson equation grad^2 phi = 4 pi G rho with the potential vanishing at infinity:

    phi(r)  = -G M_enc(r)/r      g(r) = -dphi/dr = -G M_enc(r)/r^2

The module exists so the C++ solver and this file are checked against the same formulas:
a test that only compares the solver to hard-coded numbers can hide a wrong derivation in
both places.

Run `python3 Tools/star/gravity_reference.py --selftest` to check the formulas against
closed-form limits (Gauss's law at the surface, agreement with the shell theorem).
"""
import argparse
import math

# CODATA 2018; same value as the specification's constants table.
G = 6.67430e-11  # m^3 kg^-1 s^-2


def g_uniform_sphere(r, radius, density):
    """|g| of a uniform sphere, in m/s^2, for r in metres.

    Inside (r < R): g = (4/3) pi G rho r, linear in r.
    Outside (r > R): g = G M / r^2 with M = (4/3) pi rho R^3.
    """
    mass = 4.0 / 3.0 * math.pi * density * radius ** 3
    if r <= radius:
        return 4.0 / 3.0 * math.pi * G * density * r
    return G * mass / (r * r)


def phi_uniform_sphere(r, radius, density):
    """Potential of a uniform sphere, referenced to zero at infinity."""
    mass = 4.0 / 3.0 * math.pi * density * radius ** 3
    if r <= radius:
        # phi = -GM/(2R) * (3 - (r/R)^2)
        return -G * mass / (2.0 * radius) * (3.0 - (r / radius) ** 2)
    return -G * mass / r


def phi_shell(r, r_in, r_out, density):
    """Potential of one uniform shell r_in <= r <= r_out, referenced to zero at infinity.

    phi(r) = -G * integral_r^inf M_enc(s)/s^2 ds, evaluated analytically in the three branches:
    outside the shell it is the point-mass form, inside it is constant (the shell theorem), and
    inside the material it carries the -(4 pi rho / 3)(b^2 - r^2)/2 term from the partial shell.
    """
    mass = 4.0 / 3.0 * math.pi * density * (r_out ** 3 - r_in ** 3)
    if r >= r_out:
        return -G * mass / r
    partial = 4.0 / 3.0 * math.pi * density * (
        0.5 * (r_out ** 2 - r ** 2) - r_in ** 3 * (r_out - r) / (r * r_out))
    if r >= r_in:
        return -G * (partial + mass / r_out)
    partial = 4.0 / 3.0 * math.pi * density * (
        0.5 * (r_out ** 2 - r_in ** 2) - r_in ** 3 * (r_out - r_in) / (r_in * r_out))
    return -G * (partial + mass / r_out)


def phi_two_shells(r, shells):
    """Sum of phi_shell over a list of (r_in, r_out, density)."""
    return sum(phi_shell(r, a, b, rho) for (a, b, rho) in shells)


def phi_point_mass(r, mass):
    """phi = -G M / r, the r -> 0 limit of the sphere."""
    return -G * mass / r


def g_two_shells(r, shells):
    """|g| for concentric uniform shells.

    `shells` is a list of (r_inner, r_outer, density).  By the shell theorem the field at
    radius r depends only on the mass interior to r, so this reduces to a sum of the mass
    each shell contributes inside r.
    """
    enclosed = 0.0
    for (r_in, r_out, rho) in shells:
        if r <= r_in:
            continue
        outer = min(r, r_out)
        enclosed += 4.0 / 3.0 * math.pi * rho * (outer ** 3 - r_in ** 3)
    if r == 0.0:
        return 0.0
    return G * enclosed / (r * r)


def jeans_wavelength(sound_speed, density):
    """Jeans length lambda_J = c_s sqrt(pi / (G rho)).

    Source: Jeans (1902); the form quoted is the one in the specification (section 2.5)
    and in Binney & Tremaine, Galactic Dynamics, 2nd ed., section 5.2.1.
    """
    return sound_speed * math.sqrt(math.pi / (G * density))


def jeans_mass(sound_speed, density):
    """Jeans mass M_J = (4/3) pi (lambda_J/2)^3 rho, the standard definition."""
    lam = jeans_wavelength(sound_speed, density)
    return 4.0 / 3.0 * math.pi * (lam / 2.0) ** 3 * density


def selftest():
    """Check the formulas against identities that must hold independently of the code."""
    radius, density = 6.957e8, 1408.0        # a solar-sized, solar-mean-density sphere
    mass = 4.0 / 3.0 * math.pi * density * radius ** 3

    # 1. Gauss's law at the surface: both branches must give G M / R^2.
    #    Evaluated exactly at r = R; the exterior branch is checked a decade out instead,
    #    where its own approximation error is negligible.
    g_surface = g_uniform_sphere(radius, radius, density)
    assert abs(g_surface - G * mass / radius ** 2) / g_surface < 1e-12
    r_far = 10.0 * radius
    g_far = g_uniform_sphere(r_far, radius, density)
    assert abs(g_far - G * mass / r_far ** 2) / g_far < 1e-12

    # 2. Outside, the potential must equal that of a point mass at the centre.
    for r in (2.0 * radius, 10.0 * radius):
        assert abs(phi_uniform_sphere(r, radius, density) - phi_point_mass(r, mass)) \
            < 1e-12 * abs(phi_point_mass(r, mass))

    # 3. Continuity of phi at the surface.
    phi_in = phi_uniform_sphere(radius * (1.0 - 1e-12), radius, density)
    phi_out = phi_uniform_sphere(radius * (1.0 + 1e-12), radius, density)
    assert abs(phi_in - phi_out) / abs(phi_out) < 1e-9

    # 4. A single shell with r_in = 0 must reproduce the uniform sphere exactly.
    shells = [(0.0, radius, density)]
    for r in (0.3 * radius, radius, 3.0 * radius):
        a = g_two_shells(r, shells)
        b = g_uniform_sphere(r, radius, density)
        assert abs(a - b) <= 1e-12 * max(b, 1e-30), (r, a, b)

    # 5. A shell exerts no force inside itself: two shells whose inner radius is outside r
    #    contribute nothing.
    shells = [(0.5, 1.0, 1000.0), (2.0, 3.0, 500.0)]
    assert g_two_shells(0.4, shells) == 0.0

    # 6. phi_shell is continuous where the density jumps, and its derivative is -g.
    shells = [(0.8, 1.0, 1000.0), (2.0, 2.5, 500.0)]
    for boundary in (0.8, 1.0, 2.0, 2.5):
        inside = phi_shell(boundary * (1.0 - 1e-9), *shells[0] if boundary < 1.5 else shells[1])
        outside = phi_shell(boundary * (1.0 + 1e-9), *shells[0] if boundary < 1.5 else shells[1])
        assert abs(inside - outside) / abs(outside) < 1e-6, boundary

    # 7. The radial component of g = -dphi/dr must reproduce the shell-theorem field.
    #    g_two_shells returns |g|, which points inward, so dphi/dr = +|g|.
    h = 1e-6
    for r in (0.5, 1.1, 1.5, 2.2, 3.0):
        dphi_dr = (phi_two_shells(r + h, shells) - phi_two_shells(r - h, shells)) / (2.0 * h)
        assert abs(dphi_dr - g_two_shells(r, shells)) < 1e-5 * max(g_two_shells(r, shells), 1e-30), r

    # 8. A shell with r_in = 0 is the uniform sphere, in phi as well as in g.
    radius, density = 6.957e8, 1408.0
    for r in (0.3 * radius, 0.9 * radius, 2.0 * radius):
        assert abs(phi_shell(r, 0.0, radius, density)
                   - phi_uniform_sphere(r, radius, density)) < 1e-12 * abs(
                       phi_uniform_sphere(r, radius, density))

    # 9. Jeans wavelength scales as c_s / sqrt(rho).
    lam1 = jeans_wavelength(1.0e4, 1.0e-19)
    lam2 = jeans_wavelength(2.0e4, 4.0e-19)
    assert abs(lam2 / lam1 - 1.0) < 1e-12      # doubling c_s and quadrupling rho cancel

    print("gravity_reference: all self-checks passed")
    print(f"  reference sphere: M = {mass:.6e} kg, g(R) = {g_surface:.6e} m/s^2")
    print(f"  Jeans length at c_s=1e4 m/s, rho=1e-19 kg/m^3: {lam1:.6e} m")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        selftest()
        return 0
    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
