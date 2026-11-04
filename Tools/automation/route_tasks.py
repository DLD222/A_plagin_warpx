#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""Route task runner for Docs/research_notes/最终的技术路线.md.

Usage:
  python3 Tools/automation/route_tasks.py list
  python3 Tools/automation/route_tasks.py next
  python3 Tools/automation/route_tasks.py show A1.4
  python3 Tools/automation/route_tasks.py run A1.4 --gate
  python3 Tools/automation/route_tasks.py run A1.4 --gate --accept
  python3 Tools/automation/route_tasks.py check fast
  python3 Tools/automation/route_tasks.py mark A1.4 done

State is kept in .route_state.json (gitignored); logs in .route_logs/.
A `refactor` task is only marked done with --accept: a passing build/smoke does
not prove the refactor is complete.
"""
from __future__ import annotations
import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = ROOT / "Tools/automation/route_tasks.json"
STATE = ROOT / ".route_state.json"
LOGS = ROOT / ".route_logs"

GATES = {
    "fast": [
        ["python3", "Tools/automation/gates/compliance.py"],
        ["python3", "Tools/automation/gates/audit_check.py"],
    ],
    "build+smoke": [
        ["python3", "Tools/automation/gates/compliance.py"],
        ["python3", "Tools/automation/gates/audit_check.py"],
        ["bash", "Tools/automation/gates/build_2d.sh"],
        ["bash", "Tools/automation/gates/smoke_2d.sh"],
    ],
    "full": [["bash", "Tools/automation/gates/full_gate.sh"]],
}


def load_spec() -> dict:
    spec = json.loads(SPEC.read_text())
    ids = {t["id"] for t in spec["tasks"]}
    for t in spec["tasks"]:
        for dep in t["depends"]:
            if dep not in ids:
                raise SystemExit(f"{t['id']}: unknown dependency {dep}")
    # cycle check
    visiting, done = set(), set()

    def visit(tid: str) -> None:
        if tid in done:
            return
        if tid in visiting:
            raise SystemExit(f"dependency cycle at {tid}")
        visiting.add(tid)
        t = next(x for x in spec["tasks"] if x["id"] == tid)
        for dep in t["depends"]:
            visit(dep)
        visiting.remove(tid)
        done.add(tid)

    for t in spec["tasks"]:
        visit(t["id"])
    return spec


def load_state(spec: dict) -> dict:
    if STATE.exists():
        return json.loads(STATE.read_text())
    return {t["id"]: {"status": t["status"]} for t in spec["tasks"]}


def save_state(state: dict) -> None:
    STATE.write_text(json.dumps(state, indent=1, ensure_ascii=False) + "\n")


def is_done(state: dict, tid: str) -> bool:
    return state.get(tid, {}).get("status") == "done"


def select(spec: dict, state: dict, phase: str | None, status: str | None):
    for t in spec["tasks"]:
        if phase and t["phase"] != phase:
            continue
        if status and state[t["id"]]["status"] != status:
            continue
        yield t


def cmd_list(spec, state, args):
    phase = args.phase
    status = args.status
    for t in select(spec, state, phase, status):
        deps = ",".join(t["depends"]) or "-"
        print(f"{t['id']:<5} {state[t['id']]['status']:<8} {t['kind']:<9} deps={deps:<18} {t['title']}")


def cmd_next(spec, state, args):
    for t in spec["tasks"]:
        if state[t["id"]]["status"] == "done":
            continue
        if all(is_done(state, d) for d in t["depends"]):
            print(f"{t['id']}  {t['title']}")
            print(f"  kind: {t['kind']}  gate: {t['gate']}  effort: {t['effort_days']} d")
            if t.get("automation_command"):
                print(f"  command: {t['automation_command']}")
            if t.get("acceptance"):
                print(f"  acceptance: {t['acceptance']}")
            return
    print("no pending task with satisfied dependencies")


def cmd_show(spec, state, args):
    t = next((x for x in spec["tasks"] if x["id"] == args.id), None)
    if not t:
        raise SystemExit(f"unknown task {args.id}")
    print(json.dumps({**t, "state": state[t["id"]]}, indent=1, ensure_ascii=False))


def run_step(cmd: list[str], log) -> bool:
    log.write(f"$ {' '.join(cmd)}\n")
    log.flush()
    p = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    log.write(p.stdout)
    log.flush()
    print(p.stdout, end="")
    return p.returncode == 0


def cmd_check(spec, state, args):
    level = args.level
    ok = True
    LOGS.mkdir(exist_ok=True)
    with (LOGS / f"check-{level}.log").open("w") as log:
        for cmd in GATES[level]:
            if not run_step(cmd, log):
                ok = False
                break
    print(f"gate {level}: {'PASS' if ok else 'FAIL'} (log: .route_logs/check-{level}.log)")
    return 0 if ok else 1


def cmd_run(spec, state, args):
    t = next((x for x in spec["tasks"] if x["id"] == args.id), None)
    if not t:
        raise SystemExit(f"unknown task {args.id}")
    if state[t["id"]]["status"] == "done" and not args.force:
        print(f"{t['id']} already done (use --force to rerun)")
        return 0
    for dep in t["depends"]:
        if not is_done(state, dep):
            raise SystemExit(f"{t['id']}: dependency {dep} is not done")

    LOGS.mkdir(exist_ok=True)
    log_path = LOGS / f"{t['id']}.log"
    ok = True
    with log_path.open("w") as log:
        if t.get("automation_command"):
            ok = run_step(["bash", "-lc", t["automation_command"]], log) and ok
        else:
            print(f"{t['id']} is a {t['kind']} task: implement it, then run the gate with --gate.")
        if args.gate:
            level = t["gate"]
            for cmd in GATES[level]:
                if not run_step(cmd, log):
                    ok = False
                    break

    if ok and (t["kind"] != "refactor" or args.accept or t.get("automation_command")):
        state[t["id"]] = {"status": "done", "at": time.strftime("%Y-%m-%dT%H:%M:%S"), "log": str(log_path)}
        save_state(state)
        print(f"{t['id']}: marked done")
    elif ok and t["kind"] == "refactor":
        print(f"{t['id']}: gate passed; not marked done (pass --accept after reviewing the diff)")
    else:
        print(f"{t['id']}: gate/command failed (log: {log_path})")
    return 0 if ok else 1


def cmd_report(spec, state, args):
    lines = ["# Route status", "", "<!-- SPDX-License-Identifier: BSD-3-Clause -->", "",
             "> Generated by `Tools/automation/route_tasks.py report --write`; do not edit by hand.", "",
             "| 任务 | 阶段 | 状态 | 依赖 | 标题 |", "|---|---|---|---|---|"]
    for t in spec["tasks"]:
        deps = ",".join(t["depends"]) or "-"
        lines.append(f"| {t['id']} | {t['phase']} | {state[t['id']]['status']} | {deps} | {t['title']} |")
    nxt = next((t for t in spec["tasks"] if state[t["id"]]["status"] != "done"
                and all(is_done(state, d) for d in t["depends"])), None)
    lines += ["", f"**Next:** `{nxt['id']}` — {nxt['title']}" if nxt else "**Next:** none", ""]
    text = "\n".join(lines) + "\n"
    if args.write:
        (ROOT / "Docs/research_notes/ROUTE_STATUS.md").write_text(text)
        print("wrote Docs/research_notes/ROUTE_STATUS.md")
    else:
        print(text, end="")


def cmd_mark(spec, state, args):
    if args.id not in state:
        raise SystemExit(f"unknown task {args.id}")
    state[args.id] = {"status": args.status, "at": time.strftime("%Y-%m-%dT%H:%M:%S")}
    save_state(state)
    print(f"{args.id}: {args.status}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("list")
    p.add_argument("--phase")
    p.add_argument("--status")
    p.set_defaults(func=cmd_list)

    p = sub.add_parser("next")
    p.set_defaults(func=cmd_next)

    p = sub.add_parser("show")
    p.add_argument("id")
    p.set_defaults(func=cmd_show)

    p = sub.add_parser("check")
    p.add_argument("level", choices=sorted(GATES))
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("run")
    p.add_argument("id")
    p.add_argument("--gate", action="store_true")
    p.add_argument("--accept", action="store_true")
    p.add_argument("--force", action="store_true")
    p.set_defaults(func=cmd_run)

    p = sub.add_parser("report")
    p.add_argument("--write", action="store_true")
    p.set_defaults(func=cmd_report)

    p = sub.add_parser("mark")
    p.add_argument("id")
    p.add_argument("status", choices=["pending", "done"])
    p.set_defaults(func=cmd_mark)

    args = ap.parse_args()
    spec = load_spec()
    state = load_state(spec)
    # keep JSON's "done" entries authoritative on first run
    for t in spec["tasks"]:
        if t["status"] == "done" and t["id"] not in state:
            state[t["id"]] = {"status": "done"}
    save_state(state)
    return args.func(spec, state, args)


if __name__ == "__main__":
    sys.exit(main())
