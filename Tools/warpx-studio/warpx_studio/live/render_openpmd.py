#!/usr/bin/env python3
"""One-shot renderer (used for debugging and for the "open in ParaView" hand-off).

    <paraview env>/bin/pvpython render_openpmd.py --pmd diag1/paraview.pmd \
        --out frame.png [--field rho] [--component z] [--iteration 40]
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))


def main(argv=None) -> int:
    from warpx_studio.live.render_core import Renderer

    ap = argparse.ArgumentParser()
    ap.add_argument("--pmd", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--field")
    ap.add_argument("--component")
    ap.add_argument("--iteration", type=int)
    ap.add_argument("--slice-axis", default="z")
    ap.add_argument("--size", default="960x720")
    ap.add_argument("--volume", action="store_true")
    ap.add_argument("--json", action="store_true", help="print the result as JSON")
    args = ap.parse_args(argv)

    result = Renderer().render({"pmd": args.pmd, "out": args.out, "field": args.field,
                                "component": args.component, "iteration": args.iteration,
                                "slice_axis": args.slice_axis, "size": args.size,
                                "volume": args.volume})
    if args.json:
        print(json.dumps(result))
    elif result.get("ok"):
        print(f"rendered iteration={result['iteration']} time={result['time']:.6g} "
              f"field={result['field']} component={result['component']} "
              f"range={result['range'][0]:.6g},{result['range'][1]:.6g} -> {args.out}")
    else:
        print(f"render failed: {result.get('error')}", file=sys.stderr)
    return 0 if result.get("ok") else 2


if __name__ == "__main__":
    sys.exit(main())
