#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
TIMEOUT_SECONDS=420
EXPECTED_PLAYERS=4
PAUSE_PULSES=2
PAUSE_DELAY_SECONDS=2
PAUSE_HOLD_SECONDS=1
AUTO_ENTER_DELAY_SECONDS=3
OUTDIR=""
SEED_CONFIG_PATH="$HOME/.barony/config/config.json"
SEED_BOOKS_PATH="$HOME/.barony/books/compiled_books.json"

usage() {
	cat <<'USAGE'
Usage: run_splitscreen_baseline_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --timeout <sec>               Timeout in seconds (default: 420).
  --pause-pulses <n>            Local pause/unpause pulse count (default: 2).
  --pause-delay <sec>           Delay before pause and between pulses (default: 2).
  --pause-hold <sec>            Pause hold duration before unpause (default: 1).
  --auto-enter-delay <sec>      Delay before smoke auto-enter dungeon transition (default: 3).
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

count_fixed_lines() {
	smoke_count_fixed_lines "$1" "$2"
}

seed_smoke_home_profile() {
	local home_dir="$1"
	local seed_root="$home_dir/.barony"

	if [[ -f "$SEED_CONFIG_PATH" ]]; then
		mkdir -p "$seed_root/config"
		local config_dest="$seed_root/config/config.json"
		if command -v jq >/dev/null 2>&1; then
			if ! jq '.skipintro = true | .mods = []' "$SEED_CONFIG_PATH" > "$config_dest" 2>/dev/null; then
				cp "$SEED_CONFIG_PATH" "$config_dest"
			fi
		else
			cp "$SEED_CONFIG_PATH" "$config_dest"
		fi
	fi

	if [[ -f "$SEED_BOOKS_PATH" ]]; then
		mkdir -p "$seed_root/books"
		cp "$SEED_BOOKS_PATH" "$seed_root/books/compiled_books.json"
	fi
}

extract_latest_lobby_metric() {
	local log_file="$1"
	local key="$2"
	if [[ ! -f "$log_file" ]]; then
		echo ""
		return
	fi
	local line
	line="$(rg -F "[SMOKE]: local-splitscreen lobby context=autopilot " "$log_file" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	echo "$line" | sed -nE "s/.*${key}=([0-9]+).*/\\1/p"
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
		--timeout)
			TIMEOUT_SECONDS="${2:-}"
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
		--auto-enter-delay)
			AUTO_ENTER_DELAY_SECONDS="${2:-}"
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
if ! is_uint "$TIMEOUT_SECONDS"; then
	echo "--timeout must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$PAUSE_PULSES" || ! is_uint "$PAUSE_DELAY_SECONDS" || ! is_uint "$PAUSE_HOLD_SECONDS"; then
	echo "--pause-pulses, --pause-delay, and --pause-hold must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$AUTO_ENTER_DELAY_SECONDS"; then
	echo "--auto-enter-delay must be a non-negative integer" >&2
	exit 1
fi
if (( PAUSE_PULSES > 64 )); then
	echo "--pause-pulses must be <= 64" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/splitscreen-baseline-${timestamp}-p${EXPECTED_PLAYERS}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -rf "$OUTDIR/stdout" "$OUTDIR/instance"
rm -f "$OUTDIR/pid.txt" "$OUTDIR/summary.env" "$OUTDIR/splitscreen_results.csv"

LOG_DIR="$OUTDIR/stdout"
INSTANCE_ROOT="$OUTDIR/instance"
HOME_DIR="$INSTANCE_ROOT/home-1"
STDOUT_LOG="$LOG_DIR/instance-1.stdout.log"
HOST_LOG="$HOME_DIR/.barony/log.txt"
PID_FILE="$OUTDIR/pid.txt"
mkdir -p "$LOG_DIR" "$INSTANCE_ROOT" "$HOME_DIR"
seed_smoke_home_profile "$HOME_DIR"

pid=""
cleanup() {
	if [[ -n "$pid" ]]; then
		kill "$pid" 2>/dev/null || true
		sleep 1
		if kill -0 "$pid" 2>/dev/null; then
			kill -9 "$pid" 2>/dev/null || true
		fi
	fi
	prune_models_cache "$INSTANCE_ROOT"
}
trap cleanup EXIT

env_vars=(
	"HOME=$HOME_DIR"
	"BARONY_SMOKE_AUTOPILOT=1"
	"BARONY_SMOKE_ROLE=local"
	"BARONY_SMOKE_EXPECTED_PLAYERS=$EXPECTED_PLAYERS"
	"BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN=1"
	"BARONY_SMOKE_AUTO_ENTER_DUNGEON=1"
	"BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS=$AUTO_ENTER_DELAY_SECONDS"
	"BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS=1"
	"BARONY_SMOKE_LOCAL_PAUSE_PULSES=$PAUSE_PULSES"
	"BARONY_SMOKE_LOCAL_PAUSE_DELAY_SECS=$PAUSE_DELAY_SECONDS"
	"BARONY_SMOKE_LOCAL_PAUSE_HOLD_SECS=$PAUSE_HOLD_SECONDS"
)

app_args=(
	"-windowed"
	"-size=$WINDOW_SIZE"
)
if [[ -n "$DATADIR" ]]; then
	app_args+=("-datadir=$DATADIR")
fi

log "Launching local splitscreen baseline lane"
env "${env_vars[@]}" "$APP" "${app_args[@]}" >"$STDOUT_LOG" 2>&1 &
pid="$!"
echo "$pid" > "$PID_FILE"
log "instance=1 role=local pid=$pid home=$HOME_DIR"

result="fail"
deadline=$((SECONDS + TIMEOUT_SECONDS))
lobby_snapshot_lines=0
lobby_ready_ok=0
lobby_target=0
lobby_joined=0
lobby_ready=0
lobby_countdown=0
baseline_ok_lines=0
baseline_wait_lines=0
pause_action_lines=0
pause_complete_lines=0
auto_enter_transition_lines=0
splitscreen_transition_lines=0
mapgen_count=0

while (( SECONDS < deadline )); do
	lobby_snapshot_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen lobby context=autopilot ")
	baseline_ok_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen baseline status=ok")
	baseline_wait_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen baseline status=wait")
	pause_action_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen auto-pause action=")
	pause_complete_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen auto-pause complete pulses=")
	auto_enter_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: auto-entering dungeon transition")
	splitscreen_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen transition level=")
	mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")

	lobby_target="$(extract_latest_lobby_metric "$HOST_LOG" "target")"
	lobby_joined="$(extract_latest_lobby_metric "$HOST_LOG" "joined")"
	lobby_ready="$(extract_latest_lobby_metric "$HOST_LOG" "ready")"
	lobby_countdown="$(extract_latest_lobby_metric "$HOST_LOG" "countdown")"
	for key in lobby_target lobby_joined lobby_ready lobby_countdown; do
		if [[ -z "${!key}" ]]; then
			printf -v "$key" "0"
		fi
	done

	lobby_ready_ok=0
	if (( lobby_target >= EXPECTED_PLAYERS && lobby_joined >= EXPECTED_PLAYERS && lobby_ready >= EXPECTED_PLAYERS )); then
		lobby_ready_ok=1
	fi

	pause_ok=1
	if (( PAUSE_PULSES > 0 )); then
		if (( pause_action_lines < PAUSE_PULSES * 2 || pause_complete_lines < 1 )); then
			pause_ok=0
		fi
	fi

	if (( lobby_ready_ok == 1 && baseline_ok_lines >= 1 && pause_ok == 1
		&& auto_enter_transition_lines >= 1 && splitscreen_transition_lines >= 1 && mapgen_count >= 1 )); then
		result="pass"
		break
	fi
	sleep 1
done

lobby_snapshot_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen lobby context=autopilot ")
baseline_ok_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen baseline status=ok")
baseline_wait_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen baseline status=wait")
pause_action_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen auto-pause action=")
pause_complete_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen auto-pause complete pulses=")
auto_enter_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: auto-entering dungeon transition")
splitscreen_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen transition level=")
mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")
lobby_target="$(extract_latest_lobby_metric "$HOST_LOG" "target")"
lobby_joined="$(extract_latest_lobby_metric "$HOST_LOG" "joined")"
lobby_ready="$(extract_latest_lobby_metric "$HOST_LOG" "ready")"
lobby_countdown="$(extract_latest_lobby_metric "$HOST_LOG" "countdown")"
for key in lobby_target lobby_joined lobby_ready lobby_countdown; do
	if [[ -z "${!key}" ]]; then
		printf -v "$key" "0"
	fi
done
lobby_ready_ok=0
if (( lobby_target >= EXPECTED_PLAYERS && lobby_joined >= EXPECTED_PLAYERS && lobby_ready >= EXPECTED_PLAYERS )); then
	lobby_ready_ok=1
fi

pause_ok=1
if (( PAUSE_PULSES > 0 )); then
	if (( pause_action_lines < PAUSE_PULSES * 2 || pause_complete_lines < 1 )); then
		pause_ok=0
	fi
fi
if (( lobby_ready_ok == 0 || baseline_ok_lines < 1 || pause_ok == 0
	|| auto_enter_transition_lines < 1 || splitscreen_transition_lines < 1 || mapgen_count < 1 )); then
	result="fail"
fi

CSV_PATH="$OUTDIR/splitscreen_results.csv"
{
	echo "lane,expected_players,result,lobby_ready_ok,lobby_target,lobby_joined,lobby_ready,baseline_ok_lines,baseline_wait_lines,pause_action_lines,pause_complete_lines,auto_enter_transition_lines,splitscreen_transition_lines,mapgen_count,outdir"
	echo "splitscreen-baseline-p${EXPECTED_PLAYERS},${EXPECTED_PLAYERS},${result},${lobby_ready_ok},${lobby_target},${lobby_joined},${lobby_ready},${baseline_ok_lines},${baseline_wait_lines},${pause_action_lines},${pause_complete_lines},${auto_enter_transition_lines},${splitscreen_transition_lines},${mapgen_count},${OUTDIR}"
} > "$CSV_PATH"

SUMMARY_FILE="$OUTDIR/summary.env"
{
	echo "RESULT=$result"
	echo "OUTDIR=$OUTDIR"
	echo "APP=$APP"
	echo "DATADIR=$DATADIR"
	echo "WINDOW_SIZE=$WINDOW_SIZE"
	echo "TIMEOUT_SECONDS=$TIMEOUT_SECONDS"
	echo "EXPECTED_PLAYERS=$EXPECTED_PLAYERS"
	echo "PAUSE_PULSES=$PAUSE_PULSES"
	echo "PAUSE_DELAY_SECONDS=$PAUSE_DELAY_SECONDS"
	echo "PAUSE_HOLD_SECONDS=$PAUSE_HOLD_SECONDS"
	echo "AUTO_ENTER_DELAY_SECONDS=$AUTO_ENTER_DELAY_SECONDS"
	echo "LOBBY_READY_OK=$lobby_ready_ok"
	echo "LOBBY_SNAPSHOT_LINES=$lobby_snapshot_lines"
	echo "LOBBY_TARGET=$lobby_target"
	echo "LOBBY_JOINED=$lobby_joined"
	echo "LOBBY_READY=$lobby_ready"
	echo "LOBBY_COUNTDOWN=$lobby_countdown"
	echo "LOCAL_SPLITSCREEN_BASELINE_OK_LINES=$baseline_ok_lines"
	echo "LOCAL_SPLITSCREEN_BASELINE_WAIT_LINES=$baseline_wait_lines"
	echo "LOCAL_SPLITSCREEN_PAUSE_ACTION_LINES=$pause_action_lines"
	echo "LOCAL_SPLITSCREEN_PAUSE_COMPLETE_LINES=$pause_complete_lines"
	echo "AUTO_ENTER_TRANSITION_LINES=$auto_enter_transition_lines"
	echo "LOCAL_SPLITSCREEN_TRANSITION_LINES=$splitscreen_transition_lines"
	echo "MAPGEN_COUNT=$mapgen_count"
	echo "HOST_LOG=$HOST_LOG"
	echo "STDOUT_LOG=$STDOUT_LOG"
	echo "PID_FILE=$PID_FILE"
} > "$SUMMARY_FILE"

prune_models_cache "$INSTANCE_ROOT"
log "result=$result lobbyReady=$lobby_ready_ok baselineOk=$baseline_ok_lines pauseActions=$pause_action_lines transitions=$splitscreen_transition_lines mapgen=$mapgen_count"
log "summary=$SUMMARY_FILE"

if [[ "$result" != "pass" ]]; then
	exit 1
fi
