from __future__ import annotations

import argparse

from .common import fail, log
from .lane_helpers import require_uint_specs, write_single_lane_result_files
from .logscan import file_count_fixed_lines
from .tokens import last_prefixed_metric_int, last_prefixed_metric_str
from .splitscreen_runtime import (
    EXPECTED_LOCAL_SPLITSCREEN_PLAYERS,
    base_progress_snapshot,
    base_splitscreen_env,
    prepare_splitscreen_lane,
    run_splitscreen_polling_lane,
    validate_app_environment,
)


def cmd_splitscreen_cap(ns: argparse.Namespace) -> int:
    validate_app_environment(ns.app, ns.datadir)
    require_uint_specs(
        ns,
        (
            ("--timeout", "timeout", None, None),
            ("--requested-players", "requested_players", None, None),
            ("--cap-delay", "cap_delay", None, None),
            ("--cap-verify-delay", "cap_verify_delay", None, None),
            ("--auto-enter-delay", "auto_enter_delay", None, None),
        ),
    )
    if ns.requested_players < 2 or ns.requested_players > 15:
        fail("--requested-players must be in 2..15")

    expected_cap = min(EXPECTED_LOCAL_SPLITSCREEN_PLAYERS, ns.requested_players)
    lane_ctx = prepare_splitscreen_lane(
        ns,
        outdir_name=f"splitscreen-cap-r{ns.requested_players}",
        csv_name="splitscreen_cap_results.csv",
    )
    outdir = lane_ctx.outdir
    summary_path = lane_ctx.summary_path
    csv_path = lane_ctx.csv_path
    lane_paths = lane_ctx.lane_paths
    cap_prefix = "[SMOKE]: local-splitscreen cap status="

    def snapshot() -> dict[str, int | str]:
        snap: dict[str, int | str] = {
            **base_progress_snapshot(lane_paths.host_log),
            "cap_command_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen cap command issued"
            ),
            "cap_ok_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen cap status=ok"
            ),
            "cap_fail_lines": file_count_fixed_lines(
                lane_paths.host_log, "[SMOKE]: local-splitscreen cap status=fail"
            ),
            "cap_status": last_prefixed_metric_str(lane_paths.host_log, cap_prefix, "status", ""),
        }
        for key in (
            "target",
            "cap",
            "connected",
            "connected_local",
            "over_cap_connected",
            "over_cap_local",
            "over_cap_splitscreen",
            "under_cap_nonlocal",
        ):
            snap[f"cap_{key}"] = last_prefixed_metric_int(lane_paths.host_log, cap_prefix, key, 0)
        return snap

    def is_success(snap: dict[str, int | str]) -> bool:
        return (
            int(snap["cap_command_lines"]) >= 1
            and int(snap["cap_ok_lines"]) >= 1
            and int(snap["cap_fail_lines"]) == 0
            and int(snap["auto_enter_transition_lines"]) >= 1
            and int(snap["mapgen_count"]) >= 1
            and str(snap["cap_status"]) == "ok"
            and int(snap["cap_target"]) == ns.requested_players
            and int(snap["cap_cap"]) == expected_cap
            and int(snap["cap_connected"]) == expected_cap
            and int(snap["cap_connected_local"]) == expected_cap
            and int(snap["cap_over_cap_connected"]) == 0
            and int(snap["cap_over_cap_local"]) == 0
            and int(snap["cap_over_cap_splitscreen"]) == 0
            and int(snap["cap_under_cap_nonlocal"]) == 0
        )

    env = base_splitscreen_env(ns.auto_enter_delay)
    env.update(
        {
            "BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN_CAP": "1",
            "BARONY_SMOKE_AUTO_SPLITSCREEN_CAP_TARGET": str(ns.requested_players),
            "BARONY_SMOKE_SPLITSCREEN_CAP_DELAY_SECS": str(ns.cap_delay),
            "BARONY_SMOKE_SPLITSCREEN_CAP_VERIFY_DELAY_SECS": str(ns.cap_verify_delay),
        }
    )
    result, snap = run_splitscreen_polling_lane(
        ns,
        lane_name="splitscreen cap",
        lane_paths=lane_paths,
        env=env,
        timeout_seconds=ns.timeout,
        snapshot_fn=snapshot,
        success_fn=is_success,
    )

    write_single_lane_result_files(
        summary_path=summary_path,
        csv_path=csv_path,
        csv_header=[
            "lane",
            "requested_players",
            "expected_cap",
            "result",
            "cap_status",
            "cap_command_lines",
            "cap_ok_lines",
            "cap_fail_lines",
            "cap_target",
            "cap_value",
            "cap_connected",
            "cap_connected_local",
            "cap_over_connected",
            "cap_over_local",
            "cap_over_splitscreen",
            "cap_under_nonlocal",
            "auto_enter_transition_lines",
            "mapgen_count",
            "outdir",
        ],
        csv_row=[
            f"splitscreen-cap-r{ns.requested_players}",
            ns.requested_players,
            expected_cap,
            result,
            str(snap["cap_status"]),
            int(snap["cap_command_lines"]),
            int(snap["cap_ok_lines"]),
            int(snap["cap_fail_lines"]),
            int(snap["cap_target"]),
            int(snap["cap_cap"]),
            int(snap["cap_connected"]),
            int(snap["cap_connected_local"]),
            int(snap["cap_over_cap_connected"]),
            int(snap["cap_over_cap_local"]),
            int(snap["cap_over_cap_splitscreen"]),
            int(snap["cap_under_cap_nonlocal"]),
            int(snap["auto_enter_transition_lines"]),
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
            "REQUESTED_SPLITSCREEN_PLAYERS": ns.requested_players,
            "EXPECTED_CAP": expected_cap,
            "CAP_DELAY_SECONDS": ns.cap_delay,
            "CAP_VERIFY_DELAY_SECONDS": ns.cap_verify_delay,
            "AUTO_ENTER_DELAY_SECONDS": ns.auto_enter_delay,
            "LOCAL_SPLITSCREEN_CAP_STATUS": str(snap["cap_status"]),
            "LOCAL_SPLITSCREEN_CAP_COMMAND_LINES": int(snap["cap_command_lines"]),
            "LOCAL_SPLITSCREEN_CAP_OK_LINES": int(snap["cap_ok_lines"]),
            "LOCAL_SPLITSCREEN_CAP_FAIL_LINES": int(snap["cap_fail_lines"]),
            "LOCAL_SPLITSCREEN_CAP_TARGET": int(snap["cap_target"]),
            "LOCAL_SPLITSCREEN_CAP_VALUE": int(snap["cap_cap"]),
            "LOCAL_SPLITSCREEN_CAP_CONNECTED": int(snap["cap_connected"]),
            "LOCAL_SPLITSCREEN_CAP_CONNECTED_LOCAL": int(snap["cap_connected_local"]),
            "LOCAL_SPLITSCREEN_CAP_OVER_CONNECTED": int(snap["cap_over_cap_connected"]),
            "LOCAL_SPLITSCREEN_CAP_OVER_LOCAL": int(snap["cap_over_cap_local"]),
            "LOCAL_SPLITSCREEN_CAP_OVER_SPLITSCREEN": int(snap["cap_over_cap_splitscreen"]),
            "LOCAL_SPLITSCREEN_CAP_UNDER_NONLOCAL": int(snap["cap_under_cap_nonlocal"]),
            "AUTO_ENTER_TRANSITION_LINES": int(snap["auto_enter_transition_lines"]),
            "MAPGEN_COUNT": int(snap["mapgen_count"]),
            "HOST_LOG": lane_paths.host_log,
            "STDOUT_LOG": lane_paths.stdout_log,
            "PID_FILE": lane_paths.pid_file,
        },
    )

    log(
        "result="
        f"{result} requested={ns.requested_players} expectedCap={expected_cap} "
        f"capStatus={str(snap['cap_status'])} mapgen={int(snap['mapgen_count'])}"
    )
    log(f"summary={summary_path}")
    return 1 if result != "pass" else 0
