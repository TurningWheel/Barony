#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
INSTANCES=4
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=120
CONNECT_ADDRESS="127.0.0.1:57165"
EXPECTED_PLAYERS=""
AUTO_START=0
AUTO_START_DELAY_SECS=2
AUTO_ENTER_DUNGEON=0
AUTO_ENTER_DUNGEON_DELAY_SECS=3
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
MAPGEN_PLAYERS_OVERRIDE=""
HELO_CHUNK_TX_MODE="normal"
SEED=""
OUTDIR=""
REQUIRE_HELO=""
REQUIRE_MAPGEN=0
KEEP_RUNNING=0

usage() {
	cat <<'USAGE'
Usage: run_lan_helo_chunk_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --instances <n>               Number of game instances to launch.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches.
  --timeout <sec>               Max wait for pass/fail conditions.
  --connect-address <addr>      LAN host address for clients (default: 127.0.0.1:57165).
  --expected-players <n>        Host auto-start threshold (default: instances).
  --auto-start <0|1>            Host starts game when expected players connected.
  --auto-start-delay <sec>      Delay after expected players threshold.
  --auto-enter-dungeon <0|1>    Host forces first dungeon transition after all players load.
  --auto-enter-dungeon-delay <sec>
                                Delay before forcing dungeon entry.
  --force-chunk <0|1>           Enable BARONY_SMOKE_FORCE_HELO_CHUNK.
  --chunk-payload-max <n>       Smoke chunk payload cap (64..900).
  --mapgen-players-override <n> Smoke-only mapgen scaling player count override (1..16).
  --helo-chunk-tx-mode <mode>   HELO chunk send mode: normal, reverse, even-odd,
                                duplicate-first, drop-last, duplicate-conflict-first.
  --seed <value>                Optional seed string for host run.
  --require-helo <0|1>          Require HELO chunk/reassembly checks.
  --require-mapgen <0|1>        Require dungeon mapgen summary in host log.
  --outdir <path>               Artifact directory.
  --keep-running                Do not kill launched instances on exit.
  -h, --help                    Show this help.
USAGE
}

is_uint() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

log() {
	printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

count_fixed_lines() {
	local file="$1"
	local needle="$2"
	if [[ ! -f "$file" ]]; then
		echo 0
		return
	fi
	rg -F -c "$needle" "$file" 2>/dev/null || echo 0
}

extract_mapgen_metrics() {
	local host_log="$1"
	local line
	line="$(rg -F "successfully generated a dungeon with" "$host_log" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo "0 0 0 0 0 0"
		return
	fi
	if [[ "$line" =~ with[[:space:]]+([0-9]+)[[:space:]]+rooms,[[:space:]]+([0-9]+)[[:space:]]+monsters,[[:space:]]+([0-9]+)[[:space:]]+gold,[[:space:]]+([0-9]+)[[:space:]]+items,[[:space:]]+([0-9]+)[[:space:]]+decorations ]]; then
		echo "1 ${BASH_REMATCH[1]} ${BASH_REMATCH[2]} ${BASH_REMATCH[3]} ${BASH_REMATCH[4]} ${BASH_REMATCH[5]}"
	else
		echo "0 0 0 0 0 0"
	fi
}

detect_game_start() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo 0
		return
	fi
	if rg -F -q "Starting game, game seed:" "$host_log"; then
		echo 1
	else
		echo 0
	fi
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
		--timeout)
			TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--connect-address)
			CONNECT_ADDRESS="${2:-}"
			shift 2
			;;
		--expected-players)
			EXPECTED_PLAYERS="${2:-}"
			shift 2
			;;
		--auto-start)
			AUTO_START="${2:-}"
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
		--mapgen-players-override)
			MAPGEN_PLAYERS_OVERRIDE="${2:-}"
			shift 2
			;;
		--helo-chunk-tx-mode)
			HELO_CHUNK_TX_MODE="${2:-}"
			shift 2
			;;
		--seed)
			SEED="${2:-}"
			shift 2
			;;
		--require-helo)
			REQUIRE_HELO="${2:-}"
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
		--keep-running)
			KEEP_RUNNING=1
			shift
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
if ! is_uint "$INSTANCES" || (( INSTANCES < 1 || INSTANCES > 16 )); then
	echo "--instances must be 1..16 (got: $INSTANCES)" >&2
	exit 1
fi
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$TIMEOUT_SECONDS"; then
	echo "--stagger and --timeout must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$AUTO_START" || (( AUTO_START > 1 )); then
	echo "--auto-start must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_START_DELAY_SECS"; then
	echo "--auto-start-delay must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$AUTO_ENTER_DUNGEON" || (( AUTO_ENTER_DUNGEON > 1 )); then
	echo "--auto-enter-dungeon must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_ENTER_DUNGEON_DELAY_SECS"; then
	echo "--auto-enter-dungeon-delay must be a non-negative integer" >&2
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
if [[ -n "$MAPGEN_PLAYERS_OVERRIDE" ]]; then
	if ! is_uint "$MAPGEN_PLAYERS_OVERRIDE" || (( MAPGEN_PLAYERS_OVERRIDE < 1 || MAPGEN_PLAYERS_OVERRIDE > 16 )); then
		echo "--mapgen-players-override must be 1..16" >&2
		exit 1
	fi
fi
case "$HELO_CHUNK_TX_MODE" in
	normal|reverse|evenodd|even-odd|even_odd|duplicate-first|duplicate_first|drop-last|drop_last|duplicate-conflict-first|duplicate_conflict_first)
		;;
	*)
		echo "--helo-chunk-tx-mode must be one of: normal, reverse, even-odd, duplicate-first, drop-last, duplicate-conflict-first" >&2
		exit 1
		;;
esac
if [[ -z "$EXPECTED_PLAYERS" ]]; then
	EXPECTED_PLAYERS="$INSTANCES"
fi
if ! is_uint "$EXPECTED_PLAYERS" || (( EXPECTED_PLAYERS < 1 || EXPECTED_PLAYERS > 16 )); then
	echo "--expected-players must be 1..16" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/helo-${timestamp}-p${INSTANCES}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"

if [[ -z "$REQUIRE_HELO" ]]; then
	if (( INSTANCES > 1 )); then
		REQUIRE_HELO=1
	else
		REQUIRE_HELO=0
	fi
fi
if ! is_uint "$REQUIRE_HELO" || (( REQUIRE_HELO > 1 )); then
	echo "--require-helo must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_MAPGEN" || (( REQUIRE_MAPGEN > 1 )); then
	echo "--require-mapgen must be 0 or 1" >&2
	exit 1
fi

LOG_DIR="$OUTDIR/stdout"
INSTANCE_ROOT="$OUTDIR/instances"
PID_FILE="$OUTDIR/pids.txt"
mkdir -p "$LOG_DIR" "$INSTANCE_ROOT"
: > "$PID_FILE"

declare -a PIDS=()
cleanup() {
	if (( KEEP_RUNNING )); then
		log "--keep-running enabled; leaving instances alive"
		return
	fi
	for pid in "${PIDS[@]}"; do
		kill "$pid" 2>/dev/null || true
	done
	sleep 1
	for pid in "${PIDS[@]}"; do
		if kill -0 "$pid" 2>/dev/null; then
			kill -9 "$pid" 2>/dev/null || true
		fi
	done
}
trap cleanup EXIT

launch_instance() {
	local idx="$1"
	local role="$2"
	local home_dir="$INSTANCE_ROOT/home-${idx}"
	local stdout_log="$LOG_DIR/instance-${idx}.stdout.log"
	mkdir -p "$home_dir"

	local -a env_vars=(
		"HOME=$home_dir"
		"BARONY_SMOKE_AUTOPILOT=1"
		"BARONY_SMOKE_CONNECT_DELAY_SECS=2"
		"BARONY_SMOKE_RETRY_DELAY_SECS=3"
		"BARONY_SMOKE_FORCE_HELO_CHUNK=$FORCE_CHUNK"
		"BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
		"BARONY_SMOKE_HELO_CHUNK_TX_MODE=$HELO_CHUNK_TX_MODE"
	)

	if [[ "$role" == "host" ]]; then
		env_vars+=(
			"BARONY_SMOKE_ROLE=host"
			"BARONY_SMOKE_EXPECTED_PLAYERS=$EXPECTED_PLAYERS"
			"BARONY_SMOKE_AUTO_START=$AUTO_START"
			"BARONY_SMOKE_AUTO_START_DELAY_SECS=$AUTO_START_DELAY_SECS"
			"BARONY_SMOKE_AUTO_ENTER_DUNGEON=$AUTO_ENTER_DUNGEON"
			"BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS=$AUTO_ENTER_DUNGEON_DELAY_SECS"
		)
		if [[ -n "$MAPGEN_PLAYERS_OVERRIDE" ]]; then
			env_vars+=("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS=$MAPGEN_PLAYERS_OVERRIDE")
		fi
		if [[ -n "$SEED" ]]; then
			env_vars+=("BARONY_SMOKE_SEED=$SEED")
		fi
	else
		env_vars+=(
			"BARONY_SMOKE_ROLE=client"
			"BARONY_SMOKE_CONNECT_ADDRESS=$CONNECT_ADDRESS"
		)
	fi

	env "${env_vars[@]}" "$APP" -windowed -size="$WINDOW_SIZE" >"$stdout_log" 2>&1 &
	local pid="$!"
	PIDS+=("$pid")
	printf '%s %s %s %s\n' "$pid" "$idx" "$role" "$home_dir" >> "$PID_FILE"
	log "instance=$idx role=$role pid=$pid home=$home_dir"
}

log "Artifacts: $OUTDIR"
for ((i = 1; i <= INSTANCES; ++i)); do
	if (( i == 1 )); then
		launch_instance "$i" "host"
	else
		launch_instance "$i" "client"
	fi
	sleep "$STAGGER_SECONDS"
done

HOST_LOG="$INSTANCE_ROOT/home-1/.barony/log.txt"
EXPECTED_CLIENTS=$(( INSTANCES > 1 ? INSTANCES - 1 : 0 ))
EXPECTED_CHUNK_LINES="$EXPECTED_CLIENTS"
EXPECTED_REASSEMBLED_LINES="$EXPECTED_CLIENTS"

result="fail"
deadline=$((SECONDS + TIMEOUT_SECONDS))
host_chunk_lines=0
client_reassembled_lines=0

while (( SECONDS < deadline )); do
	host_chunk_lines=$(count_fixed_lines "$HOST_LOG" "sending chunked HELO:")

	client_reassembled_lines=0
	for ((i = 2; i <= INSTANCES; ++i)); do
		client_log="$INSTANCE_ROOT/home-${i}/.barony/log.txt"
		count=$(count_fixed_lines "$client_log" "HELO reassembled:")
		client_reassembled_lines=$((client_reassembled_lines + count))
	done

	mapgen_found=0
	if [[ -f "$HOST_LOG" ]] && rg -F -q "successfully generated a dungeon with" "$HOST_LOG"; then
		mapgen_found=1
	fi
	game_start_found=$(detect_game_start "$HOST_LOG")

	helo_ok=1
	if (( REQUIRE_HELO )); then
		if (( host_chunk_lines < EXPECTED_CHUNK_LINES || client_reassembled_lines < EXPECTED_REASSEMBLED_LINES )); then
			helo_ok=0
		fi
	fi
	mapgen_ok=1
	if (( REQUIRE_MAPGEN )) && (( mapgen_found == 0 )); then
		mapgen_ok=0
	fi

	if (( helo_ok && mapgen_ok )); then
		result="pass"
		break
	fi
	sleep 1
done

game_start_found=$(detect_game_start "$HOST_LOG")
read -r mapgen_found rooms monsters gold items decorations < <(extract_mapgen_metrics "$HOST_LOG")

SUMMARY_FILE="$OUTDIR/summary.env"
{
	echo "RESULT=$result"
	echo "OUTDIR=$OUTDIR"
	echo "INSTANCES=$INSTANCES"
	echo "EXPECTED_PLAYERS=$EXPECTED_PLAYERS"
	echo "AUTO_ENTER_DUNGEON=$AUTO_ENTER_DUNGEON"
	echo "AUTO_ENTER_DUNGEON_DELAY_SECS=$AUTO_ENTER_DUNGEON_DELAY_SECS"
	echo "CONNECT_ADDRESS=$CONNECT_ADDRESS"
	echo "FORCE_CHUNK=$FORCE_CHUNK"
	echo "CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
	echo "MAPGEN_PLAYERS_OVERRIDE=$MAPGEN_PLAYERS_OVERRIDE"
	echo "HELO_CHUNK_TX_MODE=$HELO_CHUNK_TX_MODE"
	echo "HOST_CHUNK_LINES=$host_chunk_lines"
	echo "CLIENT_REASSEMBLED_LINES=$client_reassembled_lines"
	echo "EXPECTED_CHUNK_LINES=$EXPECTED_CHUNK_LINES"
	echo "EXPECTED_REASSEMBLED_LINES=$EXPECTED_REASSEMBLED_LINES"
	echo "MAPGEN_FOUND=$mapgen_found"
	echo "GAMESTART_FOUND=$game_start_found"
	echo "MAPGEN_ROOMS=$rooms"
	echo "MAPGEN_MONSTERS=$monsters"
	echo "MAPGEN_GOLD=$gold"
	echo "MAPGEN_ITEMS=$items"
	echo "MAPGEN_DECORATIONS=$decorations"
	echo "PID_FILE=$PID_FILE"
} > "$SUMMARY_FILE"

log "result=$result chunks=$host_chunk_lines reassembled=$client_reassembled_lines mapgen=$mapgen_found gamestart=$game_start_found"
log "summary=$SUMMARY_FILE"

if [[ "$result" != "pass" ]]; then
	exit 1
fi
