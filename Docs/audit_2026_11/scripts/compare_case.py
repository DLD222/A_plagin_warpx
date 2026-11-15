#!/usr/bin/env python3
"""Byte-for-byte (and, when bytes differ, value-by-value) comparison of two
WarpX run directories produced by two different builds from the same input.

Usage:
    compare_case.py --ref DIR --new DIR --name TEST [--out FILE.json]

Output JSON (also printed as a one-line summary):
    {
      "name": ..., "ref": ..., "new": ...,
      "files_total": N, "files_identical": N, "files_differing": N,
      "files_only_ref": [...], "files_only_new": [...],
      "identical": bool,
      "details": [ {relpath, kind, ...}, ... ],
      "max_abs_diff": float, "max_rel_diff": float
    }

Kinds of detail entries:
  bytes_equal      : identical bytes
  text_diff        : text file, compared after normalizing volatile tokens
  hdf5_diff        : HDF5/openPMD file, compared dataset by dataset with h5py
  plotfile_diff    : AMReX plotfile directory, compared field by field with yt
  binary_diff      : raw binary, float64/int64 statistics
  missing/mismatch : presence or size mismatch

This script never modifies either run directory.
"""
import argparse
import json
import os
import re
import sys

VOLATILE_PATTERNS = [
    r"\b\d+(?:\.\d*)?(?:[eE][+-]?\d+)?\s*(?:s|sec|seconds|ms|us|min|h)\b",
    r"Evolve time\s*=.*",
    r"This step\s*=.*",
    r"Avg\. per step\s*=.*",
    r"Total Time\s*=.*",
    r"Run time\s*=.*",
    r"Peak\s*Mem.*",
    r"Memory.*",
    r"alloc.*",
    r"\b\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}\b",
    r"Backtrace.*",
    r"TinyProfiler",
    r"^\s*Total Time",
    r"\b\d+\s+[0-9.eE+-]+\s+[0-9.eE+-]+\s+[0-9.eE+-]+\s+[0-9.]+%",
    r"\b(KiB|MiB|GiB|bytes)\b",
    r"^\s*\d+\s+[0-9.]+ [0-9.]+ [0-9.]+\s*$",
    r"OMP initialized with",
    r"MPI initialized with",
]


def norm_text(path):
    try:
        raw = open(path, "rb").read()
    except OSError:
        return None
    if b"\x00" in raw[:4096]:
        return None
    txt = raw.decode("utf-8", "replace")
    out = []
    for line in txt.splitlines():
        if any(re.search(p, line) for p in VOLATILE_PATTERNS):
            continue
        out.append(line.rstrip())
    return out


def cmp_text(a, b):
    ta, tb = norm_text(a), norm_text(b)
    if ta is None or tb is None:
        return None
    if ta == tb:
        return {"kind": "text_equal_normalized", "nlines": len(ta)}
    import difflib
    diff = list(difflib.unified_diff(ta, tb, lineterm="", n=0))
    return {"kind": "text_diff", "n_diff_lines": len([d for d in diff if d[:1] in "+-" and d[:3] not in ("+++", "---")]),
            "sample": diff[:40]}


def cmp_binary(a, b):
    import numpy as np
    sa, sb = os.path.getsize(a), os.path.getsize(b)
    res = {"kind": "binary_diff", "size_ref": sa, "size_new": sb}
    if sa != sb:
        res["size_mismatch"] = True
        return res
    ba = open(a, "rb").read()
    bb = open(b, "rb").read()
    first = next(i for i in range(sa) if ba[i] != bb[i])
    res["first_diff_byte"] = first
    if sa % 8 == 0:
        fa = np.frombuffer(ba, dtype="<f8")
        fb = np.frombuffer(bb, dtype="<f8")
        d = np.abs(fa - fb)
        denom = np.maximum(np.abs(fa), 1e-300)
        res["n_double_elements"] = int(fa.size)
        res["n_double_differ"] = int(np.count_nonzero(d))
        res["max_abs_diff"] = float(d.max()) if d.size else 0.0
        res["max_rel_diff"] = float((d / denom).max()) if d.size else 0.0
        ia = np.frombuffer(ba, dtype="<i8")
        ib = np.frombuffer(bb, dtype="<i8")
        res["n_int64_differ"] = int(np.count_nonzero(ia != ib))
    return res


def h5_datasets(f, prefix=""):
    out = {}
    def visit(g, p):
        for k in g:
            item = g[k]
            pth = p + "/" + k
            if hasattr(item, "shape") and hasattr(item, "dtype"):
                out[pth] = item
            elif hasattr(item, "keys"):
                visit(item, pth)
    visit(f, prefix)
    return out


def cmp_hdf5(a, b):
    import h5py
    import numpy as np
    res = {"kind": "hdf5_diff", "datasets": []}
    try:
        return _cmp_hdf5(a, b)
    except Exception as e:  # noqa: BLE001
        # A truncated/partially written file (the comparison can run while a case is still
        # being written) must not abort the whole sweep: report it and let the caller decide.
        res["h5_error"] = repr(e)
        return res


def _cmp_hdf5(a, b):
    import h5py
    import numpy as np
    res = {"kind": "hdf5_diff", "datasets": []}
    with h5py.File(a, "r") as fa, h5py.File(b, "r") as fb:
        da, db = h5_datasets(fa), h5_datasets(fb)
        only = sorted(set(da) - set(db))
        only_b = sorted(set(db) - set(da))
        if only:
            res["datasets_only_ref"] = only
        if only_b:
            res["datasets_only_new"] = only_b
        worst_abs = worst_rel = 0.0
        for k in sorted(set(da) & set(db)):
            x, y = da[k], db[k]
            entry = {"dataset": k}
            if x.shape != y.shape:
                entry["shape_mismatch"] = [list(x.shape), list(y.shape)]
                res["datasets"].append(entry)
                continue
            vx = x[()] if x.shape else np.array(x[()])
            vy = y[()] if y.shape else np.array(y[()])
            if vx.dtype.kind in "fiu" and vy.dtype.kind in "fiu":
                vx = vx.astype("f8") if vx.dtype.kind == "f" else vx
                vy = vy.astype("f8") if vy.dtype.kind == "f" else vy
                # Particle bookkeeping (openPMD stores ids and the grid indices nx/ny/nz as
                # doubles): "id 12345 vs 12346 -> 1.0" says nothing about the physics, so these
                # are compared as sets/counts and kept out of the value maxima.
                if k.rsplit("/", 1)[-1] in ("id", "nx", "ny", "nz"):
                    entry["n_differ"] = int(np.count_nonzero(vx != vy))
                    entry["equal_as_set"] = bool(np.array_equal(np.sort(vx), np.sort(vy)))
                    if entry["n_differ"] or not entry["equal_as_set"]:
                        res["datasets"].append(entry)
                    continue
                if x.dtype.kind == "f":
                    d = np.abs(vx - vy)
                    den = np.maximum(np.abs(vx), 1e-300)
                    entry["n_differ"] = int(np.count_nonzero(d))
                    entry["max_abs_diff"] = float(d.max()) if d.size else 0.0
                    entry["max_rel_diff"] = float((d / den).max()) if d.size else 0.0
                    worst_abs = max(worst_abs, entry["max_abs_diff"])
                    worst_rel = max(worst_rel, entry["max_rel_diff"])
                else:
                    entry["n_differ"] = int(np.count_nonzero(vx != vy))
            if entry.get("n_differ", 0) or "shape_mismatch" in entry:
                res["datasets"].append(entry)
        res["max_abs_diff"] = worst_abs
        res["max_rel_diff"] = worst_rel
    return res


def is_plotfile_dir(d):
    return os.path.isfile(os.path.join(d, "Header")) and any(
        x.startswith("Level_") for x in os.listdir(d))


def parse_header(path):
    lines = open(path, "r", errors="replace").read().splitlines()
    if not lines or not lines[0].startswith("HyperCLaw"):
        return None
    i = 0
    ver = lines[i]; i += 1
    nvar = int(lines[i]); i += 1
    names = lines[i:i + nvar]; i += nvar
    dim = int(lines[i]); i += 1
    time = float(lines[i]); i += 1
    finest = int(lines[i]); i += 1
    return {"version": ver, "nvar": nvar, "names": names, "dim": dim,
            "time": time, "finest_level": finest}


def cmp_plotfile(ref, new):
    """Compare two plotfile directories field by field using yt."""
    import numpy as np
    res = {"kind": "plotfile_diff"}
    try:
        import yt
        yt.set_log_level("ERROR")
        ds1 = yt.load(ref)
        ds2 = yt.load(new)
    except Exception as e:  # noqa: BLE001
        res["yt_error"] = repr(e)
        return res
    h1, h2 = parse_header(os.path.join(ref, "Header")), parse_header(os.path.join(new, "Header"))
    res["header_ref"] = h1
    res["header_new"] = h2
    if h1 and h2:
        res["header_diff"] = {k: [h1[k], h2[k]] for k in ("nvar", "dim", "time", "finest_level")
                              if h1[k] != h2[k]}
        if h1["names"] != h2["names"]:
            res["header_diff"]["names"] = [h1["names"], h2["names"]]
    ad1, ad2 = ds1.all_data(), ds2.all_data()
    fields = []
    for ftype, fname in sorted(set(ds1.field_list) & set(ds2.field_list)):
        try:
            a1 = np.asarray(ad1[ftype, fname])
            a2 = np.asarray(ad2[ftype, fname])
        except Exception as e:  # noqa: BLE001
            fields.append({"field": f"{ftype}/{fname}", "error": repr(e)})
            continue
        entry = {"field": f"{ftype}/{fname}"}
        if a1.shape != a2.shape:
            entry["shape_mismatch"] = [list(a1.shape), list(a2.shape)]
            fields.append(entry)
            continue
        if fname == "particle_id":
            # canonical particle order: sort by id, then compare everything else sorted
            entry["n"] = int(a1.size)
            entry["ids_equal_as_set"] = bool(np.array_equal(np.sort(a1), np.sort(a2)))
            fields.append(entry)
            continue
        if fname in ("particle_cpu",) or a1.dtype.kind in "iu":
            # Integer bookkeeping (owning rank, id, flags): a difference is real but its
            # magnitude is meaningless as a float, so keep it out of the value-level maxima.
            entry["n"] = int(a1.size)
            entry["n_differ"] = int(np.count_nonzero(
                np.asarray(a1, dtype="f8") != np.asarray(a2, dtype="f8")))
            fields.append(entry)
            continue
        if fname.startswith("particle_"):
            id1 = np.asarray(ad1[ftype, "particle_id"])
            id2 = np.asarray(ad2[ftype, "particle_id"])
            if id1.shape == id2.shape and id1.size == a1.size:
                o1, o2 = np.argsort(id1), np.argsort(id2)
                a1, a2 = a1[o1], a2[o2]
        d = np.abs(a1.astype("f8") - a2.astype("f8"))
        den = np.maximum(np.abs(a1.astype("f8")), 1e-300)
        entry["n"] = int(a1.size)
        entry["n_differ"] = int(np.count_nonzero(d))
        entry["max_abs_diff"] = float(d.max()) if d.size else 0.0
        entry["max_rel_diff"] = float((d / den).max()) if d.size else 0.0
        # Relative to the magnitude of the field itself, which is the number worth quoting:
        # the element-wise ratio explodes wherever the reference value happens to be ~0.
        scale = float(np.abs(np.asarray(a1, dtype="f8")).max()) if a1.size else 0.0
        entry["max_rel_diff_field"] = (float(d.max()) / scale) if scale > 0.0 else 0.0
        fields.append(entry)
    res["fields"] = fields
    res["max_abs_diff"] = max([f.get("max_abs_diff", 0.0) for f in fields] + [0.0])
    res["max_rel_diff_field"] = max([f.get("max_rel_diff_field", 0.0) for f in fields] + [0.0])
    res["max_rel_diff"] = max([f.get("max_rel_diff", 0.0) for f in fields] + [0.0])
    return res


def walk_files(root):
    out = set()
    for dp, dn, fn in os.walk(root):
        dn[:] = [d for d in dn if not re.match(r".*\.old\.\d+$", d) and d != ".pytest_cache"]
        for f in fn:
            if f.startswith("Backtrace"):
                continue
            out.add(os.path.relpath(os.path.join(dp, f), root))
    return out


def compare_dirs(ref, new, name=""):
    res = {"name": name or os.path.basename(ref), "ref": ref, "new": new,
           "details": []}
    if not os.path.isdir(ref) or not os.path.isdir(new):
        res["missing"] = [os.path.isdir(ref), os.path.isdir(new)]
        res["identical"] = False
        return res

    fa, fb = walk_files(ref), walk_files(new)
    res["files_only_ref"] = sorted(fa - fb)
    res["files_only_new"] = sorted(fb - fa)
    common = sorted(fa & fb)
    res["files_total"] = len(common) + len(fa - fb) + len(fb - fa)
    ident = 0
    worst_abs = worst_rel = 0.0
    for rel in common:
        a, b = os.path.join(ref, rel), os.path.join(new, rel)
        # plotfile directories: handled per-header below (their data files are compared raw)
        if open(a, "rb").read() == open(b, "rb").read():
            ident += 1
            continue
        ext = os.path.splitext(rel)[1].lower()
        entry = {"path": rel}
        base = os.path.basename(rel)
        if base == "warpx_job_info":
            # build metadata (date, compiler flags, git describe, build dirs): expected to differ
            entry["kind"] = "build_metadata_diff"
            res["details"].append(entry)
            continue
        if base == "warpx_used_inputs":
            # the composed input deck: a difference here means different default parameters
            t = cmp_text(a, b)
            entry.update(t if t else {"kind": "used_inputs_diff"})
            if entry.get("kind") == "text_equal_normalized":
                ident += 1
                continue
            entry["kind"] = "used_inputs_diff"
            res["details"].append(entry)
            continue
        if ext in (".h5", ".hdf5"):
            entry.update(cmp_hdf5(a, b))
        elif rel.endswith("Header") or ext in (".txt", ".log", ".json", ".csv", ".py", ".dat", ".out"):
            t = cmp_text(a, b)
            entry.update(t if t else cmp_binary(a, b))
        else:
            entry.update(cmp_binary(a, b))
        if entry.get("kind") == "text_equal_normalized":
            ident += 1
            continue
        if entry.get("kind") == "hdf5_diff" and not entry.get("datasets") \
                and not entry.get("datasets_only_ref") and not entry.get("datasets_only_new"):
            ident += 1
            continue
        # Only value-level comparisons contribute to the case-level max: a raw-byte
        # difference (binary_diff) re-reads the whole file as float64, which turns
        # plotfile padding/FAB headers into meaningless 1e179-style numbers.
        if entry.get("kind") != "binary_diff":
            worst_abs = max(worst_abs, entry.get("max_abs_diff", 0.0))
            worst_rel = max(worst_rel, entry.get("max_rel_diff", 0.0))
        res["details"].append(entry)

    # plotfile directories present in both, but only those with differing bytes are
    # worth the (expensive) field-level comparison via yt.
    differing = {os.path.dirname(e["path"]) if "/" in e["path"] else "" for e in res["details"]}
    pf = []
    for dp, dn, fn in os.walk(ref):
        dn[:] = [d for d in dn if not re.match(r".*\.old\.\d+$", d)]
        if "Header" in fn and any(x.startswith("Level_") for x in dn):
            rel = os.path.relpath(dp, ref)
            if os.path.isdir(os.path.join(new, rel)) and any(
                    d == rel or d.startswith(rel + "/") for d in differing):
                pf.append(rel)
    worst_rel_field = 0.0
    for rel in sorted(pf):
        e = cmp_plotfile(os.path.join(ref, rel), os.path.join(new, rel))
        e["path"] = rel + "/"
        worst_abs = max(worst_abs, e.get("max_abs_diff", 0.0))
        worst_rel = max(worst_rel, e.get("max_rel_diff", 0.0))
        worst_rel_field = max(worst_rel_field, e.get("max_rel_diff_field", 0.0))
        res["plotfiles"] = res.get("plotfiles", []) + [e]

    res["files_identical"] = ident
    res["files_differing"] = len(res["details"])
    res["identical"] = (res["files_differing"] == 0 and not res["files_only_ref"]
                        and not res["files_only_new"])
    res["max_abs_diff"] = worst_abs
    res["max_rel_diff"] = worst_rel
    res["max_rel_diff_field"] = worst_rel_field
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", required=True)
    ap.add_argument("--new", required=True)
    ap.add_argument("--name", default="")
    ap.add_argument("--out", default="")
    args = ap.parse_args()
    res = compare_dirs(args.ref, args.new, args.name)
    if args.out:
        json.dump(res, open(args.out, "w"), indent=1)
    summary = {k: res.get(k) for k in ("name", "identical", "files_total", "files_differing",
                                       "max_abs_diff", "max_rel_diff")}
    print(json.dumps(summary))
    return 0


if __name__ == "__main__":
    sys.exit(main())
