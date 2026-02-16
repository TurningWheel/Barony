from __future__ import annotations

import argparse
import subprocess
import sys
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

from .common import log
from .fs import normalize_outdir, prune_models_cache
from .local_lane import LocalSingleLanePaths, prepare_single_local_instance_lane
from .logscan import file_count_fixed_lines
from .orchestration import RunnerOrchestrator
from .parser_common import add_app_datadir_args
from .process import launch_local_instance, poll_until, terminate_process

SCRIPT_DIR = Path(__file__).resolve().parent.parent
RUNNER = SCRIPT_DIR / "smoke_runner.py"
RUNNER_PYTHON = sys.executable or "python3"
EXPECTED_LOCAL_SPLITSCREEN_PLAYERS = 4

_ORCH = RunnerOrchestrator(runner_path=RUNNER, runner_python=RUNNER_PYTHON)
validate_app_environment = _ORCH.validate_app_environment


@dataclass(frozen=True)
class SplitscreenLaneContext:
    outdir: Path
    summary_path: Path
    csv_path: Path
    lane_paths: LocalSingleLanePaths


def base_splitscreen_env(auto_enter_delay: int) -> dict[str, str]:
    return {
        "BARONY_SMOKE_AUTOPILOT": "1",
        "BARONY_SMOKE_ROLE": "local",
        "BARONY_SMOKE_EXPECTED_PLAYERS": str(EXPECTED_LOCAL_SPLITSCREEN_PLAYERS),
        "BARONY_SMOKE_AUTO_ENTER_DUNGEON": "1",
        "BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS": str(auto_enter_delay),
        "BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS": "1",
    }


def _launch_splitscreen_instance(
    ns: argparse.Namespace,
    *,
    lane_name: str,
    lane_paths: LocalSingleLanePaths,
    env: dict[str, str],
) -> subprocess.Popen[bytes]:
    log(f"Launching local {lane_name} lane")
    proc = launch_local_instance(ns.app, ns.datadir, ns.size, lane_paths.home_dir, lane_paths.stdout_log, env)
    lane_paths.pid_file.write_text(f"{proc.pid}\n", encoding="utf-8")
    log(f"instance=1 role=local pid={proc.pid} home={lane_paths.home_dir}")
    return proc


def prepare_splitscreen_lane(
    ns: argparse.Namespace,
    *,
    outdir_name: str,
    csv_name: str,
) -> SplitscreenLaneContext:
    outdir = normalize_outdir(ns.outdir, outdir_name)
    summary_path = outdir / "summary.env"
    csv_path = outdir / csv_name
    lane_paths = prepare_single_local_instance_lane(outdir, summary_path, csv_path)
    return SplitscreenLaneContext(
        outdir=outdir,
        summary_path=summary_path,
        csv_path=csv_path,
        lane_paths=lane_paths,
    )


def run_splitscreen_polling_lane(
    ns: argparse.Namespace,
    *,
    lane_name: str,
    lane_paths: LocalSingleLanePaths,
    env: dict[str, str],
    timeout_seconds: int,
    snapshot_fn: Callable[[], dict[str, int | str]],
    success_fn: Callable[[dict[str, int | str]], bool],
) -> tuple[str, dict[str, int | str]]:
    proc: subprocess.Popen[bytes] | None = None
    try:
        proc = _launch_splitscreen_instance(ns, lane_name=lane_name, lane_paths=lane_paths, env=env)
        snap, _ = poll_until(timeout_seconds, snapshot_fn, success_fn)
        return ("pass" if success_fn(snap) else "fail"), snap
    finally:
        terminate_process(proc)
        prune_models_cache(lane_paths.instance_root)


def base_progress_snapshot(host_log: Path) -> dict[str, int]:
    return {
        "auto_enter_transition_lines": file_count_fixed_lines(
            host_log, "[SMOKE]: auto-entering dungeon transition"
        ),
        "mapgen_count": file_count_fixed_lines(host_log, "successfully generated a dungeon with"),
    }


def add_splitscreen_common_args(
    parser: argparse.ArgumentParser, *, default_app: Path, timeout_default: int = 420
) -> None:
    add_app_datadir_args(parser, default_app=default_app)
    parser.add_argument("--size", default="1280x720", help="Window size.")
    parser.add_argument("--timeout", type=int, default=timeout_default, help="Timeout in seconds.")
    parser.add_argument(
        "--auto-enter-delay",
        type=int,
        default=3,
        help="Delay before smoke auto-enter dungeon transition (seconds).",
    )
    parser.add_argument("--outdir", default=None, help="Artifact directory.")
