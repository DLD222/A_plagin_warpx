"""The rendering pipeline, shared by the one-shot CLI and the resident worker.

Only ParaView/VTK imports live here, so this module must be imported by ParaView's
own interpreter (``pvpython``), not by the studio environment.

Two details matter for a live view:

* the openPMD reader is cached per ``.pmd`` path, but the cache is invalidated when
  the files on disk change - otherwise a growing series would freeze on the
  iteration it had when the renderer started;
* WarpX 2D data sits on an (x, z) plane with a single layer in y, so a ``Slice``
  along that degenerate axis collapses to a line.  The degenerate axis is detected
  and the plane is displayed directly, with the camera looking along it.
"""
from __future__ import annotations

from pathlib import Path


class Renderer:
    def __init__(self) -> None:
        self._cache: dict[str, tuple[tuple[int, float], object]] = {}
        self._view = None

    # -- reader cache ------------------------------------------------------
    @staticmethod
    def _signature(pmd: Path) -> tuple[int, float]:
        files = list(pmd.parent.glob("*.h5"))
        return (len(files), max((f.stat().st_mtime for f in files), default=0.0))

    def reader(self, pmd: str):
        from paraview.algorithms.openpmd import openPMDReader

        path = Path(pmd)
        signature = self._signature(path)
        cached = self._cache.get(pmd)
        if cached and cached[0] == signature:
            return cached[1]
        reader = openPMDReader()
        reader.SetFileName(pmd)
        reader.UpdateInformation()
        self._cache[pmd] = (signature, reader)
        return reader

    # -- pipeline ----------------------------------------------------------
    def _render_view(self):
        import paraview.simple as pvs

        if self._view is None:
            self._view = pvs.GetActiveViewOrCreate("RenderView")
        return self._view

    def render(self, request: dict) -> dict:
        import paraview.simple as pvs
        from vtkmodules.vtkCommonExecutionModel import (
            vtkStreamingDemandDrivenPipeline as SDDP,
        )

        pmd = request["pmd"]
        out = request["out"]
        reader = self.reader(pmd)
        if not reader._timemap:
            return {"ok": False, "error": "the series has no iterations yet"}

        times = sorted(reader._timemap)
        wanted = request.get("iteration")
        if wanted is None:
            time = times[-1]
            iteration = reader._timemap[time]
        else:
            matches = [t for t in times if reader._timemap[t] == int(wanted)]
            if not matches:
                return {"ok": False, "error": f"iteration {wanted} is not in the series "
                                              f"{[reader._timemap[t] for t in times]}"}
            time = matches[0]
            iteration = int(wanted)

        for port in (0, 1):
            reader.GetOutputInformation(port).Set(SDDP.UPDATE_TIME_STEP(), float(time))
        reader.Update()

        fields = reader.GetOutputDataObject(0)
        if fields.GetNumberOfPartitions() == 0:
            return {"ok": False, "error": "the iteration has no field blocks"}
        block = fields.GetPartition(0)
        arrays = [block.GetPointData().GetArrayName(i)
                  for i in range(block.GetPointData().GetNumberOfArrays())]
        if not arrays:
            return {"ok": False, "error": "the iteration has no field arrays"}

        field = request.get("field") if request.get("field") in arrays else arrays[0]
        component = None
        array = block.GetPointData().GetArray(field)
        if request.get("component") and array is not None and array.GetNumberOfComponents() > 1:
            wanted_component = str(request["component"]).lower()
            names = [array.GetComponentName(c) for c in range(array.GetNumberOfComponents())]
            for c, name in enumerate(names):
                if name and name.lower() == wanted_component:
                    component = name
                    break
            if component is None:
                index = {"x": 0, "y": 1, "z": 2}.get(wanted_component)
                if index is not None and index < array.GetNumberOfComponents():
                    component = index

        extent = block.GetExtent()
        degenerate = [i for i in range(3) if extent[2 * i] == extent[2 * i + 1]]
        axis = request.get("slice_axis", "z")
        view_axis = "xyz"[degenerate[0]] if degenerate else axis

        view = self._render_view()
        width, height = (int(v) for v in str(request.get("size", "900x680")).lower().split("x"))
        view.ViewSize = [width, height]
        view.Background = [0.06, 0.07, 0.1]

        producer = pvs.TrivialProducer()
        producer.GetClientSideObject().SetOutput(fields)
        source = producer
        slice_filter = None
        if not request.get("volume") and not degenerate:
            slice_filter = pvs.Slice(Input=producer)
            slice_filter.SliceType = "Plane"
            slice_filter.SliceType.Normal = {"x": [1.0, 0.0, 0.0], "y": [0.0, 1.0, 0.0],
                                             "z": [0.0, 0.0, 1.0]}[axis]
            index = {"x": (0, 1), "y": (2, 3), "z": (4, 5)}[axis]
            middle = 0.5 * (block.GetBounds()[index[0]] + block.GetBounds()[index[1]])
            position = middle if request.get("slice_at") is None else float(request["slice_at"])
            slice_filter.SliceType.Origin = [position if a == axis else 0.0 for a in "xyz"]
            source = slice_filter

        display = pvs.Show(source, view)
        display.Representation = "Volume" if request.get("volume") else "Surface"
        if component is not None:
            pvs.ColorBy(display, ("POINTS", field, component))
        else:
            pvs.ColorBy(display, ("POINTS", field))
        display.SetScalarBarVisibility(view, True)

        pdisp = None
        species = request.get("particles") or []
        if species and not request.get("volume"):
            # the field plane is co-planar with the particles and would hide them
            try:
                display.Opacity = 0.55
            except Exception:
                pass
        if species:
            particles = reader.GetOutputDataObject(1)
            if particles is not None and particles.GetNumberOfPartitionedDataSets():
                pprod = pvs.TrivialProducer()
                pprod.GetClientSideObject().SetOutput(particles)
                pdisp = pvs.Show(pprod, view)
                pdisp.Representation = "Point Gaussian"
                diagonal = sum((block.GetBounds()[2 * i + 1] - block.GetBounds()[2 * i]) ** 2
                               for i in range(3)) ** 0.5
                try:
                    pdisp.GaussianRadius = max(float(request.get("particle_radius")
                                                     or diagonal / 400.0), 1e-30)
                except Exception:
                    pass
                first = particles.GetPartitionedDataSet(0)
                if first is not None and first.GetNumberOfPartitions():
                    point_data = first.GetPartition(0).GetPointData()
                    if point_data.GetNumberOfArrays():
                        pvs.ColorBy(pdisp, ("POINTS", point_data.GetArrayName(0)))

        view.ResetCamera()
        camera = view.GetActiveCamera()
        focal = list(camera.GetFocalPoint())
        distance = camera.GetDistance()
        direction = {"x": [1.0, 0.0, 0.0], "y": [0.0, 1.0, 0.0],
                     "z": [0.0, 0.0, 1.0]}.get(view_axis, [1.0, -1.0, 1.0])
        if species and not request.get("volume"):
            # look slightly off-axis so the particles lift off the field plane
            direction = [0.35, 1.0, 0.25]
        norm = sum(c * c for c in direction) ** 0.5
        camera.SetPosition(*[f + distance * c / norm for f, c in zip(focal, direction)])
        camera.SetViewUp(0.0, 0.0, 1.0) if view_axis == "y" else camera.SetViewUp(0.0, 1.0, 0.0)
        pvs.Render()
        pvs.SaveScreenshot(out, view, ImageResolution=[width, height])

        data_range = array.GetRange(component) if (array is not None and component is not None) \
            else (array.GetRange() if array is not None else (0.0, 0.0))

        # Drop the per-frame proxies: the reader cache is what we want to keep.
        if pdisp is not None:
            pvs.Delete(pdisp)
            pvs.Delete(pprod)
        pvs.Delete(display)
        if slice_filter is not None:
            pvs.Delete(slice_filter)
        pvs.Delete(producer)

        species_available = []
        particles = reader.GetOutputDataObject(1)
        if particles is not None:
            for index in range(particles.GetNumberOfPartitionedDataSets()):
                block = particles.GetPartitionedDataSet(index)
                if block is not None and block.GetNumberOfPartitions():
                    species_available.append(block.GetPartition(0).GetClassName())
        return {"ok": True, "iteration": iteration, "time": time, "field": field,
                "species": len(species_available),
                "component": component, "range": list(data_range), "png": out,
                "arrays": arrays, "view_axis": view_axis}
