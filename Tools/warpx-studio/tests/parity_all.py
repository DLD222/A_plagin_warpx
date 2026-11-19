"""把 WarpX 自带的**全部**官方算例跑一遍（输入文本 + 输出数据双重比对）。

特点
  * 可断点续跑：每跑完一个算例就追加一行 JSON 到状态文件，重启不会重跑
  * 分类跳过：inputs_base_* 片段、PICMI、需要外部数据文件、plot_raw_fields 的算例
  * 单线程 + 固定随机种子 + 相对容差（默认 1e-10），全部经网页 API 运行

用法
  python tests/parity_all.py --state /tmp/parity_all.jsonl [--timeout 300] [--limit N]
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys
import time

REPO = pathlib.Path(__file__).resolve().parents[3]
RUNNER = REPO / "Tools/warpx-studio/tests/parity_run.py"

NEEDS_EXTERNAL = re.compile(
    r"load_density|read_from_file|injection_file|read_[a-z_]*_from_path|\.h5\b|openpmd_\*")
SKIP_NAME = re.compile(r"(_picmi|\.py$)")


def classify(f: pathlib.Path) -> tuple[str, str]:
    """返回 (维度, 跳过原因)；不可跑时维度为 '-'。"""
    if f.suffix == ".py" or "_picmi" in f.name:
        return "-", "PICMI/脚本"
    if f.name.startswith("inputs_base"):
        return "-", "基础片段"
    text = f.read_text(errors="ignore")
    for name in re.findall(r"^\s*FILE\s*=\s*(\S+)", text, re.M):
        base = f.parent / name
        if base.exists():
            text = base.read_text(errors="ignore") + "\n" + text
    if NEEDS_EXTERNAL.search(text):
        return "-", "需外部数据"
    if re.search(r"plot_raw_fields\s*=\s*1", text):
        return "-", "需 plotfile"
    m = re.search(r"geometry\.dims\s*=\s*(\d|RZ|rz)", text)
    if not m:
        return "-", "无 geometry.dims"
    dim = {"1": "1d", "2": "2d", "3": "3d", "RZ": "rz", "rz": "rz"}[m.group(1)]
    return dim, ""


def ensure_server() -> None:
    """确保 Studio 服务在跑：不在就拉起来（长时间批量跑必须能自愈）。"""
    import socket
    for _ in range(20):
        try:
            with socket.create_connection(("127.0.0.1", 8770), timeout=1):
                return
        except OSError:
            subprocess.Popen(
                [sys.executable, "-m", "uvicorn", "warpx_studio.web.app:app",
                 "--host", "127.0.0.1", "--port", "8770"],
                cwd=str(REPO / "Tools/warpx-studio"),
                env={**__import__("os").environ,
                     "PYTHONPATH": str(REPO / "Tools/warpx-studio")},
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                start_new_session=True)
            time.sleep(3)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--state", default="/tmp/parity_all.jsonl")
    ap.add_argument("--timeout", type=int, default=420)
    ap.add_argument("--limit", type=int, default=0, help="只跑前 N 个（0 = 全部）")
    ap.add_argument("--tolerance", type=float, default=1e-10)
    args = ap.parse_args()

    state = pathlib.Path(args.state)
    done = set()
    if state.exists():
        for line in state.read_text(errors="ignore").splitlines():
            try:
                done.add(json.loads(line)["example"])
            except Exception:
                continue
    files = sorted(p for p in (REPO / "Examples").rglob("inputs*") if p.is_file())
    todo = []
    for f in files:
        rel = str(f.relative_to(REPO))
        if rel in done:
            continue
        dim, why = classify(f)
        todo.append((rel, dim, why))
    if args.limit:
        todo = todo[: args.limit]
    print(f"总计 {len(files)} 个输入，已完成 {len(done)}，本次待跑 {len(todo)}", flush=True)

    counts = {"pass": 0, "fail": 0, "skip": 0}
    with state.open("a", encoding="utf-8") as out:
        for rel, dim, why in todo:
            if dim == "-":
                out.write(json.dumps({"example": rel, "status": "skip", "reason": why},
                                     ensure_ascii=False) + "\n")
                out.flush(); counts["skip"] += 1
                print(f"SKIP {rel} ({why})", flush=True)
                continue
            ensure_server()
            cmd = [sys.executable, str(RUNNER), "--examples", rel, "--dim", dim,
                   "--tolerance", str(args.tolerance)]
            try:
                r = subprocess.run(cmd, capture_output=True, text=True, timeout=args.timeout)
                rc = r.returncode
                detail = (r.stdout or "").strip().splitlines()[-4:]
            except subprocess.TimeoutExpired:
                rc, detail = 124, ["超时"]
            status = "pass" if rc == 0 else ("skip" if rc == 3 else "fail")
            counts[status] += 1
            out.write(json.dumps({"example": rel, "dim": dim, "status": status,
                                  "rc": rc, "detail": detail}, ensure_ascii=False) + "\n")
            out.flush()
            print(f"{status.upper()} {rel}", flush=True)
    print(json.dumps(counts, ensure_ascii=False), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
