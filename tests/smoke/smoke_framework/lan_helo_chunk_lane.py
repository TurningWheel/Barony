from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

from .common import log
from .fs import reset_paths
from .helo_metrics import (
    is_expected_fail_tx_mode,
    refresh_optional_assertion_metrics,
)
from .lan_helo_chunk_args import resolve_lane_outdir, validate_and_normalize_args
from .lan_helo_chunk_launch import (
    LaunchedSmokeInstance,
    cleanup_launched_instances,
    launch_backend_instances,
    launch_lane_instance,
)
from .lan_helo_chunk_post import collect_post_run_metrics, enforce_required_assertions
from .lan_helo_chunk_runtime import run_runtime_wait_loop
from .lan_helo_chunk_summary import build_summary_data
from .orchestration import RunnerOrchestrator
from .summary import write_summary_env

SCRIPT_DIR = Path(__file__).resolve().parent.parent
RUNNER = SCRIPT_DIR / "smoke_runner.py"
RUNNER_PYTHON = sys.executable or "python3"
_ORCH = RunnerOrchestrator(runner_path=RUNNER, runner_python=RUNNER_PYTHON)
validate_app_environment = _ORCH.validate_app_environment


def cmd_lan_helo_chunk(ns: argparse.Namespace) -> int:
    validate_app_environment(ns.app, ns.datadir)
    validate_and_normalize_args(ns)
    outdir = resolve_lane_outdir(ns)

    log_dir = outdir / "stdout"
    instance_root = outdir / "instances"
    pid_file = outdir / "pids.txt"
    summary_file = outdir / "summary.env"

    reset_paths(log_dir, instance_root, pid_file, summary_file)
    log_dir.mkdir(parents=True, exist_ok=True)
    instance_root.mkdir(parents=True, exist_ok=True)
    pid_file.write_text("", encoding="utf-8")

    seed_config_path = Path.home() / ".barony/config/config.json"
    seed_books_path = Path.home() / ".barony/books/compiled_books.json"
    mapgen_control_file = ns.mapgen_control_file.expanduser() if ns.mapgen_control_file else None
    mapgen_players_override = ns.mapgen_players_override
    connect_address = ns.connect_address

    launched_instances: list[LaunchedSmokeInstance] = []
    host_log = instance_root / "home-1/.barony/log.txt"
    host_stdout_log = log_dir / "instance-1.stdout.log"
    if ns.network_backend != "lan":
        host_log = host_stdout_log

    def launch_instance(idx: int, role: str, connect_address_value: str) -> None:
        launched = launch_lane_instance(
            ns,
            idx=idx,
            role=role,
            instance_root=instance_root,
            log_dir=log_dir,
            seed_config_path=seed_config_path,
            seed_books_path=seed_books_path,
            connect_address=connect_address_value,
            mapgen_control_file=mapgen_control_file,
            mapgen_players_override=mapgen_players_override,
        )
        launched_instances.append(launched)
        with pid_file.open("a", encoding="utf-8") as f:
            f.write(f"{launched.proc.pid} {idx} {role} {launched.home_dir}\n")
        log(f"instance={idx} role={role} pid={launched.proc.pid} home={launched.home_dir}")

    def cleanup_instances() -> None:
        cleanup_launched_instances(
            launched_instances,
            keep_running=ns.keep_running,
            logger=log,
        )

    log(f"Artifacts: {outdir}")

    backend_room_key = ""
    backend_room_key_found = 1
    backend_launch_blocked = 0

    expected_clients = ns.instances - 1 if ns.instances > 1 else 0
    expected_chunk_lines = expected_clients
    expected_reassembled_lines = expected_clients
    expected_fail_tx_mode = is_expected_fail_tx_mode(ns.helo_chunk_tx_mode)
    txmode_required = 1 if (ns.require_txmode_log and ns.helo_chunk_tx_mode != "normal") else 0
    strict_expected_fail = 1 if (ns.strict_adversarial and ns.require_helo and expected_fail_tx_mode) else 0

    result = "fail"
    start_time = time.monotonic()
    deadline = start_time + ns.timeout
    runtime: dict[str, int | str] = {
        "host_chunk_lines": 0,
        "client_reassembled_lines": 0,
        "chunk_reset_lines": 0,
        "tx_mode_log_lines": 0,
        "tx_mode_packet_plan_ok": 0,
        "tx_mode_applied": 0,
        "per_client_reassembly_counts": "",
        "mapgen_wait_reason": "none",
    }

    try:
        connect_address, backend_room_key, backend_room_key_found, backend_launch_blocked = launch_backend_instances(
            ns,
            host_log=host_log,
            launch_instance=launch_instance,
            logger=log,
            connect_address=connect_address,
        )

        client_logs = [instance_root / f"home-{idx}/.barony/log.txt" for idx in range(2, ns.instances + 1)]
        all_logs = [host_log, *client_logs]

        runtime_result, runtime = run_runtime_wait_loop(
            ns,
            host_log=host_log,
            instance_root=instance_root,
            all_logs=all_logs,
            expected_clients=expected_clients,
            expected_chunk_lines=expected_chunk_lines,
            expected_reassembled_lines=expected_reassembled_lines,
            expected_fail_tx_mode=expected_fail_tx_mode,
            txmode_required=txmode_required,
            strict_expected_fail=strict_expected_fail,
            deadline=deadline,
            backend_launch_blocked=backend_launch_blocked,
            logger=log,
        )
        if runtime_result == "pass":
            result = "pass"
        mapgen_wait_reason = str(runtime["mapgen_wait_reason"])

        post = collect_post_run_metrics(
            ns,
            host_log=host_log,
            client_logs=client_logs,
            expected_clients=expected_clients,
            deadline=deadline,
            mapgen_wait_reason=mapgen_wait_reason,
        )
        mapgen_wait_reason = str(post["MAPGEN_WAIT_REASON"])
        mapgen_reload_regen_ok = int(post["MAPGEN_RELOAD_REGEN_OK"])

        optional = refresh_optional_assertion_metrics(
            host_log=host_log,
            all_logs=all_logs,
            expected_clients=expected_clients,
            expected_players=ns.expected_players,
            ns=ns,
        )

        result = enforce_required_assertions(
            ns,
            result=result,
            optional=optional,
            mapgen_wait_reason=mapgen_wait_reason,
            mapgen_reload_regen_ok=mapgen_reload_regen_ok,
        )

        summary_data = build_summary_data(
            ns,
            result=result,
            outdir=outdir,
            connect_address=connect_address,
            backend_room_key=backend_room_key,
            backend_room_key_found=backend_room_key_found,
            backend_launch_blocked=backend_launch_blocked,
            mapgen_players_override=mapgen_players_override,
            mapgen_control_file=mapgen_control_file,
            expected_fail_tx_mode=expected_fail_tx_mode,
            runtime=runtime,
            post=post,
            optional=optional,
            mapgen_wait_reason=mapgen_wait_reason,
            mapgen_reload_regen_ok=mapgen_reload_regen_ok,
            host_log=host_log,
            pid_file=pid_file,
        )
        write_summary_env(summary_file, summary_data)

        log(
            "result="
            f"{result} backend={ns.network_backend} roomKeyFound={backend_room_key_found} "
            f"launchBlocked={backend_launch_blocked} chunks={runtime['host_chunk_lines']} "
            f"reassembled={runtime['client_reassembled_lines']} resets={runtime['chunk_reset_lines']} "
            f"txmodeApplied={runtime['tx_mode_applied']} mapgen={post['MAPGEN_FOUND']} mapgenWait={mapgen_wait_reason} "
            f"mapgenReloadRegenOk={mapgen_reload_regen_ok} gamestart={post['GAMESTART_FOUND']} "
            f"autoKick={optional['auto_kick_result']} slotLockOk={optional['default_slot_lock_ok']} "
            f"playerCountCopyOk={optional['player_count_copy_ok']} "
            f"lobbyPageStateOk={optional['lobby_page_state_ok']} "
            f"lobbyPageSweepOk={optional['lobby_page_sweep_ok']} "
            f"remoteSlotOk={optional['remote_combat_slot_bounds_ok']} "
            f"remoteEventsOk={optional['remote_combat_events_ok']} "
            f"remoteSlotFail={optional['remote_combat_slot_fail_lines']}"
        )
        log(f"summary={summary_file}")
        return 0 if result == "pass" else 1
    finally:
        cleanup_instances()
