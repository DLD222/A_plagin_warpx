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
# line: `ParmParse::getval(): electrons..profile not found in database`.
MISSING = re.compile(r"ParmParse::getval\(\):\s*(\S+)\s+not found")
ABORT = re.compile(r"amrex::Abort|MPI_ABORT")


class Job:
    """A running simulation plus its parsed progress."""

    def __init__(self, workdir: Path, binary: Path, inputs: str,
                 mpi: int = 1, omp: int = 1, max_step: int | None = None) -> None:
        self.workdir = workdir
        self.binary = binary
        self.inputs_text = inputs
        self.mpi = max(1, mpi)
        self.omp = max(1, omp)
        self.max_step = max_step
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
