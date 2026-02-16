from __future__ import annotations

import csv
import re
from collections import defaultdict
from pathlib import Path

from .csvio import append_csv_row
from .mapgen_schema import MAPGEN_TREND_METRICS
from .stats import correlation, linear_slope, parse_float_or_none, parse_int_or_none
from .summary import parse_summary_values
from .tokens import last_line_with_prefix, parse_key_value_tokens

MAPGEN_RESULT_HEADER: tuple[str, ...] = (
    "players",
    "launched_instances",
    "mapgen_players_override",
    "mapgen_players_observed",
    "run",
    "seed",
    "status",
    "start_floor",
    "host_chunk_lines",
    "client_reassembled_lines",
    "mapgen_found",
    "mapgen_level",
    "mapgen_secret",
    "mapgen_seed_observed",
    "rooms",
    "monsters",
    "gold",
    "items",
    "decorations",
    "decorations_blocking",
    "decorations_utility",
    "decorations_traps",
    "decorations_economy",
    "food_items",
    "food_servings",
    "gold_bags",
    "gold_amount",
    "item_stacks",
    "item_units",
    "run_dir",
    "mapgen_wait_reason",
    "mapgen_reload_transition_lines",
    "mapgen_generation_lines",
    "mapgen_generation_unique_seed_count",
    "mapgen_reload_regen_ok",
)

MAPGEN_METRIC_KEYS: tuple[str, ...] = (
    "rooms",
    "monsters",
    "gold",
    "items",
    "decorations",
    "decorations_blocking",
    "decorations_utility",
    "decorations_traps",
    "decorations_economy",
    "food_items",
    "food_servings",
    "gold_bags",
    "gold_amount",
    "item_stacks",
    "item_units",
    "mapgen_level",
    "mapgen_secret",
    "mapgen_seed_observed",
    "mapgen_players_observed",
)

MAPGEN_ROW_SUMMARY_FIELDS: tuple[tuple[str, str], ...] = (
    ("host_chunk_lines", "HOST_CHUNK_LINES"),
    ("client_reassembled_lines", "CLIENT_REASSEMBLED_LINES"),
    ("mapgen_found", "MAPGEN_FOUND"),
    ("mapgen_wait_reason", "MAPGEN_WAIT_REASON"),
    ("mapgen_reload_transition_lines", "MAPGEN_RELOAD_TRANSITION_LINES"),
    ("mapgen_generation_lines", "MAPGEN_GENERATION_LINES"),
    ("mapgen_generation_unique_seed_count", "MAPGEN_GENERATION_UNIQUE_SEED_COUNT"),
    ("mapgen_reload_regen_ok", "MAPGEN_RELOAD_REGEN_OK"),
)

MAPGEN_ROW_METRIC_SUMMARY_FALLBACK: dict[str, str] = {
    "mapgen_level": "MAPGEN_LEVEL",
    "mapgen_secret": "MAPGEN_SECRET",
    "mapgen_seed_observed": "MAPGEN_SEED",
    "rooms": "MAPGEN_ROOMS",
    "monsters": "MAPGEN_MONSTERS",
    "gold": "MAPGEN_GOLD",
    "items": "MAPGEN_ITEMS",
    "decorations": "MAPGEN_DECORATIONS",
    "decorations_blocking": "MAPGEN_DECOR_BLOCKING",
    "decorations_utility": "MAPGEN_DECOR_UTILITY",
    "decorations_traps": "MAPGEN_DECOR_TRAPS",
    "decorations_economy": "MAPGEN_DECOR_ECONOMY",
    "food_items": "MAPGEN_FOOD_ITEMS",
    "food_servings": "MAPGEN_FOOD_SERVINGS",
    "gold_bags": "MAPGEN_GOLD_BAGS",
    "gold_amount": "MAPGEN_GOLD_AMOUNT",
    "item_stacks": "MAPGEN_ITEM_STACKS",
    "item_units": "MAPGEN_ITEM_UNITS",
}

def build_mapgen_result_row(
    *,
    players: int,
    launched_instances: int,
    mapgen_players_override: int | str,
    mapgen_players_observed: int | str,
    run: int,
    seed: int,
    status: str,
    start_floor: int,
    run_dir: Path,
    summary_values: dict[str, str],
    metric_values: dict[str, str] | None = None,
    observed_seed: str | int | None = None,
) -> dict[str, str | int]:
    row: dict[str, str | int] = {
        "players": players,
        "launched_instances": launched_instances,
        "mapgen_players_override": mapgen_players_override,
        "mapgen_players_observed": mapgen_players_observed,
        "run": run,
        "seed": seed,
        "status": status,
        "start_floor": start_floor,
        "run_dir": run_dir,
    }
    for row_key, summary_key in MAPGEN_ROW_SUMMARY_FIELDS:
        row[row_key] = summary_values.get(summary_key, "")
    for row_key, summary_key in MAPGEN_ROW_METRIC_SUMMARY_FALLBACK.items():
        value = ""
        if metric_values is not None:
            value = metric_values.get(row_key, "")
        if value == "":
            value = summary_values.get(summary_key, "")
        row[row_key] = value
    if observed_seed is not None:
        row["mapgen_seed_observed"] = str(observed_seed)
    return row


def append_mapgen_row(csv_path: Path, row: dict[str, str | int]) -> None:
    append_csv_row(csv_path, [row.get(key, "") for key in MAPGEN_RESULT_HEADER])


def parse_mapgen_metrics_lines(host_log: Path, limit: int) -> list[dict[str, str]]:
    if limit <= 0 or not host_log.is_file():
        return []

    main_re = re.compile(
        r"with\s+([0-9]+)\s+rooms,\s+([0-9]+)\s+monsters,\s+([0-9]+)\s+gold,\s+"
        r"([0-9]+)\s+items,\s+([0-9]+)\s+decorations"
    )

    metrics: list[dict[str, str]] = []

    def current_row() -> dict[str, str] | None:
        if not metrics:
            return None
        return metrics[-1]

    with host_log.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            tokens = parse_key_value_tokens(line)
            level_text = tokens.get("level", "")
            try:
                level_num = int(level_text)
            except (TypeError, ValueError):
                level_num = 0
            if level_num < 1:
                continue
            if "successfully generated a dungeon with" in line:
                if len(metrics) >= limit:
                    continue
                match = main_re.search(line)
                if not match:
                    continue
                row: dict[str, str] = {key: "" for key in MAPGEN_METRIC_KEYS}
                row["rooms"] = match.group(1)
                row["monsters"] = match.group(2)
                row["gold"] = match.group(3)
                row["items"] = match.group(4)
                row["decorations"] = match.group(5)
                row["mapgen_level"] = tokens.get("level", "")
                row["mapgen_secret"] = tokens.get("secret", "")
                row["mapgen_seed_observed"] = tokens.get("seed", "")
                row["mapgen_players_observed"] = tokens.get("players", "")
                metrics.append(row)
            elif "mapgen food summary:" in line:
                row = current_row()
                if row is None:
                    continue
                food_tokens = parse_key_value_tokens(line)
                row["food_items"] = food_tokens.get("food", "")
                row["food_servings"] = food_tokens.get("food_servings", "")
            elif "mapgen decoration summary:" in line:
                row = current_row()
                if row is None:
                    continue
                decor_tokens = parse_key_value_tokens(line)
                row["decorations_blocking"] = decor_tokens.get("blocking", "")
                row["decorations_utility"] = decor_tokens.get("utility", "")
                row["decorations_traps"] = decor_tokens.get("traps", "")
                row["decorations_economy"] = decor_tokens.get("economy", "")
            elif "mapgen value summary:" in line:
                row = current_row()
                if row is None:
                    continue
                value_tokens = parse_key_value_tokens(line)
                row["gold_bags"] = value_tokens.get("gold_bags", "")
                row["gold_amount"] = value_tokens.get("gold_amount", "")
                row["item_stacks"] = value_tokens.get("item_stacks", "")
                row["item_units"] = value_tokens.get("item_units", "")

    return metrics


def write_mapgen_control_file(control_file: Path, players: int) -> None:
    control_file.write_text(f"{players}\n", encoding="utf-8")


def summary_values_for_mapgen(path: Path) -> dict[str, str]:
    return parse_summary_values(
        path,
        {
            "HOST_CHUNK_LINES": "",
            "CLIENT_REASSEMBLED_LINES": "",
            "MAPGEN_FOUND": "",
            "MAPGEN_WAIT_REASON": "",
            "MAPGEN_RELOAD_TRANSITION_LINES": "",
            "MAPGEN_GENERATION_LINES": "",
            "MAPGEN_GENERATION_UNIQUE_SEED_COUNT": "",
            "MAPGEN_RELOAD_REGEN_OK": "",
            "MAPGEN_ROOMS": "",
            "MAPGEN_MONSTERS": "",
            "MAPGEN_GOLD": "",
            "MAPGEN_ITEMS": "",
            "MAPGEN_DECORATIONS": "",
            "MAPGEN_DECOR_BLOCKING": "",
            "MAPGEN_DECOR_UTILITY": "",
            "MAPGEN_DECOR_TRAPS": "",
            "MAPGEN_DECOR_ECONOMY": "",
            "MAPGEN_FOOD_ITEMS": "",
            "MAPGEN_FOOD_SERVINGS": "",
            "MAPGEN_GOLD_BAGS": "",
            "MAPGEN_GOLD_AMOUNT": "",
            "MAPGEN_ITEM_STACKS": "",
            "MAPGEN_ITEM_UNITS": "",
            "MAPGEN_LEVEL": "",
            "MAPGEN_SECRET": "",
            "MAPGEN_SEED": "",
            "HOST_LOG": "",
        },
    )


def generate_mapgen_level_matrix_trends(
    combined_csv: Path,
    trends_csv: Path,
    overall_csv: Path,
    overall_md: Path,
) -> None:
    metrics = list(MAPGEN_TREND_METRICS)

    def fmt(value: float | None, digits: int) -> str:
        if value is None:
            return ""
        return f"{value:.{digits}f}"

    rows: list[tuple[int, int, dict[str, float], int, int | None, float | None]] = []
    with combined_csv.open("r", newline="", encoding="utf-8", errors="replace") as f:
        for row in csv.DictReader(f):
            if row.get("status") != "pass":
                continue
            level = parse_int_or_none(row.get("target_level"))
            players = parse_int_or_none(row.get("players"))
            if level is None or players is None:
                continue
            vals: dict[str, float] = {}
            valid = True
            for key in metrics:
                parsed = parse_float_or_none(row.get(key))
                if parsed is None:
                    valid = False
                    break
                vals[key] = parsed
            if not valid:
                continue
            observed_level = parse_int_or_none(row.get("mapgen_level"))
            level_match = 1 if observed_level is not None and observed_level == level else 0
            observed_seed = parse_int_or_none(row.get("mapgen_seed_observed"))
            generation_lines = parse_int_or_none(row.get("mapgen_generation_lines"))
            generation_unique_seed_count = parse_int_or_none(row.get("mapgen_generation_unique_seed_count"))
            reload_unique_seed_rate = None
            if (
                generation_lines is not None
                and generation_lines > 0
                and generation_unique_seed_count is not None
            ):
                reload_unique_seed_rate = (
                    100.0 * generation_unique_seed_count / generation_lines
                )
            rows.append((level, players, vals, level_match, observed_seed, reload_unique_seed_rate))

    def slope_and_corr(xs: list[float], ys: list[float]) -> tuple[float | None, float | None]:
        if not xs or not ys or len(xs) != len(ys):
            return (None, None)
        if len(xs) == 1:
            return (0.0, 0.0)
        return (linear_slope(xs, ys), correlation(xs, ys))

    by_level: dict[int, list[tuple[int, dict[str, float], int, int | None, float | None]]] = defaultdict(list)
    for level, players, vals, level_match, observed_seed, reload_unique_seed_rate in rows:
        by_level[level].append((players, vals, level_match, observed_seed, reload_unique_seed_rate))

    metric_records: dict[str, list[dict[str, float | int | None]]] = defaultdict(list)
    level_match_rates: list[float] = []
    level_observed_seed_unique_rates: list[float] = []
    level_reload_unique_seed_rates: list[float] = []

    with trends_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "target_level",
                "rows",
                "players_seen",
                "target_level_match_rate_pct",
                "observed_seed_unique_rate_pct",
                "reload_unique_seed_rate_pct",
                "metric",
                "slope",
                "correlation",
                "high_vs_low_pct",
            ]
        )
        for level in sorted(by_level.keys()):
            level_rows = by_level[level]
            by_player: dict[int, dict[str, list[float]]] = defaultdict(lambda: defaultdict(list))
            match_total = 0
            observed_seeds: list[int] = []
            reload_unique_seed_rates: list[float] = []
            for players, vals, level_match, observed_seed, reload_unique_seed_rate in level_rows:
                match_total += level_match
                for metric, value in vals.items():
                    by_player[players][metric].append(value)
                if observed_seed is not None:
                    observed_seeds.append(observed_seed)
                if reload_unique_seed_rate is not None:
                    reload_unique_seed_rates.append(reload_unique_seed_rate)

            players_sorted = sorted(by_player.keys())
            rows_count = len(level_rows)
            players_seen = len(players_sorted)
            match_rate = (100.0 * match_total / rows_count) if rows_count else 0.0
            observed_seed_unique_rate = None
            if observed_seeds:
                observed_seed_unique_rate = 100.0 * len(set(observed_seeds)) / len(observed_seeds)
            reload_unique_seed_rate = None
            if reload_unique_seed_rates:
                reload_unique_seed_rate = sum(reload_unique_seed_rates) / len(reload_unique_seed_rates)

            level_match_rates.append(match_rate)
            if observed_seed_unique_rate is not None:
                level_observed_seed_unique_rates.append(observed_seed_unique_rate)
            if reload_unique_seed_rate is not None:
                level_reload_unique_seed_rates.append(reload_unique_seed_rate)

            for metric in metrics:
                xs: list[float] = []
                ys: list[float] = []
                for player in players_sorted:
                    vals = by_player[player][metric]
                    if not vals:
                        continue
                    xs.append(float(player))
                    ys.append(sum(vals) / len(vals))
                if not xs:
                    writer.writerow(
                        [
                            level,
                            rows_count,
                            players_seen,
                            f"{match_rate:.1f}",
                            fmt(observed_seed_unique_rate, 1),
                            fmt(reload_unique_seed_rate, 1),
                            metric,
                            "",
                            "",
                            "",
                        ]
                    )
                    continue

                slope, corr = slope_and_corr(xs, ys)
                low_vals = [
                    sum(by_player[player][metric]) / len(by_player[player][metric])
                    for player in players_sorted
                    if player <= 4 and by_player[player][metric]
                ]
                high_vals = [
                    sum(by_player[player][metric]) / len(by_player[player][metric])
                    for player in players_sorted
                    if player >= 12 and by_player[player][metric]
                ]
                high_vs_low = None
                if low_vals and high_vals:
                    low_avg = sum(low_vals) / len(low_vals)
                    high_avg = sum(high_vals) / len(high_vals)
                    high_vs_low = ((high_avg - low_avg) / low_avg * 100.0) if low_avg else 0.0

                writer.writerow(
                    [
                        level,
                        rows_count,
                        players_seen,
                        f"{match_rate:.1f}",
                        fmt(observed_seed_unique_rate, 1),
                        fmt(reload_unique_seed_rate, 1),
                        metric,
                        fmt(slope, 4),
                        fmt(corr, 4),
                        fmt(high_vs_low, 1),
                    ]
                )
                if slope is not None:
                    metric_records[metric].append(
                        {
                            "level": level,
                            "slope": slope,
                            "corr": corr,
                            "high_vs_low": high_vs_low,
                        }
                    )

    mean_target_level_match_rate = (
        (sum(level_match_rates) / len(level_match_rates))
        if level_match_rates
        else None
    )
    mean_observed_seed_unique_rate = (
        (sum(level_observed_seed_unique_rates) / len(level_observed_seed_unique_rates))
        if level_observed_seed_unique_rates
        else None
    )
    mean_reload_unique_seed_rate = (
        (sum(level_reload_unique_seed_rates) / len(level_reload_unique_seed_rates))
        if level_reload_unique_seed_rates
        else None
    )

    with overall_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "metric",
                "levels_total",
                "levels_positive_slope",
                "positive_slope_pct",
                "mean_slope",
                "min_slope",
                "mean_correlation",
                "mean_high_vs_low_pct",
                "min_high_vs_low_pct",
                "mean_target_level_match_rate_pct",
                "mean_observed_seed_unique_rate_pct",
                "mean_reload_unique_seed_rate_pct",
            ]
        )
        for metric in metrics:
            records = metric_records.get(metric, [])
            if not records:
                writer.writerow(
                    [
                        metric,
                        0,
                        0,
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        fmt(mean_target_level_match_rate, 1),
                        fmt(mean_observed_seed_unique_rate, 1),
                        fmt(mean_reload_unique_seed_rate, 1),
                    ]
                )
                continue
            slopes = [float(r["slope"]) for r in records]
            corrs = [float(r["corr"]) for r in records if r["corr"] is not None]
            high_low = [float(r["high_vs_low"]) for r in records if r["high_vs_low"] is not None]
            total = len(records)
            positive = sum(1 for s in slopes if s > 0.0)
            positive_pct = (100.0 * positive / total) if total else 0.0
            mean_slope = sum(slopes) / total if total else 0.0
            min_slope = min(slopes) if slopes else 0.0
            mean_corr = (sum(corrs) / len(corrs)) if corrs else None
            mean_high = (sum(high_low) / len(high_low)) if high_low else None
            min_high = min(high_low) if high_low else None
            writer.writerow(
                [
                    metric,
                    total,
                    positive,
                    f"{positive_pct:.1f}",
                    f"{mean_slope:.4f}",
                    f"{min_slope:.4f}",
                    f"{mean_corr:.4f}" if mean_corr is not None else "",
                    f"{mean_high:.1f}" if mean_high is not None else "",
                    f"{min_high:.1f}" if min_high is not None else "",
                    fmt(mean_target_level_match_rate, 1),
                    fmt(mean_observed_seed_unique_rate, 1),
                    fmt(mean_reload_unique_seed_rate, 1),
                ]
            )

    with overall_md.open("w", encoding="utf-8") as f:
        f.write("# Mapgen Level Matrix Overall Summary\n\n")
        f.write(f"- Source CSV: `{combined_csv}`\n")
        f.write(f"- Evaluated pass rows: {len(rows)}\n")
        f.write(f"- Levels covered: {len(by_level)}\n\n")
        f.write(
            f"- Mean target-level match rate: "
            f"{fmt(mean_target_level_match_rate, 1) or 'n/a'}%\n"
        )
        f.write(
            f"- Mean observed-seed unique rate: "
            f"{fmt(mean_observed_seed_unique_rate, 1) or 'n/a'}%\n"
        )
        f.write(
            f"- Mean reload unique-seed rate: "
            f"{fmt(mean_reload_unique_seed_rate, 1) or 'n/a'}%\n\n"
        )
        f.write(
            "| Metric | Positive Slope Levels | Mean Slope | Min Slope | Mean Corr | Mean High-vs-Low % |\n"
        )
        f.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
        for metric in metrics:
            records = metric_records.get(metric, [])
            if not records:
                f.write(f"| {metric} | 0/0 | n/a | n/a | n/a | n/a |\n")
                continue
            slopes = [float(r["slope"]) for r in records]
            corrs = [float(r["corr"]) for r in records if r["corr"] is not None]
            high_low = [float(r["high_vs_low"]) for r in records if r["high_vs_low"] is not None]
            positive = sum(1 for s in slopes if s > 0.0)
            total = len(records)
            mean_slope = sum(slopes) / total
            min_slope = min(slopes)
            mean_corr = (sum(corrs) / len(corrs)) if corrs else None
            mean_high = (sum(high_low) / len(high_low)) if high_low else None
            corr_text = f"{mean_corr:.4f}" if mean_corr is not None else "n/a"
            high_text = f"{mean_high:.1f}" if mean_high is not None else "n/a"
            f.write(
                f"| {metric} | {positive}/{total} | {mean_slope:.4f} | {min_slope:.4f} | "
                f"{corr_text} | {high_text} |\n"
            )


def extract_mapgen_metrics(host_log: Path) -> tuple[int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int]:
    rows = parse_mapgen_metrics_lines(host_log, 1_000_000)
    if not rows:
        return (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

    row = rows[-1]

    def parse_int_value(key: str) -> int:
        try:
            return int(row.get(key, "0"))
        except ValueError:
            return 0

    return (
        1,
        parse_int_value("rooms"),
        parse_int_value("monsters"),
        parse_int_value("gold"),
        parse_int_value("items"),
        parse_int_value("decorations"),
        parse_int_value("decorations_blocking"),
        parse_int_value("decorations_utility"),
        parse_int_value("decorations_traps"),
        parse_int_value("decorations_economy"),
        parse_int_value("food_items"),
        parse_int_value("food_servings"),
        parse_int_value("gold_bags"),
        parse_int_value("gold_amount"),
        parse_int_value("item_stacks"),
        parse_int_value("item_units"),
        parse_int_value("mapgen_level"),
        parse_int_value("mapgen_secret"),
        parse_int_value("mapgen_seed_observed"),
    )


def collect_mapgen_generation_seeds(host_log: Path) -> str:
    if not host_log.is_file():
        return ""
    seed_re = re.compile(r"\(seed ([0-9]+)\)")
    seeds: list[str] = []
    needle = "generating a dungeon from level set '"
    with host_log.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            if needle not in line:
                continue
            match = seed_re.search(line)
            if match:
                seeds.append(match.group(1))
    return ";".join(seeds)


def collect_reload_transition_seeds(host_log: Path) -> str:
    if not host_log.is_file():
        return ""
    needle = "[SMOKE]: auto-reloading dungeon level transition"
    seeds: list[str] = []
    with host_log.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            if needle not in line:
                continue
            seed = parse_key_value_tokens(line).get("seed", "")
            if seed:
                seeds.append(seed)
    return ";".join(seeds)


def count_list_values(values: str) -> int:
    if not values:
        return 0
    return sum(1 for item in values.split(";") if item)


def count_unique_list_values(values: str) -> int:
    if not values:
        return 0
    return len({item for item in values.split(";") if item})


def count_seed_matches(expected_seeds: str, observed_seeds: str) -> int:
    if not expected_seeds or not observed_seeds:
        return 0
    seen = {item for item in observed_seeds.split(";") if item}
    return sum(1 for item in expected_seeds.split(";") if item and item in seen)
