from __future__ import annotations

import argparse
from pathlib import Path

from .core_lane import (
    cmd_helo_adversarial,
    cmd_helo_soak,
    cmd_lobby_kick_target,
    cmd_save_reload_compat,
)
from .parser_common import add_app_datadir_args

_LANE_RUNNER_DATADIR_HELP = "Optional data directory passed through to lane runner."


def _add_size_stagger_outdir(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--size", default="1280x720", help="Window size for all instances.")
    parser.add_argument("--stagger", type=int, default=1, help="Delay between launches (seconds).")
    parser.add_argument("--outdir", default=None, help="Output directory.")


def _add_chunk_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--force-chunk", type=int, default=1, help="BARONY_SMOKE_FORCE_HELO_CHUNK (0/1).")
    parser.add_argument("--chunk-payload-max", type=int, default=200, help="HELO chunk payload cap (64..900).")


def register_core_lane_parsers(
    sub: argparse._SubParsersAction[argparse.ArgumentParser],
    *,
    default_app: Path,
) -> None:
    soak = sub.add_parser("helo-soak", help="Run repeated LAN HELO soak lane")
    add_app_datadir_args(soak, default_app=default_app, datadir_help=_LANE_RUNNER_DATADIR_HELP)
    soak.add_argument("--runs", type=int, default=10, help="Number of soak runs.")
    soak.add_argument("--instances", type=int, default=8, help="Number of game instances per run.")
    _add_size_stagger_outdir(soak)
    soak.add_argument("--timeout", type=int, default=360, help="Timeout per run (seconds).")
    soak.add_argument("--auto-start-delay", type=int, default=2, help="Auto-start delay after full lobby.")
    soak.add_argument(
        "--auto-enter-dungeon",
        type=int,
        default=1,
        help="Host forces first dungeon transition after load (0/1).",
    )
    soak.add_argument(
        "--auto-enter-dungeon-delay",
        type=int,
        default=3,
        help="Delay before forced dungeon entry (seconds).",
    )
    _add_chunk_args(soak)
    soak.add_argument(
        "--helo-chunk-tx-mode",
        default="normal",
        help="HELO tx mode (normal|reverse|even-odd|duplicate-first|drop-last|duplicate-conflict-first).",
    )
    soak.add_argument("--require-mapgen", type=int, default=1, help="Require dungeon mapgen marker in each run (0/1).")
    soak.set_defaults(handler=cmd_helo_soak)

    adv = sub.add_parser("helo-adversarial", help="Run HELO tx adversarial matrix")
    add_app_datadir_args(adv, default_app=default_app, datadir_help=_LANE_RUNNER_DATADIR_HELP)
    adv.add_argument("--instances", type=int, default=4, help="Number of instances per case (2..15).")
    _add_size_stagger_outdir(adv)
    adv.add_argument("--pass-timeout", type=int, default=180, help="Timeout for expected-pass cases.")
    adv.add_argument("--fail-timeout", type=int, default=60, help="Timeout for expected-fail cases.")
    _add_chunk_args(adv)
    adv.add_argument(
        "--strict-adversarial",
        type=int,
        default=1,
        help="Enable strict per-client pass/fail assertions (0/1).",
    )
    adv.add_argument(
        "--require-txmode-log",
        type=int,
        default=1,
        help="Require tx-mode host logging in non-normal modes (0/1).",
    )
    adv.set_defaults(handler=cmd_helo_adversarial)

    kick = sub.add_parser("lobby-kick-target", help="Run lobby auto-kick target matrix")
    add_app_datadir_args(kick, default_app=default_app, datadir_help=_LANE_RUNNER_DATADIR_HELP)
    _add_size_stagger_outdir(kick)
    kick.add_argument("--timeout", type=int, default=300, help="Timeout per player-count lane (seconds).")
    kick.add_argument("--kick-delay", type=int, default=2, help="Delay before host auto-kick after full lobby.")
    kick.add_argument("--min-players", type=int, default=2, help="Minimum lobby size.")
    kick.add_argument("--max-players", type=int, default=15, help="Maximum lobby size.")
    kick.set_defaults(handler=cmd_lobby_kick_target)

    save = sub.add_parser("save-reload-compat", help="Run save/reload owner encoding sweep")
    add_app_datadir_args(save, default_app=default_app, datadir_help=_LANE_RUNNER_DATADIR_HELP)
    _add_size_stagger_outdir(save)
    save.add_argument("--timeout", type=int, default=540, help="Timeout for owner-sweep lane (seconds).")
    save.set_defaults(handler=cmd_save_reload_compat)
