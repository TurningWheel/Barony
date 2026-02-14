#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=360
PLAYER_COUNT_DELAY_SECONDS=2
OUTDIR=""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HELO_RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"

usage() {
	cat <<'USAGE'
Usage: run_lobby_slot_lock_and_kick_copy_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed to Barony via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches (default: 1).
  --timeout <sec>               Timeout per lane in seconds (default: 360).
  --player-count-delay <sec>    Delay before host auto-requests player-count change (default: 2).
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
		--player-count-delay)
			PLAYER_COUNT_DELAY_SECONDS="${2:-}"
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
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$PLAYER_COUNT_DELAY_SECONDS"; then
	echo "--stagger, --timeout and --player-count-delay must be non-negative integers" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/lobby-slot-lock-kick-copy-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -rf "$OUTDIR"/lane-*
rm -f "$OUTDIR/slot_lock_kick_copy_results.csv" "$OUTDIR/summary.env"

CSV_PATH="$OUTDIR/slot_lock_kick_copy_results.csv"
{
	echo "lane,instances,expected_players,auto_player_count_target,expected_variant,result,child_result,default_slot_lock_ok,slot_lock_snapshot_lines,player_count_copy_ok,player_count_prompt_variant,player_count_prompt_kicked,player_count_prompt_lines,outdir"
} > "$CSV_PATH"

total_lanes=0
pass_lanes=0
fail_lanes=0

run_lane() {
	local lane_name="$1"
	local instances="$2"
	local expected_players="$3"
	local auto_player_count_target="$4"
	local expected_variant="$5"
	local require_default_lock="$6"
	local require_copy="$7"

	local lane_outdir="$OUTDIR/lane-${lane_name}"
	log "Running lane ${lane_name}"

	local -a cmd=(
		"$HELO_RUNNER"
		--app "$APP"
		--instances "$instances"
		--expected-players "$expected_players"
		--size "$WINDOW_SIZE"
		--stagger "$STAGGER_SECONDS"
		--timeout "$TIMEOUT_SECONDS"
		--force-chunk 1
		--chunk-payload-max 200
		--require-helo 1
		--auto-start 0
		--outdir "$lane_outdir"
	)
	if [[ -n "$DATADIR" ]]; then
		cmd+=(--datadir "$DATADIR")
	fi
	if (( require_default_lock )); then
		cmd+=(
			--trace-slot-locks 1
			--require-default-slot-locks 1
		)
	fi
	if (( auto_player_count_target > 0 )); then
		cmd+=(
			--auto-player-count-target "$auto_player_count_target"
			--auto-player-count-delay "$PLAYER_COUNT_DELAY_SECONDS"
		)
	fi
	if (( require_copy )); then
		cmd+=(
			--trace-player-count-copy 1
			--require-player-count-copy 1
			--expect-player-count-copy-variant "$expected_variant"
		)
	fi

	if "${cmd[@]}"; then
		child_result="pass"
	else
		child_result="fail"
	fi

	local summary_file="$lane_outdir/summary.env"
	local default_slot_lock_ok
	local slot_lock_snapshot_lines
	local player_count_copy_ok
	local player_count_prompt_variant
	local player_count_prompt_kicked
	local player_count_prompt_lines
	default_slot_lock_ok="$(read_summary_value "$summary_file" "DEFAULT_SLOT_LOCK_OK")"
	slot_lock_snapshot_lines="$(read_summary_value "$summary_file" "SLOT_LOCK_SNAPSHOT_LINES")"
	player_count_copy_ok="$(read_summary_value "$summary_file" "PLAYER_COUNT_COPY_OK")"
	player_count_prompt_variant="$(read_summary_value "$summary_file" "PLAYER_COUNT_PROMPT_VARIANT")"
	player_count_prompt_kicked="$(read_summary_value "$summary_file" "PLAYER_COUNT_PROMPT_KICKED")"
	player_count_prompt_lines="$(read_summary_value "$summary_file" "PLAYER_COUNT_PROMPT_LINES")"

	if [[ -z "$default_slot_lock_ok" ]]; then
		default_slot_lock_ok=0
	fi
	if [[ -z "$slot_lock_snapshot_lines" ]]; then
		slot_lock_snapshot_lines=0
	fi
	if [[ -z "$player_count_copy_ok" ]]; then
		player_count_copy_ok=0
	fi
	if [[ -z "$player_count_prompt_variant" ]]; then
		player_count_prompt_variant="missing"
	fi
	if [[ -z "$player_count_prompt_kicked" ]]; then
		player_count_prompt_kicked=0
	fi
	if [[ -z "$player_count_prompt_lines" ]]; then
		player_count_prompt_lines=0
	fi

	local lane_result="pass"
	if [[ "$child_result" != "pass" ]]; then
		lane_result="fail"
	fi
	if (( require_default_lock )) && [[ "$default_slot_lock_ok" != "1" ]]; then
		lane_result="fail"
	fi
	if (( require_copy )) && [[ "$player_count_copy_ok" != "1" ]]; then
		lane_result="fail"
	fi
	if (( require_copy )) && [[ "$player_count_prompt_variant" != "$expected_variant" ]]; then
		lane_result="fail"
	fi

	echo "${lane_name},${instances},${expected_players},${auto_player_count_target},${expected_variant},${lane_result},${child_result},${default_slot_lock_ok},${slot_lock_snapshot_lines},${player_count_copy_ok},${player_count_prompt_variant},${player_count_prompt_kicked},${player_count_prompt_lines},${lane_outdir}" >> "$CSV_PATH"

	total_lanes=$((total_lanes + 1))
	if [[ "$lane_result" == "pass" ]]; then
		pass_lanes=$((pass_lanes + 1))
	else
		fail_lanes=$((fail_lanes + 1))
	fi

	prune_models_cache "$lane_outdir"
}

run_lane "default-slot-lock-p4" 4 4 0 "none" 1 0
run_lane "kick-copy-single-p6-to5" 6 6 5 "single" 0 1
run_lane "kick-copy-double-p6-to4" 6 6 4 "double" 0 1
run_lane "kick-copy-multi-p8-to4" 8 8 4 "multi" 0 1

overall_result="pass"
if (( fail_lanes > 0 )); then
	overall_result="fail"
fi

SUMMARY_FILE="$OUTDIR/summary.env"
{
	echo "RESULT=$overall_result"
	echo "OUTDIR=$OUTDIR"
	echo "APP=$APP"
	echo "DATADIR=$DATADIR"
	echo "TIMEOUT_SECONDS=$TIMEOUT_SECONDS"
	echo "PLAYER_COUNT_DELAY_SECONDS=$PLAYER_COUNT_DELAY_SECONDS"
	echo "TOTAL_LANES=$total_lanes"
	echo "PASS_LANES=$pass_lanes"
	echo "FAIL_LANES=$fail_lanes"
	echo "CSV_PATH=$CSV_PATH"
} > "$SUMMARY_FILE"

log "result=$overall_result pass=$pass_lanes fail=$fail_lanes total=$total_lanes"
log "csv=$CSV_PATH"
log "summary=$SUMMARY_FILE"

if [[ "$overall_result" != "pass" ]]; then
	exit 1
fi
