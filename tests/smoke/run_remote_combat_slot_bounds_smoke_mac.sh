#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=480
INSTANCES=15
PAUSE_PULSES=2
PAUSE_DELAY_SECONDS=2
PAUSE_HOLD_SECONDS=1
COMBAT_PULSES=3
COMBAT_DELAY_SECONDS=2
OUTDIR=""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HELO_RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"

usage() {
	cat <<'USAGE'
Usage: run_remote_combat_slot_bounds_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches (default: 1).
  --timeout <sec>               Timeout for lane in seconds (default: 480).
  --instances <n>               Player count for lane (default: 15, range: 3..15).
  --pause-pulses <n>            Host pause/unpause pulse count (default: 2).
  --pause-delay <sec>           Delay before pause and between pulses (default: 2).
  --pause-hold <sec>            Pause hold duration before unpause (default: 1).
  --combat-pulses <n>           Host enemy-bar pulse count (default: 3).
  --combat-delay <sec>          Delay between enemy-bar pulses (default: 2).
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
		--instances)
			INSTANCES="${2:-}"
			shift 2
			;;
		--pause-pulses)
			PAUSE_PULSES="${2:-}"
			shift 2
			;;
		--pause-delay)
			PAUSE_DELAY_SECONDS="${2:-}"
			shift 2
			;;
		--pause-hold)
			PAUSE_HOLD_SECONDS="${2:-}"
			shift 2
			;;
		--combat-pulses)
			COMBAT_PULSES="${2:-}"
			shift 2
			;;
		--combat-delay)
			COMBAT_DELAY_SECONDS="${2:-}"
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
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$TIMEOUT_SECONDS"; then
	echo "--stagger and --timeout must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$INSTANCES" || (( INSTANCES < 3 || INSTANCES > 15 )); then
	echo "--instances must be 3..15" >&2
	exit 1
fi
if ! is_uint "$PAUSE_PULSES" || ! is_uint "$PAUSE_DELAY_SECONDS" || ! is_uint "$PAUSE_HOLD_SECONDS"; then
	echo "--pause-pulses, --pause-delay, and --pause-hold must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$COMBAT_PULSES" || ! is_uint "$COMBAT_DELAY_SECONDS"; then
	echo "--combat-pulses and --combat-delay must be non-negative integers" >&2
	exit 1
fi
if (( PAUSE_PULSES > 64 || COMBAT_PULSES > 64 )); then
	echo "--pause-pulses and --combat-pulses must be <= 64" >&2
	exit 1
fi
if (( PAUSE_PULSES == 0 && COMBAT_PULSES == 0 )); then
	echo "At least one of --pause-pulses or --combat-pulses must be > 0" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/remote-combat-slot-bounds-${timestamp}-p${INSTANCES}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -rf "$OUTDIR/lane-remote-combat"
rm -f "$OUTDIR/remote_combat_results.csv" "$OUTDIR/summary.env"

lane_name="remote-combat-p${INSTANCES}"
lane_outdir="$OUTDIR/lane-remote-combat"
csv_path="$OUTDIR/remote_combat_results.csv"

{
	echo "lane,instances,result,child_result,remote_combat_slot_bounds_ok,remote_combat_events_ok,remote_combat_slot_ok_lines,remote_combat_slot_fail_lines,remote_combat_event_lines,remote_combat_event_contexts,remote_combat_auto_pause_action_lines,remote_combat_auto_pause_complete_lines,remote_combat_auto_enemy_bar_lines,remote_combat_auto_enemy_complete_lines,outdir"
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
	--auto-start 1
	--auto-start-delay 2
	--trace-remote-combat-slot-bounds 1
	--require-remote-combat-slot-bounds 1
	--require-remote-combat-events 1
	--auto-pause-pulses "$PAUSE_PULSES"
	--auto-pause-delay "$PAUSE_DELAY_SECONDS"
	--auto-pause-hold "$PAUSE_HOLD_SECONDS"
	--auto-remote-combat-pulses "$COMBAT_PULSES"
	--auto-remote-combat-delay "$COMBAT_DELAY_SECONDS"
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
remote_combat_slot_bounds_ok="$(read_summary_value "$summary_file" "REMOTE_COMBAT_SLOT_BOUNDS_OK")"
remote_combat_events_ok="$(read_summary_value "$summary_file" "REMOTE_COMBAT_EVENTS_OK")"
remote_combat_slot_ok_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_SLOT_OK_LINES")"
remote_combat_slot_fail_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_SLOT_FAIL_LINES")"
remote_combat_event_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_EVENT_LINES")"
remote_combat_event_contexts="$(read_summary_value "$summary_file" "REMOTE_COMBAT_EVENT_CONTEXTS")"
remote_combat_auto_pause_action_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_AUTO_PAUSE_ACTION_LINES")"
remote_combat_auto_pause_complete_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_AUTO_PAUSE_COMPLETE_LINES")"
remote_combat_auto_enemy_bar_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_AUTO_ENEMY_BAR_LINES")"
remote_combat_auto_enemy_complete_lines="$(read_summary_value "$summary_file" "REMOTE_COMBAT_AUTO_ENEMY_COMPLETE_LINES")"

for key in \
	remote_combat_slot_bounds_ok remote_combat_events_ok remote_combat_slot_ok_lines \
	remote_combat_slot_fail_lines remote_combat_event_lines \
	remote_combat_auto_pause_action_lines remote_combat_auto_pause_complete_lines \
	remote_combat_auto_enemy_bar_lines remote_combat_auto_enemy_complete_lines; do
	if [[ -z "${!key}" ]]; then
		printf -v "$key" "0"
	fi
done

lane_result="pass"
if [[ "$child_result" != "pass" || "$remote_combat_slot_bounds_ok" != "1" || "$remote_combat_events_ok" != "1" ]]; then
	lane_result="fail"
fi

echo "${lane_name},${INSTANCES},${lane_result},${child_result},${remote_combat_slot_bounds_ok},${remote_combat_events_ok},${remote_combat_slot_ok_lines},${remote_combat_slot_fail_lines},${remote_combat_event_lines},${remote_combat_event_contexts},${remote_combat_auto_pause_action_lines},${remote_combat_auto_pause_complete_lines},${remote_combat_auto_enemy_bar_lines},${remote_combat_auto_enemy_complete_lines},${lane_outdir}" >> "$csv_path"

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
	echo "PAUSE_PULSES=$PAUSE_PULSES"
	echo "PAUSE_DELAY_SECONDS=$PAUSE_DELAY_SECONDS"
	echo "PAUSE_HOLD_SECONDS=$PAUSE_HOLD_SECONDS"
	echo "COMBAT_PULSES=$COMBAT_PULSES"
	echo "COMBAT_DELAY_SECONDS=$COMBAT_DELAY_SECONDS"
	echo "PASS_LANES=$([[ "$lane_result" == "pass" ]] && echo 1 || echo 0)"
	echo "FAIL_LANES=$([[ "$lane_result" == "fail" ]] && echo 1 || echo 0)"
	echo "CSV_PATH=$csv_path"
	echo "LANE_OUTDIR=$lane_outdir"
} > "$summary_path"

log "result=$overall_result lane=$lane_result slotBoundsOk=$remote_combat_slot_bounds_ok eventsOk=$remote_combat_events_ok slotFail=$remote_combat_slot_fail_lines"
log "csv=$csv_path"
log "summary=$summary_path"

if [[ "$overall_result" != "pass" ]]; then
	exit 1
fi
