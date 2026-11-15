"""Configure and build WarpX from the web UI, streaming progress.

The studio never inherits an unknown toolchain: it configures a fresh build directory
under ``audit_runs/studio_builds/`` with an explicit option set, reuses the AMReX source
that is already in the checkout (no network), and reports the compiler error line by
line.  One build at a time, like one run at a time.
"""
from __future__ import annotations

import os
import subprocess
import threading
import time
from pathlib import Path

BUILD_ROOT = "audit_runs/studio_builds"

# WarpX accepts 1/2/3/RZ (uppercase), the UI talks about 1d/2d/3d/rz.
DIMS_MAP = {"1d": "1", "2d": "2", "3d": "3", "rz": "RZ"}


def clean_env() -> dict:
    """Environment for cmake: the repo's own builds use the system toolchain, not conda's.

    Without this, a server started from a conda environment configures WarpX with
    ``x86_64-conda-linux-gnu-gcc`` and the resulting binary is not the one the rest of the
    project is tested with.
    """
    drop = ("CC", "CXX", "CFLAGS", "CXXFLAGS", "CPPFLAGS", "LDFLAGS", "LDFLAGS_LD",
            "CMAKE_ARGS", "CMAKE_PREFIX_PATH", "LD_LIBRARY_PATH", "LIBRARY_PATH",
            "CPATH", "CONDA_BUILD_SYSROOT", "CONDA_PREFIX")
    env = {k: v for k, v in os.environ.items() if k not in drop}
    env["PATH"] = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
    # same as audit_build/cleanenv.sh: the local FFTW ships a pkg-config file
    env["PKG_CONFIG_PATH"] = str(Path.home() / "third_party/fftw/lib/pkgconfig")
    return env


class BuildJob:
    def __init__(self, repo: Path, name: str, dims: str, options: dict | None = None,
                 jobs: int = 8) -> None:
        self.repo = repo
        self.name = name
        self.dims = dims
        self.options = options or {}
        self.jobs = max(1, jobs)
        self.dir = repo / BUILD_ROOT / name
        self.state = "created"
        self.log: list[str] = []
        self.errors: list[str] = []
        self.started_at = time.time()
        self.finished_at: float | None = None
        self._lock = threading.Lock()

    # -- command -----------------------------------------------------------
    def configure_command(self) -> list[str]:
        """Configure exactly like the project's own audit builds do.

        ``audit_build/configure_common.sh`` is this checkout's authoritative flag list
        (local AMReX/PICSAR/openPMD sources, HDF5 and FFTW roots, MPI compilers).  Repeating
        those flags by hand drifted twice - abstract particle containers from a missing
        plugin switch, then a missing FFTW root - so the button sources that file instead.
        """
        dims = DIMS_MAP.get(self.dims.lower(), self.dims)
        overrides = " ".join(f"-D{key}={value}" for key, value in self.options.items())
        script = (
            f"set -e; "
            f"DEPS={self.repo}/audit_build/deps; "
            f"source {self.repo}/audit_build/configure_common.sh; "
            f"cmake -S {self.repo} -B {self.dir} \"${{COMMON[@]}}\" "
            f"-DWarpX_DIMS={dims} -DWarpX_PYTHON=OFF -DWarpX_PYTHON_IPO=OFF "
            # configure_common.sh passes picsar/openpmd sources but not AMReX:
            # without this the configure step downloads AMReX from GitHub.
            f"-DWarpX_amrex_src={self.repo}/audit_build/deps/amrex-008b6737 "
            # the openPMD superbuild would otherwise download these two tarballs
            + "".join(
                f"-DFETCHCONTENT_SOURCE_DIR_{var}={self.repo}/audit_build/deps/{name} "
                for var, name in (("FETCHEDNLOHMANN_JSON", "nlohmann_json-3.12.0"),
                                  ("FETCHEDTOML11", "toml11-4.2.0"))
                if (self.repo / "audit_build/deps" / name).is_dir())
            +
            f"{overrides}"
        )
        return ["bash", "-lc", script]

    # -- lifecycle ---------------------------------------------------------
    def start(self) -> None:
        self.dir.mkdir(parents=True, exist_ok=True)
        thread = threading.Thread(target=self._run, daemon=True)
        thread.start()

    def _run(self) -> None:
        for phase, command in (("configure", self.configure_command()),
                               ("build", ["cmake", "--build", str(self.dir),
                                          "-j", str(self.jobs)])):
            self.state = phase
            self._append(f"$ {' '.join(command)}")
            try:
                process = subprocess.Popen(command, cwd=self.repo, text=True,
                                           env=clean_env(),
                                           stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT,
                                           start_new_session=True)
            except FileNotFoundError:
                self._append("cmake 不在 PATH 里")
                self.state = "failed"
                self.errors.append("找不到 cmake（本机是否装了 CMake？）")
                self.finished_at = time.time()
                return
            assert process.stdout is not None
            for line in process.stdout:
                self._append(line.rstrip("\n"))
            code = process.wait()
            if code != 0:
                self.state = "failed"
                self.finished_at = time.time()
                return
        self.state = "finished"
        self.finished_at = time.time()
        binaries = sorted(self.dir.glob("bin/warpx.*"))
        self._append(f"built {len(binaries)} binary/binaries: "
                     + ", ".join(b.name for b in binaries))

    def _append(self, line: str) -> None:
        with self._lock:
            self.log.append(line)
            if len(self.log) > 6000:
                del self.log[:2000]
            low = line.lower()
            if "error:" in low or "error :" in low or "fatal error" in low:
                self.errors.append(line.strip()[:400])

    def stop(self) -> None:
        self.state = "stopped"

    def snapshot(self, tail: int = 60) -> dict:
        with self._lock:
            return {"name": self.name, "dims": self.dims, "dir": str(self.dir),
                    "state": self.state, "errors": self.errors[-5:],
                    "log": self.log[-tail:],
                    "elapsed": round((self.finished_at or time.time()) - self.started_at, 1)}
