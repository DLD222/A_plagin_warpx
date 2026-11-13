#!/usr/bin/env python3
"""Read a GONG-format stellar model and print/extract radial profiles.

The format is described in "File format for the GONG solar model project"
(https://github.com/jcd11/LRSP_models/blob/master/file-format.pdf).  Layout:

    lines 1-4   free text (title, date, directory, model file)
    line  5     four integers: n_shells, iversion, ..., n_shells
    next 40     global parameters, 5 per line
    then        n_shells records of 25 values, 5 per line

Variable order (same document, section 2.3), 1-based:
     1 r     2 ln q (q = m/M)   3 T      4 p        5 rho
     6 X     7 L(r)             8 kappa  9 eps     10 Gamma1
    11 grad_ad  12 delta  13 c_p  16 r_X  17 Z
    21 X(3He) 22 X(12C) 23 X(13C) 24 X(14N) 25 X(16O)
    29 X(2H)  30 X(4He)

Usage:
    parse_fgong.py MODEL.fgong --csv OUT.csv --rows 60
"""
import argparse
import sys

NVAR = 25                 # variables per mesh point
GLOBALS = 40              # global parameters after the header
# Some files carry a few extra trailing numbers; the reader tolerates that below.
NAMES = ["r", "lnq", "T", "p", "rho", "X", "L", "kappa", "eps",
         "Gamma1", "grad_ad", "delta", "cp", "v14", "v15",
         "r_X", "Z", "Rmr", "eps_g", "L_g",
         "X_3He", "X_12C", "X_13C", "X_14N", "X_16O"]


def load(path):
    with open(path) as f:
        lines = f.readlines()
    header = [int(v) for v in lines[4].split()[:4]]
    n_shells = header[0]
    values = []
    for line in lines[5:]:
        values.extend(float(v) for v in line.split())
    # Trust the record count over the header field: real files disagree by a record
    # (the standard model here has 2481 records for a header value of 2482).
    n_records = (len(values) - GLOBALS) // NVAR
    if n_records != n_shells:
        print(f"# note: header says {n_shells} shells, file holds {n_records} records",
              file=sys.stderr)
    n_shells = n_records
    globals_ = values[:GLOBALS]
    data = values[GLOBALS:]
    shells = [data[i * NVAR:(i + 1) * NVAR] for i in range(n_shells)]
    return header, globals_, shells


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("model")
    ap.add_argument("--csv", help="write a subsampled profile here")
    ap.add_argument("--rows", type=int, default=60,
                    help="how many radial points to write (evenly spaced in index)")
    args = ap.parse_args()

    header, globals_, shells = load(args.model)
    # Globals per the same document, section 2.2: 1 M, 2 R, 3 L, 4 Z0, 5 X0,
    # 13 model age [yr], 14 T_eff [K], 15 G [cgs].
    print(f"# shells={header[0]}  M={globals_[0]:.6e} g  R={globals_[1]:.6e} cm  "
          f"L={globals_[2]:.6e} erg/s")
    print(f"# age={globals_[12] / 3.15576e7:.3f} Gyr  Teff={globals_[13]:.1f} K  "
          f"X0={globals_[4]:.5f}  Z0={globals_[3]:.5f}")

    # Records run from the surface (index 0, r = R) to the centre (index -1, r ~ 0).
    surface, centre = shells[0], shells[-1]
    print(f"# surface: T={surface[2]:.1f} K rho={surface[4]:.3e} g/cm3 L={surface[6]:.4e} erg/s")
    print(f"# centre : T={centre[2]:.4e} K rho={centre[4]:.4e} g/cm3 "
          f"X={centre[5]:.5f} eps={centre[8]:.3e} erg/g/s kappa={centre[7]:.4e}")

    if args.csv:
        step = max(1, header[0] // args.rows)
        with open(args.csv, "w") as out:
            out.write("# Records run surface -> centre, as in the GONG file.\n")
            out.write("r_over_R,m_over_M,T_K,rho_g_cm3,P_dyn_cm2,X,Y,L_erg_s,kappa,eps\n")
            for sh in shells[::step]:
                r, lnq, T, p, rho, X, L = sh[0], sh[1], sh[2], sh[3], sh[4], sh[5], sh[6]
                out.write(f"{r / globals_[1]:.6f},{pow(2.718281828459045, lnq):.6e},"
                          f"{T:.6e},{rho:.6e},{p:.6e},{X:.6e},{1.0 - X - sh[16]:.6e},"
                          f"{L:.6e},{sh[7]:.6e},{sh[8]:.6e}\n")
        print(f"# wrote {len(range(0, header[0], step))} rows to {args.csv}")


if __name__ == "__main__":
    sys.exit(main())
