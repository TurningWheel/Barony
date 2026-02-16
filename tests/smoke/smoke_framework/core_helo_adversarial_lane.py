from __future__ import annotations

import argparse
from dataclasses import dataclass

from .common import log
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir
from .lane_helpers import require_uint_specs, run_ns_helo_child_lane
from .reports import run_optional_aggregate
from .core_runtime import AGGREGATE, run_helo_child_lane, validate_lane_environment


@dataclass(frozen=True)
class AdversarialCase:
    case_name: str
    tx_mode: str
    expected: str


ADVERSARIAL_CASES: tuple[AdversarialCase, ...] = (
    AdversarialCase("reverse-pass", "reverse", "pass"),
    AdversarialCase("even-odd-pass", "even-odd", "pass"),
    AdversarialCase("duplicate-first-pass", "duplicate-first", "pass"),
    AdversarialCase("drop-last-fail", "drop-last", "fail"),
    AdversarialCase("duplicate-conflict-first-fail", "duplicate-conflict-first", "fail"),
)


def cmd_helo_adversarial(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--instances", "instances", 2, 15),
            ("--stagger", "stagger", None, None),
            ("--pass-timeout", "pass_timeout", None, None),
            ("--fail-timeout", "fail_timeout", None, None),
            ("--force-chunk", "force_chunk", 0, 1),
            ("--chunk-payload-max", "chunk_payload_max", 64, 900),
            ("--strict-adversarial", "strict_adversarial", 0, 1),
            ("--require-txmode-log", "require_txmode_log", 0, 1),
        ),
    )

    outdir = normalize_outdir(ns.outdir, "helo-adversarial")
    runs_dir = outdir / "runs"
    runs_dir.mkdir(parents=True, exist_ok=True)
    csv_path = outdir / "adversarial_results.csv"
    mismatches = 0

    write_csv_header(
        csv_path,
        [
            "case_name",
            "tx_mode",
            "expected_result",
            "observed_result",
            "match",
            "instances",
            "network_backend",
            "host_chunk_lines",
            "client_reassembled_lines",
            "per_client_reassembly_counts",
            "chunk_reset_lines",
            "chunk_reset_reason_counts",
            "tx_mode_applied",
            "tx_mode_log_lines",
            "tx_mode_packet_plan_ok",
            "mapgen_found",
            "run_dir",
        ],
    )

    for case in ADVERSARIAL_CASES:
        run_dir = runs_dir / case.case_name
        run_dir.mkdir(parents=True, exist_ok=True)
        timeout_seconds = ns.pass_timeout if case.expected == "pass" else ns.fail_timeout
        log(
            f"Case={case.case_name} mode={case.tx_mode} expected={case.expected} "
            f"timeout={timeout_seconds}s"
        )

        _rc, observed, values, _summary = run_ns_helo_child_lane(
            run_helo_child_lane,
            ns,
            instances=ns.instances,
            expected_players=ns.instances,
            timeout=timeout_seconds,
            lane_outdir=run_dir,
            lane_args=[
                "--auto-start",
                "0",
                "--force-chunk",
                str(ns.force_chunk),
                "--chunk-payload-max",
                str(ns.chunk_payload_max),
                "--helo-chunk-tx-mode",
                case.tx_mode,
                "--network-backend",
                "lan",
                "--strict-adversarial",
                str(ns.strict_adversarial),
                "--require-txmode-log",
                str(ns.require_txmode_log),
                "--require-helo",
                "1",
                "--require-mapgen",
                "0",
            ],
            summary_defaults={
                "NETWORK_BACKEND": "",
                "HOST_CHUNK_LINES": "",
                "CLIENT_REASSEMBLED_LINES": "",
                "PER_CLIENT_REASSEMBLY_COUNTS": "",
                "CHUNK_RESET_LINES": "",
                "CHUNK_RESET_REASON_COUNTS": "",
                "TX_MODE_APPLIED": "",
                "TX_MODE_LOG_LINES": "",
                "TX_MODE_PACKET_PLAN_OK": "",
                "MAPGEN_FOUND": "",
            },
        )
        match = "1" if observed == case.expected else "0"
        if match == "0":
            mismatches += 1

        append_csv_row(
            csv_path,
            [
                case.case_name,
                case.tx_mode,
                case.expected,
                observed,
                match,
                ns.instances,
                values["NETWORK_BACKEND"],
                values["HOST_CHUNK_LINES"],
                values["CLIENT_REASSEMBLED_LINES"],
                values["PER_CLIENT_REASSEMBLY_COUNTS"],
                values["CHUNK_RESET_LINES"],
                values["CHUNK_RESET_REASON_COUNTS"],
                values["TX_MODE_APPLIED"],
                values["TX_MODE_LOG_LINES"],
                values["TX_MODE_PACKET_PLAN_OK"],
                values["MAPGEN_FOUND"],
                run_dir,
            ],
        )

    run_optional_aggregate(
        AGGREGATE,
        outdir / "smoke_aggregate_report.html",
        ["--adversarial-csv", str(csv_path)],
    )
    log(f"CSV written to {csv_path}")
    if mismatches > 0:
        log(f"Completed with {mismatches} adversarial expectation mismatch(es)")
        return 1
    log("All adversarial expectations matched")
    return 0
