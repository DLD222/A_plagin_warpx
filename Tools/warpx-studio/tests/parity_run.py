"""输出数据一致性验收（验收项 5 的数据部分）。

重要发现：**官方二进制默认不可复现**——同一条命令连跑两次，openPMD 里的数组都不一样
（实测 max|Δ| 达 1e-3 量级），原因是默认随机种子每次不同。所以"输出完全相同"必须先固定
`warpx.random_seed`；本脚本两边用同一个种子（--seed），固定后实测两次运行逐位相同。


流程（全部走网页 API，不用命令行直接跑）：
  1. 读 Examples 里指定的官方 inputs 文件，用 Studio 的 Deck 解析
  2. 通过 POST /api/run 让**主页**用 fork 的可执行文件跑一遍
  3. 同一个 inputs 用**原版**可执行文件（official_2611）跑一遍作基准
  4. 逐字段比较 openPMD 输出：形状相同且数组**逐位相同**才算通过

用法:
  python tests/parity_run.py --examples a,b,c --dim 2d [--tolerance 0]
"""
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
import time
import urllib.request

import h5py
import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from warpx_studio.deck.model import Deck  # noqa: E402

BASE = "http://127.0.0.1:8770"
REPO = pathlib.Path(__file__).resolve().parents[3]


def inline_file_directive(text: str, origin: pathlib.Path) -> str:
    """把 `FILE = 基础文件` 展开成"基础文件内容 + 当前文件内容"。

    WarpX 支持 FILE 指令（相对 inputs 所在目录解析），但网页跑的时候运行目录里只有
    一个 inputs，基础文件不在，所以要在这里内联。后出现的赋值覆盖前面的，因此
    "基础在前、测试在后"与 FILE 语义等价。
    """
    lines = text.splitlines()
    base_names = []
    kept = []
    for line in lines:
        stripped = line.split("#", 1)[0].strip()
        if "=" in stripped:
            key, value = stripped.split("=", 1)
            if key.strip().upper() == "FILE":
                base_names.extend(value.split())
                continue
        kept.append(line)
    parts = []
    for name in base_names:
        candidate = origin.parent / name
        if candidate.exists():
            parts.append(candidate.read_text(errors="ignore"))
        else:
            parts.append(f"# 未找到基础文件 {name}")
    parts.append("\n".join(kept))
    return "\n".join(parts)


def post(path: str, body: dict) -> dict:
    req = urllib.request.Request(BASE + path, data=json.dumps(body).encode(),
                                 headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=120) as r:
        return json.loads(r.read())


def get(path: str) -> dict:
    with urllib.request.urlopen(BASE + path, timeout=60) as r:
        return json.loads(r.read())


def run_via_web(deck: Deck, binary: pathlib.Path) -> pathlib.Path:
    post("/api/stop", {})
    time.sleep(0.5)
    body = {"params": dict(deck.params),
            "groups": {t: {i: dict(v) for i, v in inst.items()} for t, inst in deck.groups.items()},
            "binary": str(binary), "force": True, "mpi": 1, "omp": 2}
    out = post("/api/run", body)
    run_dir = pathlib.Path(out["run_dir"])
    deadline = time.time() + 1800
    while time.time() < deadline:
        st = get("/api/status")
        if st.get("state") in ("finished", "failed", "stopped"):
            break
        time.sleep(1.0)
    return run_dir


def find_h5(run_dir: pathlib.Path) -> list[pathlib.Path]:
    return sorted(run_dir.rglob("openpmd_*.h5"))


def compare(a: pathlib.Path, b: pathlib.Path, tol: float) -> list[str]:
    """逐字段比较两个 openPMD 文件，返回差异列表（空 = 完全一致）。"""
    diffs: list[str] = []
    with h5py.File(a) as fa, h5py.File(b) as fb:
        ita, itb = list(fa["data"].keys())[0], list(fb["data"].keys())[0]
        ga, gb = fa[f"data/{ita}/fields"], fb[f"data/{itb}/fields"]

        def walk(g, prefix=""):
            out = {}
            for k in g:
                node = g[k]
                if isinstance(node, h5py.Dataset):
                    out[prefix + k] = np.array(node)
                elif isinstance(node, h5py.Group):
                    out.update(walk(node, prefix + k + "/"))
            return out

        # 矢量字段是分组，标量是数据集：统一展开
        fa_fields, fb_fields = {}, {}
        for k in ga:
            node = ga[k]
            if isinstance(node, h5py.Dataset):
                fa_fields[k] = np.array(node)
                fb_fields[k] = np.array(gb[k]) if k in gb else None
            else:
                for c in node:
                    fa_fields[f"{k}/{c}"] = np.array(node[c])
                    fb_fields[f"{k}/{c}"] = (np.array(gb[k][c])
                                             if k in gb and c in gb[k] else None)
        for key, va in fa_fields.items():
            vb = fb_fields.get(key)
            if vb is None:
                diffs.append(f"缺少字段 {key}")
                continue
            if va.shape != vb.shape:
                diffs.append(f"{key} 形状 {va.shape} != {vb.shape}")
                continue
            if tol == 0.0:
                if not np.array_equal(va, vb):
                    n = int(np.sum(va != vb))
                    diffs.append(f"{key} 有 {n} 个元素不同（max|Δ|={np.max(np.abs(va - vb)):.3e}）")
            else:
                err = np.max(np.abs(va - vb)) if va.size else 0.0
                if err > tol:
                    diffs.append(f"{key} max|Δ|={err:.3e} > {tol:.1e}")
    return diffs


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--examples", nargs="*", default=[])
    ap.add_argument("--dim", default="2d")
    ap.add_argument("--tolerance", type=float, default=0.0)
    ap.add_argument("--seed", type=int, default=20261119)
    ap.add_argument("--force-openpmd", action="store_true", default=True)
    ap.add_argument("--no-force-openpmd", dest="force_openpmd", action="store_false")
    args = ap.parse_args()

    fork = REPO / f"audit_build/fork/bin/warpx.{args.dim}"
    ref = REPO / f"audit_build/official_2611/bin/warpx.{args.dim}"
    if not fork.exists() or not ref.exists():
        print(f"缺少可执行文件: {fork} / {ref}")
        return 2

    results = []
    for name in args.examples:
        src = REPO / name
        if not src.exists():
            results.append((name, ["输入文件不存在"]))
            continue
        raw = inline_file_directive(src.read_text(errors="ignore"), src)
        deck = Deck.from_inputs(raw)
        # 官方二进制默认随机种子每跑不同（同一条命令两次结果都不一样，已实测），
        # 所以比较前必须固定种子，否则"完全一致"无从谈起。两边用同一个值。
        if "warpx.random_seed" not in deck.params:
            deck.params["warpx.random_seed"] = str(args.seed)
        if args.force_openpmd:
            # 只用 openPMD 才能逐数组比较；这只改诊断格式，不动物理参数。
            diags = str(deck.params.get("diagnostics.diags_names", "")).strip().split()
            for diag_name in (diags or ["diag1"]):
                deck.params[f"{diag_name}.format"] = "openpmd"
        run_fork = run_via_web(deck, fork)
        run_ref = run_via_web(deck, ref)
        h5_fork, h5_ref = find_h5(run_fork), find_h5(run_ref)
        if not h5_fork or not h5_ref:
            results.append((name, [f"缺少输出 fork={len(h5_fork)} ref={len(h5_ref)}"]))
            continue
        diffs = compare(h5_ref[-1], h5_fork[-1], args.tolerance)
        results.append((name, diffs))

    ok = sum(1 for _, d in results if not d)
    for name, diffs in results:
        print(("PASS " if not diffs else "FAIL ") + name)
        for d in diffs[:4]:
            print("    ", d)
    print(f"\n{ok}/{len(results)} 个算例输出完全一致")
    return 0 if ok == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
