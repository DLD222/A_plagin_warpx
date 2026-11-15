"""Keep one PNG in sync with the newest iteration a running simulation has written.

WarpX flushes ``<diag>/openpmd_%06T.h5`` every ``intervals`` steps and keeps
``<diag>/paraview.pmd`` next to it, so the newest iteration is the highest number in
those file names.  Rendering is delegated to ``pvpython`` in the ParaView
environment: a renderer crash or a ParaView upgrade cannot take the server down.

The clock is the file system, not a timer: a frame is only re-rendered when a new
iteration actually appeared.
"""
from __future__ import annotations

import os
import re
import subprocess
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
RENDER = HERE / "render_openpmd.py"
ITERATION = re.compile(r"openpmd_(\d+)\.h5$")


def find_diag_dir(run_dir: Path) -> Path | None:
    """The directory WarpX wrote ``paraview.pmd`` into (``<run>/diag1`` for a diag named diag1)."""
    if (run_dir / "paraview.pmd").exists():
        return run_dir
    for candidate in sorted(run_dir.glob("*/paraview.pmd")):
        return candidate.parent
    return None


def newest_iteration(diag_dir: Path) -> int | None:
    if diag_dir is None or not diag_dir.is_dir():
        return None
    numbers = [int(m.group(1)) for p in diag_dir.iterdir()
               if (m := ITERATION.search(p.name))]
    return max(numbers) if numbers else None


class LiveView:
    def __init__(self, run_dir: Path, out_png: Path, field: str | None = None,
                 component: str | None = None, pvpython: str | None = None,
                 interval: float = 1.0) -> None:
        self.run_dir = run_dir
        self.out_png = out_png
        self.field = field
        self.component = component
        self.pvpython = pvpython or os.environ.get(
            "WARPS_STUDIO_PVPYTHON", os.path.expanduser("~/miniconda3/envs/paraview/bin/pvpython"))
        self.interval = interval
        self.diag_dir: Path | None = None
        self.iteration: int | None = None
        self.meta: dict = {}
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    def _loop(self) -> None:
        while not self._stop.is_set():
            try:
                self.diag_dir = find_diag_dir(self.run_dir)
                iteration = newest_iteration(self.diag_dir)
                if iteration is not None and iteration != self.iteration:
                    self.render(iteration)
            except Exception as exc:                       # never kill the server
                self.meta = {"error": f"{type(exc).__name__}: {exc}"}
            self._stop.wait(self.interval)

    def render(self, iteration: int | None = None) -> dict:
        self.diag_dir = find_diag_dir(self.run_dir)
        pmd = self.diag_dir / "paraview.pmd" if self.diag_dir else self.run_dir / "paraview.pmd"
        if not pmd.exists():
            self.meta = {"error": "no paraview.pmd yet"}
            return self.meta
        command = [self.pvpython, str(RENDER), "--pmd", str(pmd),
                   "--out", str(self.out_png)]
        if self.field:
            command += ["--field", self.field]
        if self.component:
            command += ["--component", self.component]
        if iteration is not None:
            command += ["--iteration", str(iteration)]
        started = time.time()
        proc = subprocess.run(command, capture_output=True, text=True, timeout=300)
        stdout = (proc.stdout or "") + (proc.stderr or "")
        summary = next((line for line in stdout.splitlines() if line.startswith("rendered")), "")
        self.meta = {
            "iteration": iteration,
            "png": str(self.out_png),
            "ms": int((time.time() - started) * 1000),
            "ok": proc.returncode == 0,
            "summary": summary.strip(),
            "log": stdout.strip().splitlines()[-6:],
        }
        if proc.returncode == 0:
            self.iteration = iteration
        return self.meta

    def snapshot(self) -> dict:
        return {"iteration": self.iteration, "meta": self.meta,
                "png": str(self.out_png) if self.out_png.exists() else None}
