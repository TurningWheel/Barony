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
REQUESTED_SPLITSCREEN_PLAYERS=15
CAP_DELAY_SECONDS=2
CAP_VERIFY_DELAY_SECONDS=1
AUTO_ENTER_DELAY_SECONDS=3
OUTDIR=""
SEED_CONFIG_PATH="$HOME/.barony/config/config.json"
SEED_BOOKS_PATH="$HOME/.barony/books/compiled_books.json"

usage() {
	cat <<'USAGE'
Usage: run_splitscreen_cap_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --timeout <sec>               Timeout in seconds (default: 420).
  --requested-players <n>       Requested /splitscreen player count (default: 15).
  --cap-delay <sec>             Delay before issuing /splitscreen command sequence (default: 2).
  --cap-verify-delay <sec>      Delay before evaluating cap assertions after command (default: 1).
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

extract_latest_cap_metric() {
	local log_file="$1"
	local key="$2"
	if [[ ! -f "$log_file" ]]; then
		echo ""
		return
	fi
	local line
	line="$(rg -F "[SMOKE]: local-splitscreen cap status=" "$log_file" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	echo "$line" | awk -v key="$key" '
		{
			for (i = 1; i <= NF; ++i) {
				prefix = key "=";
				if (index($i, prefix) == 1) {
					value = substr($i, length(prefix) + 1);
					if (value ~ /^[0-9]+$/) {
						print value;
						exit;
					}
				}
			}
		}
	'
}

extract_latest_cap_status() {
	local log_file="$1"
	if [[ ! -f "$log_file" ]]; then
		echo ""
		return
	fi
	local line
	line="$(rg -F "[SMOKE]: local-splitscreen cap status=" "$log_file" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	echo "$line" | sed -nE 's/.*status=(ok|fail).*/\1/p'
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
		--requested-players)
			REQUESTED_SPLITSCREEN_PLAYERS="${2:-}"
			shift 2
			;;
		--cap-delay)
			CAP_DELAY_SECONDS="${2:-}"
			shift 2
			;;
		--cap-verify-delay)
			CAP_VERIFY_DELAY_SECONDS="${2:-}"
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
if ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$REQUESTED_SPLITSCREEN_PLAYERS"; then
	echo "--timeout and --requested-players must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$CAP_DELAY_SECONDS" || ! is_uint "$CAP_VERIFY_DELAY_SECONDS" || ! is_uint "$AUTO_ENTER_DELAY_SECONDS"; then
	echo "--cap-delay, --cap-verify-delay, and --auto-enter-delay must be non-negative integers" >&2
	exit 1
fi
if (( REQUESTED_SPLITSCREEN_PLAYERS < 2 || REQUESTED_SPLITSCREEN_PLAYERS > 15 )); then
	echo "--requested-players must be in 2..15" >&2
	exit 1
fi

EXPECTED_CAP="$EXPECTED_PLAYERS"
if (( REQUESTED_SPLITSCREEN_PLAYERS < EXPECTED_CAP )); then
	EXPECTED_CAP="$REQUESTED_SPLITSCREEN_PLAYERS"
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/splitscreen-cap-${timestamp}-r${REQUESTED_SPLITSCREEN_PLAYERS}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -rf "$OUTDIR/stdout" "$OUTDIR/instance"
rm -f "$OUTDIR/pid.txt" "$OUTDIR/summary.env" "$OUTDIR/splitscreen_cap_results.csv"

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
	"BARONY_SMOKE_AUTO_ENTER_DUNGEON=1"
	"BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS=$AUTO_ENTER_DELAY_SECONDS"
	"BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS=1"
	"BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN_CAP=1"
	"BARONY_SMOKE_AUTO_SPLITSCREEN_CAP_TARGET=$REQUESTED_SPLITSCREEN_PLAYERS"
	"BARONY_SMOKE_SPLITSCREEN_CAP_DELAY_SECS=$CAP_DELAY_SECONDS"
	"BARONY_SMOKE_SPLITSCREEN_CAP_VERIFY_DELAY_SECS=$CAP_VERIFY_DELAY_SECONDS"
)

app_args=(
	"-windowed"
	"-size=$WINDOW_SIZE"
)
if [[ -n "$DATADIR" ]]; then
	app_args+=("-datadir=$DATADIR")
fi

log "Launching local splitscreen cap lane"
env "${env_vars[@]}" "$APP" "${app_args[@]}" >"$STDOUT_LOG" 2>&1 &
pid="$!"
echo "$pid" > "$PID_FILE"
log "instance=1 role=local pid=$pid home=$HOME_DIR"

result="fail"
deadline=$((SECONDS + TIMEOUT_SECONDS))
cap_command_lines=0
cap_ok_lines=0
cap_fail_lines=0
auto_enter_transition_lines=0
mapgen_count=0
cap_status=""
cap_target=0
cap_value=0
cap_connected=0
cap_connected_local=0
cap_over_connected=0
cap_over_local=0
cap_over_splitscreen=0
cap_under_nonlocal=0

while (( SECONDS < deadline )); do
	cap_command_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen cap command issued")
	cap_ok_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen cap status=ok")
	cap_fail_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen cap status=fail")
	auto_enter_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: auto-entering dungeon transition")
	mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")

	cap_status="$(extract_latest_cap_status "$HOST_LOG")"
	cap_target="$(extract_latest_cap_metric "$HOST_LOG" "target")"
	cap_value="$(extract_latest_cap_metric "$HOST_LOG" "cap")"
	cap_connected="$(extract_latest_cap_metric "$HOST_LOG" "connected")"
	cap_connected_local="$(extract_latest_cap_metric "$HOST_LOG" "connected_local")"
	cap_over_connected="$(extract_latest_cap_metric "$HOST_LOG" "over_cap_connected")"
	cap_over_local="$(extract_latest_cap_metric "$HOST_LOG" "over_cap_local")"
	cap_over_splitscreen="$(extract_latest_cap_metric "$HOST_LOG" "over_cap_splitscreen")"
	cap_under_nonlocal="$(extract_latest_cap_metric "$HOST_LOG" "under_cap_nonlocal")"

	for key in cap_target cap_value cap_connected cap_connected_local cap_over_connected cap_over_local cap_over_splitscreen cap_under_nonlocal; do
		if [[ -z "${!key}" ]]; then
			printf -v "$key" "0"
		fi
	done

	if (( cap_command_lines >= 1 && cap_ok_lines >= 1 && cap_fail_lines == 0
		&& auto_enter_transition_lines >= 1 && mapgen_count >= 1 )) \
		&& [[ "$cap_status" == "ok" ]] \
		&& (( cap_target == REQUESTED_SPLITSCREEN_PLAYERS )) \
		&& (( cap_value == EXPECTED_CAP )) \
		&& (( cap_connected == EXPECTED_CAP )) \
		&& (( cap_connected_local == EXPECTED_CAP )) \
		&& (( cap_over_connected == 0 && cap_over_local == 0 && cap_over_splitscreen == 0 && cap_under_nonlocal == 0 )); then
		result="pass"
		break
	fi
	sleep 1
done

cap_command_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen cap command issued")
cap_ok_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen cap status=ok")
cap_fail_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: local-splitscreen cap status=fail")
auto_enter_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: auto-entering dungeon transition")
mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")
cap_status="$(extract_latest_cap_status "$HOST_LOG")"
cap_target="$(extract_latest_cap_metric "$HOST_LOG" "target")"
cap_value="$(extract_latest_cap_metric "$HOST_LOG" "cap")"
cap_connected="$(extract_latest_cap_metric "$HOST_LOG" "connected")"
cap_connected_local="$(extract_latest_cap_metric "$HOST_LOG" "connected_local")"
cap_over_connected="$(extract_latest_cap_metric "$HOST_LOG" "over_cap_connected")"
cap_over_local="$(extract_latest_cap_metric "$HOST_LOG" "over_cap_local")"
cap_over_splitscreen="$(extract_latest_cap_metric "$HOST_LOG" "over_cap_splitscreen")"
cap_under_nonlocal="$(extract_latest_cap_metric "$HOST_LOG" "under_cap_nonlocal")"
for key in cap_target cap_value cap_connected cap_connected_local cap_over_connected cap_over_local cap_over_splitscreen cap_under_nonlocal; do
	if [[ -z "${!key}" ]]; then
		printf -v "$key" "0"
	fi
done

if (( cap_command_lines < 1 || cap_ok_lines < 1 || cap_fail_lines > 0
	|| auto_enter_transition_lines < 1 || mapgen_count < 1 )) \
	|| [[ "$cap_status" != "ok" ]] \
	|| (( cap_target != REQUESTED_SPLITSCREEN_PLAYERS )) \
	|| (( cap_value != EXPECTED_CAP )) \
	|| (( cap_connected != EXPECTED_CAP )) \
	|| (( cap_connected_local != EXPECTED_CAP )) \
	|| (( cap_over_connected != 0 || cap_over_local != 0 || cap_over_splitscreen != 0 || cap_under_nonlocal != 0 )); then
	result="fail"
fi

CSV_PATH="$OUTDIR/splitscreen_cap_results.csv"
{
	echo "lane,requested_players,expected_cap,result,cap_status,cap_command_lines,cap_ok_lines,cap_fail_lines,cap_target,cap_value,cap_connected,cap_connected_local,cap_over_connected,cap_over_local,cap_over_splitscreen,cap_under_nonlocal,auto_enter_transition_lines,mapgen_count,outdir"
	echo "splitscreen-cap-r${REQUESTED_SPLITSCREEN_PLAYERS},${REQUESTED_SPLITSCREEN_PLAYERS},${EXPECTED_CAP},${result},${cap_status},${cap_command_lines},${cap_ok_lines},${cap_fail_lines},${cap_target},${cap_value},${cap_connected},${cap_connected_local},${cap_over_connected},${cap_over_local},${cap_over_splitscreen},${cap_under_nonlocal},${auto_enter_transition_lines},${mapgen_count},${OUTDIR}"
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
	echo "REQUESTED_SPLITSCREEN_PLAYERS=$REQUESTED_SPLITSCREEN_PLAYERS"
	echo "EXPECTED_CAP=$EXPECTED_CAP"
	echo "CAP_DELAY_SECONDS=$CAP_DELAY_SECONDS"
	echo "CAP_VERIFY_DELAY_SECONDS=$CAP_VERIFY_DELAY_SECONDS"
	echo "AUTO_ENTER_DELAY_SECONDS=$AUTO_ENTER_DELAY_SECONDS"
	echo "LOCAL_SPLITSCREEN_CAP_STATUS=$cap_status"
	echo "LOCAL_SPLITSCREEN_CAP_COMMAND_LINES=$cap_command_lines"
	echo "LOCAL_SPLITSCREEN_CAP_OK_LINES=$cap_ok_lines"
	echo "LOCAL_SPLITSCREEN_CAP_FAIL_LINES=$cap_fail_lines"
	echo "LOCAL_SPLITSCREEN_CAP_TARGET=$cap_target"
	echo "LOCAL_SPLITSCREEN_CAP_VALUE=$cap_value"
	echo "LOCAL_SPLITSCREEN_CAP_CONNECTED=$cap_connected"
	echo "LOCAL_SPLITSCREEN_CAP_CONNECTED_LOCAL=$cap_connected_local"
	echo "LOCAL_SPLITSCREEN_CAP_OVER_CONNECTED=$cap_over_connected"
	echo "LOCAL_SPLITSCREEN_CAP_OVER_LOCAL=$cap_over_local"
	echo "LOCAL_SPLITSCREEN_CAP_OVER_SPLITSCREEN=$cap_over_splitscreen"
	echo "LOCAL_SPLITSCREEN_CAP_UNDER_NONLOCAL=$cap_under_nonlocal"
	echo "AUTO_ENTER_TRANSITION_LINES=$auto_enter_transition_lines"
	echo "MAPGEN_COUNT=$mapgen_count"
	echo "HOST_LOG=$HOST_LOG"
	echo "STDOUT_LOG=$STDOUT_LOG"
	echo "PID_FILE=$PID_FILE"
	echo "CSV_PATH=$CSV_PATH"
} > "$SUMMARY_FILE"

prune_models_cache "$INSTANCE_ROOT"
log "result=$result requested=$REQUESTED_SPLITSCREEN_PLAYERS expectedCap=$EXPECTED_CAP capStatus=$cap_status mapgen=$mapgen_count"
log "summary=$SUMMARY_FILE"

if [[ "$result" != "pass" ]]; then
	exit 1
fi
