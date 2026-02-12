#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWEEP="$SCRIPT_DIR/run_mapgen_sweep_mac.sh"
AGGREGATE="$SCRIPT_DIR/generate_smoke_aggregate_report.py"

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
LEVELS="1,7,16,25,33"
MIN_PLAYERS=1
MAX_PLAYERS=15
RUNS_PER_PLAYER=2
BASE_SEED=1000
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=0
TIMEOUT_SECONDS=180
AUTO_START_DELAY_SECS=0
AUTO_ENTER_DUNGEON=1
AUTO_ENTER_DUNGEON_DELAY_SECS=3
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
SIMULATE_MAPGEN_PLAYERS=1
INPROCESS_SIM_BATCH=1
INPROCESS_PLAYER_SWEEP=1
MAPGEN_RELOAD_SAME_LEVEL=1
MAPGEN_RELOAD_SEED_BASE=0
OUTDIR=""

usage() {
	cat <<'USAGE'
Usage: run_mapgen_level_matrix_mac.sh [options]

Options:
  --app <path>                 Barony executable path.
  --datadir <path>             Optional data directory passed through to child sweep.
  --levels <csv>               Target generated floors (default: 1,7,16,25,33).
  --min-players <n>            Start player count (default: 1).
  --max-players <n>            End player count (default: 15).
  --runs-per-player <n>        Runs for each player count per floor (default: 2).
  --base-seed <n>              Base seed value used for deterministic runs.
  --size <WxH>                 Window size for all instances.
  --stagger <sec>              Delay between instance launches.
  --timeout <sec>              Timeout per run.
  --auto-start-delay <sec>     Host auto-start delay after full lobby.
  --auto-enter-dungeon <0|1>   Host forces first dungeon transition after load.
  --auto-enter-dungeon-delay <sec>
                               Delay before forced dungeon entry.
  --force-chunk <0|1>          BARONY_SMOKE_FORCE_HELO_CHUNK setting.
  --chunk-payload-max <n>      Chunk payload cap (64..900).
  --simulate-mapgen-players <0|1>
                               Use one launched instance and simulate mapgen scaling players.
  --inprocess-sim-batch <0|1>  Forwarded to child sweep (default: 1).
  --inprocess-player-sweep <0|1>
                               Forwarded to child sweep; in simulated batch mode this runs all player counts in one runtime.
  --mapgen-reload-same-level <0|1>
                               Reload same generated level between samples (default: 1).
  --mapgen-reload-seed-base <n>
                               Base seed used for same-level reload samples (0 disables forced seed rotation).
  --outdir <path>              Output directory.
  -h, --help                   Show this help.
USAGE
}

is_uint() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

log() {
	printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

read_summary_key() {
	local key="$1"
	local file="$2"
	local line
	line="$(rg -n "^${key}=" "$file" | head -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	echo "${line#*=}"
}

while (($# > 0)); do
	case "$1" in
		--app)
			APP="${2:-}"
			shift 2
			;;
		--datadir)
			DATADIR="${2:-}"
			shift 2
			;;
		--levels)
			LEVELS="${2:-}"
			shift 2
			;;
		--min-players)
			MIN_PLAYERS="${2:-}"
			shift 2
			;;
		--max-players)
			MAX_PLAYERS="${2:-}"
			shift 2
			;;
		--runs-per-player)
			RUNS_PER_PLAYER="${2:-}"
			shift 2
			;;
		--base-seed)
			BASE_SEED="${2:-}"
			shift 2
			;;
		--size)
			WINDOW_SIZE="${2:-}"
			shift 2
			;;
		--stagger)
			STAGGER_SECONDS="${2:-}"
			shift 2
			;;
		--timeout)
			TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--auto-start-delay)
			AUTO_START_DELAY_SECS="${2:-}"
			shift 2
			;;
		--auto-enter-dungeon)
			AUTO_ENTER_DUNGEON="${2:-}"
			shift 2
			;;
		--auto-enter-dungeon-delay)
			AUTO_ENTER_DUNGEON_DELAY_SECS="${2:-}"
			shift 2
			;;
		--force-chunk)
			FORCE_CHUNK="${2:-}"
			shift 2
			;;
		--chunk-payload-max)
			CHUNK_PAYLOAD_MAX="${2:-}"
			shift 2
			;;
		--simulate-mapgen-players)
			SIMULATE_MAPGEN_PLAYERS="${2:-}"
			shift 2
			;;
		--inprocess-sim-batch)
			INPROCESS_SIM_BATCH="${2:-}"
			shift 2
			;;
		--inprocess-player-sweep)
			INPROCESS_PLAYER_SWEEP="${2:-}"
			shift 2
			;;
		--mapgen-reload-same-level)
			MAPGEN_RELOAD_SAME_LEVEL="${2:-}"
			shift 2
			;;
		--mapgen-reload-seed-base)
			MAPGEN_RELOAD_SEED_BASE="${2:-}"
			shift 2
			;;
		--outdir)
			OUTDIR="${2:-}"
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			usage
			exit 1
			;;
	esac
done

if [[ -z "$APP" || ! -x "$APP" ]]; then
	echo "Barony executable not found or not executable: $APP" >&2
	exit 1
fi
if [[ -n "$DATADIR" ]] && [[ ! -d "$DATADIR" ]]; then
	echo "--datadir must reference an existing directory: $DATADIR" >&2
	exit 1
fi
if ! is_uint "$MIN_PLAYERS" || ! is_uint "$MAX_PLAYERS" || ! is_uint "$RUNS_PER_PLAYER"; then
	echo "--min-players, --max-players and --runs-per-player must be positive integers" >&2
	exit 1
fi
if (( MIN_PLAYERS < 1 || MAX_PLAYERS > 15 || MIN_PLAYERS > MAX_PLAYERS )); then
	echo "Player range must satisfy 1 <= min <= max <= 15" >&2
	exit 1
fi
if (( RUNS_PER_PLAYER < 1 )); then
	echo "--runs-per-player must be >= 1" >&2
	exit 1
fi
if ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$AUTO_START_DELAY_SECS" || ! is_uint "$AUTO_ENTER_DUNGEON_DELAY_SECS" || ! is_uint "$STAGGER_SECONDS"; then
	echo "--timeout, --stagger, --auto-start-delay and --auto-enter-dungeon-delay must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$AUTO_ENTER_DUNGEON" || (( AUTO_ENTER_DUNGEON > 1 )); then
	echo "--auto-enter-dungeon must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$FORCE_CHUNK" || (( FORCE_CHUNK > 1 )); then
	echo "--force-chunk must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$CHUNK_PAYLOAD_MAX" || (( CHUNK_PAYLOAD_MAX < 64 || CHUNK_PAYLOAD_MAX > 900 )); then
	echo "--chunk-payload-max must be 64..900" >&2
	exit 1
fi
if ! is_uint "$SIMULATE_MAPGEN_PLAYERS" || (( SIMULATE_MAPGEN_PLAYERS > 1 )); then
	echo "--simulate-mapgen-players must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$INPROCESS_SIM_BATCH" || (( INPROCESS_SIM_BATCH > 1 )); then
	echo "--inprocess-sim-batch must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$INPROCESS_PLAYER_SWEEP" || (( INPROCESS_PLAYER_SWEEP > 1 )); then
	echo "--inprocess-player-sweep must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$MAPGEN_RELOAD_SAME_LEVEL" || (( MAPGEN_RELOAD_SAME_LEVEL > 1 )); then
	echo "--mapgen-reload-same-level must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$MAPGEN_RELOAD_SEED_BASE"; then
	echo "--mapgen-reload-seed-base must be a non-negative integer" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/mapgen-level-matrix-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"

IFS=',' read -r -a raw_levels <<< "$LEVELS"
levels=()
for raw in "${raw_levels[@]}"; do
	level="${raw//[[:space:]]/}"
	if [[ -z "$level" ]]; then
		continue
	fi
	if ! is_uint "$level" || (( level < 1 || level > 99 )); then
		echo "--levels must be comma-separated integers in 1..99 (bad value: $level)" >&2
		exit 1
	fi
	levels+=("$level")
done
if ((${#levels[@]} == 0)); then
	echo "--levels must provide at least one floor value" >&2
	exit 1
fi

log "Writing outputs to $OUTDIR"
combined_csv="$OUTDIR/mapgen_level_matrix.csv"
cat > "$combined_csv" <<'CSV'
target_level,players,launched_instances,mapgen_players_override,mapgen_players_observed,run,seed,status,start_floor,host_chunk_lines,client_reassembled_lines,mapgen_found,mapgen_level,mapgen_secret,mapgen_seed_observed,rooms,monsters,gold,items,decorations,decorations_blocking,decorations_utility,decorations_traps,decorations_economy,food_items,food_servings,gold_bags,gold_amount,item_stacks,item_units,run_dir,mapgen_wait_reason,mapgen_reload_transition_lines,mapgen_generation_lines,mapgen_generation_unique_seed_count,mapgen_reload_regen_ok
CSV

datadir_args=()
if [[ -n "$DATADIR" ]]; then
	datadir_args=(--datadir "$DATADIR")
fi

level_failures=0
for level in "${levels[@]}"; do
	start_floor="$level"
	if (( MAPGEN_RELOAD_SAME_LEVEL == 0 )); then
		start_floor=$((level - 1))
		if (( start_floor < 0 )); then
			start_floor=0
		fi
	fi
	level_seed=$((BASE_SEED + level * 100000))
	level_outdir="$OUTDIR/level-${level}"

	log "Level lane start: target_level=$level start_floor=$start_floor"
	if ! "$SWEEP" \
		--app "$APP" \
		"${datadir_args[@]}" \
		--min-players "$MIN_PLAYERS" \
		--max-players "$MAX_PLAYERS" \
		--runs-per-player "$RUNS_PER_PLAYER" \
		--base-seed "$level_seed" \
		--size "$WINDOW_SIZE" \
		--stagger "$STAGGER_SECONDS" \
		--timeout "$TIMEOUT_SECONDS" \
		--auto-start-delay "$AUTO_START_DELAY_SECS" \
		--auto-enter-dungeon "$AUTO_ENTER_DUNGEON" \
		--auto-enter-dungeon-delay "$AUTO_ENTER_DUNGEON_DELAY_SECS" \
		--force-chunk "$FORCE_CHUNK" \
		--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
			--simulate-mapgen-players "$SIMULATE_MAPGEN_PLAYERS" \
			--inprocess-sim-batch "$INPROCESS_SIM_BATCH" \
			--inprocess-player-sweep "$INPROCESS_PLAYER_SWEEP" \
			--mapgen-reload-same-level "$MAPGEN_RELOAD_SAME_LEVEL" \
			--mapgen-reload-seed-base "$MAPGEN_RELOAD_SEED_BASE" \
		--start-floor "$start_floor" \
		--outdir "$level_outdir"; then
		level_failures=$((level_failures + 1))
	fi

	level_csv="$level_outdir/mapgen_results.csv"
	if [[ ! -f "$level_csv" ]]; then
		echo "missing level csv: $level_csv" >&2
		level_failures=$((level_failures + 1))
		continue
	fi
	awk -F',' -v level="$level" 'NR>1 && NF>0 {print level "," $0}' "$level_csv" >> "$combined_csv"
done

if command -v python3 >/dev/null 2>&1; then
	python3 - <<'PY' "$combined_csv" "$OUTDIR/mapgen_level_trends.csv" "$OUTDIR/mapgen_level_overall.csv" "$OUTDIR/mapgen_level_overall.md"
import csv
import math
import sys
from collections import defaultdict

src = sys.argv[1]
out_trends = sys.argv[2]
out_overall_csv = sys.argv[3]
out_overall_md = sys.argv[4]
metrics = [
    "rooms",
    "monsters",
    "gold",
    "gold_bags",
    "gold_amount",
    "items",
    "item_stacks",
    "item_units",
    "food_servings",
    "decorations",
    "decorations_blocking",
    "decorations_utility",
    "decorations_traps",
    "decorations_economy",
]

def parse_int(value):
    try:
        return int(str(value).strip())
    except Exception:
        return None

def parse_float(value):
    try:
        return float(str(value).strip())
    except Exception:
        return None

def fmt(value, digits):
    if value is None:
        return ""
    return f"{value:.{digits}f}"

rows = []
with open(src, newline="") as f:
    for row in csv.DictReader(f):
        if row.get("status") != "pass":
            continue
        level = parse_int(row.get("target_level"))
        players = parse_int(row.get("players"))
        if level is None or players is None:
            continue
        vals = {}
        ok = True
        for k in metrics:
            parsed = parse_float(row.get(k))
            if parsed is None:
                ok = False
                break
            vals[k] = parsed
        if not ok:
            continue
        observed_level = parse_int(row.get("mapgen_level"))
        level_match = 1 if observed_level is not None and observed_level == level else 0
        observed_seed = parse_int(row.get("mapgen_seed_observed"))
        generation_lines = parse_int(row.get("mapgen_generation_lines"))
        generation_unique_seed_count = parse_int(row.get("mapgen_generation_unique_seed_count"))
        reload_unique_seed_rate = None
        if generation_lines is not None and generation_lines > 0 and generation_unique_seed_count is not None:
            reload_unique_seed_rate = 100.0 * generation_unique_seed_count / generation_lines
        rows.append((level, players, vals, level_match, observed_seed, reload_unique_seed_rate))

def slope_and_corr(xs, ys):
    if not xs or not ys or len(xs) != len(ys):
        return (None, None)
    if len(xs) == 1:
        return (0.0, 0.0)
    mx = sum(xs) / len(xs)
    my = sum(ys) / len(ys)
    den = sum((x - mx) ** 2 for x in xs)
    num = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    slope = (num / den) if den else 0.0
    var_x = sum((x - mx) ** 2 for x in xs)
    var_y = sum((y - my) ** 2 for y in ys)
    corr = 0.0
    if var_x > 0 and var_y > 0:
        corr = num / math.sqrt(var_x * var_y)
    return (slope, corr)

by_level = defaultdict(list)
for level, players, vals, level_match, observed_seed, reload_unique_seed_rate in rows:
    by_level[level].append((players, vals, level_match, observed_seed, reload_unique_seed_rate))

metric_records = defaultdict(list)
level_match_rates = []
level_observed_seed_unique_rates = []
level_reload_unique_seed_rates = []
with open(out_trends, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
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
    ])
    for level in sorted(by_level):
        level_rows = by_level[level]
        by_player = defaultdict(lambda: defaultdict(list))
        match_total = 0
        observed_seeds = []
        reload_unique_seed_rates = []
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
        reload_unique_seed_rate = (
            (sum(reload_unique_seed_rates) / len(reload_unique_seed_rates))
            if reload_unique_seed_rates else None
        )
        level_match_rates.append(match_rate)
        if observed_seed_unique_rate is not None:
            level_observed_seed_unique_rates.append(observed_seed_unique_rate)
        if reload_unique_seed_rate is not None:
            level_reload_unique_seed_rates.append(reload_unique_seed_rate)
        for metric in metrics:
            xs = []
            ys = []
            for p in players_sorted:
                vals = by_player[p][metric]
                if not vals:
                    continue
                xs.append(float(p))
                ys.append(sum(vals) / len(vals))
            if not xs:
                writer.writerow([
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
                ])
                continue
            slope, corr = slope_and_corr(xs, ys)
            low_vals = [sum(by_player[p][metric]) / len(by_player[p][metric]) for p in players_sorted if p <= 4 and by_player[p][metric]]
            high_vals = [sum(by_player[p][metric]) / len(by_player[p][metric]) for p in players_sorted if p >= 12 and by_player[p][metric]]
            high_vs_low = None
            if low_vals and high_vals:
                low_avg = sum(low_vals) / len(low_vals)
                high_avg = sum(high_vals) / len(high_vals)
                high_vs_low = ((high_avg - low_avg) / low_avg * 100.0) if low_avg else 0.0
            writer.writerow([
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
            ])
            if slope is not None:
                metric_records[metric].append({"level": level, "slope": slope, "corr": corr, "high_vs_low": high_vs_low})

mean_target_level_match_rate = (sum(level_match_rates) / len(level_match_rates)) if level_match_rates else None
mean_observed_seed_unique_rate = (
    sum(level_observed_seed_unique_rates) / len(level_observed_seed_unique_rates)
    if level_observed_seed_unique_rates else None
)
mean_reload_unique_seed_rate = (
    sum(level_reload_unique_seed_rates) / len(level_reload_unique_seed_rates)
    if level_reload_unique_seed_rates else None
)

with open(out_overall_csv, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
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
    ])
    for metric in metrics:
        records = metric_records.get(metric, [])
        if not records:
            writer.writerow([
                metric, 0, 0, "", "", "", "", "", "",
                fmt(mean_target_level_match_rate, 1),
                fmt(mean_observed_seed_unique_rate, 1),
                fmt(mean_reload_unique_seed_rate, 1),
            ])
            continue
        slopes = [r["slope"] for r in records]
        corrs = [r["corr"] for r in records if r["corr"] is not None]
        high_low = [r["high_vs_low"] for r in records if r["high_vs_low"] is not None]
        total = len(records)
        positive = sum(1 for s in slopes if s > 0.0)
        positive_pct = (100.0 * positive / total) if total else 0.0
        mean_slope = sum(slopes) / total if total else 0.0
        min_slope = min(slopes) if slopes else 0.0
        mean_corr = (sum(corrs) / len(corrs)) if corrs else None
        mean_high = (sum(high_low) / len(high_low)) if high_low else None
        min_high = min(high_low) if high_low else None
        writer.writerow([
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
        ])

with open(out_overall_md, "w", encoding="utf-8") as f:
    f.write("# Mapgen Level Matrix Overall Summary\n\n")
    f.write(f"- Source CSV: `{src}`\n")
    f.write(f"- Evaluated pass rows: {len(rows)}\n")
    f.write(f"- Levels covered: {len(by_level)}\n\n")
    f.write(f"- Mean target-level match rate: {fmt(mean_target_level_match_rate, 1) or 'n/a'}%\n")
    f.write(f"- Mean observed-seed unique rate: {fmt(mean_observed_seed_unique_rate, 1) or 'n/a'}%\n")
    f.write(f"- Mean reload unique-seed rate: {fmt(mean_reload_unique_seed_rate, 1) or 'n/a'}%\n\n")
    f.write("| Metric | Positive Slope Levels | Mean Slope | Min Slope | Mean Corr | Mean High-vs-Low % |\n")
    f.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
    for metric in metrics:
        records = metric_records.get(metric, [])
        if not records:
            f.write(f"| {metric} | 0/0 | n/a | n/a | n/a | n/a |\n")
            continue
        slopes = [r["slope"] for r in records]
        corrs = [r["corr"] for r in records if r["corr"] is not None]
        high_low = [r["high_vs_low"] for r in records if r["high_vs_low"] is not None]
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
PY
	log "Level trend summary written to $OUTDIR/mapgen_level_trends.csv"
	log "Overall trend summary written to $OUTDIR/mapgen_level_overall.csv"
	log "Overall markdown summary written to $OUTDIR/mapgen_level_overall.md"
	if [[ -f "$AGGREGATE" ]]; then
		python3 "$AGGREGATE" --output "$OUTDIR/mapgen_level_matrix_aggregate_report.html" --mapgen-matrix-csv "$combined_csv"
		log "Aggregate report written to $OUTDIR/mapgen_level_matrix_aggregate_report.html"
	fi
else
	log "python3 not found; skipped level trend summary"
fi

find "$OUTDIR" -type f -name models.cache -delete 2>/dev/null || true
log "Combined CSV written to $combined_csv"
log "Per-level outputs are under $OUTDIR/level-*"

if (( level_failures > 0 )); then
	log "Completed with $level_failures failing level lane(s)"
	exit 1
fi
