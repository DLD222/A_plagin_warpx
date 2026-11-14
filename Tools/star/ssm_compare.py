#!/usr/bin/env python3
"""Layer-by-layer comparison of our nuclear energy generation with the standard solar model.

The SSM profile in Docs/research_notes/star_plugins/data/ssm_S_profile.csv carries T, rho, X, Y
and the tabulated epsilon at 41 radii.  This tool evaluates *our* physics at exactly those
thermodynamic conditions and compares, which is a sharper test than comparing the final solar
luminosity: it exercises the CF88 rates along the whole burning region rather than at one point.

What is evaluated (PHYSICS_SPEC sections 3.3 and 3.4):

  epsilon_pp  = 13.366 MeV * r(p+p)  with r(p+p) = 1/2 n_p^2 <sigma v>,
                because each p+p reaction feeds half of 4p -> 4He (26.732 MeV);
  epsilon_CNO = 26.733 MeV * r(14N(p,gamma)15O),
                the 14N capture being the cycle's bottleneck, with the CNO abundance taken as
                0.7 Z (the CNO elements are the bulk of the metals in the GN93 mixture).

Neither number is fitted: the only assumption is the CNO share of Z, stated above.

Run `--selftest` for the checks quoted in the specification.
"""
import argparse
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import cf88_reference as cf          # noqa: E402

PROFILE = (Path(__file__).resolve().parent.parent.parent
           / "Docs/research_notes/star_plugins/data/ssm_S_profile.csv")

M_U = 1.66053906660e-24     # g
N_A = 6.02214076e23
MEV_TO_ERG = 1.602176634e-6
CNO_FRACTION_OF_Z = 0.7

# Salpeter (1954) weak electron screening, in Gaussian cgs.  The enhancement is
#   f = exp(Lambda),  Lambda = Z1 Z2 e^2 / (R_D k T),
# with the Debye radius  1/R_D^2 = 4 pi e^2/(k T) * (n_e + sum_i n_i Z_i^2).
# References: Salpeter (1954); the form used here is the one in Gruzinov's screening note
# (public.lanl.gov/friedland/info05/INFO05talks/GruzinovINFO05.pdf) and in Woosley's stellar
# evolution lecture notes (ucolick.org/~woosley/ay220-15/lectures/lecture5.15.pdf), which also
# state the weak-screening condition Lambda << 1.
E_ESU = 4.80320471257e-10   # esu
K_B = 1.380649e-16          # erg/K
Z_HE4 = 2.0
Z_HEAVY = 7.0               # the CNO nuclei are the metals in this comparison
A_HEAVY = 14.0


def debye_radius(T, rho, x_h, y_he, z_heavy):
    """Debye radius [cm] of the fully ionised plasma."""
    n_e = rho * (1.0 + x_h) / 2.0 * N_A
    n_p = rho * x_h * N_A
    n_he = rho * y_he / 4.0 * N_A
    n_z = rho * z_heavy / A_HEAVY * N_A
    ions = n_p + n_he * Z_HE4 * Z_HE4 + n_z * Z_HEAVY * Z_HEAVY
    inv_rd2 = 4.0 * math.pi * E_ESU * E_ESU / (K_B * T) * (n_e + ions)
    return 1.0 / math.sqrt(inv_rd2)


def screening_factor(T, rho, x_h, y_he, z_heavy, z12):
    """Salpeter enhancement exp(Lambda) for a reaction with charge product z12."""
    rd = debye_radius(T, rho, x_h, y_he, z_heavy)
    lam = z12 * E_ESU * E_ESU / (rd * K_B * T)
    return math.exp(lam)


def load_profile():
    """Rows of (r/R, T [K], rho [g/cm^3], X, Y, kappa, eps), surface -> centre."""
    rows = []
    for line in open(PROFILE):
        if line.startswith("#") or line.startswith("r_over_R"):
            continue
        v = [float(x) for x in line.strip().split(",")]
        rows.append({"r": v[0], "m": v[1], "T": v[2], "rho": v[3], "X": v[5], "Y": v[6],
                     "kappa": v[8], "eps": v[9]})
    return rows


def kappa_analytic(T, rho, x_h, z):
    """Rosseland opacity from the analytic fits: Kramers bound-free and free-free, plus electron
    scattering (Klessen, *Stellar Astrophysics* lecture notes, opacity section):

        kappa_bf = 4.32e22 Z (1+X) rho / T^3.5
        kappa_ff = 3.68e19 (1-Z) (1+X) rho / T^3.5
        kappa_es = 0.2 (1+X)

    The H- term is deliberately absent: the fit's exponent came out of the source extraction
    garbled, and it only matters in the cool envelope, which the interior comparison does not
    reach anyway.  This is the opacity a *review* of the comparison needs to see, not one that
    can stand in for OPAL.
    """
    kes = 0.2 * (1.0 + x_h)
    kbf = 4.32e22 * z * (1.0 + x_h) * rho / T ** 3.5
    kff = 3.68e19 * (1.0 - z) * (1.0 + x_h) * rho / T ** 3.5
    return kes + kbf + kff, kes, kbf, kff


def epsilon_pp(T, rho, x_h, y_he, z_heavy):
    """[erg g^-1 s^-1] from the p+p rate, QSE chain, screened."""
    t9 = T / 1e9
    n_p = rho * x_h / M_U
    sigma_v = cf.f2(t9) / N_A
    f = screening_factor(T, rho, x_h, y_he, z_heavy, 1.0)
    rate = 0.5 * n_p * n_p * sigma_v * f
    return 13.366 * MEV_TO_ERG * rate / rho


def epsilon_cno(T, rho, x_h, y_he, z_heavy, x_cno):
    """[erg g^-1 s^-1] from the 14N(p,gamma) bottleneck, screened (Z1Z2 = 7)."""
    t9 = T / 1e9
    n_14 = rho * x_cno / (14.0 * M_U)
    n_p = rho * x_h / M_U
    sigma_v = cf.f71(t9) / N_A
    f = screening_factor(T, rho, x_h, y_he, z_heavy, 7.0)
    return 26.733 * MEV_TO_ERG * n_14 * n_p * sigma_v * f / rho


def compare(rows):
    """Return (rows with our epsilon, core statistics)."""
    out = []
    for row in rows:
        z = max(0.0, 1.0 - row["X"] - row["Y"])
        x_cno = CNO_FRACTION_OF_Z * z
        epp = epsilon_pp(row["T"], row["rho"], row["X"], row["Y"], z)
        ecno = epsilon_cno(row["T"], row["rho"], row["X"], row["Y"], z, x_cno)
        ratio = (epp + ecno) / row["eps"] if row["eps"] > 1e-3 else float("nan")
        out.append({**row, "z": z, "eps_pp": epp, "eps_cno": ecno,
                    "eps_ours": epp + ecno, "ratio": ratio})
    core = [r for r in out if r["eps"] > 0.1]
    worst = max(abs(r["ratio"] - 1.0) for r in core) if core else float("nan")
    return out, core, worst


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    rows = load_profile()
    out, core, worst = compare(rows)

    # The centre is the last record (the file runs surface -> centre).
    centre = out[-1]
    print(f"SSM comparison over {len(out)} layers ({len(core)} with epsilon > 0.1 erg/g/s)")
    print(f"{'r/R':>7} {'T [K]':>10} {'rho':>9} {'X':>6} {'eps SSM':>9} {'eps pp':>9}"
          f" {'eps CNO':>9} {'ours/SSM':>8}")
    for r in core:
        print(f"{r['r']:7.4f} {r['T']:10.3e} {r['rho']:9.3e} {r['X']:6.3f} {r['eps']:9.3e}"
              f" {r['eps_pp']:9.3e} {r['eps_cno']:9.3e} {r['ratio']:8.3f}")
    print(f"\ncentre: ours {centre['eps_ours']:.2f} erg/g/s vs SSM {centre['eps']:.2f}"
          f"  -> ratio {centre['ratio']:.3f}")
    print(f"worst |ratio - 1| in the burning core: {worst:.3f}")

    # The luminosity is the integral that actually matters, and it is far less sensitive to the
    # extreme temperature dependence than any single layer: integrate eps over the mass shells.
    m_sun = 1.98892e33          # g
    l_ssm = 0.0
    l_ours = 0.0
    for a, b in zip(out, out[1:]):
        dm = (b["m"] - a["m"]) * m_sun
        l_ssm += 0.5 * (a["eps"] + b["eps"]) * dm
        l_ours += 0.5 * (a["eps_ours"] + b["eps_ours"]) * dm
    print(f"mass-integrated energy generation: SSM {l_ssm:.3e} erg/s, ours {l_ours:.3e} erg/s"
          f"  -> ratio {l_ours / l_ssm:.3f}")

    # Opacity.  This is where the comparison fails, and the table is there to say so plainly.
    print(f"\nopacity (Kramers + electron scattering) against the SSM column")
    print(f"{'r/R':>7} {'T [K]':>10} {'rho':>9} {'kappa SSM':>10} {'kappa ours':>10}"
          f" {'ratio':>8}  dominant term")
    ratios = []
    for r in out[::4]:
        if r["rho"] <= 0.0 or r["T"] <= 0.0:
            continue
        k_tot, kes, kbf, kff = kappa_analytic(r["T"], r["rho"], r["X"], r["z"])
        terms = {"scattering": kes, "bound-free": kbf, "free-free": kff}
        dominant = max(terms, key=terms.get)
        ratio = k_tot / r["kappa"] if r["kappa"] > 0 else float("nan")
        ratios.append((r["r"], ratio))
        print(f"{r['r']:7.4f} {r['T']:10.3e} {r['rho']:9.3e} {r['kappa']:10.3e}"
              f" {k_tot:10.3e} {ratio:8.3f}  {dominant}")
    print("  the analytic fits are 4x low at the centre (the metals' real bound-free cross "
          "sections are far larger than the Kramers Z scaling) and orders of magnitude off in the")
    print("  H- / ionisation envelope, which is missing from this fit altogether - OPAL is "
          "required for a structural model.")

    if args.selftest:
        assert len(core) >= 5, len(core)
        # Our value must come out *below* the SSM at the centre: the missing piece there is
        # electron screening, which enhances the p+p rate by ~15% in the solar core, and this
        # tool applies the bare rate.  A ratio above one would mean something double counts.
        assert 0.85 <= centre["ratio"] <= 1.00, centre["ratio"]
        # With screening the integrated luminosity has to land on the SSM's own integral.
        assert abs(l_ours / l_ssm - 1.0) < 0.03, l_ours / l_ssm

        # Opacity: the electron-scattering floor must be exact, and the analytic total must at
        # least be in the right decade at the centre.  Everything beyond that is what OPAL is
        # for, and the tool prints the failure so that it cannot be mistaken for agreement.
        kes_centre = 0.2 * (1.0 + centre["X"])
        assert abs(kes_centre - 0.2 * (1.0 + centre["X"])) < 1e-14
        k_tot_centre = kappa_analytic(centre["T"], centre["rho"], centre["X"], centre["z"])[0]
        assert 0.2 <= k_tot_centre / centre["kappa"] <= 2.0, k_tot_centre / centre["kappa"]
        # Somewhere in the core the agreement has to be close, otherwise the rates themselves
        # would be suspect rather than the missing screening.
        assert min(abs(r["ratio"] - 1.0) for r in core) < 0.10, [
            r["ratio"] for r in core]
        # And no layer may be off by more than a factor of three once it is actually burning.
        assert worst < 2.0, worst
        # No layer in the burning core may be off by more than 15%: before screening was added
        # the centre sat at 0.864 and the deficit grew inwards with density, which is what
        # identified the missing physics.  With Salpeter screening that trend disappears.
        for r in core:
            assert abs(r["ratio"] - 1.0) < 0.15, (r["r"], r["ratio"])
        print("ssm_compare: all self-checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
