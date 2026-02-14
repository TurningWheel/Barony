#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=420
PAGE_DELAY_SECONDS=2
INSTANCES=15
REQUIRE_FOCUS_MATCH=0
OUTDIR=""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HELO_RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"

usage() {
	cat <<'USAGE'
Usage: run_lobby_page_navigation_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed to Barony via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches (default: 1).
  --timeout <sec>               Timeout for the lane (default: 420).
  --page-delay <sec>            Delay between host page-sweep steps (default: 2).
  --instances <n>               Lobby size for lane (default: 15, range: 5..15).
  --require-focus-match <0|1>   Require focused widget page match during sweep (default: 0).
  --outdir <path>               Artifact directory.
  -h, --help                    Show this help.
USAGE
}

is_uint() {
	smoke_is_uint "$1"
}

log() {
	smoke_log "$*"
}

read_summary_value() {
	local summary_file="$1"
	local key="$2"
	smoke_summary_get_last "$key" "$summary_file"
}

prune_models_cache() {
	smoke_prune_models_cache "$1"
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
		--page-delay)
			PAGE_DELAY_SECONDS="${2:-}"
			shift 2
			;;
		--instances)
			INSTANCES="${2:-}"
			shift 2
			;;
		--require-focus-match)
			REQUIRE_FOCUS_MATCH="${2:-}"
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
if [[ ! -x "$HELO_RUNNER" ]]; then
	echo "Required runner missing or not executable: $HELO_RUNNER" >&2
	exit 1
fi
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$PAGE_DELAY_SECONDS"; then
	echo "--stagger, --timeout and --page-delay must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$INSTANCES" || (( INSTANCES < 5 || INSTANCES > 15 )); then
	echo "--instances must be 5..15" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_FOCUS_MATCH" || (( REQUIRE_FOCUS_MATCH > 1 )); then
	echo "--require-focus-match must be 0 or 1" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/lobby-page-navigation-${timestamp}-p${INSTANCES}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -rf "$OUTDIR/lane-page-navigation"
rm -f "$OUTDIR/page_navigation_results.csv" "$OUTDIR/summary.env"

lane_name="page-navigation-p${INSTANCES}"
lane_outdir="$OUTDIR/lane-page-navigation"
csv_path="$OUTDIR/page_navigation_results.csv"

{
	echo "lane,instances,result,child_result,lobby_page_state_ok,lobby_page_sweep_ok,lobby_page_snapshot_lines,lobby_page_unique_count,lobby_page_total_count,lobby_page_visited,lobby_focus_mismatch_lines,lobby_cards_misaligned_max,lobby_paperdolls_misaligned_max,lobby_pings_misaligned_max,lobby_warnings_max_abs_delta,lobby_countdown_max_abs_delta,outdir"
} > "$csv_path"

cmd=(
	"$HELO_RUNNER"
	--app "$APP"
	--instances "$INSTANCES"
	--expected-players "$INSTANCES"
	--size "$WINDOW_SIZE"
	--stagger "$STAGGER_SECONDS"
	--timeout "$TIMEOUT_SECONDS"
	--force-chunk 1
	--chunk-payload-max 200
	--require-helo 1
	--auto-start 0
	--trace-lobby-page-state 1
	--require-lobby-page-state 1
	--require-lobby-page-focus-match "$REQUIRE_FOCUS_MATCH"
	--auto-lobby-page-sweep 1
	--auto-lobby-page-delay "$PAGE_DELAY_SECONDS"
	--require-lobby-page-sweep 1
	--outdir "$lane_outdir"
)
if [[ -n "$DATADIR" ]]; then
	cmd+=(--datadir "$DATADIR")
fi

log "Running lane $lane_name"
if "${cmd[@]}"; then
	child_result="pass"
else
	child_result="fail"
fi

summary_file="$lane_outdir/summary.env"
lobby_page_state_ok="$(read_summary_value "$summary_file" "LOBBY_PAGE_STATE_OK")"
lobby_page_sweep_ok="$(read_summary_value "$summary_file" "LOBBY_PAGE_SWEEP_OK")"
lobby_page_snapshot_lines="$(read_summary_value "$summary_file" "LOBBY_PAGE_SNAPSHOT_LINES")"
lobby_page_unique_count="$(read_summary_value "$summary_file" "LOBBY_PAGE_UNIQUE_COUNT")"
lobby_page_total_count="$(read_summary_value "$summary_file" "LOBBY_PAGE_TOTAL_COUNT")"
lobby_page_visited="$(read_summary_value "$summary_file" "LOBBY_PAGE_VISITED")"
lobby_focus_mismatch_lines="$(read_summary_value "$summary_file" "LOBBY_FOCUS_MISMATCH_LINES")"
lobby_cards_misaligned_max="$(read_summary_value "$summary_file" "LOBBY_CARDS_MISALIGNED_MAX")"
lobby_paperdolls_misaligned_max="$(read_summary_value "$summary_file" "LOBBY_PAPERDOLLS_MISALIGNED_MAX")"
lobby_pings_misaligned_max="$(read_summary_value "$summary_file" "LOBBY_PINGS_MISALIGNED_MAX")"
lobby_warnings_max_abs_delta="$(read_summary_value "$summary_file" "LOBBY_WARNINGS_MAX_ABS_DELTA")"
lobby_countdown_max_abs_delta="$(read_summary_value "$summary_file" "LOBBY_COUNTDOWN_MAX_ABS_DELTA")"

for key in \
	lobby_page_state_ok lobby_page_sweep_ok lobby_page_snapshot_lines \
	lobby_page_unique_count lobby_page_total_count lobby_focus_mismatch_lines \
	lobby_cards_misaligned_max lobby_paperdolls_misaligned_max \
	lobby_pings_misaligned_max lobby_warnings_max_abs_delta \
	lobby_countdown_max_abs_delta; do
	if [[ -z "${!key}" ]]; then
		printf -v "$key" "0"
	fi
done

lane_result="pass"
if [[ "$child_result" != "pass" || "$lobby_page_state_ok" != "1" || "$lobby_page_sweep_ok" != "1" ]]; then
	lane_result="fail"
fi

echo "${lane_name},${INSTANCES},${lane_result},${child_result},${lobby_page_state_ok},${lobby_page_sweep_ok},${lobby_page_snapshot_lines},${lobby_page_unique_count},${lobby_page_total_count},${lobby_page_visited},${lobby_focus_mismatch_lines},${lobby_cards_misaligned_max},${lobby_paperdolls_misaligned_max},${lobby_pings_misaligned_max},${lobby_warnings_max_abs_delta},${lobby_countdown_max_abs_delta},${lane_outdir}" >> "$csv_path"

prune_models_cache "$lane_outdir"

overall_result="pass"
if [[ "$lane_result" != "pass" ]]; then
	overall_result="fail"
fi

summary_path="$OUTDIR/summary.env"
{
	echo "RESULT=$overall_result"
	echo "OUTDIR=$OUTDIR"
	echo "APP=$APP"
	echo "DATADIR=$DATADIR"
	echo "INSTANCES=$INSTANCES"
	echo "TIMEOUT_SECONDS=$TIMEOUT_SECONDS"
	echo "PAGE_DELAY_SECONDS=$PAGE_DELAY_SECONDS"
	echo "REQUIRE_FOCUS_MATCH=$REQUIRE_FOCUS_MATCH"
	echo "PASS_LANES=$([[ "$lane_result" == "pass" ]] && echo 1 || echo 0)"
	echo "FAIL_LANES=$([[ "$lane_result" == "fail" ]] && echo 1 || echo 0)"
	echo "CSV_PATH=$csv_path"
	echo "LANE_OUTDIR=$lane_outdir"
} > "$summary_path"

log "result=$overall_result lane=$lane_result"
log "csv=$csv_path"
log "summary=$summary_path"

if [[ "$overall_result" != "pass" ]]; then
	exit 1
fi
