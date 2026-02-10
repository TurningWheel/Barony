#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
HEATMAP="$SCRIPT_DIR/generate_mapgen_heatmap.py"
AGGREGATE="$SCRIPT_DIR/generate_smoke_aggregate_report.py"

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
MIN_PLAYERS=1
MAX_PLAYERS=16
RUNS_PER_PLAYER=1
BASE_SEED=1000
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=180
AUTO_START_DELAY_SECS=2
AUTO_ENTER_DUNGEON=1
AUTO_ENTER_DUNGEON_DELAY_SECS=3
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
SIMULATE_MAPGEN_PLAYERS=0
OUTDIR=""

usage() {
	cat <<'USAGE'
Usage: run_mapgen_sweep_mac.sh [options]

Options:
  --app <path>                 Barony executable path.
  --min-players <n>            Start player count (default: 1).
  --max-players <n>            End player count (default: 16).
  --runs-per-player <n>        Runs for each player count (default: 1).
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
if ! is_uint "$MIN_PLAYERS" || ! is_uint "$MAX_PLAYERS" || ! is_uint "$RUNS_PER_PLAYER"; then
	echo "--min-players, --max-players and --runs-per-player must be positive integers" >&2
	exit 1
fi
if (( MIN_PLAYERS < 1 || MAX_PLAYERS > 16 || MIN_PLAYERS > MAX_PLAYERS )); then
	echo "Player range must satisfy 1 <= min <= max <= 16" >&2
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

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/mapgen-sweep-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
RUNS_DIR="$OUTDIR/runs"
mkdir -p "$RUNS_DIR"

CSV_PATH="$OUTDIR/mapgen_results.csv"
cat > "$CSV_PATH" <<'CSV'
players,launched_instances,mapgen_players_override,run,seed,status,host_chunk_lines,client_reassembled_lines,mapgen_found,rooms,monsters,gold,items,decorations,run_dir
CSV

failures=0
total_runs=0

log "Writing outputs to $OUTDIR"
if (( SIMULATE_MAPGEN_PLAYERS )); then
	log "Mapgen sweep mode: single-instance simulated player scaling"
fi

for ((players = MIN_PLAYERS; players <= MAX_PLAYERS; ++players)); do
	for ((run = 1; run <= RUNS_PER_PLAYER; ++run)); do
		total_runs=$((total_runs + 1))
		seed=$((BASE_SEED + (players - MIN_PLAYERS) * RUNS_PER_PLAYER + run))
		run_dir="$RUNS_DIR/p${players}-r${run}"
		mkdir -p "$run_dir"

		launched_instances="$players"
		expected_players="$players"
		mapgen_players_override=""
		require_helo=0
		if (( players > 1 )); then
			require_helo=1
		fi
		if (( SIMULATE_MAPGEN_PLAYERS )); then
			launched_instances=1
			expected_players=1
			require_helo=0
			mapgen_players_override="$players"
		fi

		log "Run ${total_runs}: players=${players} launched=${launched_instances} run=${run} seed=${seed}"
		mapgen_override_args=()
		if [[ -n "$mapgen_players_override" ]]; then
			mapgen_override_args+=(--mapgen-players-override "$mapgen_players_override")
		fi
		if "$RUNNER" \
			--app "$APP" \
			--instances "$launched_instances" \
			--size "$WINDOW_SIZE" \
			--stagger "$STAGGER_SECONDS" \
			--timeout "$TIMEOUT_SECONDS" \
			--expected-players "$expected_players" \
			--auto-start 1 \
			--auto-start-delay "$AUTO_START_DELAY_SECS" \
			--auto-enter-dungeon "$AUTO_ENTER_DUNGEON" \
			--auto-enter-dungeon-delay "$AUTO_ENTER_DUNGEON_DELAY_SECS" \
			--force-chunk "$FORCE_CHUNK" \
			--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
			--seed "$seed" \
			--require-helo "$require_helo" \
			--require-mapgen 1 \
			"${mapgen_override_args[@]}" \
			--outdir "$run_dir"; then
			status="pass"
		else
			status="fail"
			failures=$((failures + 1))
		fi

		summary="$run_dir/summary.env"
		host_chunk_lines=""
		client_reassembled_lines=""
		mapgen_found=""
		rooms=""
		monsters=""
		gold=""
		items=""
		decorations=""
		if [[ -f "$summary" ]]; then
			host_chunk_lines="$(read_summary_key HOST_CHUNK_LINES "$summary")"
			client_reassembled_lines="$(read_summary_key CLIENT_REASSEMBLED_LINES "$summary")"
			mapgen_found="$(read_summary_key MAPGEN_FOUND "$summary")"
			rooms="$(read_summary_key MAPGEN_ROOMS "$summary")"
			monsters="$(read_summary_key MAPGEN_MONSTERS "$summary")"
			gold="$(read_summary_key MAPGEN_GOLD "$summary")"
			items="$(read_summary_key MAPGEN_ITEMS "$summary")"
			decorations="$(read_summary_key MAPGEN_DECORATIONS "$summary")"
		fi

		printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
			"$players" "$launched_instances" "${mapgen_players_override:-}" "$run" "$seed" "$status" \
			"${host_chunk_lines:-}" "${client_reassembled_lines:-}" "${mapgen_found:-}" \
			"${rooms:-}" "${monsters:-}" "${gold:-}" "${items:-}" "${decorations:-}" \
			"$run_dir" >> "$CSV_PATH"
	done
done

if command -v python3 >/dev/null 2>&1; then
	python3 "$HEATMAP" --input "$CSV_PATH" --output "$OUTDIR/mapgen_heatmap.html"
	log "Heatmap written to $OUTDIR/mapgen_heatmap.html"
	if [[ -f "$AGGREGATE" ]]; then
		python3 "$AGGREGATE" --output "$OUTDIR/smoke_aggregate_report.html" --mapgen-csv "$CSV_PATH"
		log "Aggregate report written to $OUTDIR/smoke_aggregate_report.html"
	fi
else
	log "python3 not found; skipped heatmap generation"
fi

log "CSV written to $CSV_PATH"
log "Completed $total_runs run(s) with $failures failure(s)"

if (( failures > 0 )); then
	exit 1
fi
