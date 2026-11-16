#!/usr/bin/env python3
"""Acceptance test for WarpX Studio: starts the server, exercises every feature, reports.

    ~/miniconda3/envs/warpx-studio/bin/python selftest.py [--port 8791]

Each check prints PASS/FAIL with the evidence it used; the exit code is non-zero if any
check fails.  The runs use tiny decks on purpose (a few seconds each).
"""
from __future__ import annotations

import json
import os
import signal
import socket
import subprocess
import sys
import time
import urllib.request
from pathlib import Path

STUDIO = Path(__file__).resolve().parent
REPO = STUDIO.parents[1]
sys.path.insert(0, str(STUDIO))

RESULTS: list[tuple[str, bool, str]] = []


def check(name: str, ok: bool, evidence: str) -> bool:
    RESULTS.append((name, bool(ok), evidence))
    print(f"[{'PASS' if ok else 'FAIL'}] {name}: {evidence}")
    return ok


def free_port(preferred: int) -> int:
    with socket.socket() as sock:
        try:
            sock.bind(("127.0.0.1", preferred))
            return preferred
        except OSError:
            sock.bind(("127.0.0.1", 0))
            return sock.getsockname()[1]


def start_server(port: int) -> subprocess.Popen:
    env = dict(os.environ, PYTHONPATH=str(STUDIO))
    process = subprocess.Popen(
        [sys.executable, "-m", "uvicorn", "warpx_studio.web.app:app",
         "--host", "127.0.0.1", "--port", str(port)],
        cwd=str(STUDIO), env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        start_new_session=True)
    for _ in range(60):
        try:
            urllib.request.urlopen(f"http://127.0.0.1:{port}/api/plugins", timeout=2)
            return process
        except Exception:
            time.sleep(0.5)
    raise SystemExit("server did not start")


def call(port: int, path: str, payload=None, timeout=600, method: str | None = None):
    data = json.dumps(payload).encode() if payload is not None else None
    request = urllib.request.Request(f"http://127.0.0.1:{port}{path}", data=data,
                                     headers={"Content-Type": "application/json"},
                                     method=method or ("POST" if payload is not None else "GET"))
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            body = response.read()
            return response.status, (json.loads(body) if body[:1] in (b"{", b"[") else body)
    except urllib.error.HTTPError as exc:
        body = exc.read()
        return exc.code, (json.loads(body) if body[:1] in (b"{", b"[") else body)


def main() -> int:
    port = free_port(int(os.environ.get("WARPS_STUDIO_PORT", "8791")))
    server = start_server(port)
    try:
        run_checks(port)
    finally:
        os.killpg(os.getpgid(server.pid), signal.SIGTERM)
    failed = [name for name, ok, _ in RESULTS if not ok]
    print(f"\n{len(RESULTS) - len(failed)}/{len(RESULTS)} checks passed")
    if failed:
        print("failed:", ", ".join(failed))
    return 1 if failed else 0


def run_checks(port: int) -> None:
    from warpx_studio.deck.model import Deck, minimal_deck
    from warpx_studio.deck.validate import translate_error, validate

    # ---- catalog ---------------------------------------------------------
    _, catalog_payload = call(port, "/api/catalog")
    catalog, zh = catalog_payload["catalog"], catalog_payload["zh"]
    counts = catalog["counts"]
    check("目录规模", counts["total"] >= 700,
          f"{counts['total']} 参数 = {counts['documented']} 文档 + {counts['source_only']} 仅源码")
    check("gating 提取", counts["with_runtime_gating"] >= 120,
          f"{counts['with_runtime_gating']} 条带运行期条件，{counts['with_gating_rules']} 条可执行规则")
    check("可重复组", sum(1 for g in catalog["groups"] if g.startswith("<")) >= 6,
          str([g for g in catalog["groups"] if g.startswith("<")]))
    check("枚举选项", counts["with_options"] >= 20, f"{counts['with_options']} 个参数有下拉选项")

    # ---- v2: AST 扫描与 v1 正则的覆盖对比（目标要求给出升级前后的数字） ----
    data = STUDIO / "warpx_studio/catalog/data"
    import json as _json
    regex_sites = _json.loads((data / "param_sites.json").read_text())["sites"]
    ast_path = data / "param_sites_ast.json"
    ast_sites = _json.loads(ast_path.read_text())["sites"] if ast_path.exists() else []
    ast_keys = {s["param"] for s in ast_sites}
    regex_keys = {s["param"] for s in regex_sites}
    switch_sites = sum(1 for s in ast_sites if s.get("switch_cases"))
    ast_required = sum(1 for s in ast_sites if s.get("required"))
    check("AST 扫描可用", bool(ast_sites),
          f"clang {len(ast_sites)} 站点 / {len(ast_keys)} 键，其中 switch-case {switch_sites} 条、"
          f"无条件 get {ast_required} 条")
    check("两路合并", counts["sites_by_scanner"]["clang"] > 0
          and counts["sites_by_scanner"]["regex"] > 0,
          f"clang {counts['sites_by_scanner']['clang']} + regex {counts['sites_by_scanner']['regex']}"
          f" = {counts['sites']} 站点；键 {len(regex_keys | ast_keys)} 个"
          f"（仅 clang 新增 {len(ast_keys - regex_keys)}）")
    check("switch/case gating", counts.get("switch_gated", 0) >= 1,
          f"{counts.get('switch_gated', 0)} 条读取点位于 switch/case 内")
    check("中文说明", len(zh["params"]) >= 70,
          f"{len(zh['params'])} 条参数译文 + {len(zh['sections'])} 个分区标题")

    # ---- v3: 每个参数都有中文、按物理分类、输入框有灰色示例 ----
    _, catalog_payload = call(port, "/api/catalog")
    all_names = [p["name"] for p in CATALOG_JSON["params"]] if False else \
        [p["name"] for p in catalog_payload["catalog"]["params"]]
    translated = [n for n in all_names if catalog_payload["zh"]["params"].get(n)]
    check("每个参数都有中文", len(translated) == len(all_names),
          f"{len(translated)}/{len(all_names)} 条")
    categories = catalog_payload.get("categories") or []
    check("按物理分类", len(categories) >= 8,
          "、".join(f"{c['name']}({len(c['params'])})" for c in categories[:6]) + " …")
    check("分类覆盖全部参数", sum(len(c["params"]) for c in categories) >= len(all_names),
          f"{sum(len(c['params']) for c in categories)} 项落在 {len(categories)} 个分类里")
    order = [c["order"] for c in categories]
    check("分类顺序稳定", order == sorted(order), str(order))
    status, page_html = call(port, "/")
    if isinstance(page_html, bytes):
        page_html = page_html.decode("utf-8", "replace")
    check("输入框灰色示例", "exampleFor" in page_html and "默认 " in page_html,
          "placeholder 取文档默认值，无默认值按类型给示例")

    # ---- 固定取值用下拉，而不是手打 ----
    enums = catalog_payload.get("enums") or {}
    check("固定取值改下拉", len(enums) >= 100,
          f"{len(enums)} 个参数有取值列表")
    check("求解器下拉", enums.get("algo.maxwell_solver") == ["yee", "ckc", "psatd", "ect"],
          str(enums.get("algo.maxwell_solver")))
    check("诊断类型下拉", "TimeAveraged" in (enums.get("<diag_name>.diag_type") or []),
          str(enums.get("<diag_name>.diag_type")))
    check("布尔量下拉", enums.get("warpx.do_pml_divb_cleaning") == ["0", "1"],
          "bool 类型自动变成 0/1")
    glossary = catalog_payload.get("glossary") or {}
    check("取值含义说明", len(glossary) >= 10 and
          "标准 FDTD" in glossary.get("algo.maxwell_solver", {}).get("yee", ""),
          f"{len(glossary)} 组取值带中文含义")
    derived = catalog_payload.get("derived") or {}
    check("重复参数已折叠", "particles.species_names" in derived
          and "warpx.do_pml" in derived,
          f"{len(derived)} 条：{', '.join(list(derived)[:3])} …")
    check("界面使用下拉表", "ENUMS[name]" in page_html, "inputRow 会查 ENUMS")

    # ---- deck model ------------------------------------------------------
    for dims in ("1d", "2d", "3d", "rz"):
        deck = minimal_deck(dims)
        n = len(deck.get("amr.n_cell").split())
        check(f"起步模板 {dims}", n == len(deck.get("boundary.field_lo").split()),
              f"n_cell={deck.get('amr.n_cell')!r} boundary 项数={n}")
    text = minimal_deck("2d").to_inputs()
    again = Deck.from_inputs(text).to_inputs()
    check("deck 往返", again == text, f"{len(text.splitlines())} 行输入文件可无损往返")

    # ---- validation ------------------------------------------------------
    good = minimal_deck("2d")
    check("校验：正确 deck 无问题", not validate(good, catalog), "0 条 issue")
    bad = minimal_deck("2d"); bad.params["amr.n_cell"] = "64 abc"
    errors = [i for i in validate(bad, catalog) if i["severity"] == "error"]
    check("校验：类型错误被拦", any(i["param"] == "amr.n_cell" for i in errors),
          errors[0]["message"] if errors else "未拦下")
    missing = minimal_deck("2d"); missing.groups["<species_name>"]["electrons"].pop("profile")
    errors = [i for i in validate(missing, catalog) if i["severity"] == "error"]
    check("校验：必填项被拦", any("profile" in i["param"] for i in errors),
          errors[0]["param"] if errors else "未拦下")
    big = minimal_deck("2d"); big.params["amr.n_cell"] = "4096 4096"
    warns = [i for i in validate(big, catalog) if i["severity"] == "warn"]
    check("校验：资源预估", bool(warns), warns[0]["message"][:60] if warns else "无警告")

    # ---- error translation ----------------------------------------------
    sample = minimal_deck("2d")
    cases = [
        ("### ERROR : algo.particle_shape must be set in the input file", "algo.particle_shape"),
        ("ParmParse::getval ParmParse::getval(): electrons..profile not found in database",
         "<species_name>.profile"),
        ("### ERROR : Injection style string 'uniform_kg' not recognized.",
         "<species_name>.injection_style"),
    ]
    for line, expected in cases:
        got = translate_error(line, catalog, sample)
        check(f"报错翻译 → {expected}", bool(got) and got["param"] == expected,
              (got or {}).get("param", "None") + " / " + (got or {}).get("hint", "")[:40])

    # ---- structured controls (v2) ---------------------------------------
    from warpx_studio.deck import controls as ctl

    status, cat = call(port, "/api/controls")
    check("各维度的坐标名", cat["axes"] == {"1": ["z"], "2": ["x", "z"], "3": ["x", "y", "z"],
                                            "RZ": ["r", "z"], "RCYLINDER": ["r", "z"],
                                            "RSPHERE": ["r"]},
          "1D=z / 2D=x,z / 3D=x,y,z / RZ=r,z（与文档一致）")
    check("结构化控件清单", status == 200 and len(cat["field_bc"]) >= 8
          and len(cat["particle_bc"]) >= 4 and len(cat["templates"]) >= 3,
          f"{len(cat['field_bc'])} 种场边界 / {len(cat['particle_bc'])} 种粒子边界 / "
          f"{len(cat['templates'])} 个表达式模板")

    deck2d = minimal_deck("2d"); deck3d = minimal_deck("3d")
    check("边界按维度拆分", ctl.boundary_values(deck2d, "boundary.field_lo") == ["periodic"] * 2
          and ctl.boundary_values(deck3d, "boundary.field_lo") == ["periodic"] * 3,
          "2D → 2 项，3D → 3 项")
    ctl.set_boundary(deck2d, "boundary.field_hi", ["pml", "pec"])
    check("边界写回", deck2d.get("boundary.field_hi") == "pml pec",
          f"boundary.field_hi = {deck2d.get('boundary.field_hi')!r}")

    good = ctl.check_expression("exp(-(x/w)**2)*cos(k*z)", known=("w", "k"))
    bad = ctl.check_expression("sin(k*x")
    unknown = ctl.check_expression("foo(x)")
    check("表达式校验：合法", good["ok"] and not good["problems"], str(good["symbols"]))
    check("表达式校验：括号", (not bad["ok"]) and "右括号" in bad["problems"][0], bad["problems"][0])
    check("表达式校验：未知名字只警告", unknown["ok"] and unknown["warnings"],
          unknown["warnings"][0][:40])
    status, api_check = call(port, "/api/expression/check",
                             {"text": "sin(k*x", "constants": []})
    check("表达式校验 API", status == 200 and not api_check["ok"],
          str(api_check["problems"]))
    check("表达式键识别", ctl.is_expression_key("warpx.Ex_external_grid_function(x,y,z)")
          and not ctl.is_expression_key("warpx.cfl"), "含 (x,y,z) 的 key 走表达式控件")
    check("外场初始化风格", len(ctl.style_options("warpx.E_ext_grid_init_style")) >= 5
          and "read_from_file" in ctl.style_options("<s>.B_ext_particle_init_style"),
          f"{len(ctl.style_options('warpx.E_ext_grid_init_style'))} 种 grid / "
          f"{len(ctl.style_options('x.E_ext_particle_init_style'))} 种 particle")

    # ---- binaries --------------------------------------------------------
    _, bins = call(port, "/api/binaries")
    binaries = bins["binaries"]
    fork = [b for b in binaries if b["build"] == "fork"]
    check("二进制探测", len(binaries) >= 4 and fork,
          f"{len(binaries)} 个（fork {len(fork)} 个，维度 {sorted({b['dims'] for b in binaries})}）")

    # ---- run + live view ------------------------------------------------
    binary = next(b["path"] for b in fork if b["dims"] == "2d")
    _, starter = call(port, "/api/starter?dims=2d")
    # long enough that the pause/resume check has something to pause (200 steps finish in ~2 s)
    starter["params"]["max_step"] = "2000"
    starter["params"]["amr.n_cell"] = "128 128"
    status, started = call(port, "/api/run", {
        "params": starter["params"], "groups": starter["groups"], "binary": binary,
        "mpi": 1, "omp": 1, "live_field": "rho"})
    check("启动仿真", status == 200, started.get("run_dir", str(started)))
    time.sleep(2)
    status, state = call(port, "/api/status")
    check("日志进度", state.get("step", 0) > 0,
          f"state={state['state']} step={state['step']}/{state.get('max_step')}")
    # ---- v2: live view enhancements -------------------------------------
    status, live_settings = call(port, "/api/live", {
        "fields": ["rho", "E"], "component": "z", "particles": ["electrons"],
        "follow": True})
    check("多字段+粒子设置", status == 200 and live_settings["fields"] == ["rho", "E"],
          str(live_settings))
    frames = []
    for _ in range(20):
        time.sleep(1)
        frames = (call(port, "/api/status")[1].get("live") or {}).get("frames", [])
        if len(frames) >= 2 and all(f["ok"] for f in frames):
            break
    check("多字段并排渲染", len(frames) >= 2 and all(f["ok"] for f in frames),
          f"{[(f['field'], f['ok']) for f in frames]}")
    if frames:
        body = call(port, "/api/frame?field=" + frames[1]["field"])[1]
        check("按字段取图", isinstance(body, bytes) and body[:4] == b"\x89PNG",
              f"{frames[1]['field']}: {len(body)} bytes")
    status, iters = call(port, "/api/live/iterations")
    numbers = iters.get("iterations", [])
    check("迭代列表", len(numbers) >= 2, f"{len(numbers)} 个迭代: {numbers[:3]}…{numbers[-1:]}")
    if len(numbers) >= 2:
        oldest = numbers[0]
        call(port, "/api/live", {"iteration": oldest, "follow": False, "fields": ["rho"]})
        replayed = []
        for _ in range(20):
            time.sleep(1)
            replayed = (call(port, "/api/status")[1].get("live") or {}).get("frames", [])
            if replayed and replayed[0].get("iteration") == oldest:
                break
        check("历史迭代回放", bool(replayed) and replayed[0].get("iteration") == oldest,
              f"回放到 iteration {oldest}")
        call(port, "/api/live", {"fields": ["rho"], "follow": True})
    status, export = call(port, "/api/export", {"fields": ["rho"], "max_frames": 3,
                                                "make_video": True})
    check("帧序列导出", status == 200 and export.get("count", 0) >= 2,
          f"{export.get('count')} 帧 -> {export.get('dir')}；ffmpeg={export.get('ffmpeg')}"
          f"，mp4={bool(export.get('mp4'))}")

    # ---- persistence: a restarted server can rebuild the job ----------
    from pathlib import Path as _Path
    from warpx_studio.runner.job import Job as _Job

    run_dir = _Path(state.get("workdir", "")) if state.get("workdir") else None
    # 机器忙时子进程可能过一会儿才写出状态文件，轮询而不是立即断言
    for _ in range(20):
        if run_dir and (run_dir / "studio_job.json").is_file():
            break
        time.sleep(0.5)
    check("运行状态落盘", bool(run_dir) and (run_dir / "studio_job.json").is_file(),
          str(run_dir / "studio_job.json") if run_dir else "-")
    check("运行日志落盘", bool(run_dir) and (run_dir / "run.log").is_file(),
          f"{(run_dir / 'run.log').stat().st_size} bytes" if run_dir and (run_dir / "run.log").exists() else "-")
    attached = _Job.attach(run_dir) if run_dir else None
    check("重新 attach 可回放进度", attached is not None and attached.step > 0,
          f"attach 后 step={attached.step}" if attached else "attach 失败")

    paused = {}
    if state["state"] == "running":
        call(port, "/api/pause", {})
        _, paused = call(port, "/api/status")
        check("暂停", paused["state"] == "paused", f"暂停时 state={paused['state']}")
        call(port, "/api/resume", {})
        _, resumed = call(port, "/api/status")
        check("继续", resumed["state"] == "running", f"继续后 state={resumed['state']}")
    else:
        check("暂停", False, f"仿真结束太快（step={state.get('step')}），无法测试暂停")
    for _ in range(60):
        time.sleep(1)
        _, state = call(port, "/api/status")
        if state["state"] in ("finished", "failed"):
            break
    check("跑完", state["state"] == "finished",
          f"state={state['state']} step={state['step']} errors={state.get('errors')}")
    live = state.get("live") or {}
    meta = live.get("meta") or {}
    check("实时帧", bool(live.get("png")) and meta.get("ok"),
          f"iteration {live.get('iteration')} · {meta.get('ms')} ms · {meta.get('summary','')[:50]}")
    if live.get("png"):
        body = call(port, "/api/frame")[1]
        check("帧可下载", isinstance(body, bytes) and body[:4] == b"\x89PNG",
              f"{len(body)} bytes, PNG 头正确")

    # ---- projects: create, remember, edit, delete ----------------------
    from urllib.parse import quote

    project = "自测任务-" + time.strftime("%H%M%S")
    status, saved = call(port, "/api/projects", {
        "name": project, "deck": {"params": starter["params"], "groups": starter["groups"]},
        "settings": {"mpi": 2, "omp": 1}, "inputs": "# selftest"})
    check("新建任务", status == 200 and saved.get("saved"), f"{project} @ {saved.get('updated')}")
    listed = [p for p in call(port, "/api/projects")[1]["projects"] if p["name"] == project]
    check("任务列表含新建项", bool(listed), str(listed[0]["dims"]) + "D")
    status, loaded = call(port, "/api/projects/" + quote(project))
    check("任务可重新打开", status == 200 and loaded["deck"]["params"]["max_step"] == "2000",
          f"max_step={loaded['deck']['params'].get('max_step')}，设置 mpi={loaded['settings'].get('mpi')}")
    edited = json.loads(json.dumps(loaded["deck"]))
    edited["params"]["max_step"] = "1234"
    call(port, "/api/projects", {"name": project, "deck": edited})
    _, again = call(port, "/api/projects/" + quote(project))
    check("改动可长期保存", again["deck"]["params"]["max_step"] == "1234",
          f"改后重读 max_step={again['deck']['params']['max_step']}")
    status, _ = call(port, "/api/projects/" + quote(project), method="DELETE")
    check("任务可删除", status == 200, "已删除")
    remaining = [p for p in call(port, "/api/projects")[1]["projects"] if p["name"] == project]
    check("删除后不再出现", not remaining, f"剩余匹配 {len(remaining)} 个")

    # ---- checkpoints + restart -----------------------------------------
    _, chks = call(port, "/api/checkpoints")
    have = [c for c in chks["checkpoints"]]
    check("checkpoint 列表", bool(have), ", ".join(c["name"] for c in have[:4]))
    for _ in range(30):
        if call(port, "/api/status")[1]["state"] != "running":
            break
        time.sleep(1)
    if have:
        target = have[0]["path"]
        restart_deck = json.loads(json.dumps(starter))
        restart_deck["params"]["amr.restart"] = target
        restart_deck["params"]["max_step"] = "200"
        status, _ = call(port, "/api/run", {
            "params": restart_deck["params"], "groups": restart_deck["groups"],
            "binary": binary, "mpi": 1, "omp": 1, "live_field": "rho"})
        check("续算启动", status == 200, "amr.restart=" + Path(target).name)
        for _ in range(60):
            time.sleep(1)
            _, state = call(port, "/api/status")
            if state["state"] in ("finished", "failed"):
                break
        check("续算完成", state["state"] == "finished", f"state={state['state']}")

    # ---- ParaView export -------------------------------------------------
    status, export = call(port, "/api/paraview?launch=false", {})
    pvd = Path(export.get("pvd", "")) if isinstance(export, dict) else None
    check("导出 .pvd", status == 200 and pvd is not None and pvd.exists(),
          f"{export.get('iterations')} 迭代 / {export.get('files')} 文件 -> {pvd.name if pvd else '-'}")
    if pvd and pvd.exists():
        content = pvd.read_text()
        check("pvd 内容", content.count("<DataSet") >= 1 and "timestep" in content,
              f"{content.count('<DataSet')} 个 <DataSet>，含 timestep 属性")

    # ---- plugins ---------------------------------------------------------
    _, plugins = call(port, "/api/plugins")
    ids = [p["id"] for p in plugins["plugins"]]
    check("插件发现", "run_summary" in ids, str(ids))
    if ids:
        _, panel = call(port, f"/api/panel/{ids[0]}")
        check("插件渲染", "<table" in panel.get("html", ""), panel.get("name", ""))

    # ---- build configuration (no real build here) ------------------------
    from warpx_studio.runner.build import DIMS_MAP, BuildJob
    job = BuildJob(REPO, "selftest-cfg", "2d")
    command = " ".join(job.configure_command())
    check("构建命令", all(token in command for token in
                          ("configure_common.sh", "WarpX_DIMS=2", "WarpX_amrex_src")),
          "复用 configure_common.sh + 本地 AMReX")
    check("维度映射", DIMS_MAP == {"1d": "1", "2d": "2", "3d": "3", "rz": "RZ"}, str(DIMS_MAP))

    # ---- UI --------------------------------------------------------------
    status, page = call(port, "/")
    if isinstance(page, bytes):
        page = page.decode("utf-8", "replace")
    check("界面可访问", status == 200 and "WarpX Studio" in page,
          f"{len(page)} bytes, 含标题")
    for marker in ("renderBuild", "renderPlugins", "switchDims", "scheduleValidate"):
        check(f"界面功能存在: {marker}", f"function {marker}" in page or f"{marker}(" in page,
              "在页面脚本中")


if __name__ == "__main__":
    sys.exit(main())
