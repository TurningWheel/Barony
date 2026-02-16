from __future__ import annotations

import argparse
from pathlib import Path

from .lobby_remote_lane import (
    cmd_lobby_page_navigation,
    cmd_lobby_slot_lock_and_kick_copy,
    cmd_remote_combat_slot_bounds,
)
from .parser_common import add_app_datadir_args


def _add_common_lane_args(parser: argparse.ArgumentParser, *, timeout_default: int) -> None:
    parser.add_argument("--size", default="1280x720", help="Window size.")
    parser.add_argument("--stagger", type=int, default=1, help="Delay between launches (seconds).")
    parser.add_argument("--timeout", type=int, default=timeout_default, help="Timeout for lane in seconds.")
    parser.add_argument("--outdir", default=None, help="Artifact directory.")


def register_lobby_remote_parsers(
    sub: argparse._SubParsersAction[argparse.ArgumentParser],
    *,
    default_app: Path,
) -> None:
    page = sub.add_parser("lobby-page-navigation", help="Run lobby page navigation/alignment lane")
    add_app_datadir_args(page, default_app=default_app)
    _add_common_lane_args(page, timeout_default=420)
    page.add_argument(
        "--page-delay",
        type=int,
        default=2,
        help="Delay between host page-sweep steps (seconds).",
    )
    page.add_argument("--instances", type=int, default=15, help="Lobby size for lane (5..15).")
    page.add_argument(
        "--require-focus-match",
        type=int,
        default=0,
        help="Require focused widget page match during sweep (0/1).",
    )
    page.set_defaults(handler=cmd_lobby_page_navigation)

    slot_lock = sub.add_parser(
        "lobby-slot-lock-kick-copy",
        help="Run lobby slot-lock baseline and kick-copy prompt matrix",
    )
    add_app_datadir_args(slot_lock, default_app=default_app)
    _add_common_lane_args(slot_lock, timeout_default=360)
    slot_lock.add_argument(
        "--player-count-delay",
        type=int,
        default=2,
        help="Delay before host auto-requests player-count change (seconds).",
    )
    slot_lock.set_defaults(handler=cmd_lobby_slot_lock_and_kick_copy)

    remote_combat = sub.add_parser(
        "remote-combat-slot-bounds",
        help="Run remote-combat slot bounds and event coverage lane",
    )
    add_app_datadir_args(remote_combat, default_app=default_app)
    _add_common_lane_args(remote_combat, timeout_default=480)
    remote_combat.add_argument("--instances", type=int, default=15, help="Player count for lane (3..15).")
    remote_combat.add_argument(
        "--pause-pulses",
        type=int,
        default=2,
        help="Host pause/unpause pulse count.",
    )
    remote_combat.add_argument(
        "--pause-delay",
        type=int,
        default=2,
        help="Delay before pause and between pause pulses (seconds).",
    )
    remote_combat.add_argument(
        "--pause-hold",
        type=int,
        default=1,
        help="Pause hold duration before unpause (seconds).",
    )
    remote_combat.add_argument(
        "--combat-pulses",
        type=int,
        default=3,
        help="Host enemy-bar pulse count.",
    )
    remote_combat.add_argument(
        "--combat-delay",
        type=int,
        default=2,
        help="Delay between enemy-bar pulses (seconds).",
    )
    remote_combat.set_defaults(handler=cmd_remote_combat_slot_bounds)
