from __future__ import annotations

import argparse
import csv
from pathlib import Path

from .common import fail, log
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir
from .mapgen import MAPGEN_RESULT_HEADER, generate_mapgen_level_matrix_trends
from .mapgen_runtime import AGGREGATE, ORCH
from .mapgen_validation import validate_mapgen_common_args
from .process import run_command
from .reports import find_python3, run_optional_aggregate


def _parse_target_levels(levels: str) -> list[int]:
    parsed_levels: list[int] = []
    for raw in levels.split(","):
        item = raw.strip()
        if item == "":
            continue
        try:
            level = int(item)
        except ValueError:
            fail(f"--levels must be comma-separated integers in 1..99 (bad value: {item})")
        if level < 1 or level > 99:
            fail(f"--levels must be comma-separated integers in 1..99 (bad value: {item})")
        parsed_levels.append(level)
    if not parsed_levels:
        fail("--levels must provide at least one floor value")
    return parsed_levels


def _validate_mapgen_matrix_args(ns: argparse.Namespace) -> None:
    validate_mapgen_common_args(ns, include_start_floor=False)


def _append_level_rows(level: int, level_csv: Path, combined_csv: Path) -> None:
    with level_csv.open("r", newline="", encoding="utf-8", errors="replace") as src:
        reader = csv.DictReader(src)
        for row in reader:
            append_csv_row(combined_csv, [level, *[row.get(col, "") for col in MAPGEN_RESULT_HEADER]])


def _generate_matrix_reports(outdir: Path, combined_csv: Path) -> None:
    python3 = find_python3()
    if not python3:
        log("python3 not found; skipped level trend summary")
        return
    trends_csv = outdir / "mapgen_level_trends.csv"
    overall_csv = outdir / "mapgen_level_overall.csv"
    overall_md = outdir / "mapgen_level_overall.md"
    generate_mapgen_level_matrix_trends(combined_csv, trends_csv, overall_csv, overall_md)
    log(f"Level trend summary written to {trends_csv}")
    log(f"Overall trend summary written to {overall_csv}")
    log(f"Overall markdown summary written to {overall_md}")
    run_optional_aggregate(
        AGGREGATE,
        outdir / "mapgen_level_matrix_aggregate_report.html",
        ["--mapgen-matrix-csv", str(combined_csv)],
    )


def _cleanup_matrix_caches(outdir: Path) -> None:
    for cache in outdir.rglob("models.cache"):
        try:
            cache.unlink()
        except OSError:
            pass


def cmd_mapgen_level_matrix(ns: argparse.Namespace) -> int:
    ORCH.validate_lane_environment(ns.app, ns.datadir)
    _validate_mapgen_matrix_args(ns)
    levels = _parse_target_levels(ns.levels)

    outdir = normalize_outdir(ns.outdir, "mapgen-level-matrix")
    combined_csv = outdir / "mapgen_level_matrix.csv"
    write_csv_header(combined_csv, ["target_level", *MAPGEN_RESULT_HEADER])

    log(f"Writing outputs to {outdir}")
    level_failures = 0
    for level in levels:
        start_floor = level if ns.mapgen_reload_same_level else max(level - 1, 0)
        level_seed = ns.base_seed + level * 100000
        level_outdir = outdir / f"level-{level}"
        log(f"Level lane start: target_level={level} start_floor={start_floor}")
        cmd = ORCH.build_nested_runner_cmd(
            lane="mapgen-sweep",
            app=ns.app,
            datadir=ns.datadir,
            lane_outdir=level_outdir,
            lane_args=[
                "--min-players",
                str(ns.min_players),
                "--max-players",
                str(ns.max_players),
                "--runs-per-player",
                str(ns.runs_per_player),
                "--base-seed",
                str(level_seed),
                "--size",
                ns.size,
                "--stagger",
                str(ns.stagger),
                "--timeout",
                str(ns.timeout),
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
                "--simulate-mapgen-players",
                str(ns.simulate_mapgen_players),
                "--inprocess-sim-batch",
                str(ns.inprocess_sim_batch),
                "--inprocess-player-sweep",
                str(ns.inprocess_player_sweep),
                "--mapgen-reload-same-level",
                str(ns.mapgen_reload_same_level),
                "--mapgen-reload-seed-base",
                str(ns.mapgen_reload_seed_base),
                "--start-floor",
                str(start_floor),
            ],
        )
        if run_command(cmd) != 0:
            level_failures += 1

        level_csv = level_outdir / "mapgen_results.csv"
        if not level_csv.is_file():
            log(f"missing level csv: {level_csv}")
            level_failures += 1
            continue
        _append_level_rows(level, level_csv, combined_csv)

    _generate_matrix_reports(outdir, combined_csv)
    _cleanup_matrix_caches(outdir)
    log(f"Combined CSV written to {combined_csv}")
    log(f"Per-level outputs are under {outdir / 'level-*'}")
    if level_failures > 0:
        log(f"Completed with {level_failures} failing level lane(s)")
        return 1
    return 0
