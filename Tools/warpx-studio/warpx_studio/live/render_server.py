#!/usr/bin/env python3
"""Resident render worker: JSON request per line on stdin, JSON reply per line.

Starting ``pvpython`` costs about two seconds, which dominates a live view that
redraws every few seconds.  This worker imports ParaView once and then answers
requests in a fraction of a second.  It is started by :mod:`warpx_studio.live.service`
and never talks to anything else.
"""
from __future__ import annotations

import json
import os
import sys


def _streams():
    """Return (stdin, stdout) as plain text streams.

    ``pvpython`` replaces ``sys.stdin``/``sys.stdout`` with VTK capture helpers that are
    not iterable and swallow output, so the worker talks to the real file descriptors.
    """
    try:
        return (os.fdopen(os.dup(0), "r", encoding="utf-8", buffering=1),
                os.fdopen(os.dup(1), "w", encoding="utf-8", buffering=1))
    except OSError:
        return sys.stdin, sys.stdout


def main() -> int:
    sys.path.insert(0, str(__import__("pathlib").Path(__file__).resolve().parents[2]))
    from warpx_studio.live.render_core import Renderer

    renderer = Renderer()
    stdin, stdout = _streams()
    for line in stdin:
        line = line.strip()
        if not line:
            continue
        try:
            request = json.loads(line)
        except json.JSONDecodeError as exc:
            stdout.write(json.dumps({"ok": False, "error": f"bad request: {exc}"}) + "\n")
            stdout.flush()
            continue
        if request.get("quit"):
            stdout.write(json.dumps({"ok": True, "bye": True}) + "\n")
            stdout.flush()
            return 0
        try:
            reply = renderer.render(request)
        except Exception as exc:                      # a bad frame must not kill the worker
            reply = {"ok": False, "error": f"{type(exc).__name__}: {exc}"}
        stdout.write(json.dumps(reply) + "\n")
        stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
