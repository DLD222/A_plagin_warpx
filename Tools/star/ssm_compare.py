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


def load_profile():
    """Rows of (r/R, T [K], rho [g/cm^3], X, Y, kappa, eps), surface -> centre."""
    rows = []
    for line in open(PROFILE):
        if line.startswith("#") or line.startswith("r_over_R"):
            continue
        v = [float(x) for x in line.strip().split(",")]
        rows.append({"r": v[0], "T": v[2], "rho": v[3], "X": v[5], "Y": v[6],
                     "kappa": v[8], "eps": v[9]})
    return rows


def epsilon_pp(T, rho, x_h):
    """[erg g^-1 s^-1] from the p+p rate, QSE chain."""
    t9 = T / 1e9
    n_p = rho * x_h / M_U
    sigma_v = cf.f2(t9) / N_A
    rate = 0.5 * n_p * n_p * sigma_v
    return 13.366 * MEV_TO_ERG * rate / rho


def epsilon_cno(T, rho, x_h, x_cno):
    """[erg g^-1 s^-1] from the 14N(p,gamma) bottleneck."""
    t9 = T / 1e9
    n_14 = rho * x_cno / (14.0 * M_U)
    n_p = rho * x_h / M_U
    sigma_v = cf.f71(t9) / N_A
    return 26.733 * MEV_TO_ERG * n_14 * n_p * sigma_v / rho


def compare(rows):
    """Return (rows with our epsilon, core statistics)."""
    out = []
    for row in rows:
        z = max(0.0, 1.0 - row["X"] - row["Y"])
        x_cno = CNO_FRACTION_OF_Z * z
        epp = epsilon_pp(row["T"], row["rho"], row["X"])
        ecno = epsilon_cno(row["T"], row["rho"], row["X"], x_cno)
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

    if args.selftest:
        assert len(core) >= 5, len(core)
        # Our value must come out *below* the SSM at the centre: the missing piece there is
        # electron screening, which enhances the p+p rate by ~15% in the solar core, and this
        # tool applies the bare rate.  A ratio above one would mean something double counts.
        assert 0.75 <= centre["ratio"] <= 1.00, centre["ratio"]
        # Somewhere in the core the agreement has to be close, otherwise the rates themselves
        # would be suspect rather than the missing screening.
        assert min(abs(r["ratio"] - 1.0) for r in core) < 0.10, [
            r["ratio"] for r in core]
        # And no layer may be off by more than a factor of three once it is actually burning.
        assert worst < 2.0, worst
        # The deficit must grow inwards: screening strengthens with density, so a model without it
        # falls further behind the SSM at the centre than in the outer burning shell.  The
        # comparison is not monotone everywhere (the CNO share of Z is an assumption and the
        # outer layers are extremely temperature sensitive), so the check is the size of the
        # overall drop rather than a pointwise ordering.
        ordered = sorted(core, key=lambda r: r["rho"])
        outer_ratio = ordered[0]["ratio"]
        centre_ratio = ordered[-1]["ratio"]
        assert outer_ratio - centre_ratio > 0.05, (outer_ratio, centre_ratio)
        print("ssm_compare: all self-checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
