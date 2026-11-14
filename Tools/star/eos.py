#!/usr/bin/env python3
"""Equation of state with hydrogen and helium ionisation (Saha).

The structural model so far used an ideal gas of fully ionised material, which is right in the
core and wrong in the envelope, where hydrogen and helium recombine: that changes the mean
molecular weight, the pressure at a given temperature and - through the latent heat - the
adiabatic gradient, and it is what sets the radius of a solar model.

The ionisation fractions follow the Saha equation

    n_{i+1} n_e / n_i = 2 (2 pi m_e k T / h^2)^(3/2) (U_{i+1}/U_i) exp(-chi_i / k T),

with the partition functions taken as their ground-state values (U = 1) and the ionisation
energies from the NIST atomic spectra database:

    chi_H   = 13.5984 eV
    chi_HeI = 24.5874 eV
    chi_HeII = 54.4178 eV

The metals are assumed fully ionised and are lumped into one species with Z = 7, A = 14, matching
the rest of this project's tools.

Given (rho, T) the electron density is found by bisection on charge neutrality; the pressure is
the sum over all particles plus radiation, and the specific internal energy carries the ionisation
energy of every ion so that the adiabatic gradient can be obtained from the first law,

    du + P d(1/rho) = 0,

without needing the entropy.

Specification: PHYSICS_SPEC.md section 4.3d.
"""
import argparse
import math

K_B = 1.380649e-16           # erg/K
M_E = 9.1093837015e-28       # g
H_PLANCK = 6.62607015e-27    # erg s
M_U = 1.66053906660e-24      # g
A_RAD_CGS = 7.5657e-15       # erg cm^-3 K^-4
EV_TO_ERG = 1.602176634e-12

CHI_H = 13.5984 * EV_TO_ERG
CHI_HE1 = 24.5874 * EV_TO_ERG
CHI_HE2 = 54.4178 * EV_TO_ERG

Z_HEAVY = 7.0                # the metals, lumped
A_HEAVY = 14.0


E_ESU = 4.80320471257e-10     # esu


def _saha_constant(t_k, chi):
    """The Saha constant 2 (2 pi m_e k T/h^2)^(3/2) exp(-chi/kT) in cm^-3."""
    if chi <= 0.0:
        # The lowered ionisation potential has gone negative: the bound level has been squeezed
        # into the continuum, so this species is fully ionised.  This is the pressure-ionisation
        # limit and it is what makes the dense solar interior fully ionised (the plain Saha
        # equation, even with the Debye lowering in the exponential, still leaves it 15% neutral
        # because k T ~ 1.3 keV makes the Boltzmann factor almost insensitive to chi).
        return float("inf")
    exponent = -chi / (K_B * t_k)
    if exponent > 700.0:
        return float("inf")
    return 2.0 * (2.0 * math.pi * M_E * K_B * t_k / H_PLANCK ** 2) ** 1.5 * math.exp(exponent)


def _debye_radius(t_k, n_e, ions):
    """Debye radius [cm] of the plasma: 1/R_D^2 = 4 pi e^2/(k T) (n_e + sum n_i Z_i^2)."""
    inv_rd2 = 4.0 * math.pi * E_ESU * E_ESU / (K_B * t_k) * (n_e + ions)
    if inv_rd2 <= 0.0:
        return float("inf")
    return 1.0 / math.sqrt(inv_rd2)


def state(rho, t_k, x_h, y_he, z_metal):
    """Ionisation state at (rho [g/cm^3], T [K]).

    Returns a dict with the number densities [cm^-3] and the derived quantities:
    n_e, n_h, n_hp, n_he, n_hep, n_hepp, n_z, pressure [dyn/cm^2 -> returned in Pa],
    internal energy [erg/g], and the mean molecular weight per free particle.
    """
    m_u = M_U
    n_h_tot = rho * x_h / m_u
    n_he_tot = rho * y_he / (4.0 * m_u)
    n_z = rho * z_metal / (A_HEAVY * m_u)

    # Continuum lowering (the Debye-Hueckel "pressure ionisation" correction): inside a dense
    # plasma the neighbouring ions and electrons screen the Coulomb barrier, which lowers the
    # ionisation potential by (Z+1) e^2/R_D.  Without it the Saha equation leaves the solar
    # centre 17% neutral, which is wrong - the centre is fully ionised.  Citable to the standard
    # Debye-Hueckel treatment of the Saha equation (e.g. Kippenhahn, Weigert & Weiss, ch. 15).
    phi_h = _saha_constant(t_k, CHI_H)
    phi_he1 = _saha_constant(t_k, CHI_HE1)
    phi_he2 = _saha_constant(t_k, CHI_HE2)

    def solve(ne):
        # H: n_hp/n_h = phi_h/ne
        ratio = phi_h / ne if math.isfinite(phi_h) else float("inf")
        if math.isinf(ratio):
            n_hp, n_h = n_h_tot, 0.0
        else:
            n_hp = n_h_tot * ratio / (1.0 + ratio)
            n_h = n_h_tot - n_hp
        # He: two successive ionisations.
        r1 = phi_he1 / ne if math.isfinite(phi_he1) else float("inf")
        r2 = phi_he2 / ne if math.isfinite(phi_he2) else float("inf")
        if math.isinf(r1):
            n_hep, n_hepp, n_he = 0.0, n_he_tot, 0.0
        elif math.isinf(r2):
            n_hep, n_hepp, n_he = 0.0, n_he_tot, 0.0
        else:
            denom = 1.0 + r1 + r1 * r2
            n_hep = n_he_tot * r1 / denom
            n_hepp = n_he_tot * r1 * r2 / denom
            n_he = n_he_tot * 1.0 / denom
        return n_h, n_hp, n_he, n_hep, n_hepp

    def charge_residual(ne):
        n_h, n_hp, n_he, n_hep, n_hepp = solve(ne)
        # Guard the infinite-ratio case (a fully ionised species) against 0 * inf.
        return (0.0 if n_hp != n_hp else n_hp) + (0.0 if n_hep != n_hep else n_hep) \
            + 2.0 * (0.0 if n_hepp != n_hepp else n_hepp) + Z_HEAVY * n_z - ne

    # Bisection on log(n_e): the residual is monotone decreasing in n_e.
    lo = math.log10(max(1.0e-30, 1.0e-10 * n_h_tot))
    hi = math.log10(max(1.0, 10.0 * (n_h_tot + n_he_tot + Z_HEAVY * n_z)))
    flo = charge_residual(10.0 ** lo)
    for _ in range(50):
        mid = 0.5 * (lo + hi)
        fmid = charge_residual(10.0 ** mid)
        if (fmid > 0.0) == (flo > 0.0):
            lo, flo = mid, fmid
        else:
            hi = mid
    ne = 10.0 ** (0.5 * (lo + hi))

    # One refinement pass with continuum lowering, using the Debye radius of the state just
    # found: this is what turns the dense interior fully ionised while leaving the thin envelope
    # untouched.
    for _ in range(3):
        n_h, n_hp, n_he, n_hep, n_hepp = solve(ne)
        ions = n_hp + n_hep * 4.0 + n_hepp * 9.0 + n_z * Z_HEAVY ** 2
        rd = _debye_radius(t_k, ne, ions)
        if not math.isfinite(rd):
            break
        lowering = E_ESU * E_ESU / rd
        phi_h = _saha_constant(t_k, CHI_H - lowering)
        phi_he1 = _saha_constant(t_k, CHI_HE1 - 2.0 * lowering)
        phi_he2 = _saha_constant(t_k, CHI_HE2 - 2.0 * lowering)
        lo = math.log10(max(1.0e-30, 1.0e-10 * n_h_tot))
        hi = math.log10(max(1.0, 10.0 * (n_h_tot + n_he_tot + Z_HEAVY * n_z)))
        flo = charge_residual(10.0 ** lo)
        for _ in range(40):
            mid = 0.5 * (lo + hi)
            fmid = charge_residual(10.0 ** mid)
            if (fmid > 0.0) == (flo > 0.0):
                lo, flo = mid, fmid
            else:
                hi = mid
        ne = 10.0 ** (0.5 * (lo + hi))

    n_h, n_hp, n_he, n_hep, n_hepp = solve(ne)
    n_total = n_h + n_hp + n_he + n_hep + n_hepp + ne + n_z

    pressure = n_total * K_B * t_k + 0.25 * A_RAD_CGS / 7.5657e-15 * 0.0   # electrons+ions
    pressure = n_total * K_B * t_k                                         # dyn/cm^2
    radiation = 7.5657e-15 * t_k ** 4 / 3.0                                # dyn/cm^2
    pressure += radiation

    energy = (1.5 * n_total * K_B * t_k
              + CHI_H * n_hp + CHI_HE1 * n_hep + CHI_HE2 * n_hepp) / rho     # erg/g

    return {
        "n_e": ne, "n_h": n_h, "n_hp": n_hp, "n_he": n_he, "n_hep": n_hep,
        "n_hepp": n_hepp, "n_z": n_z, "n_total": n_total,
        "pressure": pressure,                                            # dyn/cm^2
        "energy": energy,                                                # erg/g
        "mu": rho / (n_total * M_U),
    }


def pressure(rho, t_k, x_h, y_he, z_metal):
    return state(rho, t_k, x_h, y_he, z_metal)["pressure"]


def density_for_pressure(p_target, t_k, x_h, y_he, z_metal, tolerance=1.0e-10):
    """Invert rho(P, T) by bisection on log(rho).

    p_target in dyn/cm^2.  The pressure is monotone in density at fixed temperature, so this is
    well posed; the only subtlety is that a cold, mostly neutral gas needs a wide bracket.
    """
    lo, hi = -30.0, 12.0
    for _ in range(45):
        mid = 0.5 * (lo + hi)
        p = pressure(10.0 ** mid, t_k, x_h, y_he, z_metal)
        if p < p_target:
            lo = mid
        else:
            hi = mid
        if hi - lo < tolerance:
            break
    return 10.0 ** (0.5 * (lo + hi))


def density_for_pressure_safe(p_target, t_k, x_h, y_he, z_metal, rho_guess=None,
                              iterations=40):
    """Invert rho(P, T) with a bracket-keeping Newton and a bisection fallback.

    The pressure is monotone in density at fixed temperature, so a bracket can be grown from the
    guess until it contains the target; Newton steps are then taken inside it but are thrown away
    in favour of the bisection midpoint whenever they leave it.  This is what the structure
    integrator needs: the plain Newton (density_newton) is faster but was seeded from a cache that
    the Runge-Kutta stages overwrite, which is not a property a thermodynamic routine should have.
    """
    def log_p(log_rho):
        return pressure(10.0 ** log_rho, t_k, x_h, y_he, z_metal)

    if rho_guess is None or not (rho_guess > 0.0):
        rho_guess = 1.0
    lo = math.log10(rho_guess) - 0.5
    hi = lo + 1.0
    for _ in range(60):
        if log_p(lo) <= p_target:
            break
        lo -= 1.0
    for _ in range(60):
        if log_p(hi) >= p_target:
            break
        hi += 1.0
    for _ in range(iterations):
        mid = 0.5 * (lo + hi)
        p_mid = log_p(mid)
        if p_mid < p_target:
            lo = mid
        else:
            hi = mid
        if hi - lo < 1.0e-12:
            break
    return 10.0 ** (0.5 * (lo + hi))


def nabla_ad(rho, t_k, x_h, y_he, z_metal, relative_step=1.0e-4):
    """Adiabatic temperature gradient from the first law, du + P d(1/rho) = 0.

    A small step in temperature is taken, the density that keeps the gas adiabatic between the
    two states is found by bisection, and the two pressure changes give
    grad_ad = d ln T / d ln P along that adiabat.
    """
    base = state(rho, t_k, x_h, y_he, z_metal)
    t_new = t_k * (1.0 + relative_step)
    target = base["energy"]

    def adiabatic_residual(rho_new):
        s = state(rho_new, t_new, x_h, y_he, z_metal)
        return s["energy"] + s["pressure"] * (1.0 / rho_new - 1.0 / rho) - target

    lo = math.log10(rho * 0.5)
    hi = math.log10(rho * 1.5)
    f_lo = adiabatic_residual(10.0 ** lo)
    for _ in range(18):
        mid = 0.5 * (lo + hi)
        f_mid = adiabatic_residual(10.0 ** mid)
        if (f_mid > 0.0) == (f_lo > 0.0):
            lo, f_lo = mid, f_mid
        else:
            hi = mid
        if hi - lo < 1.0e-12:
            break
    rho_new = 10.0 ** (0.5 * (lo + hi))
    p_new = state(rho_new, t_new, x_h, y_he, z_metal)["pressure"]
    return math.log(t_new / t_k) / math.log(p_new / base["pressure"])


def selftest():
    # 1. The solar centre: fully ionised, so the mean molecular weight must be the ideal-gas one.
    centre = state(152.0, 1.558e7, 0.34, 0.64, 0.02)
    print(f"solar centre: n_e = {centre['n_e']:.4e} cm^-3, mu = {centre['mu']:.4f},"
          f" P = {centre['pressure'] * 0.1:.4e} Pa")
    assert abs(centre["mu"] - 0.85) < 0.05, centre["mu"]
    assert abs(centre["pressure"] * 0.1 / 2.3e16 - 1.0) < 0.15, centre["pressure"] * 0.1

    # 2. A cool, thin layer: hydrogen must be mostly neutral and the molecular weight near the
    #    neutral value 1/(2X + 3Y/4 + Z/14) ~ 1.2 for the solar mixture.
    cool = state(1.0e-6, 6.0e3, 0.70, 0.28, 0.02)
    # Neutral molecular weight: 1/mu = X/1 + Y/4 + Z/14 (no electrons counted).
    neutral_mu = 1.0 / (0.70 / 1.0 + 0.28 / 4.0 + 0.02 / 14.0)
    print(f"cool layer  : n_H+ / n_H = {cool['n_hp'] / max(cool['n_h'], 1e-300):.3e},"
          f" mu = {cool['mu']:.3f} (neutral {neutral_mu:.3f})")
    assert cool["mu"] > 0.98 * neutral_mu, (cool["mu"], neutral_mu)

    # 3. Ionisation raises the mean molecular weight per particle back towards the ionised value
    #    as the temperature rises: check the direction of the change.
    mus = [state(1.0e-4, t, 0.70, 0.28, 0.02)["mu"] for t in (6e3, 1e4, 2e4, 1e5)]
    print(f"mu(T) at fixed rho: {['%.3f' % m for m in mus]}")
    assert mus[0] > mus[-1], mus

    # 4. The adiabatic gradient: 0.4 for a fully ionised monatomic gas, and *lower* in the
    #    hydrogen ionisation zone where the latent heat softens the equation of state.
    deep = nabla_ad(1.0, 1.0e7, 0.70, 0.28, 0.02)
    ionization = nabla_ad(1.0e-4, 1.2e4, 0.70, 0.28, 0.02)
    print(f"grad_ad: deep interior {deep:.4f} (0.4 expected), H ionisation zone {ionization:.4f}")
    assert abs(deep - 0.4) < 0.02, deep
    assert ionization < deep, (ionization, deep)

    # 5. The bracket-keeping inversion (used by any formulation that needs rho(P, T)).
    rho_safe = density_for_pressure_safe(1.0e15, 6.0e6, 0.70, 0.28, 0.02, 1.0)
    p_safe = pressure(rho_safe, 6.0e6, 0.70, 0.28, 0.02)
    print(f"safe inversion: rho = {rho_safe:.4e} g/cm^3 -> P = {p_safe * 0.1:.4e} Pa")
    assert abs(p_safe / 1.0e15 - 1.0) < 1e-6, p_safe

    # 6. Density inversion round trip.
    rho = density_for_pressure(1.0e15, 6.0e6, 0.70, 0.28, 0.02)
    p_back = pressure(rho, 6.0e6, 0.70, 0.28, 0.02)
    print(f"density inversion: rho = {rho:.4e} g/cm^3 -> P = {p_back * 0.1:.4e} Pa"
          f" (target 1.0e15 dyn/cm^2 = 1.0e14 Pa)")
    assert abs(p_back / 1.0e15 - 1.0) < 1e-6, p_back

    print("eos: Saha ionisation, adiabatic gradient and density inversion all check out")


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
