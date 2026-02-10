#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
AGGREGATE="$SCRIPT_DIR/generate_smoke_aggregate_report.py"

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
RUNS=10
INSTANCES=8
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=360
AUTO_START_DELAY_SECS=2
AUTO_ENTER_DUNGEON=1
AUTO_ENTER_DUNGEON_DELAY_SECS=3
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
HELO_CHUNK_TX_MODE="normal"
REQUIRE_MAPGEN=1
OUTDIR=""

usage() {
	cat <<'USAGE'
Usage: run_lan_helo_soak_mac.sh [options]

Options:
  --app <path>                   Barony executable path.
  --datadir <path>               Optional data directory passed through to runner via -datadir=<path>.
  --runs <n>                     Number of soak runs (default: 10).
  --instances <n>                Number of game instances per run (default: 8).
  --size <WxH>                   Window size for all instances.
  --stagger <sec>                Delay between instance launches.
  --timeout <sec>                Timeout per run.
  --auto-start-delay <sec>       Host auto-start delay after full lobby.
  --auto-enter-dungeon <0|1>     Host forces first dungeon transition after load.
  --auto-enter-dungeon-delay <s> Delay before forced dungeon entry.
  --force-chunk <0|1>            BARONY_SMOKE_FORCE_HELO_CHUNK setting.
  --chunk-payload-max <n>        HELO chunk payload cap (64..900).
  --helo-chunk-tx-mode <mode>    HELO tx mode (normal|reverse|even-odd|duplicate-first|drop-last|duplicate-conflict-first).
  --require-mapgen <0|1>         Require dungeon mapgen marker in each run.
  --outdir <path>                Output directory.
  -h, --help                     Show this help.
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
		--runs)
			RUNS="${2:-}"
			shift 2
			;;
		--instances)
			INSTANCES="${2:-}"
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
		--helo-chunk-tx-mode)
			HELO_CHUNK_TX_MODE="${2:-}"
			shift 2
			;;
		--require-mapgen)
			REQUIRE_MAPGEN="${2:-}"
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
if ! is_uint "$RUNS" || (( RUNS < 1 )); then
	echo "--runs must be >= 1" >&2
	exit 1
fi
if ! is_uint "$INSTANCES" || (( INSTANCES < 1 || INSTANCES > 15 )); then
	echo "--instances must be 1..15" >&2
	exit 1
fi
if ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$STAGGER_SECONDS" || ! is_uint "$AUTO_START_DELAY_SECS" || ! is_uint "$AUTO_ENTER_DUNGEON_DELAY_SECS"; then
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
if ! is_uint "$REQUIRE_MAPGEN" || (( REQUIRE_MAPGEN > 1 )); then
	echo "--require-mapgen must be 0 or 1" >&2
	exit 1
fi
case "$HELO_CHUNK_TX_MODE" in
	normal|reverse|evenodd|even-odd|even_odd|duplicate-first|duplicate_first|drop-last|drop_last|duplicate-conflict-first|duplicate_conflict_first)
		;;
	*)
		echo "--helo-chunk-tx-mode must be one of: normal, reverse, even-odd, duplicate-first, drop-last, duplicate-conflict-first" >&2
		exit 1
		;;
esac

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/soak-${timestamp}-p${INSTANCES}-n${RUNS}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
RUNS_DIR="$OUTDIR/runs"
mkdir -p "$RUNS_DIR"

CSV_PATH="$OUTDIR/soak_results.csv"
cat > "$CSV_PATH" <<'CSV'
run,status,instances,host_chunk_lines,client_reassembled_lines,mapgen_found,gamestart_found,tx_mode,run_dir
CSV

failures=0
datadir_args=()
if [[ -n "$DATADIR" ]]; then
	datadir_args=(--datadir "$DATADIR")
fi
for ((run = 1; run <= RUNS; ++run)); do
	run_dir="$RUNS_DIR/r${run}"
	mkdir -p "$run_dir"
	log "Run ${run}/${RUNS}: instances=${INSTANCES}"

	if "$RUNNER" \
		--app "$APP" \
		"${datadir_args[@]}" \
		--instances "$INSTANCES" \
		--size "$WINDOW_SIZE" \
		--stagger "$STAGGER_SECONDS" \
		--timeout "$TIMEOUT_SECONDS" \
		--expected-players "$INSTANCES" \
		--auto-start 1 \
		--auto-start-delay "$AUTO_START_DELAY_SECS" \
		--auto-enter-dungeon "$AUTO_ENTER_DUNGEON" \
		--auto-enter-dungeon-delay "$AUTO_ENTER_DUNGEON_DELAY_SECS" \
		--force-chunk "$FORCE_CHUNK" \
		--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
		--helo-chunk-tx-mode "$HELO_CHUNK_TX_MODE" \
		--require-mapgen "$REQUIRE_MAPGEN" \
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
	gamestart_found=""
	if [[ -f "$summary" ]]; then
		host_chunk_lines="$(read_summary_key HOST_CHUNK_LINES "$summary")"
		client_reassembled_lines="$(read_summary_key CLIENT_REASSEMBLED_LINES "$summary")"
		mapgen_found="$(read_summary_key MAPGEN_FOUND "$summary")"
		gamestart_found="$(read_summary_key GAMESTART_FOUND "$summary")"
	fi

	printf '%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
		"$run" "$status" "$INSTANCES" \
		"${host_chunk_lines:-}" "${client_reassembled_lines:-}" "${mapgen_found:-}" "${gamestart_found:-}" \
		"$HELO_CHUNK_TX_MODE" "$run_dir" >> "$CSV_PATH"
done

if command -v python3 >/dev/null 2>&1 && [[ -f "$AGGREGATE" ]]; then
	python3 "$AGGREGATE" --output "$OUTDIR/smoke_aggregate_report.html" --soak-csv "$CSV_PATH"
	log "Aggregate report written to $OUTDIR/smoke_aggregate_report.html"
fi

log "CSV written to $CSV_PATH"
log "Completed $RUNS run(s) with $failures failure(s)"
if (( failures > 0 )); then
	exit 1
fi
