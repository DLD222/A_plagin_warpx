#!/usr/bin/env python3
"""Spot-check one case (or a list) against both official releases.

For every case whose run directory exists in all three build trees, print

    case, max_abs_diff vs 26.10, max_abs_diff vs 26.11

so the difference can be read off directly: a case that differs from 26.10 but is
bit-identical against 26.11 is upstream evolution (see UPSTREAM_BASE.md), not a
fork defect.

Usage:
    tri_compare.py --cases NAME[,NAME...] [--build-fork DIR] [--build-2610 DIR]
                   [--build-2611 DIR]
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import compare_case as cc  # noqa: E402


def one(build, name):
    d = os.path.join(build, 'bin', name)
    return d if os.path.isdir(d) else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cases', required=True,
                    help='comma-separated case names, or @file with one per line')
    ap.add_argument('--build-fork', default='audit_build/fork')
    ap.add_argument('--build-2610', default='audit_build/official')
    ap.add_argument('--build-2611', default='audit_build/official_2611')
    args = ap.parse_args()

    spec = args.cases
    if spec.startswith('@'):
        names = [l.strip() for l in open(spec[1:]) if l.strip()]
    else:
        names = [n.strip() for n in spec.split(',') if n.strip()]

    print(f'{"case":58s} {"vs 26.10":>12s} {"vs 26.11":>12s}  verdict')
    for n in names:
        df, d0, d1 = one(args.build_fork, n), one(args.build_2610, n), one(args.build_2611, n)
        if not (df and d0 and d1):
            missing = [nm for nm, d in (('fork', df), ('26.10', d0), ('26.11', d1)) if not d]
            print(f'{n[:58]:58s} {"-":>12s} {"-":>12s}  missing run: {",".join(missing)}')
            continue
        r0 = cc.compare_dirs(d0, df, n)
        r1 = cc.compare_dirs(d1, df, n)
        m0 = r0.get('max_abs_diff', 0.0) or 0.0
        m1 = r1.get('max_abs_diff', 0.0) or 0.0
        if m0 == 0.0 and m1 == 0.0:
            verdict = 'identical to both'
        elif m0 > 0.0 and m1 == 0.0:
            verdict = 'upstream evolution (matches 26.11)'
        elif m0 == 0.0 and m1 > 0.0:
            verdict = 'fork lags 26.11'
        else:
            verdict = 'fork-specific difference'
        print(f'{n[:58]:58s} {m0:12.4e} {m1:12.4e}  {verdict}')


if __name__ == '__main__':
    sys.exit(main())
