from __future__ import annotations

import argparse
import subprocess
import time
from pathlib import Path
from typing import Callable

from .common import log
from .csvio import write_csv_header
from .fs import normalize_outdir
from .lane_status import pass_fail
from .logscan import file_count_fixed_lines
from .mapgen import (
    MAPGEN_METRIC_KEYS,
    MAPGEN_RESULT_HEADER,
    append_mapgen_row,
    build_mapgen_result_row,
    parse_mapgen_metrics_lines,
    summary_values_for_mapgen,
    write_mapgen_control_file,
)
from .mapgen_runtime import AGGREGATE, ORCH, SCRIPT_DIR
from .mapgen_validation import validate_mapgen_common_args
from .process import run_command
from .reports import find_python3, run_optional_aggregate


def _validate_mapgen_sweep_args(ns: argparse.Namespace) -> None:
    validate_mapgen_common_args(ns, include_start_floor=True)


def _lane_status_and_summary(
    cmd: list[str], run_dir: Path, default_host_log: Path
) -> tuple[str, dict[str, str], Path]:
    status = pass_fail(run_command(cmd) == 0)
    summary_values = summary_values_for_mapgen(run_dir / "summary.env")
    host_log = default_host_log
    host_log_from_summary = summary_values.get("HOST_LOG", "")
    if host_log_from_summary:
        host_log = Path(host_log_from_summary)
    return status, summary_values, host_log


def _append_mapgen_runs(
    *,
    csv_path: Path,
    players: int,
    launched_instances: int,
    mapgen_players_override: int | str,
    runs_per_player: int,
    seed_for_run: Callable[[int], int],
    status: str,
    start_floor: int,
    run_dir: Path,
    summary_values: dict[str, str],
    metrics: list[dict[str, str]] | None,
    require_metrics: bool,
    enforce_observed_players: bool,
) -> bool:
    row_failures = False
    metric_values = metrics or []
    for run in range(1, runs_per_player + 1):
        seed = seed_for_run(run)
        row_status = status
        metric: dict[str, str] = {key: "" for key in MAPGEN_METRIC_KEYS}
        if run <= len(metric_values):
            metric = metric_values[run - 1]
        elif require_metrics:
            row_status = "fail"

        observed_players = metric.get("mapgen_players_observed", "")
        if observed_players == "":
            if mapgen_players_override == "":
                observed_players = str(players)
            else:
                observed_players = str(mapgen_players_override)

        if enforce_observed_players:
            try:
                if int(observed_players) != players:
                    row_status = "fail"
            except ValueError:
                row_status = "fail"

        observed_seed = metric.get("mapgen_seed_observed", "") or str(seed)
        if row_status != "pass":
            row_failures = True

        append_mapgen_row(
            csv_path,
            build_mapgen_result_row(
                players=players,
                launched_instances=launched_instances,
                mapgen_players_override=mapgen_players_override,
                mapgen_players_observed=observed_players,
                run=run,
                seed=seed,
                status=row_status,
                start_floor=start_floor,
                run_dir=run_dir,
                summary_values=summary_values,
                metric_values=metric,
                observed_seed=observed_seed,
            ),
        )
    return row_failures


def _single_runtime_enabled(ns: argparse.Namespace) -> bool:
    if not (
        ns.simulate_mapgen_players
        and ns.inprocess_sim_batch
        and ns.inprocess_player_sweep
        and ns.mapgen_reload_same_level
    ):
        if ns.simulate_mapgen_players and ns.inprocess_sim_batch and ns.inprocess_player_sweep:
            log(
                "Single-runtime player sweep requires --mapgen-reload-same-level 1; "
                "falling back to per-player batch mode"
            )
        return False
    player_span = ns.max_players - ns.min_players + 1
    total_samples = player_span * ns.runs_per_player
    if total_samples <= 256:
        return True
    log(
        "Single-runtime sweep disabled: "
        f"requested samples={total_samples} exceeds auto-transition cap (256)"
    )
    return False


def _run_single_runtime_player_sweep(
    ns: argparse.Namespace,
    *,
    csv_path: Path,
    runs_dir: Path,
) -> int:
    player_span = ns.max_players - ns.min_players + 1
    total_samples = player_span * ns.runs_per_player
    launched_instances = 1
    expected_players = 1
    require_helo = 0
    run_dir = runs_dir / f"p{ns.min_players}-p{ns.max_players}-single-runtime"
    run_dir.mkdir(parents=True, exist_ok=True)
    control_file = run_dir / "mapgen_players_override.txt"
    write_mapgen_control_file(control_file, ns.min_players)

    seed_base = ns.base_seed + 1
    batch_transition_repeats = total_samples
    reload_seed_base = ns.mapgen_reload_seed_base
    if reload_seed_base == 0:
        reload_seed_base = seed_base * 100
    per_sample_timeout_budget = max(ns.auto_enter_dungeon_delay + 9, 10)
    min_timeout = 120 + total_samples * per_sample_timeout_budget
    single_runtime_timeout = max(ns.timeout, min_timeout)
    if single_runtime_timeout != ns.timeout:
        log(
            "Single-runtime sweep timeout auto-bump: "
            f"requested={ns.timeout}s recommended={min_timeout}s"
        )

    cmd = ORCH.build_mapgen_lane_cmd(
        app=ns.app,
        datadir=ns.datadir,
        launched_instances=launched_instances,
        expected_players=expected_players,
        size=ns.size,
        stagger=ns.stagger,
        timeout=single_runtime_timeout,
        lane_outdir=run_dir,
        auto_start_delay=ns.auto_start_delay,
        auto_enter_dungeon=ns.auto_enter_dungeon,
        auto_enter_dungeon_delay=ns.auto_enter_dungeon_delay,
        force_chunk=ns.force_chunk,
        chunk_payload_max=ns.chunk_payload_max,
        seed=seed_base,
        require_helo=require_helo,
        start_floor=ns.start_floor,
        mapgen_reload_same_level=1,
        mapgen_reload_seed_base=reload_seed_base,
        auto_enter_dungeon_repeats=batch_transition_repeats,
        mapgen_samples=total_samples,
        mapgen_players_override=ns.min_players,
        mapgen_control_file=control_file,
    )

    log(
        "Single-runtime sweep: "
        f"players={ns.min_players}..{ns.max_players} samples={total_samples} "
        f"repeats={batch_transition_repeats} seed={seed_base} timeout={single_runtime_timeout}s"
    )
    proc = subprocess.Popen(cmd)
    host_log = run_dir / "instances/home-1/.barony/log.txt"
    next_switch_count = ns.runs_per_player
    next_player = ns.min_players + 1
    last_written_player = ns.min_players
    while proc.poll() is None:
        if host_log.is_file():
            generated_so_far = file_count_fixed_lines(host_log, "successfully generated a dungeon with")
            while next_player <= ns.max_players and generated_so_far >= next_switch_count:
                write_mapgen_control_file(control_file, next_player)
                last_written_player = next_player
                log(
                    "Single-runtime sweep control update: "
                    f"sample={generated_so_far} mapgen_players={next_player}"
                )
                next_player += 1
                next_switch_count += ns.runs_per_player
        time.sleep(1)
    status = pass_fail(proc.returncode == 0)
    log(
        "Single-runtime sweep complete: "
        f"final_control_player={last_written_player} status={status}"
    )

    summary_values = summary_values_for_mapgen(run_dir / "summary.env")
    host_log_from_summary = summary_values.get("HOST_LOG", "")
    if host_log_from_summary:
        host_log = Path(host_log_from_summary)
    metrics = parse_mapgen_metrics_lines(host_log, total_samples)

    row_failures = False
    sample_offset = 0
    for players in range(ns.min_players, ns.max_players + 1):
        lane_metrics = metrics[sample_offset : sample_offset + ns.runs_per_player]
        sample_offset += ns.runs_per_player
        lane_failed = _append_mapgen_runs(
            csv_path=csv_path,
            players=players,
            launched_instances=launched_instances,
            mapgen_players_override=players,
            runs_per_player=ns.runs_per_player,
            seed_for_run=lambda run, p=players: ns.base_seed
            + (p - ns.min_players) * ns.runs_per_player
            + run,
            status=status,
            start_floor=ns.start_floor,
            run_dir=run_dir,
            summary_values=summary_values,
            metrics=lane_metrics,
            require_metrics=True,
            enforce_observed_players=True,
        )
        if lane_failed:
            row_failures = True

    return 1 if status != "pass" or len(metrics) < total_samples or row_failures else 0


def _run_batch_or_standard(
    ns: argparse.Namespace,
    *,
    csv_path: Path,
    runs_dir: Path,
) -> tuple[int, int]:
    failures = 0
    total_runs = 0

    for players in range(ns.min_players, ns.max_players + 1):
        if ns.simulate_mapgen_players and ns.inprocess_sim_batch:
            total_runs += ns.runs_per_player
            run_dir = runs_dir / f"p{players}-batch"
            run_dir.mkdir(parents=True, exist_ok=True)

            launched_instances = 1
            expected_players = 1
            require_helo = 0
            mapgen_players_override = players
            seed_base = ns.base_seed + (players - ns.min_players) * ns.runs_per_player + 1
            batch_transition_repeats = ns.runs_per_player * 2
            if ns.mapgen_reload_same_level:
                batch_transition_repeats = ns.runs_per_player
            elif batch_transition_repeats < ns.runs_per_player + 2:
                batch_transition_repeats = ns.runs_per_player + 2
            batch_transition_repeats = min(batch_transition_repeats, 256)
            reload_seed_base = ns.mapgen_reload_seed_base
            if ns.mapgen_reload_same_level and reload_seed_base == 0:
                reload_seed_base = seed_base * 100

            log(
                "Batch run: "
                f"players={players} launched={launched_instances} samples={ns.runs_per_player} "
                f"repeats={batch_transition_repeats} seed={seed_base}"
            )
            cmd = ORCH.build_mapgen_lane_cmd(
                app=ns.app,
                datadir=ns.datadir,
                launched_instances=launched_instances,
                expected_players=expected_players,
                size=ns.size,
                stagger=ns.stagger,
                timeout=ns.timeout,
                lane_outdir=run_dir,
                auto_start_delay=ns.auto_start_delay,
                auto_enter_dungeon=ns.auto_enter_dungeon,
                auto_enter_dungeon_delay=ns.auto_enter_dungeon_delay,
                force_chunk=ns.force_chunk,
                chunk_payload_max=ns.chunk_payload_max,
                seed=seed_base,
                require_helo=require_helo,
                start_floor=ns.start_floor,
                mapgen_reload_same_level=ns.mapgen_reload_same_level,
                mapgen_reload_seed_base=reload_seed_base,
                auto_enter_dungeon_repeats=batch_transition_repeats,
                mapgen_samples=ns.runs_per_player,
                mapgen_players_override=mapgen_players_override,
            )

            status, summary_values, host_log = _lane_status_and_summary(
                cmd, run_dir, run_dir / "instances/home-1/.barony/log.txt"
            )
            metrics = parse_mapgen_metrics_lines(host_log, ns.runs_per_player)
            if status == "pass" and len(metrics) < ns.runs_per_player:
                status = "fail"

            row_failures = _append_mapgen_runs(
                csv_path=csv_path,
                players=players,
                launched_instances=launched_instances,
                mapgen_players_override=mapgen_players_override,
                runs_per_player=ns.runs_per_player,
                seed_for_run=lambda run, base=seed_base: base + run - 1,
                status=status,
                start_floor=ns.start_floor,
                run_dir=run_dir,
                summary_values=summary_values,
                metrics=metrics,
                require_metrics=True,
                enforce_observed_players=False,
            )
            if status != "pass" or row_failures:
                failures += 1
            continue

        for run in range(1, ns.runs_per_player + 1):
            total_runs += 1
            seed = ns.base_seed + (players - ns.min_players) * ns.runs_per_player + run
            run_dir = runs_dir / f"p{players}-r{run}"
            run_dir.mkdir(parents=True, exist_ok=True)

            launched_instances = players
            expected_players = players
            mapgen_players_override: int | str = ""
            require_helo = 1 if players > 1 else 0
            if ns.simulate_mapgen_players:
                launched_instances = 1
                expected_players = 1
                require_helo = 0
                mapgen_players_override = players

            log(
                f"Run {total_runs}: players={players} launched={launched_instances} "
                f"run={run} seed={seed}"
            )
            cmd = ORCH.build_mapgen_lane_cmd(
                app=ns.app,
                datadir=ns.datadir,
                launched_instances=launched_instances,
                expected_players=expected_players,
                size=ns.size,
                stagger=ns.stagger,
                timeout=ns.timeout,
                lane_outdir=run_dir,
                auto_start_delay=ns.auto_start_delay,
                auto_enter_dungeon=ns.auto_enter_dungeon,
                auto_enter_dungeon_delay=ns.auto_enter_dungeon_delay,
                force_chunk=ns.force_chunk,
                chunk_payload_max=ns.chunk_payload_max,
                seed=seed,
                require_helo=require_helo,
                start_floor=ns.start_floor,
                mapgen_reload_same_level=ns.mapgen_reload_same_level,
                mapgen_reload_seed_base=ns.mapgen_reload_seed_base,
                mapgen_players_override=mapgen_players_override,
            )

            status, summary_values, _host_log = _lane_status_and_summary(
                cmd, run_dir, run_dir / "instances/home-1/.barony/log.txt"
            )
            row_failures = _append_mapgen_runs(
                csv_path=csv_path,
                players=players,
                launched_instances=launched_instances,
                mapgen_players_override=mapgen_players_override,
                runs_per_player=1,
                seed_for_run=lambda _run, s=seed: s,
                status=status,
                start_floor=ns.start_floor,
                run_dir=run_dir,
                summary_values=summary_values,
                metrics=None,
                require_metrics=False,
                enforce_observed_players=False,
            )
            if status != "pass" or row_failures:
                failures += 1

    return total_runs, failures


def _generate_mapgen_reports(csv_path: Path, outdir: Path) -> None:
    python3 = find_python3()
    heatmap_path = SCRIPT_DIR / "generate_mapgen_heatmap.py"
    if python3 and heatmap_path.is_file():
        subprocess.run(
            [
                python3,
                str(heatmap_path),
                "--input",
                str(csv_path),
                "--output",
                str(outdir / "mapgen_heatmap.html"),
            ],
            check=False,
        )
        log(f"Heatmap written to {outdir / 'mapgen_heatmap.html'}")
        run_optional_aggregate(
            AGGREGATE,
            outdir / "smoke_aggregate_report.html",
            ["--mapgen-csv", str(csv_path)],
        )
    elif not python3:
        log("python3 not found; skipped heatmap generation")


def cmd_mapgen_sweep(ns: argparse.Namespace) -> int:
    ORCH.validate_lane_environment(ns.app, ns.datadir)
    _validate_mapgen_sweep_args(ns)

    outdir = normalize_outdir(ns.outdir, "mapgen-sweep")
    runs_dir = outdir / "runs"
    runs_dir.mkdir(parents=True, exist_ok=True)
    csv_path = outdir / "mapgen_results.csv"
    write_csv_header(csv_path, list(MAPGEN_RESULT_HEADER))

    log(f"Writing outputs to {outdir}")
    if ns.simulate_mapgen_players:
        log("Mapgen sweep mode: single-instance simulated player scaling")
        if ns.inprocess_sim_batch:
            log("Mapgen sweep submode: in-process batch collection enabled")
        if ns.inprocess_player_sweep:
            log("Mapgen sweep submode: in-process single-runtime player sweep enabled")

    failures = 0
    total_runs = 0
    if _single_runtime_enabled(ns):
        player_span = ns.max_players - ns.min_players + 1
        total_runs = player_span * ns.runs_per_player
        failures += _run_single_runtime_player_sweep(ns, csv_path=csv_path, runs_dir=runs_dir)
    else:
        total_runs, failures = _run_batch_or_standard(ns, csv_path=csv_path, runs_dir=runs_dir)

    _generate_mapgen_reports(csv_path, outdir)
    log(f"CSV written to {csv_path}")
    log(f"Completed {total_runs} run(s) with {failures} failure(s)")
    return 1 if failures > 0 else 0
