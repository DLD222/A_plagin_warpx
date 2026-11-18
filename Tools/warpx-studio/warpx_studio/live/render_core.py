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
            # The orientation axes widget is a second set of coordinates drawn on top of the
            # picture; a 2D slice only needs one colour scale and one frame.
            try:
                self._view.OrientationAxesVisibility = 0
                self._view.CenterAxesVisibility = 0
            except Exception:
                pass
        return self._view

    def render(self, request: dict) -> dict:
        import paraview.simple as pvs
        from vtkmodules.vtkCommonExecutionModel import (
            vtkStreamingDemandDrivenPipeline as SDDP,
        )

        pmd = request["pmd"]
        # `out` is not needed in listing mode, so it is fetched lazily below.
        out = request.get("out")
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

        # Listing mode: report exactly what the file contains.  The web UI builds its
        # quantity menu from this instead of assuming that every field has x/y/z
        # components (charge density rho is a scalar and has none).
        if request.get("list"):
            catalogue = []
            for name in arrays:
                arr = block.GetPointData().GetArray(name)
                ncomp = arr.GetNumberOfComponents() if arr is not None else 1
                comps = []
                if ncomp > 1:
                    for c in range(ncomp):
                        cname = arr.GetComponentName(c) if arr is not None else None
                        comps.append({"index": c, "name": cname or "xyz"[c] if c < 3 else str(c),
                                      "axis": "xyz"[c] if c < 3 else str(c)})
                catalogue.append({"name": name, "ncomp": ncomp, "components": comps})
            return {"ok": True, "iteration": iteration, "quantities": catalogue}

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
        # 颜色连续：在映射到色标之前先做插值，否则网格级色块会肉眼可见
        try:
            display.InterpolateScalarsBeforeMapping = 1
            display.UseLookupTableScalarRange = 1
        except Exception:
            pass
        display.SetScalarBarVisibility(view, True)

        # Label the bar with the quantity actually shown.  When a component is selected by
        # index ParaView keeps the vector's name and titles the bar "<field>Magnitude",
        # which is wrong (and misleading when comparing saved pictures).
        try:
            lut = pvs.GetColorTransferFunction(field)
            bar = pvs.GetScalarBar(lut, view)
            # ParaView appends "Magnitude" as the component title even when a single
            # component is shown; clear it so the bar reads "B_y" and not "B_y Magnitude".
            try:
                bar.ComponentTitle = ""
            except Exception:
                pass
            if component is None:
                bar.Title = field
            elif isinstance(component, str):
                bar.Title = f"{field}_{component}"
            else:
                axis = {0: "x", 1: "y", 2: "z"}.get(int(component), str(component))
                bar.Title = f"{field}_{axis}"
            # One colour -> one number, with one consistent format.  ParaView's default
            # produced labels like "7.2e+00" next to "7.15" and "7.148" on the same bar,
            # i.e. the same colour appeared to carry several values.  Fix the format and
            # the number of ticks so the scale is monotone and unambiguous.
            bar.AddRangeLabels = 1
            bar.RangeLabelFormat = "%-6.3g"
            try:
                bar.MaximumNumberOfLabels = 5
            except Exception:
                pass
            try:
                lut_numbers = 5
                lut.SetNumberOfTableValues(256)
                lut_numbers = 0
            except Exception:
                pass
        except Exception:
            pass

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
                try:
                    pdisp.GaussianRadius = max(float(request.get("particle_radius") or 0.0), 1e-30)
                    pdisp.Opacity = 0.35
                except Exception:
                    pass
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
        # Camera placement decides whether +x points right on screen.  With the camera on
        # the +y side and up = +z the screen-right axis is -x, i.e. the picture comes out
        # mirrored: the flow then appears to run into the obstacle from the wrong side and
        # the bow shock looks reversed.  Look from -y instead so +x is to the right.
        direction = {"x": [1.0, 0.0, 0.0], "y": [0.0, -1.0, 0.0],
                     "z": [0.0, 0.0, 1.0]}.get(view_axis, [1.0, -1.0, 1.0])
        # 二维就是一张平面：始终正视，不为了粒子做斜视（斜视会让用户以为图像被旋转了）

        # Interactive camera: the web page sends the orbit angles / zoom / pan it wants,
        # so rotation and zooming happen in the browser instead of a separate GUI window.
        cam = request.get("camera") or {}
        azimuth = float(cam.get("azimuth", 0.0))     # degrees around the view-up axis
        elevation = float(cam.get("elevation", 0.0))  # degrees towards the view-up axis
        zoom = float(cam.get("zoom", 1.0))           # >1 zooms in
        pan = list(cam.get("pan") or [0.0, 0.0])      # fraction of the view height (x, z)
        while len(pan) < 3:
            pan.append(0.0)

        norm = sum(c * c for c in direction) ** 0.5
        dir_unit = [c / norm for c in direction]
        if azimuth or elevation:
            import math
            up = [0.0, 1.0, 0.0] if view_axis != "y" else [0.0, 0.0, 1.0]
            # rotate dir_unit around `up` by azimuth, then tilt by elevation
            ax, ay, az = up
            ca, sa = math.cos(math.radians(azimuth)), math.sin(math.radians(azimuth))
            dot = dir_unit[0]*ax + dir_unit[1]*ay + dir_unit[2]*az
            cross = [dir_unit[1]*az - dir_unit[2]*ay,
                     dir_unit[2]*ax - dir_unit[0]*az,
                     dir_unit[0]*ay - dir_unit[1]*ax]
            rotated = [dir_unit[i]*ca + cross[i]*sa + ax*dot*(1-ca) if i == 0 else
                       dir_unit[i]*ca + cross[i]*sa + ay*dot*(1-ca) if i == 1 else
                       dir_unit[i]*ca + cross[i]*sa + az*dot*(1-ca) for i in range(3)]
            ce, se = math.cos(math.radians(elevation)), math.sin(math.radians(elevation))
            # tilt towards `up`
            dir_unit = [rotated[i]*ce + up[i]*se for i in range(3)]
            n2 = sum(c * c for c in dir_unit) ** 0.5 or 1.0
            dir_unit = [c / n2 for c in dir_unit]

        distance = distance / max(zoom, 1e-3)
        # pan in the view plane: horizontal -> x, vertical -> z for the x-z plane views
        pan_offset = [pan[0] * distance * 0.5, 0.0, pan[1] * distance * 0.5]
        focal = [focal[i] + pan_offset[i] for i in range(3)]
        camera.SetPosition(*[f + distance * c for f, c in zip(focal, dir_unit)])
        camera.SetFocalPoint(*focal)
        camera.SetViewUp(0.0, 0.0, 1.0) if view_axis == "y" else camera.SetViewUp(0.0, 1.0, 0.0)
        pvs.Render()
        pvs.SaveScreenshot(out, view, ImageResolution=[width, height])

        data_range = array.GetRange(component) if (array is not None and component is not None) \
            else (array.GetRange() if array is not None else (0.0, 0.0))

        # 色标必须显式移除：只 Delete(display) 会把色标留在视图里，
        # 连续切换物理量就会出现好几个色标叠在一起（用户看到的三条色带）。
        try:
            display.SetScalarBarVisibility(view, False)
            bar = display.GetScalarBarActor(view)
            if bar is not None:
                view.RemoveScalarBarActor(bar)
        except Exception:
            pass
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
                "arrays": arrays, "view_axis": view_axis,
                "camera": {"azimuth": cam.get("azimuth", 0.0),
                           "elevation": cam.get("elevation", 0.0),
                           "zoom": cam.get("zoom", 1.0),
                           "pan": cam.get("pan", [0.0, 0.0])}}
