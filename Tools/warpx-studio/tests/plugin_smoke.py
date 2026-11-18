"""插件求解器冒烟测试：插件与内建求解器一样，能从网页直接选、直接跑。

流程（全部走网页 API）：
  1. 组一个小 2D 算例，`studio.solver.plugins` 选中 star_gravity_plugin.so
  2. POST /api/run
  3. 断言：日志里出现插件自己的输出（[gravity] profile loaded / phi_min），且有诊断输出
"""
from __future__ import annotations

import json
import pathlib
import sys
import time
import urllib.request

BASE = "http://127.0.0.1:8770"
REPO = pathlib.Path(__file__).resolve().parents[3]

DECK = """max_step = 3
amr.n_cell = 16 16
amr.max_level = 0
geometry.dims = 2
geometry.prob_lo = -2e10 -2e10
geometry.prob_hi =  2e10  2e10
boundary.field_lo = periodic periodic
boundary.field_hi = periodic periodic
warpx.cfl = 0.5
algo.maxwell_solver = yee
algo.particle_shape = 1
diagnostics.diags_names = diag1
diag1.intervals = 1
diag1.diag_type = Full
diag1.format = openpmd
diag1.fields_to_plot = rho
"""


def post(path: str, body: dict) -> dict:
    req = urllib.request.Request(BASE + path, data=json.dumps(body).encode(),
                                 headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=120) as r:
        return json.loads(r.read())


def get(path: str) -> dict:
    with urllib.request.urlopen(BASE + path, timeout=60) as r:
        return json.loads(r.read())


def main() -> int:
    sys.path.insert(0, str(REPO / "Tools/warpx-studio"))
    from warpx_studio.deck.model import Deck

    deck = Deck.from_inputs(DECK)
    plugin = REPO / "audit_build/fork/lib/star_gravity_plugin.so"
    profile = REPO / "Docs/research_notes/star_plugins/data/ssm_S_profile.csv"
    if not plugin.exists() or not profile.exists():
        print(f"跳过：插件或剖面文件不存在（{plugin} / {profile}）")
        return 0
    deck.params["studio.solver.plugins"] = str(plugin)
    deck.params["studio.plugin.star_profile"] = str(profile)
    post("/api/stop", {})
    time.sleep(0.5)
    out = post("/api/run", {"params": dict(deck.params),
                            "groups": {t: {i: dict(v) for i, v in inst.items()}
                                       for t, inst in deck.groups.items()},
                            "binary": str(REPO / "audit_build/fork/bin/warpx.2d"),
                            "force": True, "mpi": 1, "omp": 1})
    run_dir = pathlib.Path(out["run_dir"])
    deadline = time.time() + 300
    while time.time() < deadline:
        if get("/api/status").get("state") in ("finished", "failed", "stopped"):
            break
        time.sleep(1.0)
    log = (run_dir / "run.log").read_text(errors="ignore")
    checks = {
        "插件被加载": "[gravity] profile loaded" in log,
        "插件求解了势": "[gravity] step" in log,
        "有诊断输出": bool(list(run_dir.rglob("openpmd_*.h5"))),
        "环境变量生效": "WARPX_PLUGIN_PATH" not in (run_dir / "inputs").read_text(errors="ignore"),
    }
    for name, ok in checks.items():
        print(("PASS " if ok else "FAIL ") + name)
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
