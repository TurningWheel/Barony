from __future__ import annotations

import argparse
import os
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from .helo_metrics import extract_smoke_room_key
from .local_lane import seed_smoke_home_profile
from .process import launch_local_instance, terminate_process_group


@dataclass
class LaunchedSmokeInstance:
    idx: int
    role: str
    home_dir: Path
    proc: subprocess.Popen[bytes]


def _build_instance_env(
    ns: argparse.Namespace,
    *,
    role: str,
    home_dir: Path,
    connect_address: str,
    mapgen_control_file: Path | None,
    mapgen_players_override: int | None,
) -> dict[str, str]:
    env = os.environ.copy()
    if ns.network_backend == "lan":
        env["HOME"] = str(home_dir)

    env.update(
        {
            "BARONY_SMOKE_AUTOPILOT": "1",
            "BARONY_SMOKE_NETWORK_BACKEND": ns.network_backend,
            "BARONY_SMOKE_CONNECT_DELAY_SECS": "2",
            "BARONY_SMOKE_RETRY_DELAY_SECS": "3",
            "BARONY_SMOKE_FORCE_HELO_CHUNK": str(ns.force_chunk),
            "BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX": str(ns.chunk_payload_max),
            "BARONY_SMOKE_HELO_CHUNK_TX_MODE": ns.helo_chunk_tx_mode,
        }
    )
    if ns.trace_remote_combat_slot_bounds:
        env["BARONY_SMOKE_TRACE_REMOTE_COMBAT_SLOT_BOUNDS"] = "1"

    if role == "host":
        env.update(
            {
                "BARONY_SMOKE_ROLE": "host",
                "BARONY_SMOKE_EXPECTED_PLAYERS": str(ns.expected_players),
                "BARONY_SMOKE_AUTO_START": str(ns.auto_start),
                "BARONY_SMOKE_AUTO_START_DELAY_SECS": str(ns.auto_start_delay),
                "BARONY_SMOKE_AUTO_ENTER_DUNGEON": str(ns.auto_enter_dungeon),
                "BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS": str(ns.auto_enter_dungeon_delay),
                "BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS": str(ns.auto_enter_dungeon_repeats),
            }
        )
        if ns.auto_kick_target_slot > 0:
            env.update(
                {
                    "BARONY_SMOKE_AUTO_KICK_TARGET_SLOT": str(ns.auto_kick_target_slot),
                    "BARONY_SMOKE_AUTO_KICK_DELAY_SECS": str(ns.auto_kick_delay),
                }
            )
        if ns.trace_account_labels:
            env["BARONY_SMOKE_TRACE_ACCOUNT_LABELS"] = "1"
        if ns.trace_slot_locks:
            env["BARONY_SMOKE_TRACE_SLOT_LOCKS"] = "1"
        if ns.trace_player_count_copy:
            env["BARONY_SMOKE_TRACE_PLAYER_COUNT_COPY"] = "1"
        if ns.trace_lobby_page_state:
            env["BARONY_SMOKE_TRACE_LOBBY_PAGE_STATE"] = "1"
        if ns.auto_player_count_target > 0:
            env.update(
                {
                    "BARONY_SMOKE_AUTO_PLAYER_COUNT_TARGET": str(ns.auto_player_count_target),
                    "BARONY_SMOKE_AUTO_PLAYER_COUNT_DELAY_SECS": str(ns.auto_player_count_delay),
                }
            )
        if ns.auto_lobby_page_sweep:
            env.update(
                {
                    "BARONY_SMOKE_AUTO_LOBBY_PAGE_SWEEP": "1",
                    "BARONY_SMOKE_AUTO_LOBBY_PAGE_DELAY_SECS": str(ns.auto_lobby_page_delay),
                }
            )
        if ns.auto_pause_pulses > 0:
            env.update(
                {
                    "BARONY_SMOKE_AUTO_PAUSE_PULSES": str(ns.auto_pause_pulses),
                    "BARONY_SMOKE_AUTO_PAUSE_DELAY_SECS": str(ns.auto_pause_delay),
                    "BARONY_SMOKE_AUTO_PAUSE_HOLD_SECS": str(ns.auto_pause_hold),
                }
            )
        if ns.auto_remote_combat_pulses > 0:
            env.update(
                {
                    "BARONY_SMOKE_AUTO_REMOTE_COMBAT_PULSES": str(ns.auto_remote_combat_pulses),
                    "BARONY_SMOKE_AUTO_REMOTE_COMBAT_DELAY_SECS": str(ns.auto_remote_combat_delay),
                }
            )
        if mapgen_players_override is not None:
            env["BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS"] = str(mapgen_players_override)
        if mapgen_control_file is not None:
            env["BARONY_SMOKE_MAPGEN_CONTROL_FILE"] = str(mapgen_control_file)
        if ns.mapgen_reload_same_level:
            env["BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL"] = "1"
            if ns.mapgen_reload_seed_base > 0:
                env["BARONY_SMOKE_MAPGEN_RELOAD_SEED_BASE"] = str(ns.mapgen_reload_seed_base)
        if ns.start_floor > 0:
            env["BARONY_SMOKE_START_FLOOR"] = str(ns.start_floor)
        if ns.seed:
            env["BARONY_SMOKE_SEED"] = ns.seed
        return env

    env.update(
        {
            "BARONY_SMOKE_ROLE": "client",
            "BARONY_SMOKE_CONNECT_ADDRESS": connect_address,
        }
    )
    return env


def launch_lane_instance(
    ns: argparse.Namespace,
    *,
    idx: int,
    role: str,
    instance_root: Path,
    log_dir: Path,
    seed_config_path: Path,
    seed_books_path: Path,
    connect_address: str,
    mapgen_control_file: Path | None,
    mapgen_players_override: int | None,
) -> LaunchedSmokeInstance:
    home_dir = instance_root / f"home-{idx}"
    stdout_log = log_dir / f"instance-{idx}.stdout.log"
    home_dir.mkdir(parents=True, exist_ok=True)
    seed_smoke_home_profile(home_dir, seed_config_path, seed_books_path)

    env = _build_instance_env(
        ns,
        role=role,
        home_dir=home_dir,
        connect_address=connect_address,
        mapgen_control_file=mapgen_control_file,
        mapgen_players_override=mapgen_players_override,
    )

    proc = launch_local_instance(
        app=ns.app,
        datadir=ns.datadir,
        size=ns.size,
        home_dir=home_dir,
        stdout_log=stdout_log,
        extra_env=env,
        set_home=(ns.network_backend == "lan"),
    )
    return LaunchedSmokeInstance(idx=idx, role=role, home_dir=home_dir, proc=proc)


def cleanup_launched_instances(
    instances: list[LaunchedSmokeInstance],
    *,
    keep_running: bool,
    logger: Callable[[str], None],
) -> None:
    terminate_process_group(
        [instance.proc for instance in instances],
        keep_running=keep_running,
        keep_running_message="--keep-running enabled; leaving instances alive",
        logger=logger,
        grace_seconds=1.0,
    )
    for instance in instances:
        cache = instance.home_dir / ".barony/models.cache"
        try:
            cache.unlink()
        except OSError:
            pass


def launch_backend_instances(
    ns: argparse.Namespace,
    *,
    host_log: Path,
    launch_instance: Callable[[int, str, str], None],
    logger: Callable[[str], None],
    connect_address: str,
) -> tuple[str, str, int, int]:
    if ns.network_backend == "lan":
        for idx in range(1, ns.instances + 1):
            launch_instance(idx, "host" if idx == 1 else "client", connect_address)
            if ns.stagger > 0:
                time.sleep(ns.stagger)
        return connect_address, "", 1, 0

    launch_instance(1, "host", connect_address)
    if ns.stagger > 0:
        time.sleep(ns.stagger)
    room_key_wait_timeout = min(ns.timeout, 180)
    logger(f"Waiting up to {room_key_wait_timeout}s for {ns.network_backend} room key")
    room_key_deadline = time.monotonic() + room_key_wait_timeout
    backend_room_key = ""
    while time.monotonic() < room_key_deadline:
        backend_room_key = extract_smoke_room_key(host_log, ns.network_backend)
        if backend_room_key:
            break
        time.sleep(1)
    if not backend_room_key:
        logger(f"Failed to capture {ns.network_backend} room key from host log")
        return connect_address, "", 0, 1

    connect_address = backend_room_key
    logger(f"Using {ns.network_backend} room key {connect_address} for client joins")
    for idx in range(2, ns.instances + 1):
        launch_instance(idx, "client", connect_address)
        if ns.stagger > 0:
            time.sleep(ns.stagger)
    return connect_address, backend_room_key, 1, 0
