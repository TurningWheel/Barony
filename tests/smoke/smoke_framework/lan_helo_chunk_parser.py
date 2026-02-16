from __future__ import annotations

import argparse
from pathlib import Path

from .lan_helo_chunk_lane import cmd_lan_helo_chunk
from .parser_common import add_app_datadir_args


def _add_base_lane_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--instances", type=int, default=4, help="Number of game instances to launch (1..15).")
    parser.add_argument("--size", default="1280x720", help="Window size.")
    parser.add_argument("--stagger", type=int, default=1, help="Delay between launches.")
    parser.add_argument("--timeout", type=int, default=120, help="Timeout for pass/fail checks in seconds.")
    parser.add_argument(
        "--connect-address",
        default="127.0.0.1:57165",
        help="Client connect target. LAN uses host:port; online uses lobby key.",
    )
    parser.add_argument(
        "--expected-players",
        type=int,
        default=None,
        help="Host auto-start threshold (default: --instances).",
    )
    parser.add_argument("--auto-start", type=int, default=0, help="Host starts game when expected players connected.")
    parser.add_argument("--auto-start-delay", type=int, default=2, help="Delay after expected players threshold.")
    parser.add_argument(
        "--auto-enter-dungeon",
        type=int,
        default=0,
        help="Host forces first dungeon transition after all players load (0/1).",
    )
    parser.add_argument(
        "--auto-enter-dungeon-delay",
        type=int,
        default=3,
        help="Delay before forcing dungeon entry.",
    )
    parser.add_argument(
        "--auto-enter-dungeon-repeats",
        type=int,
        default=None,
        help="Max smoke-driven dungeon transitions (default: --mapgen-samples).",
    )
    parser.add_argument("--force-chunk", type=int, default=1, help="Enable BARONY_SMOKE_FORCE_HELO_CHUNK (0/1).")
    parser.add_argument("--chunk-payload-max", type=int, default=200, help="Smoke chunk payload cap (64..900).")
    parser.add_argument(
        "--helo-chunk-tx-mode",
        default="normal",
        help="HELO tx mode: normal, reverse, even-odd, duplicate-first, drop-last, duplicate-conflict-first.",
    )
    parser.add_argument(
        "--network-backend",
        default="lan",
        help="Network backend to execute (lan|steam|eos).",
    )
    parser.add_argument("--strict-adversarial", type=int, default=0, help="Enable strict adversarial assertions (0/1).")
    parser.add_argument("--require-txmode-log", type=int, default=0, help="Require tx-mode host logs in non-normal modes (0/1).")
    parser.add_argument("--seed", default="", help="Optional seed string for host run.")
    parser.add_argument("--require-helo", type=int, default=None, help="Require HELO chunk/reassembly checks (0/1).")
    parser.add_argument("--require-mapgen", type=int, default=0, help="Require dungeon mapgen summary in host log (0/1).")
    parser.add_argument("--mapgen-samples", type=int, default=1, help="Required number of mapgen summary lines.")
    parser.add_argument("--outdir", default=None, help="Artifact directory.")
    parser.add_argument("--keep-running", action="store_true", help="Do not kill launched instances on exit.")


def _add_mapgen_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--mapgen-players-override",
        type=int,
        default=None,
        help="Smoke-only mapgen scaling player override (1..15).",
    )
    parser.add_argument(
        "--mapgen-control-file",
        type=Path,
        default=None,
        help="Optional file used for dynamic mapgen player override.",
    )
    parser.add_argument(
        "--mapgen-reload-same-level",
        type=int,
        default=0,
        help="Smoke-only gameplay autopilot reloads the same dungeon level between samples (0/1).",
    )
    parser.add_argument(
        "--mapgen-reload-seed-base",
        type=int,
        default=0,
        help="Base seed used for same-level reload samples (0 disables forced seed rotation).",
    )
    parser.add_argument("--start-floor", type=int, default=0, help="Smoke-only host start floor (0..99).")


def _add_lobby_trace_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--trace-account-labels", type=int, default=0, help="Trace lobby account labels on host (0/1).")
    parser.add_argument("--require-account-labels", type=int, default=0, help="Require account-label coverage for remote slots (0/1).")
    parser.add_argument("--auto-kick-target-slot", type=int, default=0, help="Host smoke autopilot kicks this player slot (1..14, 0 disables).")
    parser.add_argument("--auto-kick-delay", type=int, default=2, help="Delay after full lobby before auto-kick.")
    parser.add_argument("--require-auto-kick", type=int, default=0, help="Require smoke auto-kick verification before pass (0/1).")
    parser.add_argument("--trace-slot-locks", type=int, default=0, help="Trace slot-lock snapshots during lobby initialization (0/1).")
    parser.add_argument("--require-default-slot-locks", type=int, default=0, help="Require default host slot-lock assertions (0/1).")
    parser.add_argument("--auto-player-count-target", type=int, default=0, help="Host autopilot requested lobby player-count target (2..15, 0 disables).")
    parser.add_argument("--auto-player-count-delay", type=int, default=2, help="Delay after full lobby before host requests player-count change.")
    parser.add_argument("--trace-player-count-copy", type=int, default=0, help="Trace occupied-slot count-reduction prompt copy (0/1).")
    parser.add_argument("--require-player-count-copy", type=int, default=0, help="Require player-count prompt trace before pass (0/1).")
    parser.add_argument(
        "--expect-player-count-copy-variant",
        default="",
        help="Expected prompt variant: single, double, multi, warning-only, none.",
    )
    parser.add_argument("--trace-lobby-page-state", type=int, default=0, help="Trace lobby page/focus/alignment snapshots while paging (0/1).")
    parser.add_argument("--require-lobby-page-state", type=int, default=0, help="Require lobby page alignment assertions before pass (0/1).")
    parser.add_argument("--require-lobby-page-focus-match", type=int, default=0, help="Require focused widget page match in traced lobby snapshots (0/1).")
    parser.add_argument("--auto-lobby-page-sweep", type=int, default=0, help="Host autopilot sweeps visible lobby pages after full lobby (0/1).")
    parser.add_argument("--auto-lobby-page-delay", type=int, default=2, help="Delay between host auto page changes.")
    parser.add_argument("--require-lobby-page-sweep", type=int, default=0, help="Require full visible-page sweep coverage before pass (0/1).")


def _add_remote_combat_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--trace-remote-combat-slot-bounds", type=int, default=0, help="Trace remote-combat slot bounds/events (0/1).")
    parser.add_argument("--require-remote-combat-slot-bounds", type=int, default=0, help="Require zero remote-combat slot-check failures and at least one success (0/1).")
    parser.add_argument("--require-remote-combat-events", type=int, default=0, help="Require remote-combat event coverage (0/1).")
    parser.add_argument("--auto-pause-pulses", type=int, default=0, help="Host autopilot issues pause/unpause pulses (0 disables).")
    parser.add_argument("--auto-pause-delay", type=int, default=2, help="Delay before each pause pulse.")
    parser.add_argument("--auto-pause-hold", type=int, default=1, help="Pause hold duration before unpause.")
    parser.add_argument("--auto-remote-combat-pulses", type=int, default=0, help="Host autopilot triggers enemy-bar combat pulses (0 disables).")
    parser.add_argument("--auto-remote-combat-delay", type=int, default=2, help="Delay between host enemy-bar combat pulses.")


def register_lan_helo_chunk_parser(
    sub: argparse._SubParsersAction[argparse.ArgumentParser],
    *,
    default_app: Path,
) -> None:
    helo_chunk = sub.add_parser("lan-helo-chunk", help="Run base host/client HELO chunk lane")
    add_app_datadir_args(helo_chunk, default_app=default_app)
    _add_base_lane_args(helo_chunk)
    _add_mapgen_args(helo_chunk)
    _add_lobby_trace_args(helo_chunk)
    _add_remote_combat_args(helo_chunk)
    helo_chunk.set_defaults(handler=cmd_lan_helo_chunk)
