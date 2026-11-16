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
    return HTMLResponse((HERE / "static/index.html").read_text(encoding="utf-8"))


@app.get("/api/catalog")
def api_catalog():
    payload = {"catalog": load_json("catalog.json"), "zh": load_json("zh.json")}
    try:
        payload["categories"] = load_json("categories.json")["categories"]
    except FileNotFoundError:
        payload["categories"] = []
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
    if not STATE["job"]:
        raise HTTPException(400, "nothing to pause")
    STATE["job"].pause()
    return {"state": "paused"}


@app.post("/api/resume")
def api_resume():
    if not STATE["job"]:
        raise HTTPException(400, "nothing to resume")
    STATE["job"].resume()
    return {"state": "running"}


@app.post("/api/paraview")
def api_paraview(launch: bool = True):
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
    launched = False
    if launch:
        gui = shutil.which("paraview") or os.path.expanduser("~/.local/bin/paraview")
        if Path(gui).exists():
            subprocess.Popen([gui, summary["pvd"]], start_new_session=True,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            launched = True
    return {**summary, "launched": launched}


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


@app.get("/api/frame")
def api_frame(field: str | None = None):
    live: LiveView | None = STATE["live"]
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


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=int(os.environ.get("WARPS_STUDIO_PORT", "8770")))
