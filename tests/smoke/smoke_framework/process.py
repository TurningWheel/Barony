from __future__ import annotations

import os
import subprocess
import time
from pathlib import Path
from typing import Callable, Sequence


def launch_local_instance(
    app: Path,
    datadir: Path | None,
    size: str,
    home_dir: Path,
    stdout_log: Path,
    extra_env: dict[str, str],
    *,
    set_home: bool = True,
) -> subprocess.Popen[bytes]:
    env = os.environ.copy()
    if set_home:
        env["HOME"] = str(home_dir)
    env.update(extra_env)

    args = [str(app), "-windowed", f"-size={size}"]
    if datadir:
        args.append(f"-datadir={datadir}")

    stdout_log.parent.mkdir(parents=True, exist_ok=True)
    with stdout_log.open("w", encoding="utf-8", errors="replace") as out:
        proc = subprocess.Popen(args, stdout=out, stderr=subprocess.STDOUT, env=env)
    return proc


def terminate_process(proc: subprocess.Popen[bytes] | None) -> None:
    if proc is None:
        return
    if proc.poll() is not None:
        return
    try:
        proc.terminate()
    except OSError:
        return
    try:
        proc.wait(timeout=1)
    except subprocess.TimeoutExpired:
        try:
            proc.kill()
        except OSError:
            return


def terminate_process_group(
    processes: Sequence[subprocess.Popen[bytes]],
    *,
    keep_running: bool = False,
    keep_running_message: str | None = None,
    logger: Callable[[str], None] | None = None,
    grace_seconds: float = 1.0,
) -> None:
    if keep_running:
        if keep_running_message and logger is not None:
            logger(keep_running_message)
        return
    for proc in processes:
        if proc.poll() is None:
            try:
                proc.terminate()
            except OSError:
                pass
    if grace_seconds > 0:
        time.sleep(grace_seconds)
    for proc in processes:
        if proc.poll() is None:
            try:
                proc.kill()
            except OSError:
                pass


def poll_until(
    timeout_seconds: int,
    snapshot_fn: Callable[[], dict[str, int | str]],
    success_fn: Callable[[dict[str, int | str]], bool],
    interval_seconds: float = 1.0,
) -> tuple[dict[str, int | str], bool]:
    deadline = time.monotonic() + timeout_seconds
    snapshot = snapshot_fn()
    while True:
        snapshot = snapshot_fn()
        if success_fn(snapshot):
            return snapshot, True
        if time.monotonic() >= deadline:
            return snapshot, False
        time.sleep(interval_seconds)


def run_command(cmd: Sequence[str], extra_env: dict[str, str] | None = None) -> int:
    env = None
    if extra_env:
        env = os.environ.copy()
        env.update(extra_env)
    proc = subprocess.run(cmd, check=False, env=env)
    return proc.returncode
