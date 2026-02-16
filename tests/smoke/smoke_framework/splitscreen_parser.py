from __future__ import annotations

import argparse
from pathlib import Path

from .splitscreen_baseline_lane import cmd_splitscreen_baseline
from .splitscreen_cap_lane import cmd_splitscreen_cap
from .splitscreen_runtime import add_splitscreen_common_args


def register_splitscreen_lane_parsers(
    sub: argparse._SubParsersAction[argparse.ArgumentParser], *, default_app: Path
) -> None:
    baseline = sub.add_parser(
        "splitscreen-baseline",
        help="Run local 4-player splitscreen baseline lane",
    )
    add_splitscreen_common_args(baseline, default_app=default_app)
    baseline.add_argument(
        "--pause-pulses",
        type=int,
        default=2,
        help="Local pause/unpause pulse count.",
    )
    baseline.add_argument(
        "--pause-delay",
        type=int,
        default=2,
        help="Delay before pause and between pulses (seconds).",
    )
    baseline.add_argument(
        "--pause-hold",
        type=int,
        default=1,
        help="Pause hold duration before unpause (seconds).",
    )
    baseline.set_defaults(handler=cmd_splitscreen_baseline)

    cap = sub.add_parser(
        "splitscreen-cap",
        help="Run local splitscreen cap clamp lane",
    )
    add_splitscreen_common_args(cap, default_app=default_app)
    cap.add_argument(
        "--requested-players",
        type=int,
        default=15,
        help="Requested /splitscreen player count (2..15).",
    )
    cap.add_argument(
        "--cap-delay",
        type=int,
        default=2,
        help="Delay before issuing /splitscreen command sequence (seconds).",
    )
    cap.add_argument(
        "--cap-verify-delay",
        type=int,
        default=1,
        help="Delay before evaluating cap assertions after command (seconds).",
    )
    cap.set_defaults(handler=cmd_splitscreen_cap)
