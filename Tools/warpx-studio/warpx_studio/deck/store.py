"""Named projects: a deck plus its run settings, saved on disk and editable later.

The form used to live only in the browser tab: closing it lost everything except the
``inputs`` file of a finished run.  A project is the missing "document":

    audit_runs/studio_projects/<slug>/project.json   {name, deck, settings, timestamps}
    audit_runs/studio_projects/<slug>/inputs         a human-readable snapshot

``audit_runs/`` is gitignored, so projects are user data, not repository content.  Saving
is an explicit action from the UI; ``updated`` is bumped every time, and the last opened
project is remembered so a restarted browser comes back to where it was.
"""
from __future__ import annotations

import hashlib
import json
import re
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOT = REPO / "audit_runs/studio_projects"
LAST = REPO / "audit_runs/studio_projects/.last"


# Symbols that carry meaning in a project name but are not filesystem-safe.  Without this
# mapping `B∥` and `B⊥` both collapse to `B-`, so two projects silently overwrite each other.
SLUG_SYMBOLS = {"∥": "par", "⊥": "perp", "‖": "par", "×": "x", "→": "to"}


def slug(name: str) -> str:
    """Filesystem-safe directory name that stays readable."""
    text = name or ""
    for symbol, word in SLUG_SYMBOLS.items():
        text = text.replace(symbol, word)
    cleaned = re.sub(r"[^0-9A-Za-z\u4e00-\u9fff._-]+", "-", text.strip())
    cleaned = cleaned.strip("-._")[:60]
    return cleaned or f"project-{int(time.time())}"


def _owner(folder_name: str) -> str | None:
    manifest = ROOT / folder_name / "project.json"
    if not manifest.is_file():
        return None
    try:
        return json.loads(manifest.read_text(encoding="utf-8")).get("name")
    except (OSError, json.JSONDecodeError):
        return None


def _hash_suffix(name: str) -> str:
    return hashlib.sha1(name.encode("utf-8")).hexdigest()[:6]


def _existing_folder(name: str) -> str | None:
    """Folder that already holds `name`, if any.

    `slug()` is lossy (`a b` and `a-b` both give `a-b`), so a project may live in the
    plain folder or in the hash-suffixed one.  Checking both keeps lookups independent of
    whether the *other* project with the same slug still exists.
    """
    base = slug(name)
    for candidate in (base, f"{base}-{_hash_suffix(name)}"):
        if _owner(candidate) == name:
            return candidate
    return None


def _project_file(name: str) -> Path:
    folder = _existing_folder(name)
    if folder is None:
        base = slug(name)
        # The plain folder is free, or it belongs to a different project: in the latter
        # case use a name-derived suffix instead of overwriting that project.
        folder = base if _owner(base) is None else f"{base}-{_hash_suffix(name)}"
    return ROOT / folder / "project.json"


def list_projects() -> list[dict]:
    if not ROOT.is_dir():
        return []
    out = []
    for path in sorted(ROOT.glob("*/project.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        deck = data.get("deck") or {}
        params = deck.get("params") or {}
        groups = deck.get("groups") or {}
        out.append({
            "name": data.get("name") or path.parent.name,
            "slug": path.parent.name,
            "updated": data.get("updated"),
            "created": data.get("created"),
            "dims": params.get("geometry.dims", "?"),
            "species": len(groups.get("<species_name>", {})),
            "diags": len(groups.get("<diag_name>", {})),
            "notes": data.get("notes", ""),
            "last_run": data.get("last_run"),
        })
    out.sort(key=lambda item: item.get("updated") or "", reverse=True)
    return out


def load(name: str) -> dict:
    path = _project_file(name)
    if not path.is_file():
        raise FileNotFoundError(name)
    return json.loads(path.read_text(encoding="utf-8"))


def save(name: str, deck: dict, settings: dict | None = None,
         notes: str | None = None, inputs: str | None = None) -> dict:
    path = _project_file(name)
    folder = path.parent
    folder.mkdir(parents=True, exist_ok=True)
    now = time.strftime("%Y-%m-%dT%H:%M:%S")
    try:
        previous = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        previous = {}
    data = {
        "name": name,
        "created": previous.get("created") or now,
        "updated": now,
        "deck": {"params": deck.get("params", {}), "groups": deck.get("groups", {})},
        "settings": settings or previous.get("settings") or {},
        "notes": notes if notes is not None else previous.get("notes", ""),
        "last_run": previous.get("last_run"),
        "runs": previous.get("runs", []),
    }
    path.write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    if inputs:
        (folder / "inputs").write_text(inputs, encoding="utf-8")
    set_last(name)
    return data


def delete(name: str) -> bool:
    path = _project_file(name)
    if not path.is_file():
        return False
    folder = path.parent
    for item in folder.iterdir():
        item.unlink(missing_ok=True)
    folder.rmdir()
    return True


def record_run(name: str, run_dir: str) -> None:
    """Remember where the last simulation of this project wrote its output."""
    try:
        data = load(name)
    except FileNotFoundError:
        return
    data["last_run"] = run_dir
    data.setdefault("runs", []).append({"dir": run_dir, "at": time.strftime("%Y-%m-%dT%H:%M:%S")})
    data["runs"] = data["runs"][-20:]
    _project_file(name).write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n",
                                   encoding="utf-8")


def set_last(name: str) -> None:
    LAST.parent.mkdir(parents=True, exist_ok=True)
    LAST.write_text(name, encoding="utf-8")


def last_opened() -> str | None:
    try:
        return LAST.read_text(encoding="utf-8").strip() or None
    except OSError:
        return None
