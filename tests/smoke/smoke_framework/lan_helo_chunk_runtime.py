from __future__ import annotations

import argparse
import time
from pathlib import Path
from typing import Callable

from .helo_metrics import (
    is_helo_player_slot_coverage_ok,
    refresh_optional_assertion_metrics,
    tx_mode_packet_plan_valid,
)
from .logscan import LogScanCache, file_count_fixed_lines


def _collect_client_reassembly_state(
    instance_root: Path,
    instances: int,
    *,
    scan_cache: LogScanCache | None = None,
) -> tuple[int, int, str, int, int]:
    client_reassembled_lines = 0
    chunk_reset_lines = 0
    per_client_reassembly_counts = ""
    all_clients_exact_one = 1
    all_clients_zero = 1
    for idx in range(2, instances + 1):
        client_log = instance_root / f"home-{idx}/.barony/log.txt"
        count = file_count_fixed_lines(client_log, "HELO reassembled:", scan_cache=scan_cache)
        client_reassembled_lines += count
        reset_count = file_count_fixed_lines(client_log, "HELO chunk timeout/reset", scan_cache=scan_cache)
        chunk_reset_lines += reset_count
        if per_client_reassembly_counts:
            per_client_reassembly_counts += ";"
        per_client_reassembly_counts += f"{idx}:{count}"
        if count != 1:
            all_clients_exact_one = 0
        if count != 0:
            all_clients_zero = 0
    return (
        client_reassembled_lines,
        chunk_reset_lines,
        per_client_reassembly_counts,
        all_clients_exact_one,
        all_clients_zero,
    )


def run_runtime_wait_loop(
    ns: argparse.Namespace,
    *,
    host_log: Path,
    instance_root: Path,
    all_logs: list[Path],
    expected_clients: int,
    expected_chunk_lines: int,
    expected_reassembled_lines: int,
    expected_fail_tx_mode: int,
    txmode_required: int,
    strict_expected_fail: int,
    deadline: float,
    backend_launch_blocked: int,
    logger: Callable[[str], None],
) -> tuple[str, dict[str, int | str]]:
    state: dict[str, int | str] = {
        "host_chunk_lines": 0,
        "client_reassembled_lines": 0,
        "chunk_reset_lines": 0,
        "tx_mode_log_lines": 0,
        "tx_mode_packet_plan_ok": 0,
        "tx_mode_applied": 0,
        "per_client_reassembly_counts": "",
        "mapgen_wait_reason": "none",
    }

    if backend_launch_blocked:
        logger("Skipping handshake wait loop: backend client launch prerequisites were not met")
        return "fail", state

    mapgen_reload_complete_tick = 0.0
    result = "fail"
    scan_cache = LogScanCache()
    while time.monotonic() < deadline:
        host_chunk_lines = file_count_fixed_lines(
            host_log,
            "sending chunked HELO:",
            scan_cache=scan_cache,
        )
        if ns.helo_chunk_tx_mode == "normal":
            tx_mode_log_lines = 0
            tx_mode_packet_plan_ok = 1
            tx_mode_applied = 0
        else:
            tx_mode_log_lines = file_count_fixed_lines(
                host_log,
                f"[SMOKE]: HELO chunk tx mode={ns.helo_chunk_tx_mode}",
                scan_cache=scan_cache,
            )
            tx_mode_packet_plan_ok = tx_mode_packet_plan_valid(
                host_log,
                ns.helo_chunk_tx_mode,
                scan_cache=scan_cache,
            )
            tx_mode_applied = 1 if tx_mode_log_lines > 0 else 0

        (
            client_reassembled_lines,
            chunk_reset_lines,
            per_client_reassembly_counts,
            all_clients_exact_one,
            all_clients_zero,
        ) = _collect_client_reassembly_state(
            instance_root,
            ns.instances,
            scan_cache=scan_cache,
        )

        mapgen_count = file_count_fixed_lines(
            host_log,
            "successfully generated a dungeon with",
            scan_cache=scan_cache,
        )
        reload_transition_lines = 0
        if ns.mapgen_reload_same_level:
            reload_transition_lines = file_count_fixed_lines(
                host_log,
                "[SMOKE]: auto-reloading dungeon level transition",
                scan_cache=scan_cache,
            )

        helo_ok = 1
        if ns.require_helo:
            if ns.strict_adversarial:
                if expected_fail_tx_mode:
                    helo_ok = 0
                elif host_chunk_lines < expected_chunk_lines or all_clients_exact_one == 0:
                    helo_ok = 0
            elif host_chunk_lines < expected_chunk_lines or client_reassembled_lines < expected_reassembled_lines:
                helo_ok = 0
            if (
                expected_clients > 0
                and is_helo_player_slot_coverage_ok(host_log, expected_clients, scan_cache=scan_cache) == 0
            ):
                helo_ok = 0

        txmode_ok = 1
        if txmode_required and (tx_mode_log_lines < expected_clients or tx_mode_packet_plan_ok == 0):
            txmode_ok = 0

        mapgen_ok = 1
        if ns.require_mapgen and mapgen_count < ns.mapgen_samples:
            mapgen_ok = 0

        if ns.require_mapgen and ns.auto_enter_dungeon and ns.mapgen_reload_same_level and ns.auto_enter_dungeon_repeats > 0:
            if reload_transition_lines >= ns.auto_enter_dungeon_repeats and mapgen_count < ns.mapgen_samples:
                if mapgen_reload_complete_tick == 0:
                    mapgen_reload_complete_tick = time.monotonic()
                elif time.monotonic() - mapgen_reload_complete_tick >= 5:
                    state["mapgen_wait_reason"] = "reload-complete-no-mapgen-samples"
                    break
            else:
                mapgen_reload_complete_tick = 0

        optional = refresh_optional_assertion_metrics(
            host_log=host_log,
            all_logs=all_logs,
            expected_clients=expected_clients,
            expected_players=ns.expected_players,
            ns=ns,
            scan_cache=scan_cache,
        )

        state["host_chunk_lines"] = host_chunk_lines
        state["client_reassembled_lines"] = client_reassembled_lines
        state["chunk_reset_lines"] = chunk_reset_lines
        state["tx_mode_log_lines"] = tx_mode_log_lines
        state["tx_mode_packet_plan_ok"] = tx_mode_packet_plan_ok
        state["tx_mode_applied"] = tx_mode_applied
        state["per_client_reassembly_counts"] = per_client_reassembly_counts

        if strict_expected_fail:
            if all_clients_zero == 0:
                break
            if chunk_reset_lines > 0 and txmode_ok:
                break
        elif (
            helo_ok
            and mapgen_ok
            and txmode_ok
            and int(optional["account_label_ok"])
            and int(optional["auto_kick_ok"])
            and int(optional["default_slot_lock_ok"])
            and int(optional["player_count_copy_ok"])
            and int(optional["lobby_page_state_ok"])
            and int(optional["lobby_page_sweep_ok"])
            and int(optional["remote_combat_slot_bounds_ok"])
            and int(optional["remote_combat_events_ok"])
        ):
            result = "pass"
            break

        time.sleep(1)

    return result, state
