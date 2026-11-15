"""Example plugin: a compact summary of the current run (copy it to write your own)."""

NAME = "运行摘要"


def panel(context: dict) -> str:
    import html

    job = context.get("job") or {}
    live = context.get("live") or {}
    meta = live.get("meta") or {}
    run_dir = context.get("run_dir") or "（还没有运行）"
    rows = [
        ("运行目录", str(run_dir)),
        ("状态", str(job.get("state", "idle"))),
        ("已走步数", f"{job.get('step', 0)}" + (f" / {job['max_step']}" if job.get("max_step") else "")),
        ("物理时间", f"{job.get('time', 0.0):.6g} s" if job else "-"),
        ("当前 dt", f"{job.get('dt', 0.0):.6g} s" if job else "-"),
        ("最近一帧", f"iteration {live.get('iteration')} · {meta.get('ms', '-')} ms"
                     if live.get("iteration") is not None else "尚未渲染"),
        ("渲染色标", meta.get("summary", "-")),
        ("网格", str((context.get("params") or {}).get("amr.n_cell", "-"))),
    ]
    body = "".join(f"<tr><th style='text-align:left;color:#9aa3b5;padding:2px 10px 2px 0'>"
                   f"{html.escape(str(key))}</th><td>{html.escape(str(value))}</td></tr>"
                   for key, value in rows)
    return f"<table style='font-size:13px'>{body}</table>"
