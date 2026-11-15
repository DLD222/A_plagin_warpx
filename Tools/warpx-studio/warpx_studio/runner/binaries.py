"""Find runnable WarpX binaries in this checkout and read their build features.

The fork encodes the build configuration in the executable name
(``warpx.2d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED``), so the UI can label what a binary
supports without launching it.
"""
from __future__ import annotations

import re
from pathlib import Path

FEATURES = {
    "MPI": "MPI", "OMP": "OpenMP", "OPMD": "openPMD", "FFT": "spectral (PSATD)",
    "EB": "embedded boundary", "QED": "QED", "PSATD": "PSATD", "GPU": "GPU",
}
PATTERN = re.compile(r"^warpx\.([0-9]+d|rz)((?:\.[A-Za-z0-9]+)*)$", re.I)


def describe(path: Path) -> dict:
    match = PATTERN.match(path.name)
    dims = match.group(1).lower() if match else "?"
    tags = [t for t in (match.group(2).split(".") if match else []) if t]
    features = [FEATURES.get(t.upper(), t) for t in tags]
    return {"path": str(path), "name": path.name, "dims": dims,
            "build": path.parent.parent.name, "features": features,
            "mtime": path.stat().st_mtime}


def discover(root: Path, extra_dirs: list[Path] | None = None) -> list[dict]:
    """Return every ``warpx.*`` executable under the usual build directories."""
    roots = [root / "audit_build", root / "build", root / "build_multi",
             root / "build_star", root / "build_multi_hdf5_fft",
             root / "audit_runs/studio_builds"]
    roots.extend(extra_dirs or [])
    found: dict[str, dict] = {}
    for base in roots:
        if not base.exists():
            continue
        for candidate in sorted(base.glob("*/bin/warpx.*")) + sorted(base.glob("bin/warpx.*")):
            if candidate.is_file() and candidate.stat().st_mode & 0o111:
                found[str(candidate.resolve())] = describe(candidate)
    return sorted(found.values(), key=lambda item: (item["dims"], item["name"]))
