"""Run one WarpX simulation as a supervised child process.

The UI needs four things from a run: live progress, the log, a way to stop it, and
a way to pause it.  WarpX prints ``STEP n starts ...`` and
``STEP n ends. TIME = ... DT = ...`` per step, which is turned into a progress
record here instead of leaving the user to read a scrolling log.
"""
from __future__ import annotations

import json
import os
import re
import shutil
import signal
import subprocess
import threading
import time
from pathlib import Path

STEP_START = re.compile(r"^STEP (\d+) starts")
STEP_END = re.compile(r"^STEP (\d+) ends\.\s*TIME = (\S+)\s*DT = (\S+)")
ERROR = re.compile(r"^### ERROR\s*:\s*(.+)$")
# A required key that is missing is reported *before* the abort, without a "### ERROR"
# line.  Three spellings are seen in practice:
#   ParmParse::getval(): electrons..profile not found in database
#   ParmParse::sgetarr ParmParse::sgetarr(): electrons..density_function(x,y,z) not found ...
#   electrons.profile not found in database
# The old regex only matched the first, so the UI showed the MPI abort tail instead of
# the actionable "函数参数表少写坐标" hint for the density_function case.
MISSING = re.compile(r"(\S+)\s+not found in database")
ABORT = re.compile(r"amrex::Abort|MPI_ABORT")

# 这些是 openPMD 自己的根属性，用户文本不能覆盖，否则文件不再是合法 openPMD。
HDF5_RESERVED = {
    "openPMD", "openPMDextension", "basePath", "meshesPath", "particlesPath",
    "iterationEncoding", "iterationFormat", "software", "softwareVersion", "date",
}


def apply_hdf5_metadata(run_dir: Path, metadata: dict | None) -> int:
    """把网页里填的自定义根属性写进本次运行的所有 openpmd_*.h5。

    只写 HDF5 attributes；不创建/修改物理场。写失败只记录，不影响算例状态。
    """
    if not isinstance(metadata, dict) or not metadata:
        return 0
    attrs = metadata.get("attributes") if isinstance(metadata.get("attributes"), dict) else metadata
    if not isinstance(attrs, dict) or not attrs:
        return 0
    try:
        import h5py
    except ImportError:
        return 0
    applied = {k: v for k, v in attrs.items()
               if isinstance(k, str) and k and k not in HDF5_RESERVED}
    written = 0
    for path in run_dir.rglob("openpmd_*.h5"):
        try:
            with h5py.File(path, "a") as handle:
                for key, value in applied.items():
                    if isinstance(value, (str, int, float, bool)):
                        handle.attrs[key] = value
                    elif value is not None:
                        handle.attrs[key] = json.dumps(value, ensure_ascii=False)
                written += 1
        except Exception:                              # noqa: BLE001
            continue
    try:
        (run_dir / "hdf5_metadata.json").write_text(
            json.dumps(applied, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    except OSError:
        pass
    return written


class Job:
    """A running simulation plus its parsed progress."""

    def __init__(self, workdir: Path, binary: Path, inputs: str,
                 mpi: int = 1, omp: int = 1, max_step: int | None = None,
                 hdf5_metadata: dict | None = None) -> None:
        self.workdir = workdir
        self.binary = binary
        self.inputs_text = inputs
        self.mpi = max(1, mpi)
        self.omp = max(1, omp)
        self.max_step = max_step
        self.hdf5_metadata = hdf5_metadata
        self.process: subprocess.Popen | None = None
        self.pid: int | None = None
        self.pgid: int | None = None
        self.log_path = workdir / "run.log"
        self.state_path = workdir / "studio_job.json"
        self.state = "created"
        self.step = 0
        self.time = 0.0
        self.dt = 0.0
        self.exit_code: int | None = None
        self.errors: list[str] = []
        self.log: list[str] = []
        self._lock = threading.Lock()
        self._reader: threading.Thread | None = None
        self.started_at = time.time()

    # -- lifecycle ---------------------------------------------------------
    env_extra: dict | None = None

    def start(self) -> None:
        self.workdir.mkdir(parents=True, exist_ok=True)
        (self.workdir / "inputs").write_text(self.inputs_text)
        command = [str(self.binary), "inputs"]
        if self.mpi > 1:
            mpiexec = shutil.which("mpiexec") or shutil.which("mpirun")
            if mpiexec:
                command = [mpiexec, "-n", str(self.mpi), *command]
        env = os.environ.copy()
        env["OMP_NUM_THREADS"] = str(self.omp)
        # 插件求解器由 deck 的 studio.solver.* 选择，通过环境变量交给运行时
        # （WARPX_PLUGIN_PATH / WARPX_STAR_PROFILE）。这样插件和内建求解器同一入口。
        for key, value in (self.env_extra or {}).items():
            env[key] = str(value)
        # The simulation writes straight to a log file, not into a pipe: if this server
        # dies, a broken pipe would kill WarpX with SIGPIPE and the run could never be
        # re-attached.
        self._log_handle = open(self.log_path, "a", encoding="utf-8", buffering=1)
        self.process = subprocess.Popen(
            command, cwd=self.workdir, env=env, text=True,
            stdout=self._log_handle, stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        self.pid = self.process.pid
        self.pgid = os.getpgid(self.process.pid)
        self.state = "running"
        self.state_path.write_text(json.dumps({
            "pid": self.pid, "pgid": self.pgid, "binary": str(self.binary),
            "mpi": self.mpi, "omp": self.omp, "max_step": self.max_step,
            "started_at": self.started_at, "log": str(self.log_path),
        }, indent=1))
        self._reader = threading.Thread(target=self._pump, daemon=True)
        self._reader.start()

    def _handle_line(self, line: str) -> None:
        """Update progress/errors from one log line (shared with replay())."""
        with self._lock:
            self.log.append(line)
            if len(self.log) > 4000:
                del self.log[:1000]
            start = STEP_START.match(line)
            end = STEP_END.match(line)
            error = ERROR.search(line)
            missing = MISSING.search(line)
            if start:
                self.step = int(start.group(1))
            elif end:
                self.step = int(end.group(1))
                try:
                    self.time = float(end.group(2))
                    self.dt = float(end.group(3))
                except ValueError:
                    pass
            elif error:
                self.errors.append(error.group(1).strip())
            elif missing:
                self.errors.append(line.strip())

    def _pump(self) -> None:
        """Tail the log file until the simulation is gone."""
        position = 0
        while True:
            try:
                with open(self.log_path, "r", encoding="utf-8", errors="replace") as handle:
                    handle.seek(position)
                    chunk = handle.read()
                    position = handle.tell()
            except OSError:
                chunk = ""
            for line in chunk.splitlines():
                self._handle_line(line)
            if self.process is not None and self.process.poll() is not None:
                break
            if self.process is None and not self._alive():
                break
            time.sleep(0.2)
        if self.process is not None:
            self.exit_code = self.process.wait()
        try:
            self._log_handle.close()
        except Exception:
            pass
        self.state = "finished" if self.exit_code in (0, None) else "failed"
        try:
            apply_hdf5_metadata(self.workdir, self.hdf5_metadata)
        except Exception as exc:                       # noqa: BLE001
            self.errors.append(f"HDF5 metadata: {exc}")
        try:
            self.state_path.unlink()
        except OSError:
            pass

    def _alive(self) -> bool:
        if self.pid is None:
            return False
        try:
            os.kill(self.pid, 0)
            return True
        except (ProcessLookupError, PermissionError):
            return False

    # -- control -----------------------------------------------------------
    def signal(self, sig: int) -> None:
        if self.process and self.process.poll() is None:
            try:
                os.killpg(os.getpgid(self.process.pid), sig)
            except ProcessLookupError:
                pass

    def signal_group(self, sig: int) -> None:
        if self.pgid:
            try:
                os.killpg(self.pgid, sig)
            except ProcessLookupError:
                pass

    def stop(self) -> None:
        self.signal(signal.SIGTERM)
        time.sleep(1.5)
        self.signal_group(signal.SIGKILL)
        self.state = "stopped"
        try:
            self.state_path.unlink()
        except OSError:
            pass

    def pause(self) -> None:
        self.signal(signal.SIGSTOP)
        self.state = "paused"

    def resume(self) -> None:
        self.signal(signal.SIGCONT)
        self.state = "running"

    # -- re-attaching after a server restart -------------------------------
    @classmethod
    def attach(cls, workdir: Path) -> "Job | None":
        """Rebuild a Job for a simulation that is still running (server was restarted).

        WarpX keeps running as its own process group; only this server lost the handle.
        The progress is replayed from ``run.log``, so the UI shows the same step it did
        before the restart without touching the simulation.
        """
        state_path = workdir / "studio_job.json"
        if not state_path.is_file():
            return None
        try:
            data = json.loads(state_path.read_text())
        except (OSError, json.JSONDecodeError):
            return None
        alive = True
        try:
            os.kill(int(data["pid"]), 0)
        except (ProcessLookupError, PermissionError):
            alive = False
        job = cls(workdir, Path(data["binary"]), "", mpi=data.get("mpi", 1),
                  omp=data.get("omp", 1), max_step=data.get("max_step"))
        job.pid = int(data["pid"])
        job.pgid = int(data.get("pgid") or 0) or None
        job.started_at = float(data.get("started_at") or time.time())
        job.log_path = Path(data.get("log") or (workdir / "run.log"))
        job.state = "running" if alive else "failed"
        if job.log_path.is_file():
            job.replay(job.log_path.read_text(errors="ignore").splitlines())
        if alive:
            threading.Thread(target=job._pump, daemon=True).start()
        if not alive:
            try:
                state_path.unlink()
            except OSError:
                pass
        return job

    def replay(self, lines: list[str]) -> None:
        """Rebuild step/time/dt and the log tail from an existing log file."""
        for line in lines[-4000:]:
            self._handle_line(line)

    # -- reporting ---------------------------------------------------------
    def snapshot(self, tail: int = 40) -> dict:
        with self._lock:
            return {
                "state": self.state,
                "step": self.step,
                "max_step": self.max_step,
                "time": self.time,
                "dt": self.dt,
                "exit_code": self.exit_code,
                "errors": self.errors[-5:],
                "log": self.log[-tail:],
                "workdir": str(self.workdir),
                "elapsed": round(time.time() - self.started_at, 1),
            }
