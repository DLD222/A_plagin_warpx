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

from warpx_studio.deck.model import Deck, minimal_2d_deck
from warpx_studio.deck.validate import translate_error, validate
from warpx_studio.live.service import LiveView
from warpx_studio.runner.binaries import discover
from warpx_studio.runner.job import Job

HERE = Path(__file__).resolve().parent
STUDIO = HERE.parents[1]
REPO = HERE.parents[3]
DATA = HERE.parents[0] / "catalog/data"   # warpx_studio/catalog/data
RUNS = REPO / "audit_runs/studio"

app = FastAPI(title="WarpX Studio", version="0.1.0")
app.mount("/static", StaticFiles(directory=str(HERE / "static")), name="static")
STATE: dict = {"job": None, "live": None, "run_dir": None}
CATALOG: dict = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))


class DeckPayload(BaseModel):
    params: dict[str, str] = Field(default_factory=dict)
    groups: dict[str, dict[str, dict[str, str]]] = Field(default_factory=dict)


class RunPayload(DeckPayload):
    binary: str
    force: bool = False
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
    return {"catalog": load_json("catalog.json"), "zh": load_json("zh.json")}


@app.get("/api/binaries")
def api_binaries():
    return {"binaries": discover(REPO)}


@app.get("/api/starter")
def api_starter():
    return deck_from(minimal_2d_deck())


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


@app.get("/api/frame")
def api_frame():
    live: LiveView | None = STATE["live"]
    if live is None:
        raise HTTPException(404, "no live view")
    snapshot = live.snapshot()
    if not snapshot["png"]:
        raise HTTPException(404, "no frame rendered yet")
    return FileResponse(snapshot["png"], media_type="image/png",
                        headers={"Cache-Control": "no-store"})


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=int(os.environ.get("WARPS_STUDIO_PORT", "8770")))
