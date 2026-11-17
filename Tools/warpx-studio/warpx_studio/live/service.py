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
SERVER = HERE / "render_server.py"
ITERATION = re.compile(r"openpmd_(\d+)\.h5$")


def find_diag_dir(run_dir: Path) -> Path | None:
    """The directory WarpX wrote ``paraview.pmd`` into (``<run>/diag1`` for a diag named diag1)."""
    if (run_dir / "paraview.pmd").exists():
        return run_dir
    # Decks commonly use a `diags/` sub-directory (e.g. `diags/diag1/paraview.pmd`), so the
    # search has to be recursive, not one level deep.
    for candidate in sorted(run_dir.rglob("paraview.pmd")):
        return candidate.parent
    return None


def newest_iteration(diag_dir: Path) -> int | None:
    if diag_dir is None or not diag_dir.is_dir():
        return None
    numbers = [int(m.group(1)) for p in diag_dir.iterdir()
               if (m := ITERATION.search(p.name))]
    return max(numbers) if numbers else None


class RenderWorker:
    """A resident ``pvpython`` process that answers one JSON request per line.

    Starting pvpython costs ~2 s and dominated every frame; the worker pays that once
    and then renders in a fraction of a second.  If it dies (bad frame, ParaView
    crash) the worker is restarted on the next request and the error is reported
    instead of taking the server down.
    """

    def __init__(self, pvpython: str, timeout: float = 120.0) -> None:
        self.pvpython = pvpython
        self.timeout = timeout
        self.process: subprocess.Popen | None = None
        self._lock = threading.Lock()

    def _ensure(self) -> None:
        if self.process is not None and self.process.poll() is None:
            return
        self.process = subprocess.Popen(
            [self.pvpython, str(SERVER)], text=True, bufsize=1,
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )

    def render(self, request: dict) -> dict:
        import json

        with self._lock:
            self._ensure()
            assert self.process is not None
            try:
                self.process.stdin.write(json.dumps(request) + "\n")
                self.process.stdin.flush()
                line = self.process.stdout.readline()
                if not line:
                    raise RuntimeError("renderer closed its pipe")
                return json.loads(line)
            except Exception as exc:                  # restart next time
                self.stop()
                return {"ok": False, "error": f"renderer failed: {type(exc).__name__}: {exc}"}

    def stop(self) -> None:
        process, self.process = self.process, None
        if process is None or process.poll() is not None:
            return
        try:
            process.stdin.write('{"quit": true}\n')
            process.stdin.flush()
            process.wait(timeout=5)
        except Exception:
            process.kill()


class LiveView:
    def __init__(self, run_dir: Path, out_png: Path, field: str | None = None,
                 component: str | None = None, pvpython: str | None = None,
                 interval: float = 1.0) -> None:
        self.run_dir = run_dir
        self.out_png = out_png
        self.fields = [field or "rho"]
        self.component = component
        self.particles: list[str] = []
        self.fixed_iteration: int | None = None
        self.frames: list[dict] = []
        self.pvpython = pvpython or os.environ.get(
            "WARPS_STUDIO_PVPYTHON", os.path.expanduser("~/miniconda3/envs/paraview/bin/pvpython"))
        self.interval = interval
        self.diag_dir: Path | None = None
        self.iteration: int | None = None
        self.meta: dict = {}
        self.worker = RenderWorker(self.pvpython)
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
        target = self.fixed_iteration if self.fixed_iteration is not None else iteration
        frames = []
        started = time.time()
        for field in self.fields:
            request = {"pmd": str(pmd), "out": str(self.png_for(field)), "field": field}
            if self.component:
                request["component"] = self.component
            if self.particles:
                request["particles"] = self.particles
            if target is not None:
                request["iteration"] = int(target)
            reply = self.worker.render(request)
            frames.append({
                "field": field,
                "ok": bool(reply.get("ok")),
                "png": str(self.png_for(field)) if reply.get("ok") else None,
                "summary": (f"{reply['field']}"
                            + (f"[{reply['component']}]" if reply.get("component") is not None else "")
                            + f" {reply['range'][0]:.4g}..{reply['range'][1]:.4g}"
                            if reply.get("ok") else f"failed: {reply.get('error')}"),
                "iteration": reply.get("iteration"),
                "time": reply.get("time"),
            })
        self.frames = frames
        first_ok = next((f for f in frames if f["ok"]), None)
        self.meta = {
            "iteration": first_ok["iteration"] if first_ok else None,
            "png": first_ok["png"] if first_ok else None,
            "ms": int((time.time() - started) * 1000 / max(1, len(frames))),
            "ok": bool(first_ok),
            "summary": (f"iteration {first_ok['iteration']} · "
                        + " | ".join(f"{f['field']}" for f in frames)) if first_ok
            else (frames[0].get("summary") if frames else "no frames"),
        }
        if first_ok:
            self.iteration = first_ok["iteration"]
        return self.meta

    def configure(self, fields=None, component=None, particles=None,
                  iteration="keep", follow=True) -> dict:
        """Change what the live view renders (fields list, particle overlay, time)."""
        if fields:
            self.fields = [f for f in fields if f]
        if component is not None:
            self.component = component or None
        if particles is not None:
            self.particles = list(particles)
        if iteration == "latest":
            self.fixed_iteration = None
        elif isinstance(iteration, int):
            self.fixed_iteration = iteration
        if not follow and self.fixed_iteration is None and self.iteration is not None:
            self.fixed_iteration = self.iteration
        self.iteration = None          # force a re-render with the new settings
        return self.settings()

    def settings(self) -> dict:
        return {"fields": self.fields, "component": self.component,
                "particles": self.particles, "iteration": self.fixed_iteration,
                "follow": self.fixed_iteration is None}

    def png_for(self, field: str) -> Path:
        return self.out_png.with_name(f"{self.out_png.stem}_{field}{self.out_png.suffix}")

    def snapshot(self) -> dict:
        return {"iteration": self.iteration, "meta": self.meta,
                "settings": self.settings(), "frames": self.frames,
                "png": str(self.png_for(self.fields[0])) if self.fields
                and self.png_for(self.fields[0]).exists() else None}
