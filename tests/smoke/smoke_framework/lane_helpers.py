from __future__ import annotations

import argparse
from collections.abc import Callable, Sequence
from pathlib import Path
from typing import Any

from .common import require_uint
from .csvio import append_csv_row, write_csv_header
from .lane_matrix import single_lane_counts
from .lane_status import result_from_failures
from .summary import write_summary_env

UIntSpec = tuple[str, str, int | None, int | None]


def require_uint_specs(ns: argparse.Namespace, specs: Sequence[UIntSpec]) -> None:
    for cli_name, attr, minimum, maximum in specs:
        value = getattr(ns, attr)
        require_uint(cli_name, value, minimum=minimum, maximum=maximum)


def run_ns_helo_child_lane(
    run_helo_child_lane: Callable[..., tuple[int, str, dict[str, str], Path]],
    ns: argparse.Namespace,
    *,
    instances: int,
    expected_players: int,
    timeout: int,
    lane_outdir: Path,
    lane_args: Sequence[str],
    summary_defaults: dict[str, str] | None = None,
    extra_env: dict[str, str] | None = None,
) -> tuple[int, str, dict[str, str], Path]:
    kwargs: dict[str, Any] = {
        "app": ns.app,
        "datadir": ns.datadir,
        "instances": instances,
        "expected_players": expected_players,
        "size": ns.size,
        "stagger": ns.stagger,
        "timeout": timeout,
        "lane_outdir": lane_outdir,
        "lane_args": lane_args,
    }
    if summary_defaults is not None:
        kwargs["summary_defaults"] = summary_defaults
    if extra_env is not None:
        kwargs["extra_env"] = extra_env
    return run_helo_child_lane(**kwargs)


def build_default_helo_lane_args(*extra_args: str, auto_start: int) -> list[str]:
    return [
        "--force-chunk",
        "1",
        "--chunk-payload-max",
        "200",
        "--require-helo",
        "1",
        "--auto-start",
        str(auto_start),
        *extra_args,
    ]


def single_lane_rollup(lane_result: str) -> tuple[str, int, int]:
    pass_lanes, fail_lanes = single_lane_counts(lane_result)
    return result_from_failures(fail_lanes), pass_lanes, fail_lanes


def build_single_lane_summary_payload(
    *,
    lane_result: str,
    outdir: Path,
    app: Path,
    datadir: Path | None,
    csv_path: Path,
    lane_outdir: Path | None = None,
    extra: dict[str, Any] | None = None,
) -> dict[str, Any]:
    overall_result, pass_lanes, fail_lanes = single_lane_rollup(lane_result)
    payload: dict[str, Any] = {
        "RESULT": overall_result,
        "OUTDIR": outdir,
        "APP": app,
        "DATADIR": datadir or "",
        "PASS_LANES": pass_lanes,
        "FAIL_LANES": fail_lanes,
        "CSV_PATH": csv_path,
    }
    if lane_outdir is not None:
        payload["LANE_OUTDIR"] = lane_outdir
    if extra:
        payload.update(extra)
    return payload


def write_single_lane_result_files(
    *,
    summary_path: Path,
    csv_path: Path,
    csv_header: Sequence[str],
    csv_row: Sequence[str | int | float | Path],
    lane_result: str,
    outdir: Path,
    app: Path,
    datadir: Path | None,
    lane_outdir: Path | None = None,
    summary_extra: dict[str, Any] | None = None,
) -> dict[str, Any]:
    write_csv_header(csv_path, csv_header)
    append_csv_row(csv_path, csv_row)
    payload = build_single_lane_summary_payload(
        lane_result=lane_result,
        outdir=outdir,
        app=app,
        datadir=datadir,
        csv_path=csv_path,
        lane_outdir=lane_outdir,
        extra=summary_extra,
    )
    write_summary_env(summary_path, payload)
    return payload
