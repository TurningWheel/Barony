from __future__ import annotations

import argparse

from .common import fail, log
from .lane_helpers import require_uint_specs, write_single_lane_result_files
from .logscan import file_count_fixed_lines
from .tokens import last_prefixed_metric_int
from .splitscreen_runtime import (
    EXPECTED_LOCAL_SPLITSCREEN_PLAYERS,
    base_progress_snapshot,
    base_splitscreen_env,
    prepare_splitscreen_lane,
    run_splitscreen_polling_lane,
    validate_app_environment,
)


def cmd_splitscreen_baseline(ns: argparse.Namespace) -> int:
    validate_app_environment(ns.app, ns.datadir)
    require_uint_specs(
        ns,
        (
            ("--timeout", "timeout", None, None),
            ("--pause-pulses", "pause_pulses", None, None),
            ("--pause-delay", "pause_delay", None, None),
            ("--pause-hold", "pause_hold", None, None),
            ("--auto-enter-delay", "auto_enter_delay", None, None),
        ),
    )
    if ns.pause_pulses > 64:
        fail("--pause-pulses must be <= 64")

    lane_ctx = prepare_splitscreen_lane(
        ns,
        outdir_name="splitscreen-baseline-p4",
        csv_name="splitscreen_results.csv",
    )
    outdir = lane_ctx.outdir
    summary_path = lane_ctx.summary_path
    csv_path = lane_ctx.csv_path
    lane_paths = lane_ctx.lane_paths
    lobby_prefix = "[SMOKE]: local-splitscreen lobby context=autopilot "

    def snapshot() -> dict[str, int | str]:
        snap: dict[str, int | str] = {
            **base_progress_snapshot(lane_paths.host_log),
            "lobby_snapshot_lines": file_count_fixed_lines(lane_paths.host_log, lobby_prefix),
            "baseline_ok_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen baseline status=ok"
            ),
            "baseline_wait_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen baseline status=wait"
            ),
            "pause_action_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen auto-pause action="
            ),
            "pause_complete_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen auto-pause complete pulses="
            ),
            "splitscreen_transition_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen transition level="
            ),
        }
        snap["lobby_target"] = last_prefixed_metric_int(lane_paths.host_log, lobby_prefix, "target", 0)
        snap["lobby_joined"] = last_prefixed_metric_int(lane_paths.host_log, lobby_prefix, "joined", 0)
        snap["lobby_ready"] = last_prefixed_metric_int(lane_paths.host_log, lobby_prefix, "ready", 0)
        snap["lobby_countdown"] = last_prefixed_metric_int(lane_paths.host_log, lobby_prefix, "countdown", 0)
        return snap

    def lobby_ready_ok(snap: dict[str, int | str]) -> int:
        if (
            int(snap["lobby_target"]) >= EXPECTED_LOCAL_SPLITSCREEN_PLAYERS
            and int(snap["lobby_joined"]) >= EXPECTED_LOCAL_SPLITSCREEN_PLAYERS
            and int(snap["lobby_ready"]) >= EXPECTED_LOCAL_SPLITSCREEN_PLAYERS
        ):
            return 1
        return 0

    def pause_ok(snap: dict[str, int | str]) -> int:
        if ns.pause_pulses == 0:
            return 1
        if int(snap["pause_action_lines"]) < ns.pause_pulses * 2:
            return 0
        if int(snap["pause_complete_lines"]) < 1:
            return 0
        return 1

    def is_success(snap: dict[str, int | str]) -> bool:
        return (
            lobby_ready_ok(snap) == 1
            and int(snap["baseline_ok_lines"]) >= 1
            and pause_ok(snap) == 1
            and int(snap["auto_enter_transition_lines"]) >= 1
            and int(snap["splitscreen_transition_lines"]) >= 1
            and int(snap["mapgen_count"]) >= 1
        )

    env = base_splitscreen_env(ns.auto_enter_delay)
    env.update(
        {
            "BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN": "1",
            "BARONY_SMOKE_LOCAL_PAUSE_PULSES": str(ns.pause_pulses),
            "BARONY_SMOKE_LOCAL_PAUSE_DELAY_SECS": str(ns.pause_delay),
            "BARONY_SMOKE_LOCAL_PAUSE_HOLD_SECS": str(ns.pause_hold),
        }
    )
    result, snap = run_splitscreen_polling_lane(
        ns,
        lane_name="splitscreen baseline",
        lane_paths=lane_paths,
        env=env,
        timeout_seconds=ns.timeout,
        snapshot_fn=snapshot,
        success_fn=is_success,
    )
    ready_ok = lobby_ready_ok(snap)

    write_single_lane_result_files(
        summary_path=summary_path,
        csv_path=csv_path,
        csv_header=[
            "lane",
            "expected_players",
            "result",
            "lobby_ready_ok",
            "lobby_target",
            "lobby_joined",
            "lobby_ready",
            "baseline_ok_lines",
            "baseline_wait_lines",
            "pause_action_lines",
            "pause_complete_lines",
            "auto_enter_transition_lines",
            "splitscreen_transition_lines",
            "mapgen_count",
            "outdir",
        ],
        csv_row=[
            "splitscreen-baseline-p4",
            EXPECTED_LOCAL_SPLITSCREEN_PLAYERS,
            result,
            ready_ok,
            int(snap["lobby_target"]),
            int(snap["lobby_joined"]),
            int(snap["lobby_ready"]),
            int(snap["baseline_ok_lines"]),
            int(snap["baseline_wait_lines"]),
            int(snap["pause_action_lines"]),
            int(snap["pause_complete_lines"]),
            int(snap["auto_enter_transition_lines"]),
            int(snap["splitscreen_transition_lines"]),
            int(snap["mapgen_count"]),
            outdir,
        ],
        lane_result=result,
        outdir=outdir,
        app=ns.app,
        datadir=ns.datadir,
        summary_extra={
            "WINDOW_SIZE": ns.size,
            "TIMEOUT_SECONDS": ns.timeout,
            "EXPECTED_PLAYERS": EXPECTED_LOCAL_SPLITSCREEN_PLAYERS,
            "PAUSE_PULSES": ns.pause_pulses,
            "PAUSE_DELAY_SECONDS": ns.pause_delay,
            "PAUSE_HOLD_SECONDS": ns.pause_hold,
            "AUTO_ENTER_DELAY_SECONDS": ns.auto_enter_delay,
            "LOBBY_READY_OK": ready_ok,
            "LOBBY_SNAPSHOT_LINES": int(snap["lobby_snapshot_lines"]),
            "LOBBY_TARGET": int(snap["lobby_target"]),
            "LOBBY_JOINED": int(snap["lobby_joined"]),
            "LOBBY_READY": int(snap["lobby_ready"]),
            "LOBBY_COUNTDOWN": int(snap["lobby_countdown"]),
            "LOCAL_SPLITSCREEN_BASELINE_OK_LINES": int(snap["baseline_ok_lines"]),
            "LOCAL_SPLITSCREEN_BASELINE_WAIT_LINES": int(snap["baseline_wait_lines"]),
            "LOCAL_SPLITSCREEN_PAUSE_ACTION_LINES": int(snap["pause_action_lines"]),
            "LOCAL_SPLITSCREEN_PAUSE_COMPLETE_LINES": int(snap["pause_complete_lines"]),
            "AUTO_ENTER_TRANSITION_LINES": int(snap["auto_enter_transition_lines"]),
            "LOCAL_SPLITSCREEN_TRANSITION_LINES": int(snap["splitscreen_transition_lines"]),
            "MAPGEN_COUNT": int(snap["mapgen_count"]),
            "HOST_LOG": lane_paths.host_log,
            "STDOUT_LOG": lane_paths.stdout_log,
            "PID_FILE": lane_paths.pid_file,
        },
    )

    log(
        "result="
        f"{result} lobbyReady={ready_ok} "
        f"baselineOk={int(snap['baseline_ok_lines'])} "
        f"pauseActions={int(snap['pause_action_lines'])} "
        f"transitions={int(snap['splitscreen_transition_lines'])} "
        f"mapgen={int(snap['mapgen_count'])}"
    )
    log(f"summary={summary_path}")
    return 1 if result != "pass" else 0
