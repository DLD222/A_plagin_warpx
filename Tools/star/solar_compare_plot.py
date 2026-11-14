#!/usr/bin/env python3
"""The solar-model comparison figure: our physics against the standard solar model.

Runs the structural diagnostic (our equation of state, OPAL opacity, mixing length and reaction
network, started from the SSM's own centre) and plots the result against the SSM profile, layer
by layer, in the five quantities that the physics enters through:

    T(r)      the structure
    rho(r)    the structure
    L(r)      the energy generation, integrated
    eps(r)    the reaction network
    kappa(r)  the opacity

Saves Docs/research_notes/star_plugins/solar_comparison.png and prints the table the
specification quotes.

Run: python3 Tools/star/solar_compare_plot.py
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt          # noqa: E402

import stellar_structure as ss           # noqa: E402
from ssm_compare import epsilon_pp, epsilon_cno, kappa_analytic, CNO_FRACTION_OF_Z  # noqa: E402
from opal_opacity import kappa_combined as opal_kappa  # noqa: E402

OUT = Path(__file__).resolve().parent.parent.parent / "Docs/research_notes/star_plugins"
R_SUN = ss.R_SUN
M_SUN = ss.M_SUN


def load_ssm():
    rows = []
    for line in open(ss.PROFILE):
        if line.startswith("#") or line.startswith("r_over_R"):
            continue
        v = [float(x) for x in line.strip().split(",")]
        rows.append({"r": v[0], "m": v[1] * M_SUN, "T": v[2], "rho": v[3],
                     "P": v[4] * 0.1, "X": v[5], "Y": v[6], "L": v[7], "kappa": v[8],
                     "eps": v[9]})
    return rows


def run_model(number_of_samples=241):
    """Integrate our physics from the SSM's centre and return the profile as arrays."""
    rows = sorted(load_ssm(), key=lambda q: q["m"])
    mass = rows[-1]["m"]

    def x_of_m(m):
        if m <= rows[0]["m"]:
            return rows[0]["X"]
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
    samples = ss.integrate_solar_structure(mass, x_of_m, z_of_m, centre["P"], centre["T"],
                                           n_steps=240, alpha_mlt=1.5)
    samples = [s for s in samples if s[3] > 0.0 and s[2] > 0.0]
    m = np.array([s[0] for s in samples])
    r = np.array([s[1] for s in samples])
    rho = np.array([s[2] for s in samples]) * 1.0e-3        # kg/m^3 -> g/cm^3
    t = np.array([s[3] for s in samples])
    lum = np.array([s[4] for s in samples])
    x = np.array([x_of_m(v) for v in m])
    z = np.array([z_of_m(v) for v in m])
    y = np.maximum(0.0, 1.0 - x - z)

    eps = np.array([epsilon_pp(t[i], rho[i], x[i], y[i], z[i])
                    + epsilon_cno(t[i], rho[i], x[i], y[i], z[i], CNO_FRACTION_OF_Z * z[i])
                    for i in range(len(m))])
    kap = np.array([opal_kappa(t[i], rho[i], x[i], z[i]) for i in range(len(m))])
    ssm = rows
    return {"m": m, "r": r, "rho": rho, "T": t, "L": lum, "eps": eps, "kappa": kap,
            "ssm": ssm, "mass": mass}


def interp_log(x_target, x_src, y_src):
    """Interpolate a positive quantity logarithmically (the profiles span many decades)."""
    return 10.0 ** np.interp(np.log10(np.maximum(x_target, 1e-300)),
                             np.log10(x_src), np.log10(np.maximum(y_src, 1e-300)))


def interpolate_ssm(ssm, mass):
    m = np.array([q["m"] for q in ssm])
    out = {}
    for key in ("T", "rho", "L", "eps", "kappa", "r"):
        out[key] = np.array([q[key] for q in ssm])
    out["m"] = m
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--show", action="store_true")
    args = ap.parse_args()

    result = run_model()
    ssm = interpolate_ssm(result["ssm"], result["mass"])
    m_over_M = result["m"] / result["mass"]
    ssm_m_over_M = ssm["m"] / result["mass"]

    # Everything is plotted against m/M on a linear axis: the interior is flat in T and rho over
    # most of the mass, so a log mass axis hides exactly the structure being compared.
    fig, axes = plt.subplots(2, 3, figsize=(15.0, 8.0))
    panels = [
        ("T", r"$T$ [K]", "temperature"),
        ("rho", r"$\rho$ [g cm$^{-3}$]", "density"),
        ("L", r"$L$ [W]", "luminosity"),
        ("eps", r"$\epsilon$ [erg g$^{-1}$ s$^{-1}$]", "energy generation (ours)"),
        ("kappa", r"$\kappa$ [cm$^2$ g$^{-1}$]", "opacity (ours)"),
    ]
    for ax, (key, label, title) in zip(axes.flat, panels):
        ours = result[key].astype(float)
        # The SSM is plotted as its own tabulated points rather than interpolated: several of its
        # columns are exactly zero outside the burning region, and interpolating a logarithm
        # through those zeros would invent decades of spurious values.
        ref = ssm[key].astype(float)
        if key == "L":
            ref = ref * 1.0e-7          # the SSM file carries erg/s, ours is in W
        keep_ref = ref > 0.0
        keep_ours = ours > 0.0
        ax.semilogy(ssm_m_over_M[keep_ref], ref[keep_ref], "ko", ms=3.5,
                    label="SSM (Christensen-Dalsgaard)")
        ax.semilogy(m_over_M[keep_ours], ours[keep_ours], "C3-", lw=1.6, label="this work")
        ax.set_xlabel(r"$m/M_\odot$")
        ax.set_ylabel(label)
        ax.set_title(title)
        ax.set_xlim(0.0, 1.0)
        ax.grid(True, which="both", alpha=0.3)
    axes.flat[5].axis("off")
    handles, labels = axes.flat[0].get_legend_handles_labels()
    axes.flat[5].legend(handles, labels, loc="center", fontsize=11)
    axes.flat[5].text(0.02, 0.62,
                      f"L = {result['L'][-1] / 3.846e26:.3f} L$_\\odot$\n"
                      f"R = {result['r'][-1] / R_SUN:.3f} R$_\\odot$",
                      fontsize=13, family="monospace")
    fig.suptitle("Solar model layer by layer: our EOS + OPAL opacity + MLT + reaction network",
                 fontsize=13)
    fig.tight_layout()
    out = OUT / "solar_comparison.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")

    # The table the specification quotes.
    print(f"\n{'m/M':>7} {'r/R ours':>9} {'T ratio':>8} {'rho ratio':>10} {'eps ratio':>10}"
          f" {'kappa ratio':>11}")
    for target in (0.01, 0.05, 0.1, 0.2, 0.4, 0.6, 0.8, 0.9, 0.99):
        i = int(np.argmin(np.abs(m_over_M - target)))
        j = int(np.argmin(np.abs(ssm_m_over_M - target)))
        print(f"{m_over_M[i]:7.3f} {result['r'][i] / R_SUN:9.3f}"
              f" {result['T'][i] / ssm['T'][j]:8.3f} {result['rho'][i] / ssm['rho'][j]:10.3f}"
              f" {result['eps'][i] / max(ssm['eps'][j], 1e-30):10.3f}"
              f" {result['kappa'][i] / ssm['kappa'][j]:11.3f}")

    if args.show:
        plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
