from __future__ import annotations

import argparse
import time
from pathlib import Path

from .helo_metrics import (
    collect_chunk_reset_reason_counts,
    collect_helo_player_slots,
    collect_missing_helo_player_slots,
    detect_game_start,
    is_helo_player_slot_coverage_ok,
)
from .logscan import file_count_fixed_lines
from .mapgen import (
    collect_mapgen_generation_seeds,
    collect_reload_transition_seeds,
    count_list_values,
    count_seed_matches,
    count_unique_list_values,
    extract_mapgen_metrics,
)


def collect_post_run_metrics(
    ns: argparse.Namespace,
    *,
    host_log: Path,
    client_logs: list[Path],
    expected_clients: int,
    deadline: float,
    mapgen_wait_reason: str,
) -> dict[str, int | str]:
    game_start_found = detect_game_start(host_log)
    (
        mapgen_found,
        rooms,
        monsters,
        gold,
        items,
        decorations,
        decor_blocking,
        decor_utility,
        decor_traps,
        decor_economy,
        food_items,
        food_servings,
        gold_bags,
        gold_amount,
        item_stacks,
        item_units,
        mapgen_level,
        mapgen_secret,
        mapgen_seed,
    ) = extract_mapgen_metrics(host_log)

    mapgen_count = file_count_fixed_lines(host_log, "successfully generated a dungeon with")
    if (
        mapgen_wait_reason == "none"
        and ns.require_mapgen
        and mapgen_count < ns.mapgen_samples
        and time.monotonic() >= deadline
    ):
        mapgen_wait_reason = "timeout-before-mapgen-samples"

    reload_transition_lines = file_count_fixed_lines(host_log, "[SMOKE]: auto-reloading dungeon level transition")
    reload_transition_seeds = collect_reload_transition_seeds(host_log)
    reload_transition_seed_count = count_list_values(reload_transition_seeds)
    mapgen_generation_count = file_count_fixed_lines(host_log, "generating a dungeon from level set '")
    mapgen_generation_seeds = collect_mapgen_generation_seeds(host_log)
    mapgen_generation_seed_count = count_list_values(mapgen_generation_seeds)
    mapgen_generation_unique_seed_count = count_unique_list_values(mapgen_generation_seeds)
    mapgen_reload_seed_match_count = count_seed_matches(reload_transition_seeds, mapgen_generation_seeds)
    mapgen_reload_regen_ok = 1
    if ns.mapgen_reload_same_level and reload_transition_lines > 0:
        if ns.mapgen_reload_seed_base > 0:
            if mapgen_reload_seed_match_count < reload_transition_lines:
                mapgen_reload_regen_ok = 0
        elif mapgen_generation_count < reload_transition_lines:
            mapgen_reload_regen_ok = 0

    return {
        "GAMESTART_FOUND": game_start_found,
        "MAPGEN_FOUND": mapgen_found,
        "MAPGEN_COUNT": mapgen_count,
        "MAPGEN_WAIT_REASON": mapgen_wait_reason,
        "MAPGEN_ROOMS": rooms,
        "MAPGEN_MONSTERS": monsters,
        "MAPGEN_GOLD": gold,
        "MAPGEN_ITEMS": items,
        "MAPGEN_DECORATIONS": decorations,
        "MAPGEN_DECOR_BLOCKING": decor_blocking,
        "MAPGEN_DECOR_UTILITY": decor_utility,
        "MAPGEN_DECOR_TRAPS": decor_traps,
        "MAPGEN_DECOR_ECONOMY": decor_economy,
        "MAPGEN_FOOD_ITEMS": food_items,
        "MAPGEN_FOOD_SERVINGS": food_servings,
        "MAPGEN_GOLD_BAGS": gold_bags,
        "MAPGEN_GOLD_AMOUNT": gold_amount,
        "MAPGEN_ITEM_STACKS": item_stacks,
        "MAPGEN_ITEM_UNITS": item_units,
        "MAPGEN_LEVEL": mapgen_level,
        "MAPGEN_SECRET": mapgen_secret,
        "MAPGEN_SEED": mapgen_seed,
        "MAPGEN_RELOAD_TRANSITION_LINES": reload_transition_lines,
        "MAPGEN_RELOAD_TRANSITION_SEEDS": reload_transition_seeds,
        "MAPGEN_RELOAD_TRANSITION_SEED_COUNT": reload_transition_seed_count,
        "MAPGEN_GENERATION_LINES": mapgen_generation_count,
        "MAPGEN_GENERATION_SEEDS": mapgen_generation_seeds,
        "MAPGEN_GENERATION_SEED_COUNT": mapgen_generation_seed_count,
        "MAPGEN_GENERATION_UNIQUE_SEED_COUNT": mapgen_generation_unique_seed_count,
        "MAPGEN_RELOAD_SEED_MATCH_COUNT": mapgen_reload_seed_match_count,
        "MAPGEN_RELOAD_REGEN_OK": mapgen_reload_regen_ok,
        "HELO_PLAYER_SLOTS": collect_helo_player_slots(host_log),
        "HELO_MISSING_PLAYER_SLOTS": collect_missing_helo_player_slots(host_log, expected_clients),
        "HELO_PLAYER_SLOT_COVERAGE_OK": is_helo_player_slot_coverage_ok(host_log, expected_clients),
        "CHUNK_RESET_REASON_COUNTS": collect_chunk_reset_reason_counts(client_logs),
    }


def enforce_required_assertions(
    ns: argparse.Namespace,
    *,
    result: str,
    optional: dict[str, int | str],
    mapgen_wait_reason: str,
    mapgen_reload_regen_ok: int,
) -> str:
    if ns.require_account_labels and int(optional["account_label_slot_coverage_ok"]) == 0:
        result = "fail"
    if ns.require_auto_kick and str(optional["auto_kick_result"]) != "ok":
        result = "fail"
    if ns.require_default_slot_locks and int(optional["default_slot_lock_ok"]) == 0:
        result = "fail"
    if ns.require_player_count_copy and int(optional["player_count_copy_ok"]) == 0:
        result = "fail"
    if ns.require_lobby_page_state and int(optional["lobby_page_state_ok"]) == 0:
        result = "fail"
    if ns.require_lobby_page_sweep and int(optional["lobby_page_sweep_ok"]) == 0:
        result = "fail"
    if ns.require_remote_combat_slot_bounds and int(optional["remote_combat_slot_bounds_ok"]) == 0:
        result = "fail"
    if ns.require_remote_combat_events and int(optional["remote_combat_events_ok"]) == 0:
        result = "fail"
    if mapgen_wait_reason != "none":
        result = "fail"
    if ns.require_mapgen and ns.mapgen_reload_same_level and mapgen_reload_regen_ok == 0:
        result = "fail"
    return result
