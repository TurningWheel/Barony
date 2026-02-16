from __future__ import annotations

import argparse
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path

from .common import fail, log
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir, prune_models_cache, reset_paths
from .lane_helpers import (
    build_default_helo_lane_args,
    require_uint_specs,
    run_ns_helo_child_lane,
    write_single_lane_result_files,
)
from .lane_matrix import compute_lane_result, update_lane_counts
from .orchestration import RunnerOrchestrator
from .summary import write_summary_env

SCRIPT_DIR = Path(__file__).resolve().parent.parent
RUNNER = SCRIPT_DIR / "smoke_runner.py"
RUNNER_PYTHON = sys.executable or "python3"
_ORCH = RunnerOrchestrator(runner_path=RUNNER, runner_python=RUNNER_PYTHON)
validate_lane_environment = _ORCH.validate_lane_environment
run_helo_child_lane = _ORCH.run_helo_child_lane


@dataclass(frozen=True)
class SlotLockKickCopyLane:
    lane_name: str
    instances: int
    expected_players: int
    auto_player_count_target: int
    expected_variant: str
    require_default_lock: bool
    require_copy: bool


SLOT_LOCK_KICK_COPY_LANES: tuple[SlotLockKickCopyLane, ...] = (
    SlotLockKickCopyLane("default-slot-lock-p4", 4, 4, 0, "none", True, False),
    SlotLockKickCopyLane("kick-copy-single-p6-to5", 6, 6, 5, "single", False, True),
    SlotLockKickCopyLane("kick-copy-double-p6-to4", 6, 6, 4, "double", False, True),
    SlotLockKickCopyLane("kick-copy-multi-p8-to4", 8, 8, 4, "multi", False, True),
)

def cmd_lobby_page_navigation(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--stagger", "stagger", None, None),
            ("--timeout", "timeout", None, None),
            ("--page-delay", "page_delay", None, None),
            ("--instances", "instances", 5, 15),
            ("--require-focus-match", "require_focus_match", 0, 1),
        ),
    )

    outdir = normalize_outdir(ns.outdir, f"lobby-page-navigation-p{ns.instances}")
    lane_outdir = outdir / "lane-page-navigation"
    csv_path = outdir / "page_navigation_results.csv"
    summary_path = outdir / "summary.env"
    reset_paths(lane_outdir, csv_path, summary_path)

    lane_name = f"page-navigation-p{ns.instances}"
    csv_header = [
        "lane",
        "instances",
        "result",
        "child_result",
        "lobby_page_state_ok",
        "lobby_page_sweep_ok",
        "lobby_page_snapshot_lines",
        "lobby_page_unique_count",
        "lobby_page_total_count",
        "lobby_page_visited",
        "lobby_focus_mismatch_lines",
        "lobby_cards_misaligned_max",
        "lobby_paperdolls_misaligned_max",
        "lobby_pings_misaligned_max",
        "lobby_warnings_max_abs_delta",
        "lobby_countdown_max_abs_delta",
        "outdir",
    ]

    log(f"Running lane {lane_name}")
    _rc, child_result, values, _summary_file = run_ns_helo_child_lane(
        run_helo_child_lane,
        ns,
        instances=ns.instances,
        expected_players=ns.instances,
        timeout=ns.timeout,
        lane_outdir=lane_outdir,
        lane_args=build_default_helo_lane_args(
            "--trace-lobby-page-state",
            "1",
            "--require-lobby-page-state",
            "1",
            "--require-lobby-page-focus-match",
            str(ns.require_focus_match),
            "--auto-lobby-page-sweep",
            "1",
            "--auto-lobby-page-delay",
            str(ns.page_delay),
            "--require-lobby-page-sweep",
            "1",
            auto_start=0,
        ),
        summary_defaults={
            "LOBBY_PAGE_STATE_OK": "0",
            "LOBBY_PAGE_SWEEP_OK": "0",
            "LOBBY_PAGE_SNAPSHOT_LINES": "0",
            "LOBBY_PAGE_UNIQUE_COUNT": "0",
            "LOBBY_PAGE_TOTAL_COUNT": "0",
            "LOBBY_PAGE_VISITED": "",
            "LOBBY_FOCUS_MISMATCH_LINES": "0",
            "LOBBY_CARDS_MISALIGNED_MAX": "0",
            "LOBBY_PAPERDOLLS_MISALIGNED_MAX": "0",
            "LOBBY_PINGS_MISALIGNED_MAX": "0",
            "LOBBY_WARNINGS_MAX_ABS_DELTA": "0",
            "LOBBY_COUNTDOWN_MAX_ABS_DELTA": "0",
        },
    )
    lane_result = compute_lane_result(
        child_result,
        values["LOBBY_PAGE_STATE_OK"] == "1",
        values["LOBBY_PAGE_SWEEP_OK"] == "1",
    )

    summary_payload = write_single_lane_result_files(
        summary_path=summary_path,
        csv_path=csv_path,
        csv_header=csv_header,
        csv_row=[
            lane_name,
            ns.instances,
            lane_result,
            child_result,
            values["LOBBY_PAGE_STATE_OK"],
            values["LOBBY_PAGE_SWEEP_OK"],
            values["LOBBY_PAGE_SNAPSHOT_LINES"],
            values["LOBBY_PAGE_UNIQUE_COUNT"],
            values["LOBBY_PAGE_TOTAL_COUNT"],
            values["LOBBY_PAGE_VISITED"],
            values["LOBBY_FOCUS_MISMATCH_LINES"],
            values["LOBBY_CARDS_MISALIGNED_MAX"],
            values["LOBBY_PAPERDOLLS_MISALIGNED_MAX"],
            values["LOBBY_PINGS_MISALIGNED_MAX"],
            values["LOBBY_WARNINGS_MAX_ABS_DELTA"],
            values["LOBBY_COUNTDOWN_MAX_ABS_DELTA"],
            lane_outdir,
        ],
        lane_result=lane_result,
        outdir=outdir,
        app=ns.app,
        datadir=ns.datadir,
        lane_outdir=lane_outdir,
        summary_extra={
            "INSTANCES": ns.instances,
            "TIMEOUT_SECONDS": ns.timeout,
            "PAGE_DELAY_SECONDS": ns.page_delay,
            "REQUIRE_FOCUS_MATCH": ns.require_focus_match,
        },
    )
    prune_models_cache(lane_outdir)
    overall_result = str(summary_payload["RESULT"])

    log(f"result={overall_result} lane={lane_result}")
    log(f"csv={csv_path}")
    log(f"summary={summary_path}")
    return 1 if overall_result != "pass" else 0


def cmd_remote_combat_slot_bounds(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--stagger", "stagger", None, None),
            ("--timeout", "timeout", None, None),
            ("--instances", "instances", 3, 15),
            ("--pause-pulses", "pause_pulses", None, None),
            ("--pause-delay", "pause_delay", None, None),
            ("--pause-hold", "pause_hold", None, None),
            ("--combat-pulses", "combat_pulses", None, None),
            ("--combat-delay", "combat_delay", None, None),
        ),
    )
    if ns.pause_pulses > 64 or ns.combat_pulses > 64:
        fail("--pause-pulses and --combat-pulses must be <= 64")
    if ns.pause_pulses == 0 and ns.combat_pulses == 0:
        fail("At least one of --pause-pulses or --combat-pulses must be > 0")

    outdir = normalize_outdir(ns.outdir, f"remote-combat-slot-bounds-p{ns.instances}")
    lane_outdir = outdir / "lane-remote-combat"
    csv_path = outdir / "remote_combat_results.csv"
    summary_path = outdir / "summary.env"
    reset_paths(lane_outdir, csv_path, summary_path)

    lane_name = f"remote-combat-p{ns.instances}"
    csv_header = [
        "lane",
        "instances",
        "result",
        "child_result",
        "remote_combat_slot_bounds_ok",
        "remote_combat_events_ok",
        "remote_combat_slot_ok_lines",
        "remote_combat_slot_fail_lines",
        "remote_combat_event_lines",
        "remote_combat_event_contexts",
        "remote_combat_auto_pause_action_lines",
        "remote_combat_auto_pause_complete_lines",
        "remote_combat_auto_enemy_bar_lines",
        "remote_combat_auto_enemy_complete_lines",
        "outdir",
    ]

    log(f"Running lane {lane_name}")
    _rc, child_result, values, _summary_file = run_ns_helo_child_lane(
        run_helo_child_lane,
        ns,
        instances=ns.instances,
        expected_players=ns.instances,
        timeout=ns.timeout,
        lane_outdir=lane_outdir,
        lane_args=build_default_helo_lane_args(
            "--auto-start-delay",
            "2",
            "--trace-remote-combat-slot-bounds",
            "1",
            "--require-remote-combat-slot-bounds",
            "1",
            "--require-remote-combat-events",
            "1",
            "--auto-pause-pulses",
            str(ns.pause_pulses),
            "--auto-pause-delay",
            str(ns.pause_delay),
            "--auto-pause-hold",
            str(ns.pause_hold),
            "--auto-remote-combat-pulses",
            str(ns.combat_pulses),
            "--auto-remote-combat-delay",
            str(ns.combat_delay),
            auto_start=1,
        ),
        summary_defaults={
            "REMOTE_COMBAT_SLOT_BOUNDS_OK": "0",
            "REMOTE_COMBAT_EVENTS_OK": "0",
            "REMOTE_COMBAT_SLOT_OK_LINES": "0",
            "REMOTE_COMBAT_SLOT_FAIL_LINES": "0",
            "REMOTE_COMBAT_EVENT_LINES": "0",
            "REMOTE_COMBAT_EVENT_CONTEXTS": "",
            "REMOTE_COMBAT_AUTO_PAUSE_ACTION_LINES": "0",
            "REMOTE_COMBAT_AUTO_PAUSE_COMPLETE_LINES": "0",
            "REMOTE_COMBAT_AUTO_ENEMY_BAR_LINES": "0",
            "REMOTE_COMBAT_AUTO_ENEMY_COMPLETE_LINES": "0",
        },
    )
    lane_result = compute_lane_result(
        child_result,
        values["REMOTE_COMBAT_SLOT_BOUNDS_OK"] == "1",
        values["REMOTE_COMBAT_EVENTS_OK"] == "1",
    )

    summary_payload = write_single_lane_result_files(
        summary_path=summary_path,
        csv_path=csv_path,
        csv_header=csv_header,
        csv_row=[
            lane_name,
            ns.instances,
            lane_result,
            child_result,
            values["REMOTE_COMBAT_SLOT_BOUNDS_OK"],
            values["REMOTE_COMBAT_EVENTS_OK"],
            values["REMOTE_COMBAT_SLOT_OK_LINES"],
            values["REMOTE_COMBAT_SLOT_FAIL_LINES"],
            values["REMOTE_COMBAT_EVENT_LINES"],
            values["REMOTE_COMBAT_EVENT_CONTEXTS"],
            values["REMOTE_COMBAT_AUTO_PAUSE_ACTION_LINES"],
            values["REMOTE_COMBAT_AUTO_PAUSE_COMPLETE_LINES"],
            values["REMOTE_COMBAT_AUTO_ENEMY_BAR_LINES"],
            values["REMOTE_COMBAT_AUTO_ENEMY_COMPLETE_LINES"],
            lane_outdir,
        ],
        lane_result=lane_result,
        outdir=outdir,
        app=ns.app,
        datadir=ns.datadir,
        lane_outdir=lane_outdir,
        summary_extra={
            "INSTANCES": ns.instances,
            "TIMEOUT_SECONDS": ns.timeout,
            "PAUSE_PULSES": ns.pause_pulses,
            "PAUSE_DELAY_SECONDS": ns.pause_delay,
            "PAUSE_HOLD_SECONDS": ns.pause_hold,
            "COMBAT_PULSES": ns.combat_pulses,
            "COMBAT_DELAY_SECONDS": ns.combat_delay,
        },
    )
    prune_models_cache(lane_outdir)
    overall_result = str(summary_payload["RESULT"])

    log(
        "result="
        f"{overall_result} lane={lane_result} "
        f"slotBoundsOk={values['REMOTE_COMBAT_SLOT_BOUNDS_OK']} "
        f"eventsOk={values['REMOTE_COMBAT_EVENTS_OK']} "
        f"slotFail={values['REMOTE_COMBAT_SLOT_FAIL_LINES']}"
    )
    log(f"csv={csv_path}")
    log(f"summary={summary_path}")
    return 1 if overall_result != "pass" else 0


def cmd_lobby_slot_lock_and_kick_copy(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--stagger", "stagger", None, None),
            ("--timeout", "timeout", None, None),
            ("--player-count-delay", "player_count_delay", None, None),
        ),
    )

    outdir = normalize_outdir(ns.outdir, "lobby-slot-lock-kick-copy")
    for lane_path in outdir.glob("lane-*"):
        if lane_path.is_dir():
            shutil.rmtree(lane_path, ignore_errors=True)
    csv_path = outdir / "slot_lock_kick_copy_results.csv"
    summary_path = outdir / "summary.env"
    reset_paths(csv_path, summary_path)

    write_csv_header(
        csv_path,
        [
            "lane",
            "instances",
            "expected_players",
            "auto_player_count_target",
            "expected_variant",
            "result",
            "child_result",
            "default_slot_lock_ok",
            "slot_lock_snapshot_lines",
            "player_count_copy_ok",
            "player_count_prompt_variant",
            "player_count_prompt_kicked",
            "player_count_prompt_lines",
            "outdir",
        ],
    )

    total_lanes = 0
    pass_lanes = 0
    fail_lanes = 0

    for lane in SLOT_LOCK_KICK_COPY_LANES:
        lane_outdir = outdir / f"lane-{lane.lane_name}"
        log(f"Running lane {lane.lane_name}")

        lane_args: list[str] = []
        if lane.require_default_lock:
            lane_args.extend(["--trace-slot-locks", "1", "--require-default-slot-locks", "1"])
        if lane.auto_player_count_target > 0:
            lane_args.extend(
                [
                    "--auto-player-count-target",
                    str(lane.auto_player_count_target),
                    "--auto-player-count-delay",
                    str(ns.player_count_delay),
                ]
            )
        if lane.require_copy:
            lane_args.extend(
                [
                    "--trace-player-count-copy",
                    "1",
                    "--require-player-count-copy",
                    "1",
                    "--expect-player-count-copy-variant",
                    lane.expected_variant,
                ]
            )

        _rc, child_result, values, _summary_file = run_ns_helo_child_lane(
            run_helo_child_lane,
            ns,
            instances=lane.instances,
            expected_players=lane.expected_players,
            timeout=ns.timeout,
            lane_outdir=lane_outdir,
            lane_args=build_default_helo_lane_args(*lane_args, auto_start=0),
            summary_defaults={
                "DEFAULT_SLOT_LOCK_OK": "0",
                "SLOT_LOCK_SNAPSHOT_LINES": "0",
                "PLAYER_COUNT_COPY_OK": "0",
                "PLAYER_COUNT_PROMPT_VARIANT": "missing",
                "PLAYER_COUNT_PROMPT_KICKED": "0",
                "PLAYER_COUNT_PROMPT_LINES": "0",
            },
        )

        lane_result = compute_lane_result(
            child_result,
            (not lane.require_default_lock) or values["DEFAULT_SLOT_LOCK_OK"] == "1",
            (not lane.require_copy) or values["PLAYER_COUNT_COPY_OK"] == "1",
            (not lane.require_copy) or values["PLAYER_COUNT_PROMPT_VARIANT"] == lane.expected_variant,
        )

        append_csv_row(
            csv_path,
            [
                lane.lane_name,
                lane.instances,
                lane.expected_players,
                lane.auto_player_count_target,
                lane.expected_variant,
                lane_result,
                child_result,
                values["DEFAULT_SLOT_LOCK_OK"],
                values["SLOT_LOCK_SNAPSHOT_LINES"],
                values["PLAYER_COUNT_COPY_OK"],
                values["PLAYER_COUNT_PROMPT_VARIANT"],
                values["PLAYER_COUNT_PROMPT_KICKED"],
                values["PLAYER_COUNT_PROMPT_LINES"],
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
            "TIMEOUT_SECONDS": ns.timeout,
            "PLAYER_COUNT_DELAY_SECONDS": ns.player_count_delay,
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
