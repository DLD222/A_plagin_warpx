"""Run one WarpX simulation as a supervised child process.

The UI needs four things from a run: live progress, the log, a way to stop it, and
a way to pause it.  WarpX prints ``STEP n starts ...`` and
``STEP n ends. TIME = ... DT = ...`` per step, which is turned into a progress
record here instead of leaving the user to read a scrolling log.
"""
from __future__ import annotations

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
        self.process = subprocess.Popen(
            command, cwd=self.workdir, env=env, text=True, bufsize=1,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        self.state = "running"
        self._reader = threading.Thread(target=self._pump, daemon=True)
        self._reader.start()

    def _pump(self) -> None:
        assert self.process is not None and self.process.stdout is not None
        for line in self.process.stdout:
            line = line.rstrip("\n")
            with self._lock:
                self.log.append(line)
                if len(self.log) > 4000:
                    del self.log[:1000]
                start = STEP_START.match(line)
                end = STEP_END.match(line)
                error = ERROR.search(line)
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
                elif ABORT.search(line) and not self.errors:
                    self.errors.append(line.strip())
        self.exit_code = self.process.wait()
        with self._lock:
            self.state = "finished" if self.exit_code == 0 else "failed"

    # -- control -----------------------------------------------------------
    def signal(self, sig: int) -> None:
        if self.process and self.process.poll() is None:
            try:
                os.killpg(os.getpgid(self.process.pid), sig)
            except ProcessLookupError:
                pass

    def stop(self) -> None:
        self.signal(signal.SIGTERM)
        time.sleep(1.5)
        self.signal(signal.SIGKILL)
        self.state = "stopped"

    def pause(self) -> None:
        self.signal(signal.SIGSTOP)
        self.state = "paused"

    def resume(self) -> None:
        self.signal(signal.SIGCONT)
        self.state = "running"

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
