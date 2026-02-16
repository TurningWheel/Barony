from __future__ import annotations

import argparse
from pathlib import Path


def add_app_datadir_args(
    parser: argparse.ArgumentParser,
    *,
    default_app: Path,
    datadir_help: str = "Optional data directory passed to Barony via -datadir=<path>.",
) -> None:
    parser.add_argument("--app", type=Path, default=default_app, help="Barony executable path.")
    parser.add_argument("--datadir", type=Path, default=None, help=datadir_help)
