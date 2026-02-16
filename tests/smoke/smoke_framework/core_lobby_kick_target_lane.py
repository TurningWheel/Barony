from __future__ import annotations

import argparse
import shutil

from .common import fail, log
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir, prune_models_cache, reset_paths
from .lane_helpers import build_default_helo_lane_args, require_uint_specs, run_ns_helo_child_lane
from .lane_matrix import compute_lane_result, update_lane_counts
from .summary import write_summary_env
from .core_runtime import run_helo_child_lane, validate_lane_environment


def cmd_lobby_kick_target(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--stagger", "stagger", None, None),
            ("--timeout", "timeout", None, None),
            ("--kick-delay", "kick_delay", None, None),
            ("--min-players", "min_players", 2, 15),
            ("--max-players", "max_players", 2, 15),
        ),
    )
    if ns.min_players > ns.max_players:
        fail("--min-players cannot be greater than --max-players")

    outdir = normalize_outdir(
        ns.outdir,
        f"lobby-kick-target-p{ns.min_players}to{ns.max_players}",
    )

    for old_lane in outdir.glob("p*"):
        if old_lane.is_dir():
            shutil.rmtree(old_lane, ignore_errors=True)
    csv_path = outdir / "kick_target_results.csv"
    summary_path = outdir / "summary.env"
    reset_paths(csv_path, summary_path)

    total_lanes = 0
    pass_lanes = 0
    fail_lanes = 0

    write_csv_header(
        csv_path,
        [
            "lane",
            "instances",
            "target_slot",
            "result",
            "child_result",
            "auto_kick_result",
            "auto_kick_ok_lines",
            "auto_kick_fail_lines",
            "host_chunk_lines",
            "client_reassembled_lines",
            "outdir",
        ],
    )

    for instances in range(ns.min_players, ns.max_players + 1):
        target_slot = instances - 1
        lane_name = f"p{instances}-kick-slot{target_slot}"
        lane_outdir = outdir / lane_name
        lane_outdir.mkdir(parents=True, exist_ok=True)
        log(f"Running lane {lane_name}")

        _rc, child_result, values, _summary_file = run_ns_helo_child_lane(
            run_helo_child_lane,
            ns,
            instances=instances,
            expected_players=instances,
            timeout=ns.timeout,
            lane_outdir=lane_outdir,
            lane_args=build_default_helo_lane_args(
                "--auto-kick-target-slot",
                str(target_slot),
                "--auto-kick-delay",
                str(ns.kick_delay),
                "--require-auto-kick",
                "1",
                auto_start=0,
            ),
            summary_defaults={
                "AUTO_KICK_RESULT": "missing",
                "AUTO_KICK_OK_LINES": "0",
                "AUTO_KICK_FAIL_LINES": "0",
                "HOST_CHUNK_LINES": "0",
                "CLIENT_REASSEMBLED_LINES": "0",
            },
        )
        auto_kick_result = values["AUTO_KICK_RESULT"]
        auto_kick_ok_lines = values["AUTO_KICK_OK_LINES"]
        auto_kick_fail_lines = values["AUTO_KICK_FAIL_LINES"]
        host_chunk_lines = values["HOST_CHUNK_LINES"]
        client_reassembled_lines = values["CLIENT_REASSEMBLED_LINES"]

        lane_result = compute_lane_result(
            child_result,
            auto_kick_result == "ok",
            auto_kick_fail_lines == "0",
        )

        append_csv_row(
            csv_path,
            [
                lane_name,
                instances,
                target_slot,
                lane_result,
                child_result,
                auto_kick_result,
                auto_kick_ok_lines,
                auto_kick_fail_lines,
                host_chunk_lines,
                client_reassembled_lines,
                lane_outdir,
            ],
        )

        total_lanes, pass_lanes, fail_lanes = update_lane_counts(
            lane_result,
            total_lanes=total_lanes,
            pass_lanes=pass_lanes,
            fail_lanes=fail_lanes,
        )

        prune_models_cache(lane_outdir)

    overall_result = "pass" if fail_lanes == 0 else "fail"
    write_summary_env(
        summary_path,
        {
            "RESULT": overall_result,
            "OUTDIR": outdir,
            "APP": ns.app,
            "DATADIR": ns.datadir or "",
            "MIN_PLAYERS": ns.min_players,
            "MAX_PLAYERS": ns.max_players,
            "TIMEOUT_SECONDS": ns.timeout,
            "KICK_DELAY_SECONDS": ns.kick_delay,
            "TOTAL_LANES": total_lanes,
            "PASS_LANES": pass_lanes,
            "FAIL_LANES": fail_lanes,
            "CSV_PATH": csv_path,
        },
    )

    log(f"result={overall_result} pass={pass_lanes} fail={fail_lanes} total={total_lanes}")
    log(f"csv={csv_path}")
    log(f"summary={summary_path}")
    return 1 if overall_result != "pass" else 0
