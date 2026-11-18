"""FastAPI server for WarpX Studio (form -> validate -> run -> live view)."""
from __future__ import annotations

import json
import os
import time
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse, HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

from warpx_studio.deck import controls, store
from warpx_studio.deck.model import Deck, minimal_deck
from warpx_studio.deck.validate import translate_error, validate
from warpx_studio import plugins as studio_plugins
from warpx_studio.live.service import LiveView
from warpx_studio.runner.binaries import discover
from warpx_studio.runner.build import BuildJob
from warpx_studio.runner.job import Job

HERE = Path(__file__).resolve().parent
STUDIO = HERE.parents[1]
REPO = HERE.parents[3]
DATA = HERE.parents[0] / "catalog/data"   # warpx_studio/catalog/data
RUNS = REPO / "audit_runs/studio"

app = FastAPI(title="WarpX Studio", version="0.1.0")
app.mount("/static", StaticFiles(directory=str(HERE / "static")), name="static")
STATE: dict = {"job": None, "live": None, "run_dir": None}
# The ParaView GUI launched from the web UI.  It is tracked so the UI can also close it:
# under WSLg the window manager usually cannot close the window (the Qt process keeps
# running and the window stays on screen), so an explicit "close" endpoint is the only
# reliable way to get rid of it.
PARAVIEW: dict = {"proc": None, "pvd": None}
BUILD: dict = {"job": None}
CATALOG: dict = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))


def reattach() -> None:
    """Pick up a simulation that kept running while this server was down."""
    if STATE.get("job") is not None:
        return
    for run_dir in sorted(RUNS.glob("*"), reverse=True):
        job = Job.attach(run_dir)
        if job is None or job.state != "running":
            continue
        live = LiveView(run_dir, run_dir / "live.png")
        live.start()
        STATE.update({"job": job, "live": live, "run_dir": run_dir})
        return


@app.on_event("startup")
def _startup() -> None:
    reattach()


class DeckPayload(BaseModel):
    params: dict[str, str] = Field(default_factory=dict)
    groups: dict[str, dict[str, dict[str, str]]] = Field(default_factory=dict)


class RunPayload(DeckPayload):
    binary: str
    force: bool = False
    project: str | None = None
    mpi: int = 1
    omp: int = 1
    live_field: str | None = "rho"
    live_component: str | None = None


def load_json(name: str):
    return json.loads((DATA / name).read_text(encoding="utf-8"))


def deck_from(payload: DeckPayload) -> Deck:
    deck = Deck()
    for key, value in payload.params.items():
        deck.set(key, value)
    for template, instances in payload.groups.items():
        for name, values in instances.items():
            for key, value in values.items():
                deck.set_group_value(template, name, key, value)
    return deck


@app.get("/", response_class=HTMLResponse)
def index() -> HTMLResponse:
    # 不做缓存：改完界面后刷新就能看到新版本，避免"改了却没生效"的错觉
    return HTMLResponse((HERE / "static/index.html").read_text(encoding="utf-8"),
                        headers={"Cache-Control": "no-store, must-revalidate"})


@app.get("/api/catalog")
def api_catalog():
    payload = {"catalog": load_json("catalog.json"), "zh": load_json("zh.json")}
    try:
        payload["categories"] = load_json("categories.json")["categories"]
    except FileNotFoundError:
        payload["categories"] = []
    try:
        payload["enums"] = load_json("enums.json")["enums"]
        payload["glossary"] = load_json("enums.json").get("glossary", {})
        payload["derived"] = load_json("enums.json").get("derived", {})
        payload["hints"] = load_json("enums.json").get("hints", {})
    except FileNotFoundError:
        payload["enums"], payload["glossary"], payload["derived"] = {}, {}, {}
        payload["hints"] = {}
    return payload


@app.get("/api/binaries")
def api_binaries():
    return {"binaries": discover(REPO)}


@app.get("/api/starter")
def api_starter(dims: str = "2d"):
    try:
        return deck_from(minimal_deck(dims))
    except ValueError as exc:
        raise HTTPException(400, str(exc)) from exc


@app.get("/api/projects")
def api_projects():
    return {"projects": store.list_projects(), "last": store.last_opened()}


@app.get("/api/projects/{name}")
def api_project_load(name: str):
    try:
        return store.load(name)
    except FileNotFoundError as exc:
        raise HTTPException(404, f"没有这个项目: {name}") from exc


class ProjectPayload(BaseModel):
    name: str
    deck: dict = Field(default_factory=dict)
    settings: dict = Field(default_factory=dict)
    notes: str | None = None
    inputs: str | None = None


@app.post("/api/projects")
def api_project_save(payload: ProjectPayload):
    deck = payload.deck or {}
    if not deck.get("params") and not deck.get("groups"):
        raise HTTPException(400, "项目内容为空")
    data = store.save(payload.name, deck, payload.settings, payload.notes, payload.inputs)
    return {"saved": True, "name": data["name"], "updated": data["updated"]}


@app.delete("/api/projects/{name}")
def api_project_delete(name: str):
    return {"deleted": store.delete(name)}


@app.get("/api/controls")
def api_controls():
    return controls.catalogue()


class ExpressionPayload(BaseModel):
    text: str = ""
    constants: list[str] = Field(default_factory=list)


@app.post("/api/expression/check")
def api_expression_check(payload: ExpressionPayload):
    return controls.check_expression(payload.text, known=tuple(payload.constants))


@app.post("/api/validate")
def api_validate(payload: DeckPayload):
    return {"issues": validate(deck_from(payload), CATALOG)}


@app.post("/api/preview")
def api_preview(payload: DeckPayload):
    deck = deck_from(payload)
    return {"inputs": deck.to_inputs()}


@app.post("/api/run")
def api_run(payload: RunPayload):
    binary = Path(payload.binary)
    if not binary.is_file():
        raise HTTPException(400, f"binary not found: {binary}")
    if STATE["job"] is not None and STATE["job"].snapshot()["state"] in ("running", "paused"):
        raise HTTPException(409, "a simulation is already running")
    if STATE["live"] is not None:
        STATE["live"].stop()

    deck = deck_from(payload)
    issues = validate(deck, CATALOG)
    errors = [issue for issue in issues if issue["severity"] == "error"]
    if errors and not payload.force:
        return JSONResponse(status_code=400, content={
            "detail": "参数校验未通过", "issues": issues})
    run_dir = RUNS / time.strftime("%Y%m%d-%H%M%S")
    run_dir.mkdir(parents=True, exist_ok=True)
    max_step = None
    try:
        max_step = int(str(deck.get("max_step", "")).strip())
    except ValueError:
        pass
    job = Job(run_dir, binary, deck.to_inputs(), mpi=payload.mpi, omp=payload.omp,
              max_step=max_step)
    job.env_extra = deck.env() if hasattr(deck, "env") else {}
    job.start()
    live = LiveView(run_dir, run_dir / "live.png", payload.live_field,
                    payload.live_component)
    live.start()
    STATE.update({"job": job, "live": live, "run_dir": run_dir, "deck": deck})
    if payload.project:
        store.record_run(payload.project, str(run_dir))
    return {"run_dir": str(run_dir), "started": True}


@app.get("/api/status")
def api_status():
    job: Job | None = STATE["job"]
    live: LiveView | None = STATE["live"]
    if job is None:
        return {"state": "idle", "live": None}
    snapshot = job.snapshot()
    snapshot["live"] = live.snapshot() if live else None
    deck = STATE.get("deck")
    diagnosis = [d for d in (translate_error(line, CATALOG, deck)
                             for line in snapshot["errors"]) if d]
    snapshot["diagnosis"] = diagnosis
    return snapshot


@app.post("/api/stop")
def api_stop():
    if STATE["job"]:
        STATE["job"].stop()
    if STATE["live"]:
        STATE["live"].stop()
    return {"state": "stopped"}


@app.post("/api/pause")
def api_pause():
    job = STATE["job"]
    if not job:
        raise HTTPException(400, "nothing to pause")
    # A finished run cannot be paused: doing so used to mark it "paused" and the resume
    # below would then mark it "running" forever (the monitor thread had already exited).
    if job.state not in ("running",):
        raise HTTPException(400, f"run is {job.state}, not running")
    job.pause()
    return {"state": job.state}


@app.post("/api/resume")
def api_resume():
    job = STATE["job"]
    if not job:
        raise HTTPException(400, "nothing to resume")
    if job.state != "paused":
        raise HTTPException(400, f"run is {job.state}, not paused")
    job.resume()
    return {"state": job.state}


@app.post("/api/paraview")
def api_paraview(launch: bool = False):
    """Export the current run to VTK and hand it to the ParaView GUI."""
    import shutil
    import subprocess

    run_dir = STATE.get("run_dir")
    if not run_dir:
        raise HTTPException(400, "no run yet")
    pmd = next(iter(sorted(Path(run_dir).rglob("paraview.pmd"))), None)
    if pmd is None:
        raise HTTPException(400, "this run has no openPMD diagnostics")
    pvpython = os.environ.get(
        "WARPS_STUDIO_PVPYTHON",
        os.path.expanduser("~/miniconda3/envs/paraview/bin/pvpython"))
    export = STUDIO / "warpx_studio/live/export_vtk.py"
    out_dir = Path(run_dir) / "paraview_vtk"
    result = subprocess.run([pvpython, str(export), "--pmd", str(pmd),
                             "--out", str(out_dir), "--json"],
                            capture_output=True, text=True, timeout=900)
    if result.returncode != 0:
        raise HTTPException(500, (result.stderr or result.stdout)[-500:])
    summary = json.loads(result.stdout.strip().splitlines()[-1])
    # 任何情况下都不再启动外部 ParaView GUI：它会弹窗、打断操作、而且窗口关不掉。
    # 需要看图请用网页内的交互查看器（无头渲染，不弹窗）。
    # 只有显式设置环境变量 WARPX_STUDIO_ALLOW_PV_GUI=1 时才允许（默认禁止）。
    launched = False
    if launch and os.environ.get("WARPX_STUDIO_ALLOW_PV_GUI") == "1":
        gui = shutil.which("paraview") or os.path.expanduser("~/.local/bin/paraview")
        if Path(gui).exists():
            close_paraview()
            proc = subprocess.Popen([gui, summary["pvd"]], start_new_session=True,
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            PARAVIEW.update({"proc": proc, "pvd": summary["pvd"]})
            launched = True
    return {**summary, "launched": launched, "pid": (PARAVIEW["proc"].pid
                                                     if PARAVIEW["proc"] else None)}


def close_paraview() -> dict:
    """Terminate the ParaView GUI started by the web UI (SIGTERM, then SIGKILL)."""
    import signal
    import time

    proc = PARAVIEW.get("proc")
    pvd = PARAVIEW.get("pvd")
    killed = False
    if proc is not None and proc.poll() is None:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except (ProcessLookupError, PermissionError):
            pass
        for _ in range(20):
            if proc.poll() is not None:
                break
            time.sleep(0.1)
        if proc.poll() is None:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            try:
                proc.wait(timeout=5)
            except Exception:
                pass
        killed = proc.poll() is not None
    PARAVIEW.update({"proc": None, "pvd": None})
    return {"closed": killed, "pvd": pvd}


@app.post("/api/paraview/close")
def api_paraview_close():
    return close_paraview()


@app.post("/api/paraview/close-all")
def api_paraview_close_all():
    """Force-kill every ParaView on this machine.

    The window's own close button does not work under WSLg for a process we spawned in
    its own session, and the tracked PID may have been re-parented, so the UI needs a
    way that always works.
    """
    import subprocess
    killed = close_paraview()
    # Only the GUI, never the resident render worker (`bin/pvpython`): killing that
    # would break the in-page viewer until it restarts.
    for pattern in (r"bin/paraview( |$)", r"bin/paraview-[0-9]"):
        subprocess.run(["pkill", "-f", pattern], capture_output=True)
    running = subprocess.run(["pgrep", "-fc", "paraview"], capture_output=True, text=True)
    return {"closed": True, "tracked": killed.get("closed", False),
            "remaining": (running.stdout or "0").strip()}


@app.get("/api/paraview/status")
def api_paraview_status():
    proc = PARAVIEW.get("proc")
    return {"running": bool(proc is not None and proc.poll() is None),
            "pid": proc.pid if proc is not None else None,
            "pvd": PARAVIEW.get("pvd")}


class BuildPayload(BaseModel):
    dims: str = "2d"
    name: str | None = None
    jobs: int = 8
    options: dict[str, str] = Field(default_factory=dict)


@app.post("/api/build")
def api_build(payload: BuildPayload):
    if BUILD["job"] is not None and BUILD["job"].state in ("configure", "build"):
        raise HTTPException(409, "a build is already running")
    name = payload.name or f"{payload.dims}-{time.strftime('%H%M%S')}"
    job = BuildJob(REPO, name, payload.dims, payload.options, jobs=payload.jobs)
    job.start()
    BUILD["job"] = job
    return {"name": name, "dir": str(job.dir), "started": True}


@app.get("/api/build/status")
def api_build_status():
    if BUILD["job"] is None:
        return {"state": "idle"}
    return BUILD["job"].snapshot()


@app.get("/api/checkpoints")
def api_checkpoints():
    """Checkpoint directories produced by earlier runs of this studio."""
    found = []
    for run_dir in sorted(RUNS.glob("*"), reverse=True)[:40]:
        for chk in sorted(run_dir.rglob("chk[0-9]*")):
            if chk.is_dir():
                found.append({"path": str(chk), "run": run_dir.name,
                              "name": chk.name})
    return {"checkpoints": found[:40]}


@app.get("/api/plugins")
def api_plugins():
    return {"plugins": studio_plugins.discover()}


@app.get("/api/panel/{plugin_id}")
def api_panel(plugin_id: str):
    job = STATE.get("job")
    live = STATE.get("live")
    context = {
        "run_dir": str(STATE["run_dir"]) if STATE.get("run_dir") else None,
        "job": job.snapshot() if job else None,
        "live": live.snapshot() if live else None,
        "params": STATE["deck"].params if STATE.get("deck") else {},
    }
    return studio_plugins.render(plugin_id, context)


class LivePayload(BaseModel):
    fields: list[str] = Field(default_factory=list)
    component: str | None = None
    particles: list[str] = Field(default_factory=list)
    iteration: int | None = None
    follow: bool = True


@app.post("/api/live")
def api_live(payload: LivePayload):
    live: LiveView | None = STATE.get("live")
    if live is None:
        raise HTTPException(400, "no run yet")
    return live.configure(fields=payload.fields, component=payload.component,
                          particles=payload.particles,
                          iteration="latest" if payload.follow and payload.iteration is None
                          else payload.iteration,
                          follow=payload.follow)


@app.get("/api/live/iterations")
def api_iterations():
    live: LiveView | None = STATE.get("live")
    if live is None:
        raise HTTPException(400, "no run yet")
    diag = live.diag_dir or (Path(STATE["run_dir"]) if STATE.get("run_dir") else None)
    numbers = []
    if diag and diag.is_dir():
        import re as _re
        for path in diag.iterdir():
            match = _re.match(r"openpmd_(\d+)\.h5$", path.name)
            if match:
                numbers.append(int(match.group(1)))
    return {"iterations": sorted(numbers), "current": live.iteration}


class ExportPayload(BaseModel):
    fields: list[str] = Field(default_factory=lambda: ["rho"])
    component: str | None = None
    particles: list[str] = Field(default_factory=list)
    max_frames: int = 40
    make_video: bool = True


@app.post("/api/export")
def api_export(payload: ExportPayload):
    """Render a PNG sequence (and an mp4 when ffmpeg exists) for the current run."""
    import shutil
    import subprocess

    live: LiveView | None = STATE.get("live")
    if live is None:
        raise HTTPException(400, "no run yet")
    diag = live.diag_dir or live.run_dir
    import re as _re
    numbers = sorted(int(m.group(1)) for path in diag.iterdir()
                     if (m := _re.match(r"openpmd_(\d+)\.h5$", path.name)))
    if not numbers:
        raise HTTPException(400, "this run has no openPMD output yet")
    step = max(1, len(numbers) // max(1, payload.max_frames))
    numbers = numbers[::step][:payload.max_frames]
    out_dir = Path(STATE["run_dir"]) / "export"
    out_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for iteration in numbers:
        for field in payload.fields or ["rho"]:
            request = {"pmd": str(diag / "paraview.pmd"),
                       "out": str(out_dir / f"{field}_{iteration:08d}.png"),
                       "field": field, "iteration": iteration}
            if payload.component:
                request["component"] = payload.component
            if payload.particles:
                request["particles"] = payload.particles
            reply = live.worker.render(request)
            if reply.get("ok"):
                written.append(Path(reply["png"]).name)
    mp4 = None
    ffmpeg = shutil.which("ffmpeg")
    if payload.make_video and ffmpeg and written:
        pattern = str(out_dir / f"{(payload.fields or ['rho'])[0]}_%08d.png")
        target = out_dir / f"{(payload.fields or ['rho'])[0]}.mp4"
        subprocess.run([ffmpeg, "-y", "-framerate", "10", "-pattern_type", "glob",
                        "-i", pattern, "-pix_fmt", "yuv420p", str(target)],
                       capture_output=True, timeout=600)
        mp4 = str(target) if target.exists() else None
    return {"frames": written, "count": len(written), "dir": str(out_dir), "mp4": mp4,
            "ffmpeg": bool(ffmpeg)}


FRAME_CACHE = Path()          # set together with STATE["run_dir"] on the first request


def _render_frame(iteration: int | None, field: str, component: str | None,
                  particles: list[str] | None,
                  camera: dict | None = None) -> Path:
    """Render one arbitrary (iteration, field, component) frame and cache it on disk.

    The live view only renders the newest iteration for its configured fields; the
    timeline in the web UI needs any iteration on demand, so this is a separate path
    that reuses the same resident pvpython worker.
    """
    live: LiveView | None = STATE.get("live")
    if live is None:
        raise HTTPException(404, "no live view")
    diag = live.diag_dir or Path(STATE["run_dir"])
    pmd = diag / "paraview.pmd"
    if not pmd.exists():
        raise HTTPException(404, "this run has no openPMD output")

    tag_parts = [field, component or "all"]
    if particles:
        tag_parts.append("p" + "-".join(sorted(particles)))
    tag_parts.append("latest" if iteration is None else f"{int(iteration):08d}")
    if camera:
        tag_parts.append("c" + "_".join(f"{k}{camera[k]}" for k in sorted(camera)
                                        if camera[k] not in (None, 0, 0.0, 1.0)))
    out = Path(STATE["run_dir"]) / "web_frames" / ("_".join(tag_parts) + ".png")
    if out.exists():
        return out
    out.parent.mkdir(parents=True, exist_ok=True)
    request = {"pmd": str(pmd), "out": str(out), "field": field}
    if component:
        request["component"] = component
    if particles:
        request["particles"] = list(particles)
    if iteration is not None:
        request["iteration"] = int(iteration)
    if camera:
        request["camera"] = camera
    reply = live.worker.render(request)
    if not reply.get("ok"):
        raise HTTPException(500, reply.get("error") or "render failed")
    return out


@app.get("/api/frame")
def api_frame(field: str | None = None, component: str | None = None,
              iteration: int | None = None, particles: str | None = None,
              az: float = 0.0, el: float = 0.0, zoom: float = 1.0,
              px: float = 0.0, pz: float = 0.0):
    # Backwards compatible: with no extra arguments this serves the live view's own frame.
    if field and (component or iteration is not None or particles
                  or az or el or zoom != 1.0 or px or pz):
        species = [s.strip() for s in particles.split(",") if s.strip()] if particles else None
        camera = {"azimuth": az, "elevation": el, "zoom": zoom, "pan": [px, pz]}
        return FileResponse(_render_frame(iteration, field, component, species, camera),
                            media_type="image/png", headers={"Cache-Control": "no-store"})
    live: LiveView | None = STATE.get("live")
    if live is None:
        raise HTTPException(404, "no live view")
    snapshot = live.snapshot()
    target = None
    if field:
        match = next((f for f in snapshot["frames"] if f["field"] == field and f.get("png")), None)
        target = match["png"] if match else None
    target = target or snapshot["png"]
    if not target:
        raise HTTPException(404, "no frame rendered yet")
    return FileResponse(target, media_type="image/png",
                        headers={"Cache-Control": "no-store"})


# ---------------------------------------------------------------------------
# Saved pictures: the user picks a (iteration, quantity) frame and keeps it, and can
# delete it again from the gallery.  Files live under <run>/snapshots/ so they survive
# a server restart together with the run.
# ---------------------------------------------------------------------------
class SnapshotPayload(BaseModel):
    name: str
    iteration: int | None = None
    field: str = "rho"
    component: str | None = None
    particles: list[str] = Field(default_factory=list)


def _snapshot_dir() -> Path:
    run = STATE.get("run_dir")
    if not run:
        raise HTTPException(400, "no run yet")
    folder = Path(run) / "snapshots"
    folder.mkdir(parents=True, exist_ok=True)
    return folder


def _snapshot_index(folder: Path) -> list[dict]:
    path = folder / "index.json"
    if not path.is_file():
        return []
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return []


def _write_index(folder: Path, entries: list[dict]) -> None:
    (folder / "index.json").write_text(json.dumps(entries, ensure_ascii=False, indent=1) + "\n",
                                       encoding="utf-8")


def _safe_name(name: str) -> str:
    import re as _re
    cleaned = _re.sub(r"[^0-9A-Za-z\u4e00-\u9fff._-]+", "-", (name or "").strip()).strip("-._")
    return cleaned[:60] or "snapshot"


QUANTITIES_CACHE: dict = {"run": None, "items": []}


@app.get("/api/paths")
def api_paths():
    """输入文件与输出文件的真实路径，界面上直接显示给用户。"""
    job = STATE.get("job")
    workdir = getattr(job, "run_dir", None) if job else None
    if workdir is None:
        workdir = STATE.get("run_dir")
    if not workdir:
        return {"workdir": None, "inputs": None, "diag_dir": None}
    base = Path(workdir)
    inputs = base / "inputs"
    diag = None
    pmd = sorted(base.rglob("paraview.pmd"))
    if pmd:
        diag = str(pmd[0].parent)
    else:
        for cand in sorted(base.rglob("openpmd_*.h5")):
            diag = str(cand.parent)
            break
    return {"workdir": str(base),
            "inputs": str(inputs) if inputs.is_file() else None,
            "diag_dir": diag,
            "snapshots": str(base / "snapshots"),
            "frames": str(base / "web_frames")}


@app.get("/api/quantities")
def api_quantities():
    """List the field quantities this run can render, with their component counts.

    The list comes from the file itself (the resident renderer enumerates the arrays),
    so the UI cannot offer something the output does not contain — in particular it
    will not offer x/y/z for a scalar such as the charge density `rho`.
    """
    live: LiveView | None = STATE.get("live")
    if live is None:
        raise HTTPException(400, "no run yet")
    run = STATE.get("run_dir")
    if QUANTITIES_CACHE.get("run") == run and QUANTITIES_CACHE.get("items"):
        return {"quantities": QUANTITIES_CACHE["items"],
                "diag": str(live.diag_dir or run)}
    diag = live.diag_dir or Path(run)
    pmd = diag / "paraview.pmd"
    if not pmd.exists():
        raise HTTPException(404, "this run has no openPMD output")
    reply = live.worker.render({"pmd": str(pmd), "list": True})
    if not reply.get("ok"):
        raise HTTPException(500, reply.get("error") or "cannot inspect the output")
    items = reply.get("quantities", [])
    QUANTITIES_CACHE.update({"run": run, "items": items})
    return {"quantities": items, "diag": str(diag)}


@app.get("/api/snapshots")
def api_snapshots():
    folder = _snapshot_dir()
    return {"snapshots": _snapshot_index(folder), "dir": str(folder)}


@app.post("/api/snapshots")
def api_snapshot_save(payload: SnapshotPayload):
    folder = _snapshot_dir()
    stem = _safe_name(payload.name)
    entries = _snapshot_index(folder)
    if any(e.get("stem") == stem for e in entries):
        raise HTTPException(409, f"已有同名图片：{payload.name}")
    source = _render_frame(payload.iteration, payload.field, payload.component,
                           payload.particles or None)
    target = folder / f"{stem}.png"
    target.write_bytes(source.read_bytes())
    entries.append({
        "stem": stem,
        "name": payload.name,
        "file": target.name,
        "iteration": payload.iteration,
        "field": payload.field,
        "component": payload.component,
        "particles": payload.particles,
        "created": time.strftime("%Y-%m-%dT%H:%M:%S"),
    })
    _write_index(folder, entries)
    return {"saved": True, "name": payload.name, "file": target.name, "snapshots": entries}


@app.get("/api/snapshot/{stem}")
def api_snapshot_image(stem: str):
    folder = _snapshot_dir()
    target = folder / f"{_safe_name(stem)}.png"
    if not target.is_file():
        raise HTTPException(404, "no such snapshot")
    return FileResponse(target, media_type="image/png", headers={"Cache-Control": "no-store"})


@app.delete("/api/snapshot/{stem}")
def api_snapshot_delete(stem: str):
    folder = _snapshot_dir()
    entries = _snapshot_index(folder)
    keep = [e for e in entries if e.get("stem") != _safe_name(stem)]
    removed = len(entries) - len(keep)
    (folder / f"{_safe_name(stem)}.png").unlink(missing_ok=True)
    _write_index(folder, keep)
    return {"deleted": removed, "snapshots": keep}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=int(os.environ.get("WARPS_STUDIO_PORT", "8770")))
