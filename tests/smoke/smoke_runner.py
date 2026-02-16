#!/usr/bin/env python3
"""Unified smoke orchestration runner."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Sequence

from smoke_framework.churn_statusfx_parser import (
    register_join_leave_churn_parser,
    register_status_effect_queue_init_parser,
)
from smoke_framework.core_parser import register_core_lane_parsers
from smoke_framework.lan_helo_chunk_parser import register_lan_helo_chunk_parser
from smoke_framework.lobby_remote_parser import register_lobby_remote_parsers
from smoke_framework.mapgen_parser import register_mapgen_lane_parsers
from smoke_framework.self_check_lane import register_framework_self_check_parser
from smoke_framework.splitscreen_parser import register_splitscreen_lane_parsers


DEFAULT_APP = (
    Path.home()
    / "Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Unified smoke orchestration runner")
    sub = parser.add_subparsers(dest="lane", required=True)

    register_core_lane_parsers(sub, default_app=DEFAULT_APP)
    register_join_leave_churn_parser(sub, default_app=DEFAULT_APP)
    register_status_effect_queue_init_parser(sub, default_app=DEFAULT_APP)
    register_lobby_remote_parsers(sub, default_app=DEFAULT_APP)
    register_splitscreen_lane_parsers(sub, default_app=DEFAULT_APP)
    register_mapgen_lane_parsers(sub, default_app=DEFAULT_APP)
    register_lan_helo_chunk_parser(sub, default_app=DEFAULT_APP)
    register_framework_self_check_parser(sub)

    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    ns = parser.parse_args(argv)
    return int(ns.handler(ns))


if __name__ == "__main__":
    raise SystemExit(main())
