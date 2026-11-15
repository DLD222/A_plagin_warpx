#!/usr/bin/env python3
"""Export a WarpX openPMD diagnostic to VTK files plus a ``.pvd`` collection.

``.pvd`` is the one format ParaView opens with a working time slider, so this is what
the "open in ParaView" button hands over: fields become ``.vti`` (image data) or
``.vtu`` (particles), and the collection lists them per iteration.

    <paraview env>/bin/pvpython export_vtk.py --pmd diag1/paraview.pmd --out vtk/
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))


def write_object(obj, basepath: Path) -> str:
    from vtkmodules.vtkIOXML import vtkXMLImageDataWriter, vtkXMLUnstructuredGridWriter

    if obj.IsA("vtkImageData"):
        path = basepath.with_suffix(".vti")
        writer = vtkXMLImageDataWriter()
    else:
        path = basepath.with_suffix(".vtu")
        writer = vtkXMLUnstructuredGridWriter()
    writer.SetFileName(str(path))
    writer.SetInputData(obj)
    try:
        writer.SetCompressionLevel(5)
    except Exception:
        pass
    writer.Write()
    return path.name


def write_pvd(path: Path, entries: list[tuple[float, int, str]]) -> None:
    lines = ['<?xml version="1.0"?>', '<VTKFile type="Collection" version="0.1">',
             "  <Collection>"]
    for time, part, name in entries:
        lines.append(f'    <DataSet timestep="{time:.9g}" group="" part="{part}" file="{name}"/>')
    lines += ["  </Collection>", "</VTKFile>"]
    path.write_text("\n".join(lines) + "\n")


def main(argv=None) -> int:
    from vtkmodules.vtkCommonExecutionModel import (
        vtkStreamingDemandDrivenPipeline as SDDP,
    )

    from warpx_studio.live.render_core import Renderer

    ap = argparse.ArgumentParser()
    ap.add_argument("--pmd", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--max-iterations", type=int, default=0,
                    help="only the newest N iterations (0 = all)")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    reader = Renderer().reader(args.pmd)
    if not reader._timemap:
        print("the series has no iterations", file=sys.stderr)
        return 2

    times = sorted(reader._timemap)
    if args.max_iterations:
        times = times[-args.max_iterations:]

    entries: list[tuple[float, int, str]] = []
    stem = Path(args.pmd).parent.name or "openpmd"
    for time in times:
        iteration = reader._timemap[time]
        for port in (0, 1):
            reader.GetOutputInformation(port).Set(SDDP.UPDATE_TIME_STEP(), float(time))
        reader.Update()
        fields = reader.GetOutputDataObject(0)
        count = fields.GetNumberOfPartitions() if fields is not None else 0
        for k in range(count):
            name = write_object(fields.GetPartition(k),
                                out / f"{stem}_{iteration:08d}_fields{k}")
            entries.append((time, k, name))

    pvd = out / f"{stem}.pvd"
    write_pvd(pvd, entries)
    summary = {"ok": True, "pvd": str(pvd), "iterations": len(times), "files": len(entries)}
    if args.json:
        print(json.dumps(summary))
    else:
        print(f"exported {len(entries)} file(s) for {len(times)} iteration(s) -> {pvd}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
