"""Pluggable panels: drop a module in this directory (or in ``~/.config/warpx-studio/plugins``).

A plugin module only has to expose

    NAME = "显示名"
    def panel(context: dict) -> str: ...      # HTML fragment

``context`` carries ``run_dir`` (str | None), ``job`` (the /api/status snapshot or None),
``live`` (the live-view snapshot) and ``params`` (the flat deck keys).  Exceptions raised by
a plugin are shown inside its panel, so a broken plugin cannot take the server or another
plugin down.
"""
from __future__ import annotations

import importlib.util
import traceback
from pathlib import Path

HERE = Path(__file__).resolve().parent
USER_DIR = Path.home() / ".config/warpx-studio/plugins"


def _load(path: Path):
    spec = importlib.util.spec_from_file_location(f"warpx_studio_plugin_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def discover() -> list[dict]:
    """Return [{id, name, path}] for every plugin that has a NAME and a panel()."""
    found: dict[str, dict] = {}
    for directory in (HERE, USER_DIR):
        if not directory.is_dir():
            continue
        for path in sorted(directory.glob("*.py")):
            if path.name.startswith("_"):
                continue
            try:
                module = _load(path)
            except Exception:
                continue
            if not (hasattr(module, "NAME") and callable(getattr(module, "panel", None))):
                continue
            found.setdefault(path.stem, {"id": path.stem, "name": module.NAME,
                                         "path": str(path), "source": directory.name})
    return sorted(found.values(), key=lambda item: item["name"])


def render(plugin_id: str, context: dict) -> dict:
    """Render one plugin panel; never raises."""
    for entry in discover():
        if entry["id"] != plugin_id:
            continue
        try:
            module = _load(Path(entry["path"]))
            return {"id": plugin_id, "name": entry["name"], "html": module.panel(context)}
        except Exception as exc:
            detail = traceback.format_exc(limit=3)
            return {"id": plugin_id, "name": entry["name"],
                    "html": f'<div class="warn bad">插件出错：{type(exc).__name__}: {exc}'
                            f'<pre style="font-size:11px">{detail}</pre></div>'}
    return {"id": plugin_id, "name": plugin_id, "html": '<div class="warn bad">没有这个插件</div>'}
