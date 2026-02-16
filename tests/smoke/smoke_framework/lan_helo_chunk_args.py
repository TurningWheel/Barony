from __future__ import annotations

import argparse
from pathlib import Path

from .common import fail, require_uint
from .fs import normalize_outdir
from .helo_metrics import canonicalize_lan_tx_mode


def _require_binary_flag(name: str, value: int) -> None:
    require_uint(name, value, minimum=0, maximum=1)


def validate_and_normalize_args(ns: argparse.Namespace) -> None:
    require_uint("--instances", ns.instances, minimum=1, maximum=15)
    require_uint("--stagger", ns.stagger)
    require_uint("--timeout", ns.timeout)
    require_uint("--auto-start-delay", ns.auto_start_delay)
    require_uint("--auto-enter-dungeon-delay", ns.auto_enter_dungeon_delay)
    if ns.auto_enter_dungeon_repeats is not None:
        require_uint("--auto-enter-dungeon-repeats", ns.auto_enter_dungeon_repeats, minimum=1, maximum=256)
    require_uint("--chunk-payload-max", ns.chunk_payload_max, minimum=64, maximum=900)
    if ns.mapgen_players_override is not None:
        require_uint("--mapgen-players-override", ns.mapgen_players_override, minimum=1, maximum=15)
    require_uint("--mapgen-reload-seed-base", ns.mapgen_reload_seed_base)
    require_uint("--start-floor", ns.start_floor, minimum=0, maximum=99)
    require_uint("--mapgen-samples", ns.mapgen_samples, minimum=1)
    require_uint("--auto-kick-target-slot", ns.auto_kick_target_slot, minimum=0, maximum=14)
    require_uint("--auto-kick-delay", ns.auto_kick_delay)
    require_uint("--auto-player-count-target", ns.auto_player_count_target, minimum=0, maximum=15)
    require_uint("--auto-player-count-delay", ns.auto_player_count_delay)
    require_uint("--auto-lobby-page-delay", ns.auto_lobby_page_delay)
    require_uint("--auto-pause-pulses", ns.auto_pause_pulses, minimum=0, maximum=64)
    require_uint("--auto-pause-delay", ns.auto_pause_delay)
    require_uint("--auto-pause-hold", ns.auto_pause_hold)
    require_uint("--auto-remote-combat-pulses", ns.auto_remote_combat_pulses, minimum=0, maximum=64)
    require_uint("--auto-remote-combat-delay", ns.auto_remote_combat_delay)

    for flag_name, flag_value in [
        ("--auto-start", ns.auto_start),
        ("--auto-enter-dungeon", ns.auto_enter_dungeon),
        ("--force-chunk", ns.force_chunk),
        ("--mapgen-reload-same-level", ns.mapgen_reload_same_level),
        ("--strict-adversarial", ns.strict_adversarial),
        ("--require-txmode-log", ns.require_txmode_log),
        ("--require-mapgen", ns.require_mapgen),
        ("--trace-account-labels", ns.trace_account_labels),
        ("--require-account-labels", ns.require_account_labels),
        ("--require-auto-kick", ns.require_auto_kick),
        ("--trace-slot-locks", ns.trace_slot_locks),
        ("--require-default-slot-locks", ns.require_default_slot_locks),
        ("--trace-player-count-copy", ns.trace_player_count_copy),
        ("--require-player-count-copy", ns.require_player_count_copy),
        ("--trace-lobby-page-state", ns.trace_lobby_page_state),
        ("--require-lobby-page-state", ns.require_lobby_page_state),
        ("--require-lobby-page-focus-match", ns.require_lobby_page_focus_match),
        ("--auto-lobby-page-sweep", ns.auto_lobby_page_sweep),
        ("--require-lobby-page-sweep", ns.require_lobby_page_sweep),
        ("--trace-remote-combat-slot-bounds", ns.trace_remote_combat_slot_bounds),
        ("--require-remote-combat-slot-bounds", ns.require_remote_combat_slot_bounds),
        ("--require-remote-combat-events", ns.require_remote_combat_events),
    ]:
        _require_binary_flag(flag_name, flag_value)

    if ns.expected_players is None:
        ns.expected_players = ns.instances
    require_uint("--expected-players", ns.expected_players, minimum=1, maximum=15)

    normalized_mode = canonicalize_lan_tx_mode(ns.helo_chunk_tx_mode)
    if normalized_mode is None:
        fail(
            "--helo-chunk-tx-mode must be one of: normal, reverse, even-odd, duplicate-first, "
            "drop-last, duplicate-conflict-first"
        )
    ns.helo_chunk_tx_mode = normalized_mode

    if ns.require_helo is None:
        ns.require_helo = 1 if ns.instances > 1 else 0
    _require_binary_flag("--require-helo", ns.require_helo)

    if ns.network_backend not in {"lan", "steam", "eos"}:
        fail("--network-backend must be one of: lan, steam, eos")
    if ns.auto_player_count_target > 0 and ns.auto_player_count_target < 2:
        fail("--auto-player-count-target must be 2..15 when enabled")
    if ns.require_account_labels and not ns.trace_account_labels:
        fail("--require-account-labels requires --trace-account-labels 1")
    if ns.require_auto_kick and ns.auto_kick_target_slot == 0:
        fail("--require-auto-kick requires --auto-kick-target-slot > 0")
    if ns.auto_kick_target_slot > 0 and ns.auto_kick_target_slot >= ns.expected_players:
        fail("--auto-kick-target-slot must be less than --expected-players")
    if ns.require_default_slot_locks and not ns.trace_slot_locks:
        fail("--require-default-slot-locks requires --trace-slot-locks 1")
    if ns.require_player_count_copy and not ns.trace_player_count_copy:
        fail("--require-player-count-copy requires --trace-player-count-copy 1")
    if ns.require_player_count_copy and ns.auto_player_count_target == 0:
        fail("--require-player-count-copy requires --auto-player-count-target > 0")
    if ns.expect_player_count_copy_variant:
        if ns.expect_player_count_copy_variant not in {"single", "double", "multi", "warning-only", "none"}:
            fail("--expect-player-count-copy-variant must be one of: single, double, multi, warning-only, none")
        if not ns.trace_player_count_copy:
            fail("--expect-player-count-copy-variant requires --trace-player-count-copy 1")
        if ns.auto_player_count_target == 0:
            fail("--expect-player-count-copy-variant requires --auto-player-count-target > 0")
    if ns.require_lobby_page_state and not ns.trace_lobby_page_state:
        fail("--require-lobby-page-state requires --trace-lobby-page-state 1")
    if ns.require_lobby_page_focus_match and not ns.trace_lobby_page_state:
        fail("--require-lobby-page-focus-match requires --trace-lobby-page-state 1")
    if ns.require_lobby_page_focus_match and not ns.require_lobby_page_state:
        fail("--require-lobby-page-focus-match requires --require-lobby-page-state 1")
    if ns.require_lobby_page_sweep and not ns.trace_lobby_page_state:
        fail("--require-lobby-page-sweep requires --trace-lobby-page-state 1")
    if ns.require_lobby_page_sweep and not ns.auto_lobby_page_sweep:
        fail("--require-lobby-page-sweep requires --auto-lobby-page-sweep 1")
    if (ns.require_remote_combat_slot_bounds or ns.require_remote_combat_events) and not ns.trace_remote_combat_slot_bounds:
        fail(
            "--require-remote-combat-slot-bounds/--require-remote-combat-events "
            "require --trace-remote-combat-slot-bounds 1"
        )
    if ns.require_remote_combat_events and ns.auto_pause_pulses == 0 and ns.auto_remote_combat_pulses == 0:
        fail(
            "--require-remote-combat-events requires at least one of "
            "--auto-pause-pulses or --auto-remote-combat-pulses"
        )

    if ns.auto_enter_dungeon_repeats is None:
        ns.auto_enter_dungeon_repeats = ns.mapgen_samples


def resolve_lane_outdir(ns: argparse.Namespace) -> Path:
    return normalize_outdir(ns.outdir, f"helo-p{ns.instances}")
