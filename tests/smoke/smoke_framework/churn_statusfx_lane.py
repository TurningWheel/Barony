from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .common import log, require_uint
from .churn_join_leave import cmd_join_leave_churn as run_join_leave_churn
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir, prune_models_cache, reset_paths
from .logscan import collect_instance_logs
from .lane_matrix import compute_lane_result
from .orchestration import RunnerOrchestrator
from .process import run_command
from .statusfx import collect_statusfx_lane_metrics
from .summary import parse_summary_key_last, write_summary_env

SCRIPT_DIR = Path(__file__).resolve().parent.parent
RUNNER = SCRIPT_DIR / "smoke_runner.py"
RUNNER_PYTHON = sys.executable or "python3"

STATUSFX_STARTUP_PLAYER_COUNTS: tuple[int, ...] = (1, 5, 15)
STATUSFX_REJOIN_PLAYER_COUNTS: tuple[int, ...] = (5, 15)
STATUSFX_NOT_APPLICABLE = "n/a"

STATUSFX_QUEUE_RESULT_HEADER: list[str] = [
    "lane",
    "instances",
    "churn_cycles",
    "churn_count",
    "result",
    "child_result",
    "init_slots",
    "init_missing_slots",
    "init_slot_coverage_ok",
    "init_mismatch_lines",
    "create_slots",
    "create_missing_slots",
    "create_slot_coverage_ok",
    "create_mismatch_lines",
    "update_slots",
    "update_missing_slots",
    "update_slot_coverage_ok",
    "update_mismatch_lines",
    "artifact",
]

STATUSFX_STARTUP_LANE_ARGS: list[str] = [
    "--force-chunk",
    "1",
    "--chunk-payload-max",
    "200",
    "--auto-start",
    "1",
    "--auto-start-delay",
    "2",
    "--auto-enter-dungeon",
    "1",
    "--auto-enter-dungeon-delay",
    "3",
    "--require-mapgen",
    "1",
]

_ORCH = RunnerOrchestrator(runner_path=RUNNER, runner_python=RUNNER_PYTHON)
validate_app_environment = _ORCH.validate_app_environment
validate_lane_environment = _ORCH.validate_lane_environment
build_nested_runner_cmd = _ORCH.build_nested_runner_cmd
run_helo_child_lane = _ORCH.run_helo_child_lane


def _collect_statusfx_metrics(
    *,
    lane_name: str,
    lane_outdir: Path,
    instances: int,
) -> dict[str, str | int] | None:
    log_files = collect_instance_logs(lane_outdir)
    if not log_files:
        log(f"No runtime logs found for {lane_name}: {lane_outdir}")
        return None
    return collect_statusfx_lane_metrics(log_files, instances)


def _append_statusfx_row(
    *,
    csv_path: Path,
    lane_name: str,
    instances: int,
    churn_cycles: int,
    churn_count: int,
    lane_result: str,
    child_result: str,
    metrics: dict[str, str | int],
    include_create_update: bool,
    lane_outdir: Path,
) -> None:
    row: list[str | int | Path] = [
        lane_name,
        instances,
        churn_cycles,
        churn_count,
        lane_result,
        child_result,
        metrics["init_slots"],
        metrics["init_missing_slots"],
        int(metrics["init_slot_coverage_ok"]),
        int(metrics["init_mismatch_lines"]),
    ]
    if include_create_update:
        row.extend(
            [
                metrics["create_slots"],
                metrics["create_missing_slots"],
                int(metrics["create_slot_coverage_ok"]),
                int(metrics["create_mismatch_lines"]),
                metrics["update_slots"],
                metrics["update_missing_slots"],
                int(metrics["update_slot_coverage_ok"]),
                int(metrics["update_mismatch_lines"]),
            ]
        )
    else:
        row.extend([STATUSFX_NOT_APPLICABLE] * 8)
    row.append(lane_outdir)
    append_csv_row(csv_path, row)


def _evaluate_startup_lane(
    *,
    csv_path: Path,
    lane_outdir: Path,
    instances: int,
    child_result: str,
) -> bool:
    lane_name = f"startup-p{instances}"
    metrics = _collect_statusfx_metrics(
        lane_name=lane_name,
        lane_outdir=lane_outdir,
        instances=instances,
    )
    if metrics is None:
        return False

    init_ok = int(metrics["init_slot_coverage_ok"])
    create_ok = int(metrics["create_slot_coverage_ok"])
    update_ok = int(metrics["update_slot_coverage_ok"])
    init_mismatch = int(metrics["init_mismatch_lines"])
    create_mismatch = int(metrics["create_mismatch_lines"])
    update_mismatch = int(metrics["update_mismatch_lines"])

    lane_result = compute_lane_result(
        child_result,
        init_ok == 1,
        create_ok == 1,
        update_ok == 1,
        init_mismatch == 0,
        create_mismatch == 0,
        update_mismatch == 0,
    )

    _append_statusfx_row(
        csv_path=csv_path,
        lane_name=lane_name,
        instances=instances,
        churn_cycles=0,
        churn_count=0,
        lane_result=lane_result,
        child_result=child_result,
        metrics=metrics,
        include_create_update=True,
        lane_outdir=lane_outdir,
    )

    if lane_result != "pass":
        log(f"Status-effect queue startup lane failed: {lane_name}")
    return lane_result == "pass"


def _load_join_fail_lines(summary_file: Path) -> int:
    join_fail_lines = parse_summary_key_last(summary_file, "JOIN_FAIL_LINES") or "0"
    try:
        return int(join_fail_lines)
    except ValueError:
        return 0


def _evaluate_rejoin_lane(
    *,
    csv_path: Path,
    lane_outdir: Path,
    instances: int,
    churn_count: int,
    child_result: str,
) -> bool:
    lane_name = f"rejoin-p{instances}"
    metrics = _collect_statusfx_metrics(
        lane_name=lane_name,
        lane_outdir=lane_outdir,
        instances=instances,
    )
    if metrics is None:
        return False

    init_ok = int(metrics["init_slot_coverage_ok"])
    init_mismatch = int(metrics["init_mismatch_lines"])

    join_fail_int = _load_join_fail_lines(lane_outdir / "summary.env")
    if join_fail_int > 0:
        log(
            f"warning: {lane_name} observed JOIN_FAIL_LINES={join_fail_int} "
            "(known intermittent lobby-full retry behavior)"
        )

    lane_result = compute_lane_result(
        child_result,
        init_ok == 1,
        init_mismatch == 0,
    )

    _append_statusfx_row(
        csv_path=csv_path,
        lane_name=lane_name,
        instances=instances,
        churn_cycles=2,
        churn_count=churn_count,
        lane_result=lane_result,
        child_result=child_result,
        metrics=metrics,
        include_create_update=False,
        lane_outdir=lane_outdir,
    )

    if lane_result != "pass":
        log(f"Status-effect queue rejoin lane failed: {lane_name}")
    return lane_result == "pass"


def _build_rejoin_lane_args(ns: argparse.Namespace, *, instances: int, churn_count: int) -> list[str]:
    return [
        "--instances",
        str(instances),
        "--churn-cycles",
        "2",
        "--churn-count",
        str(churn_count),
        "--size",
        ns.size,
        "--stagger",
        str(ns.stagger),
        "--initial-timeout",
        str(ns.cycle_timeout),
        "--cycle-timeout",
        str(ns.cycle_timeout),
        "--settle",
        "5",
        "--churn-gap",
        "3",
        "--force-chunk",
        "1",
        "--chunk-payload-max",
        "200",
        "--trace-join-rejects",
        "1",
    ]


def _build_statusfx_summary(
    ns: argparse.Namespace,
    *,
    outdir: Path,
    csv_path: Path,
) -> dict[str, str | int | Path]:
    return {
        "RESULT": "pass",
        "OUTDIR": outdir,
        "DATADIR": ns.datadir or "",
        "STARTUP_PLAYER_COUNTS": ";".join(str(v) for v in STATUSFX_STARTUP_PLAYER_COUNTS),
        "REJOIN_PLAYER_COUNTS": ";".join(str(v) for v in STATUSFX_REJOIN_PLAYER_COUNTS),
        "STARTUP_TIMEOUT_SECONDS": ns.startup_timeout,
        "CYCLE_TIMEOUT_SECONDS": ns.cycle_timeout,
        "CSV_PATH": csv_path,
    }


def cmd_join_leave_churn(ns: argparse.Namespace) -> int:
    validate_app_environment(ns.app, ns.datadir)
    return run_join_leave_churn(ns)


def cmd_status_effect_queue_init(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)
    require_uint("--stagger", ns.stagger)
    require_uint("--startup-timeout", ns.startup_timeout)
    require_uint("--cycle-timeout", ns.cycle_timeout)

    outdir = normalize_outdir(ns.outdir, "statusfx-queue-init")
    summary_path = outdir / "summary.env"
    csv_path = outdir / "status_effect_queue_results.csv"
    reset_paths(summary_path, csv_path)
    write_csv_header(csv_path, STATUSFX_QUEUE_RESULT_HEADER)

    log(f"Artifacts: {outdir}")
    trace_env = {"BARONY_SMOKE_TRACE_STATUS_EFFECT_QUEUE": "1"}

    for instances in STATUSFX_STARTUP_PLAYER_COUNTS:
        lane_outdir = outdir / f"startup-p{instances}"
        log(f"startup lane instances={instances}")
        rc, child_result, _values, _summary = run_helo_child_lane(
            app=ns.app,
            datadir=ns.datadir,
            instances=instances,
            expected_players=instances,
            size=ns.size,
            stagger=ns.stagger,
            timeout=ns.startup_timeout,
            lane_outdir=lane_outdir,
            extra_env=trace_env,
            lane_args=STATUSFX_STARTUP_LANE_ARGS,
        )
        if rc != 0:
            return rc
        if not _evaluate_startup_lane(
            csv_path=csv_path,
            lane_outdir=lane_outdir,
            instances=instances,
            child_result=child_result,
        ):
            return 1
        prune_models_cache(lane_outdir)

    for instances in STATUSFX_REJOIN_PLAYER_COUNTS:
        lane_outdir = outdir / f"rejoin-p{instances}"
        churn_count = 4 if instances >= 15 else 2
        log(f"rejoin lane instances={instances} churn_count={churn_count}")
        cmd = build_nested_runner_cmd(
            lane="join-leave-churn",
            app=ns.app,
            datadir=ns.datadir,
            lane_outdir=lane_outdir,
            lane_args=_build_rejoin_lane_args(ns, instances=instances, churn_count=churn_count),
        )

        rc = run_command(cmd, trace_env)
        if rc != 0:
            return rc
        child_result = parse_summary_key_last(lane_outdir / "summary.env", "RESULT") or "fail"
        if not _evaluate_rejoin_lane(
            csv_path=csv_path,
            lane_outdir=lane_outdir,
            instances=instances,
            churn_count=churn_count,
            child_result=child_result,
        ):
            return 1
        prune_models_cache(lane_outdir)

    write_summary_env(summary_path, _build_statusfx_summary(ns, outdir=outdir, csv_path=csv_path))

    log(f"summary={summary_path}")
    log(f"csv={csv_path}")
    return 0
