#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
AGGREGATE="$SCRIPT_DIR/generate_smoke_aggregate_report.py"

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
INSTANCES=4
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
PASS_TIMEOUT_SECONDS=180
FAIL_TIMEOUT_SECONDS=60
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
OUTDIR=""

usage() {
	cat <<'USAGE'
Usage: run_helo_adversarial_smoke_mac.sh [options]

Options:
  --app <path>                 Barony executable path.
  --instances <n>              Number of instances per case (default: 4, min: 2).
  --size <WxH>                 Window size.
  --stagger <sec>              Delay between launches.
  --pass-timeout <sec>         Timeout for expected-pass cases.
  --fail-timeout <sec>         Timeout for expected-fail cases.
  --force-chunk <0|1>          BARONY_SMOKE_FORCE_HELO_CHUNK setting.
  --chunk-payload-max <n>      HELO chunk payload cap (64..900).
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
		--pass-timeout)
			PASS_TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--fail-timeout)
			FAIL_TIMEOUT_SECONDS="${2:-}"
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
if ! is_uint "$INSTANCES" || (( INSTANCES < 2 || INSTANCES > 16 )); then
	echo "--instances must be 2..16" >&2
	exit 1
fi
if ! is_uint "$PASS_TIMEOUT_SECONDS" || ! is_uint "$FAIL_TIMEOUT_SECONDS" || ! is_uint "$STAGGER_SECONDS"; then
	echo "--pass-timeout, --fail-timeout and --stagger must be non-negative integers" >&2
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

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/helo-adversarial-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
RUNS_DIR="$OUTDIR/runs"
mkdir -p "$RUNS_DIR"

CSV_PATH="$OUTDIR/adversarial_results.csv"
cat > "$CSV_PATH" <<'CSV'
case_name,tx_mode,expected_result,observed_result,match,instances,host_chunk_lines,client_reassembled_lines,mapgen_found,run_dir
CSV

mismatches=0

while IFS='|' read -r case_name tx_mode expected; do
	[[ -z "$case_name" ]] && continue
	run_dir="$RUNS_DIR/$case_name"
	mkdir -p "$run_dir"
	timeout_seconds="$PASS_TIMEOUT_SECONDS"
	if [[ "$expected" == "fail" ]]; then
		timeout_seconds="$FAIL_TIMEOUT_SECONDS"
	fi

	log "Case=$case_name mode=$tx_mode expected=$expected timeout=${timeout_seconds}s"
	if "$RUNNER" \
		--app "$APP" \
		--instances "$INSTANCES" \
		--size "$WINDOW_SIZE" \
		--stagger "$STAGGER_SECONDS" \
		--timeout "$timeout_seconds" \
		--expected-players "$INSTANCES" \
		--auto-start 0 \
		--force-chunk "$FORCE_CHUNK" \
		--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
		--helo-chunk-tx-mode "$tx_mode" \
		--require-helo 1 \
		--require-mapgen 0 \
		--outdir "$run_dir"; then
		observed="pass"
	else
		observed="fail"
	fi

	match="1"
	if [[ "$observed" != "$expected" ]]; then
		match="0"
		mismatches=$((mismatches + 1))
	fi

	summary="$run_dir/summary.env"
	host_chunk_lines=""
	client_reassembled_lines=""
	mapgen_found=""
	if [[ -f "$summary" ]]; then
		host_chunk_lines="$(read_summary_key HOST_CHUNK_LINES "$summary")"
		client_reassembled_lines="$(read_summary_key CLIENT_REASSEMBLED_LINES "$summary")"
		mapgen_found="$(read_summary_key MAPGEN_FOUND "$summary")"
	fi

	printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
		"$case_name" "$tx_mode" "$expected" "$observed" "$match" "$INSTANCES" \
		"${host_chunk_lines:-}" "${client_reassembled_lines:-}" "${mapgen_found:-}" "$run_dir" >> "$CSV_PATH"
done <<'CASES'
baseline|normal|pass
reverse_order|reverse|pass
even_odd_order|even-odd|pass
duplicate_first|duplicate-first|pass
drop_last|drop-last|fail
conflicting_duplicate|duplicate-conflict-first|fail
CASES

if command -v python3 >/dev/null 2>&1 && [[ -f "$AGGREGATE" ]]; then
	python3 "$AGGREGATE" --output "$OUTDIR/smoke_aggregate_report.html" --adversarial-csv "$CSV_PATH"
	log "Aggregate report written to $OUTDIR/smoke_aggregate_report.html"
fi

log "CSV written to $CSV_PATH"
if (( mismatches > 0 )); then
	log "Completed with $mismatches adversarial expectation mismatch(es)"
	exit 1
fi
log "All adversarial expectations matched"
