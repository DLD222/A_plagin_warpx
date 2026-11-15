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
  fork-divergence     differs from both                     -> fork refactor / fork-only plugins: audit target

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
        rows.append(dict(test=n, vs_2610=da, vs_2611=db, upstream_changed=dc,
                         bucket=bucket,
                         max_abs_2610=(A.get(n) or {}).get('max_abs_diff', ''),
                         max_abs_2611=(B.get(n) or {}).get('max_abs_diff', '')))

    counts = {}
    for r in rows:
        counts[r['bucket']] = counts.get(r['bucket'], 0) + 1

    print('bucket counts')
    for k in ('identical', 'upstream-evolution', 'behind-2611',
              'fork-divergence', 'unavailable'):
        print(f'  {k:20s} {counts.get(k, 0):4d}')

    for bucket in ('fork-divergence', 'behind-2611', 'upstream-evolution'):
        sel = [r for r in rows if r['bucket'] == bucket]
        if not sel:
            continue
        print(f'\n## {bucket} ({len(sel)})')
        print(f'{"test":58s} {"vs26.10":>7s} {"vs26.11":>7s} {"upstr":>6s} '
              f'{"maxabs2610":>12s} {"maxabs2611":>12s}')
        for r in sel:
            print(f'{r["test"][:58]:58s} {str(r["vs_2610"]):>7s} {str(r["vs_2611"]):>7s} '
                  f'{str(r["upstream_changed"]):>6s} '
                  f'{str(r["max_abs_2610"])[:12]:>12s} {str(r["max_abs_2611"])[:12]:>12s}')

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
