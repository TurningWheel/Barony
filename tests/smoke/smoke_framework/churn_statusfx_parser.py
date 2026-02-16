from __future__ import annotations

import argparse
from pathlib import Path

from .churn_statusfx_lane import cmd_join_leave_churn, cmd_status_effect_queue_init
from .parser_common import add_app_datadir_args


def _add_common_display_and_output_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--size", default="1280x720", help="Window size.")
    parser.add_argument("--outdir", default=None, help="Output directory.")


def register_join_leave_churn_parser(
    sub: argparse._SubParsersAction[argparse.ArgumentParser],
    *,
    default_app: Path,
) -> None:
    churn = sub.add_parser("join-leave-churn", help="Run LAN join/leave churn lane")
    add_app_datadir_args(churn, default_app=default_app)
    churn.add_argument("--instances", type=int, default=8, help="Total host+client instances (3..15).")
    churn.add_argument("--churn-cycles", type=int, default=2, help="Number of kill/rejoin cycles.")
    churn.add_argument("--churn-count", type=int, default=2, help="Clients churned per cycle.")
    _add_common_display_and_output_args(churn)
    churn.add_argument("--stagger", type=int, default=1, help="Delay between launches (seconds).")
    churn.add_argument(
        "--initial-timeout",
        type=int,
        default=180,
        help="Timeout for initial full lobby handshake (seconds).",
    )
    churn.add_argument(
        "--cycle-timeout",
        type=int,
        default=240,
        help="Timeout for each churn-cycle rejoin (seconds).",
    )
    churn.add_argument("--settle", type=int, default=5, help="Wait after initial full join before churn (seconds).")
    churn.add_argument("--churn-gap", type=int, default=3, help="Delay between kill and relaunch phases (seconds).")
    churn.add_argument("--connect-address", default="127.0.0.1:57165", help="Host address for clients.")
    churn.add_argument("--force-chunk", type=int, default=1, help="BARONY_SMOKE_FORCE_HELO_CHUNK (0/1).")
    churn.add_argument("--chunk-payload-max", type=int, default=200, help="HELO chunk payload cap (64..900).")
    churn.add_argument(
        "--helo-chunk-tx-mode",
        default="normal",
        help="HELO tx mode (normal|reverse|even-odd|duplicate-first|drop-last|duplicate-conflict-first).",
    )
    churn.add_argument("--auto-ready", type=int, default=0, help="Enable BARONY_SMOKE_AUTO_READY on clients (0/1).")
    churn.add_argument(
        "--trace-ready-sync",
        type=int,
        default=0,
        help="Enable host ready-sync trace logs (0/1).",
    )
    churn.add_argument(
        "--require-ready-sync",
        type=int,
        default=0,
        help="Assert ready snapshot queue/send coverage (0/1).",
    )
    churn.add_argument(
        "--trace-join-rejects",
        type=int,
        default=0,
        help="Enable host join-reject slot-state trace logs (0/1).",
    )
    churn.add_argument(
        "--keep-running",
        action="store_true",
        help="Do not kill launched instances on exit.",
    )
    churn.set_defaults(handler=cmd_join_leave_churn)


def register_status_effect_queue_init_parser(
    sub: argparse._SubParsersAction[argparse.ArgumentParser],
    *,
    default_app: Path,
) -> None:
    statusfx = sub.add_parser(
        "status-effect-queue-init",
        help="Run status-effect queue startup and rejoin safety lanes",
    )
    add_app_datadir_args(statusfx, default_app=default_app)
    _add_common_display_and_output_args(statusfx)
    statusfx.add_argument("--stagger", type=int, default=1, help="Delay between launches (seconds).")
    statusfx.add_argument(
        "--startup-timeout",
        type=int,
        default=540,
        help="Timeout for each startup lane (seconds).",
    )
    statusfx.add_argument(
        "--cycle-timeout",
        type=int,
        default=360,
        help="Timeout for each rejoin cycle lane (seconds).",
    )
    statusfx.set_defaults(handler=cmd_status_effect_queue_init)
