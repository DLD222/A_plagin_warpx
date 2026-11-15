#!/usr/bin/env python3
"""Render the newest iteration of a WarpX openPMD diagnostic to a PNG.

Run with ParaView's own interpreter (the openPMD reader is a ParaView Python
algorithm and needs ``openpmd_api``)::

    <paraview env>/bin/pvpython render_openpmd.py \
        --pmd diag1/paraview.pmd --out frame.png [--field rho] [--iteration 40]

It is deliberately a separate process: the web server lives in the studio conda
environment, ParaView lives in its own, and a crash in a renderer must never take
the server (or the simulation) down with it.
"""
from __future__ import annotations

import argparse
import sys


def main(argv=None) -> int:
    import paraview.simple as pvs
    from paraview.algorithms.openpmd import openPMDReader
    from vtkmodules.vtkCommonExecutionModel import (
        vtkStreamingDemandDrivenPipeline as SDDP,
    )

    ap = argparse.ArgumentParser()
    ap.add_argument("--pmd", required=True, help="path to paraview.pmd")
    ap.add_argument("--out", required=True, help="PNG to write")
    ap.add_argument("--field", default=None, help="mesh to show (default: first available)")
    ap.add_argument("--component", default=None, help="x/y/z/magnitude for vector fields")
    ap.add_argument("--iteration", type=int, default=None, help="iteration to render (default: newest)")
    ap.add_argument("--slice-axis", default="z", choices=["x", "y", "z"])
    ap.add_argument("--slice-at", type=float, default=None, help="slice position (default: middle)")
    ap.add_argument("--size", default="960x720", help="WxH")
    ap.add_argument("--volume", action="store_true", help="volume rendering instead of a slice")
    args = ap.parse_args(argv)

    reader = openPMDReader()
    reader.SetFileName(args.pmd)
    reader.UpdateInformation()
    if not reader._timemap:
        print("no iterations in the series yet", file=sys.stderr)
        return 2

    times = sorted(reader._timemap)
    if args.iteration is None:
        time = times[-1]
        iteration = reader._timemap[time]
    else:
        matches = [t for t in times if reader._timemap[t] == args.iteration]
        if not matches:
            print(f"iteration {args.iteration} not in {[reader._timemap[t] for t in times]}",
                  file=sys.stderr)
            return 2
        time = matches[0]
        iteration = args.iteration

    for port in (0, 1):
        reader.GetOutputInformation(port).Set(SDDP.UPDATE_TIME_STEP(), float(time))
    reader.Update()

    fields = reader.GetOutputDataObject(0)
    producer = pvs.TrivialProducer()
    producer.GetClientSideObject().SetOutput(fields)

    arrays = []
    if fields.GetNumberOfPartitions() > 0:
        point_data = fields.GetPartition(0).GetPointData()
        arrays = [point_data.GetArrayName(i) for i in range(point_data.GetNumberOfArrays())]
    if not arrays:
        print("the diagnostic has no field arrays yet", file=sys.stderr)
        return 2
    field = args.field if args.field in arrays else arrays[0]

    view = pvs.GetActiveViewOrCreate("RenderView")
    width, height = (int(v) for v in args.size.lower().split("x"))
    view.ViewSize = [width, height]
    view.Background = [0.07, 0.07, 0.1]

    # WarpX 2D data lives on an (x, z) plane: the y extent is a single layer, so slicing
    # along y (or using the plane itself as a slice) degenerates to a line.  Detect the
    # degenerate axis and display the block directly, looking along that axis.
    extent = fields.GetPartition(0).GetExtent() if fields.GetNumberOfPartitions() else (0, 0, 0, 0, 0, 0)
    degenerate = [i for i in range(3) if extent[2 * i] == extent[2 * i + 1]]
    view_axis = "xyz"[degenerate[0]] if degenerate else args.slice_axis

    source = producer
    if not args.volume and not degenerate:
        source = pvs.Slice(Input=producer)
        source.SliceType = "Plane"
        normal = {"x": (1.0, 0.0, 0.0), "y": (0.0, 1.0, 0.0), "z": (0.0, 0.0, 1.0)}[args.slice_axis]
        source.SliceType.Normal = list(normal)
        middle = 0.0
        if fields.GetNumberOfPartitions() > 0:
            bounds = fields.GetPartition(0).GetBounds()
            axis_index = {"x": (0, 1), "y": (2, 3), "z": (4, 5)}[args.slice_axis]
            middle = 0.5 * (bounds[axis_index[0]] + bounds[axis_index[1]])
        position = middle if args.slice_at is None else args.slice_at
        source.SliceType.Origin = [position if a == args.slice_axis else 0.0 for a in "xyz"]

    display = pvs.Show(source, view)
    display.Representation = "Volume" if args.volume else "Surface"

    array = fields.GetPartition(0).GetPointData().GetArray(field)
    component = None
    if args.component and array is not None and array.GetNumberOfComponents() > 1:
        names = [array.GetComponentName(c) for c in range(array.GetNumberOfComponents())]
        for c, name in enumerate(names):
            if name and name.lower() == args.component.lower():
                component = name
                break
        if component is None:
            index = {"x": 0, "y": 1, "z": 2}.get(args.component.lower())
            if index is not None and index < array.GetNumberOfComponents():
                component = index
    pvs.ColorBy(display, ("POINTS", field, component) if component is not None
                else ("POINTS", field))
    display.SetScalarBarVisibility(view, True)

    view.ResetCamera()
    camera = view.GetActiveCamera()
    focal = list(camera.GetFocalPoint())
    distance = camera.GetDistance()
    direction = {"x": [1.0, 0.0, 0.0], "y": [0.0, 1.0, 0.0],
                 "z": [0.0, 0.0, 1.0], "iso": [1.0, -1.0, 1.0]}[
                     view_axis if not args.volume else "iso"]
    norm = sum(c * c for c in direction) ** 0.5
    camera.SetPosition(*[f + distance * c / norm for f, c in zip(focal, direction)])
    # keep the in-plane axis vertical: x-z plane -> z up, x-y plane -> y up
    camera.SetViewUp(0.0, 0.0, 1.0) if view_axis == "y" else camera.SetViewUp(0.0, 1.0, 0.0)
    pvs.Render()
    pvs.SaveScreenshot(args.out, view, ImageResolution=[width, height])
    data_range = array.GetRange(component) if (array and component is not None) else \
        (array.GetRange() if array else (0.0, 0.0))
    print(f"rendered iteration={iteration} time={time:.6g} field={field} "
          f"component={component} range={data_range[0]:.6g},{data_range[1]:.6g} -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
