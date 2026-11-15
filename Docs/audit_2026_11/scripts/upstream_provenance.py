#!/usr/bin/env python3
"""Which upstream WarpX version does each file of this fork correspond to?

The fork (A_plagin_warpx) restructured `Source/` into `Source/Core` and
`Source/Plugins/*` and replaced the `WarpX::` static configuration with a
runtime-context service, so a plain `diff` against upstream is dominated by
mechanical noise.  This script instead compares the *sets of normalized source
lines* of a file with the same basename in three upstream trees, and reports
which one it matches best.

Normalization drops comments, `#include` lines, blank lines and trailing
whitespace; lines are compared as an unordered set (Jaccard similarity), which
is insensitive to reordering and to the renaming of the surrounding API.

Usage:
    upstream_provenance.py --fork <fork/Source> \
        --upstream 26.10=/tmp/up_26.10 26.11=/tmp/up_26.11 dev=/tmp/up_dev \
        [--csv OUT.csv]

Output: a table of files whose best upstream match is not the oldest version,
plus a summary and a CSV with every file and its three similarities.
"""
import argparse
import collections
import csv
import os
import re
import sys

SUFFIXES = ('.H', '.h', '.cpp', '.cc', '.hpp')
_INCLUDE = re.compile(r'^\s*#\s*include\b')
_COMMENT_LINE = re.compile(r'^\s*(//|/\*|\*)')
_BLOCK = re.compile(r'/\*.*?\*/', re.S)
_LINECOMMENT = re.compile(r'//[^\n]*')


def normalize(path):
    try:
        text = open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return None
    text = _BLOCK.sub('', text)
    text = _LINECOMMENT.sub('', text)
    lines = set()
    for raw in text.split('\n'):
        line = raw.strip()
        if not line or _INCLUDE.match(line) or _COMMENT_LINE.match(line):
            continue
        # collapse whitespace so that clang-format style differences do not count
        line = re.sub(r'\s+', ' ', line)
        lines.add(line)
    return lines


def index(root):
    by_name = collections.defaultdict(list)
    for dirpath, _dirnames, filenames in os.walk(root):
        for fn in filenames:
            if fn.endswith(SUFFIXES):
                by_name[fn].append(os.path.join(dirpath, fn))
    return by_name


def jaccard(a, b):
    if not a or not b:
        return 0.0
    inter = len(a & b)
    union = len(a | b)
    return inter / union if union else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--fork', required=True)
    ap.add_argument('--upstream', nargs='+', required=True,
                    help='label=dir pairs, oldest first')
    ap.add_argument('--csv', default='')
    ap.add_argument('--min-lines', type=int, default=5)
    args = ap.parse_args()

    upstream = []
    for spec in args.upstream:
        label, _, path = spec.partition('=')
        upstream.append((label, index(path)))
    labels = [label for label, _ in upstream]

    fork_index = index(args.fork)
    rows = []
    unmatched = []
    for name, paths in sorted(fork_index.items()):
        cands = [idx.get(name, []) for _, idx in upstream]
        if not all(len(c) == 1 for c in cands):
            unmatched.append((name, [len(c) for c in cands]))
            continue
        src = normalize(paths[0])
        if src is None or len(src) < args.min_lines:
            continue
        sims = []
        for (label, _idx), found in zip(upstream, cands):
            other = normalize(found[0])
            sims.append(jaccard(src, other))
        best = labels[sims.index(max(sims))]
        rows.append(dict(name=name, fork=paths[0], best=best,
                         nlines=len(src),
                         sims=dict(zip(labels, sims))))

    # Summary
    print(f'# fork files matched by unique basename: {len(rows)}; '
          f'not uniquely mappable: {len(unmatched)}')
    counts = collections.Counter(r['best'] for r in rows)
    for label in labels:
        c = counts.get(label, 0)
        print(f'#   best match {label:>6s}: {c:4d} files '
              f'({100.0*c/max(1,len(rows)):5.1f}%)')
    newest = labels[-1]
    print(f'# files that match {newest} better than {labels[0]}: '
          f'{sum(1 for r in rows if r["sims"][newest] > r["sims"][labels[0]])}')

    print()
    print(f'{"file":52s} {"n":>5s} ' +
          ' '.join(f'{lab:>7s}' for lab in labels) + '  best')
    for r in sorted(rows, key=lambda r: r['sims'][newest] - r['sims'][labels[0]],
                    reverse=True):
        if r['best'] == labels[0]:
            continue
        print(f'{r["name"][:52]:52s} {r["nlines"]:5d} ' +
              ' '.join(f'{r["sims"][lab]:7.3f}' for lab in labels) +
              f'  {r["best"]}')

    if args.csv:
        with open(args.csv, 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['basename', 'fork_path', 'nlines', 'best'] +
                       [f'sim_{lab}' for lab in labels])
            for r in rows:
                w.writerow([r['name'], r['fork'], r['nlines'], r['best']] +
                           [f'{r["sims"][lab]:.6f}' for lab in labels])
        print(f'\n# wrote {args.csv}', file=sys.stderr)


if __name__ == '__main__':
    sys.exit(main())
