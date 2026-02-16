from __future__ import annotations

import argparse
import re
from pathlib import Path

from .common import fail, log
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir, prune_models_cache, reset_paths
from .lane_helpers import require_uint_specs, run_ns_helo_child_lane
from .logscan import file_count_matching_lines, file_last_matching_line
from .summary import write_summary_env
from .core_runtime import run_helo_child_lane, validate_lane_environment


def cmd_save_reload_compat(ns: argparse.Namespace) -> int:
    validate_lane_environment(ns.app, ns.datadir)

    require_uint_specs(
        ns,
        (
            ("--stagger", "stagger", None, None),
            ("--timeout", "timeout", None, None),
        ),
    )

    outdir = normalize_outdir(ns.outdir, "save-reload-compat")
    summary_path = outdir / "summary.env"
    csv_path = outdir / "save_reload_owner_encoding_results.csv"
    lane_outdir = outdir / "owner-sweep"
    reset_paths(lane_outdir, summary_path, csv_path)

    log(f"Artifacts: {outdir}")

    rc, _child_result, values, _lane_summary = run_ns_helo_child_lane(
        run_helo_child_lane,
        ns,
        instances=1,
        expected_players=1,
        timeout=ns.timeout,
        lane_outdir=lane_outdir,
        lane_args=[
            "--force-chunk",
            "1",
            "--chunk-payload-max",
            "200",
            "--auto-start",
            "1",
            "--auto-start-delay",
            "1",
            "--auto-enter-dungeon",
            "1",
            "--auto-enter-dungeon-delay",
            "2",
            "--require-mapgen",
            "1",
        ],
        summary_defaults={"HOST_LOG": ""},
        extra_env={"BARONY_SMOKE_SAVE_RELOAD_OWNER_SWEEP": "1"},
    )
    if rc != 0:
        return rc

    host_log = values.get("HOST_LOG", "")
    if not host_log:
        fail(f"Host log was not found after owner sweep lane: {host_log}")
    host_log_path = Path(host_log)
    if not host_log_path.is_file():
        fail(f"Host log was not found after owner sweep lane: {host_log}")

    regular_pass_count = 0
    regular_fail_count = 0
    legacy_pass_count = 0
    legacy_fail_count = 0

    write_csv_header(csv_path, ["lane", "players_connected", "result", "artifact"])

    for players_connected in range(1, 16):
        lane = f"p{players_connected}"
        pattern = (
            r"\[SMOKE\]: save_reload_owner lane="
            + re.escape(lane)
            + r" players_connected="
            + str(players_connected)
            + r" result=pass"
        )
        result = "pass" if file_count_matching_lines(host_log_path, pattern) > 0 else "fail"
        if result == "pass":
            regular_pass_count += 1
        else:
            regular_fail_count += 1
        append_csv_row(csv_path, [lane, players_connected, result, lane_outdir])

    for legacy_lane in ("legacy-empty", "legacy-short", "legacy-long"):
        pattern = (
            r"\[SMOKE\]: save_reload_owner lane="
            + re.escape(legacy_lane)
            + r" .* result=pass"
        )
        result = "pass" if file_count_matching_lines(host_log_path, pattern) > 0 else "fail"
        if result == "pass":
            legacy_pass_count += 1
        else:
            legacy_fail_count += 1
        append_csv_row(csv_path, [legacy_lane, 8, result, lane_outdir])

    owner_fail_lines = file_count_matching_lines(host_log_path, r"\[SMOKE\]: save_reload_owner .*status=fail")
    sweep_line = file_last_matching_line(host_log_path, r"\[SMOKE\]: save_reload_owner sweep result=")
    sweep_match = re.search(r"result=([a-z]+)", sweep_line)
    sweep_result = sweep_match.group(1) if sweep_match else "fail"

    overall_result = "pass"
    if (
        regular_fail_count > 0
        or legacy_fail_count > 0
        or owner_fail_lines > 0
        or sweep_result != "pass"
    ):
        overall_result = "fail"

    write_summary_env(
        summary_path,
        {
            "RESULT": overall_result,
            "OUTDIR": outdir,
            "LANE_OUTDIR": lane_outdir,
            "CSV_PATH": csv_path,
            "HOST_LOG": host_log_path,
            "DATADIR": ns.datadir or "",
            "REGULAR_PASS_COUNT": regular_pass_count,
            "REGULAR_FAIL_COUNT": regular_fail_count,
            "LEGACY_PASS_COUNT": legacy_pass_count,
            "LEGACY_FAIL_COUNT": legacy_fail_count,
            "OWNER_FAIL_LINES": owner_fail_lines,
            "SWEEP_RESULT": sweep_result,
            "SWEEP_LINE": sweep_line,
        },
    )

    prune_models_cache(lane_outdir)

    log(f"summary={summary_path}")
    log(f"csv={csv_path}")

    if overall_result != "pass":
        log(f"Save/reload owner-encoding sweep failed; see {host_log_path}")
        return 1
    return 0
