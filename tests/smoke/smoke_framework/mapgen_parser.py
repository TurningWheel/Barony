from __future__ import annotations

import argparse
from pathlib import Path

from .mapgen_matrix_lane import cmd_mapgen_level_matrix
from .mapgen_sweep_lane import cmd_mapgen_sweep
from .parser_common import add_app_datadir_args


def _add_mapgen_common_args(
    parser: argparse.ArgumentParser,
    *,
    runs_per_player_default: int,
    stagger_default: int,
    timeout_default: int,
    auto_start_delay_default: int,
    simulate_mapgen_players_default: int,
    inprocess_sim_batch_default: int,
    inprocess_player_sweep_default: int,
    mapgen_reload_same_level_default: int,
    include_start_floor: bool,
) -> None:
    parser.add_argument("--min-players", type=int, default=1, help="Start player count (1..15).")
    parser.add_argument("--max-players", type=int, default=15, help="End player count (1..15).")
    parser.add_argument(
        "--runs-per-player",
        type=int,
        default=runs_per_player_default,
        help="Runs per player count.",
    )
    parser.add_argument("--base-seed", type=int, default=1000, help="Base seed value.")
    parser.add_argument("--size", default="1280x720", help="Window size for instances.")
    parser.add_argument("--stagger", type=int, default=stagger_default, help="Delay between launches (seconds).")
    parser.add_argument("--timeout", type=int, default=timeout_default, help="Timeout per run (seconds).")
    parser.add_argument(
        "--auto-start-delay",
        type=int,
        default=auto_start_delay_default,
        help="Host auto-start delay after full lobby.",
    )
    parser.add_argument(
        "--auto-enter-dungeon",
        type=int,
        default=1,
        help="Host forces first dungeon transition after load (0/1).",
    )
    parser.add_argument(
        "--auto-enter-dungeon-delay",
        type=int,
        default=3,
        help="Delay before forced dungeon entry (seconds).",
    )
    parser.add_argument("--force-chunk", type=int, default=1, help="BARONY_SMOKE_FORCE_HELO_CHUNK (0/1).")
    parser.add_argument("--chunk-payload-max", type=int, default=200, help="HELO chunk payload cap (64..900).")
    parser.add_argument(
        "--simulate-mapgen-players",
        type=int,
        default=simulate_mapgen_players_default,
        help="Use one launched instance and simulate mapgen scaling players (0/1).",
    )
    parser.add_argument(
        "--inprocess-sim-batch",
        type=int,
        default=inprocess_sim_batch_default,
        help="In simulated mode, gather all runs-per-player samples from one runtime (0/1).",
    )
    parser.add_argument(
        "--inprocess-player-sweep",
        type=int,
        default=inprocess_player_sweep_default,
        help="In simulated+batch mode, sweep all player counts in one runtime (0/1).",
    )
    if include_start_floor:
        parser.add_argument("--start-floor", type=int, default=0, help="Smoke-only host start floor (0..99).")
    parser.add_argument(
        "--mapgen-reload-same-level",
        type=int,
        default=mapgen_reload_same_level_default,
        help="Reload same generated level between samples (0/1).",
    )
    parser.add_argument(
        "--mapgen-reload-seed-base",
        type=int,
        default=0,
        help="Base seed used for same-level reload samples (0 disables forced seed rotation).",
    )


def register_mapgen_lane_parsers(
    sub: argparse._SubParsersAction[argparse.ArgumentParser], *, default_app: Path
) -> None:
    mapgen_sweep = sub.add_parser("mapgen-sweep", help="Run mapgen player-count sweep lane")
    add_app_datadir_args(
        mapgen_sweep,
        default_app=default_app,
        datadir_help="Optional data directory passed through to lane runner.",
    )
    _add_mapgen_common_args(
        mapgen_sweep,
        runs_per_player_default=1,
        stagger_default=1,
        timeout_default=180,
        auto_start_delay_default=2,
        simulate_mapgen_players_default=0,
        inprocess_sim_batch_default=1,
        inprocess_player_sweep_default=1,
        mapgen_reload_same_level_default=0,
        include_start_floor=True,
    )
    mapgen_sweep.add_argument("--outdir", default=None, help="Output directory.")
    mapgen_sweep.set_defaults(handler=cmd_mapgen_sweep)

    mapgen_matrix = sub.add_parser(
        "mapgen-level-matrix",
        help="Run per-floor mapgen sweep matrix and trend summaries",
    )
    add_app_datadir_args(
        mapgen_matrix,
        default_app=default_app,
        datadir_help="Optional data directory passed through to child sweeps.",
    )
    mapgen_matrix.add_argument(
        "--levels",
        default="1,7,16,33",
        help="Comma-separated target generated floors (1..99).",
    )
    _add_mapgen_common_args(
        mapgen_matrix,
        runs_per_player_default=2,
        stagger_default=0,
        timeout_default=180,
        auto_start_delay_default=0,
        simulate_mapgen_players_default=1,
        inprocess_sim_batch_default=1,
        inprocess_player_sweep_default=1,
        mapgen_reload_same_level_default=1,
        include_start_floor=False,
    )
    mapgen_matrix.add_argument("--outdir", default=None, help="Output directory.")
    mapgen_matrix.set_defaults(handler=cmd_mapgen_level_matrix)
