from __future__ import annotations

import argparse

from .common import fail, require_uint


def validate_mapgen_common_args(ns: argparse.Namespace, *, include_start_floor: bool) -> None:
    require_uint("--min-players", ns.min_players, minimum=1, maximum=15)
    require_uint("--max-players", ns.max_players, minimum=1, maximum=15)
    require_uint("--runs-per-player", ns.runs_per_player, minimum=1)
    if ns.min_players > ns.max_players:
        fail("Player range must satisfy 1 <= min <= max <= 15")
    require_uint("--base-seed", ns.base_seed)
    require_uint("--stagger", ns.stagger)
    require_uint("--timeout", ns.timeout)
    require_uint("--auto-start-delay", ns.auto_start_delay)
    require_uint("--auto-enter-dungeon", ns.auto_enter_dungeon, minimum=0, maximum=1)
    require_uint("--auto-enter-dungeon-delay", ns.auto_enter_dungeon_delay)
    require_uint("--force-chunk", ns.force_chunk, minimum=0, maximum=1)
    require_uint("--chunk-payload-max", ns.chunk_payload_max, minimum=64, maximum=900)
    require_uint("--simulate-mapgen-players", ns.simulate_mapgen_players, minimum=0, maximum=1)
    require_uint("--inprocess-sim-batch", ns.inprocess_sim_batch, minimum=0, maximum=1)
    require_uint("--inprocess-player-sweep", ns.inprocess_player_sweep, minimum=0, maximum=1)
    require_uint("--mapgen-reload-same-level", ns.mapgen_reload_same_level, minimum=0, maximum=1)
    require_uint("--mapgen-reload-seed-base", ns.mapgen_reload_seed_base)
    if include_start_floor:
        require_uint("--start-floor", ns.start_floor, minimum=0, maximum=99)
