from __future__ import annotations

import argparse

from .common import fail, log
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir
from .helo_metrics import canonicalize_lan_tx_mode
from .lane_helpers import require_uint_specs, run_ns_helo_child_lane
from .reports import run_optional_aggregate
from .core_runtime import AGGREGATE, run_helo_child_lane, validate_lane_environment


def cmd_helo_soak(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--runs", "runs", 1, None),
            ("--instances", "instances", 1, 15),
            ("--stagger", "stagger", None, None),
            ("--timeout", "timeout", None, None),
            ("--auto-start-delay", "auto_start_delay", None, None),
            ("--auto-enter-dungeon-delay", "auto_enter_dungeon_delay", None, None),
            ("--auto-enter-dungeon", "auto_enter_dungeon", 0, 1),
            ("--force-chunk", "force_chunk", 0, 1),
            ("--chunk-payload-max", "chunk_payload_max", 64, 900),
            ("--require-mapgen", "require_mapgen", 0, 1),
        ),
    )
    normalized_mode = canonicalize_lan_tx_mode(ns.helo_chunk_tx_mode)
    if normalized_mode is None:
        fail(
            "--helo-chunk-tx-mode must be one of: normal, reverse, even-odd, "
            "duplicate-first, drop-last, duplicate-conflict-first"
        )
    ns.helo_chunk_tx_mode = normalized_mode

    outdir = normalize_outdir(ns.outdir, f"soak-p{ns.instances}-n{ns.runs}")
    runs_dir = outdir / "runs"
    runs_dir.mkdir(parents=True, exist_ok=True)
    csv_path = outdir / "soak_results.csv"
    failures = 0

    write_csv_header(
        csv_path,
        [
            "run",
            "status",
            "instances",
            "host_chunk_lines",
            "client_reassembled_lines",
            "mapgen_found",
            "gamestart_found",
            "tx_mode",
            "run_dir",
        ],
    )

    for run_idx in range(1, ns.runs + 1):
        run_dir = runs_dir / f"r{run_idx}"
        run_dir.mkdir(parents=True, exist_ok=True)
        log(f"Run {run_idx}/{ns.runs}: instances={ns.instances}")

        rc, status, values, _summary = run_ns_helo_child_lane(
            run_helo_child_lane,
            ns,
            instances=ns.instances,
            expected_players=ns.instances,
            timeout=ns.timeout,
            lane_outdir=run_dir,
            lane_args=[
                "--auto-start",
                "1",
                "--auto-start-delay",
                str(ns.auto_start_delay),
                "--auto-enter-dungeon",
                str(ns.auto_enter_dungeon),
                "--auto-enter-dungeon-delay",
                str(ns.auto_enter_dungeon_delay),
                "--force-chunk",
                str(ns.force_chunk),
                "--chunk-payload-max",
                str(ns.chunk_payload_max),
                "--helo-chunk-tx-mode",
                ns.helo_chunk_tx_mode,
                "--require-mapgen",
                str(ns.require_mapgen),
            ],
            summary_defaults={
                "HOST_CHUNK_LINES": "",
                "CLIENT_REASSEMBLED_LINES": "",
                "MAPGEN_FOUND": "",
                "GAMESTART_FOUND": "",
            },
        )
        if rc != 0:
            failures += 1

        append_csv_row(
            csv_path,
            [
                run_idx,
                status,
                ns.instances,
                values["HOST_CHUNK_LINES"],
                values["CLIENT_REASSEMBLED_LINES"],
                values["MAPGEN_FOUND"],
                values["GAMESTART_FOUND"],
                ns.helo_chunk_tx_mode,
                run_dir,
            ],
        )

    run_optional_aggregate(
        AGGREGATE,
        outdir / "smoke_aggregate_report.html",
        ["--soak-csv", str(csv_path)],
    )
    log(f"CSV written to {csv_path}")
    log(f"Completed {ns.runs} run(s) with {failures} failure(s)")
    return 1 if failures > 0 else 0
