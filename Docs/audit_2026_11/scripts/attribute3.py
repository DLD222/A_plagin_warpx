#!/usr/bin/env python3
"""Three-way attribution of the fork's case-level differences.

The fork's source is a snapshot of upstream `development` taken between the 26.10
and 26.11 releases (see `UPSTREAM_BASE.md`), so a difference against 26.10 alone
cannot be blamed on the fork.  This script combines three pairwise case-by-case
comparisons and puts every case into one of four buckets:

  A  fork vs official 26.10      (compare_all.py --a audit_build/fork --b audit_build/official)
  B  fork vs official 26.11      (compare_all.py --a audit_build/fork --b audit_build/official_2611)
  C  official 26.10 vs 26.11     (compare_all.py --a audit_build/official --b audit_build/official_2611)

buckets:
  identical           identical to both releases
  upstream-evolution  differs from 26.10 but matches 26.11  -> inherited upstream change, not a fork defect
  behind-2611         matches 26.10 but differs from 26.11   -> one of the upstream commits the fork lacks
  fork-lags-2611      differs from both, but the fork sits on the 26.10 side of the upstream
                      change -> one of the upstream commits the fork does not carry; not a fork bug
  fork-divergence     differs from both by a comparable amount -> fork refactor / fork-only
                      plugins: audit target

Usage:
    attribute3.py --a cases_a.csv --b cases_b.csv --c cases_c.csv [--out DIR]
"""
import argparse
import csv
import os
import sys


def load(path):
    with open(path, newline='') as fh:
        return {row['test']: row for row in csv.DictReader(fh)}


def value_diff(row):
    """Value-level difference: max_abs_diff > 0.

    A case can differ byte-wise while every quantity agrees (build metadata, the composed
    input deck's AMReX defaults); that is not a physics difference and must not be mixed
    with one.  Returns None when the case was not compared.
    """
    if row is None:
        return None
    try:
        return float(row.get('max_abs_diff') or 0.0) > 0.0
    except (TypeError, ValueError):
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--a', required=True, help='fork vs official 26.10 cases.csv')
    ap.add_argument('--b', required=True, help='fork vs official 26.11 cases.csv')
    ap.add_argument('--c', required=True, help='official 26.10 vs 26.11 cases.csv')
    ap.add_argument('--out', default='')
    args = ap.parse_args()

    A, B, C = load(args.a), load(args.b), load(args.c)
    names = sorted(set(A) | set(B) | set(C))
    rows = []
    for n in names:
        da, db, dc = value_diff(A.get(n)), value_diff(B.get(n)), value_diff(C.get(n))
        if da is None or db is None:
            bucket = 'unavailable'
        elif not da and not db:
            bucket = 'identical'
        elif da and not db:
            bucket = 'upstream-evolution'
        elif not da and db:
            bucket = 'behind-2611'
        else:
            bucket = 'fork-divergence'
        def _m(row):
            try:
                return float((row or {}).get('max_abs_diff') or 0.0)
            except (TypeError, ValueError):
                return 0.0
        m0, m1, mc = _m(A.get(n)), _m(B.get(n)), _m(C.get(n))
        # For a case that differs from both releases, decide whether the fork sits on the
        # 26.10 side of the upstream change (|fork-26.10| much smaller than |26.10-26.11|,
        # i.e. the fork simply does not carry that upstream commit) or whether it differs
        # from both by a comparable amount (a fork-specific difference).
        if bucket == 'fork-divergence':
            if mc > 0.0 and m0 < 0.25 * mc and m1 >= 0.25 * mc:
                bucket = 'fork-lags-2611'
        rows.append(dict(test=n, vs_2610=da, vs_2611=db, upstream_changed=dc,
                         bucket=bucket,
                         max_abs_2610=m0, max_abs_2611=m1, max_abs_upstream=mc))

    counts = {}
    for r in rows:
        counts[r['bucket']] = counts.get(r['bucket'], 0) + 1

    print('bucket counts')
    for k in ('identical', 'upstream-evolution', 'behind-2611', 'fork-lags-2611',
              'fork-divergence', 'unavailable'):
        print(f'  {k:20s} {counts.get(k, 0):4d}')

    for bucket in ('fork-divergence', 'fork-lags-2611', 'behind-2611',
                   'upstream-evolution'):
        sel = [r for r in rows if r['bucket'] == bucket]
        if not sel:
            continue
        print(f'\n## {bucket} ({len(sel)})')
        print(f'{"test":52s} {"vs26.10":>10s} {"vs26.11":>10s} {"2610-2611":>10s} {"upstr":>6s}')
        for r in sel:
            print(f'{r["test"][:52]:52s} {r["max_abs_2610"]:10.3e} {r["max_abs_2611"]:10.3e} '
                  f'{r["max_abs_upstream"]:10.3e} {str(r["upstream_changed"]):>6s}')

    if args.out:
        os.makedirs(args.out, exist_ok=True)
        path = os.path.join(args.out, 'attribution3.csv')
        with open(path, 'w', newline='') as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)
        print(f'\nwrote {path}', file=sys.stderr)


if __name__ == '__main__':
    sys.exit(main())
