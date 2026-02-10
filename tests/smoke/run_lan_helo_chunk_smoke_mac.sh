#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
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
AUTO_ENTER_DUNGEON_REPEATS=""
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
MAPGEN_PLAYERS_OVERRIDE=""
HELO_CHUNK_TX_MODE="normal"
NETWORK_BACKEND="lan"
STRICT_ADVERSARIAL=0
REQUIRE_TXMODE_LOG=0
SEED=""
OUTDIR=""
REQUIRE_HELO=""
REQUIRE_MAPGEN=0
MAPGEN_SAMPLES=1
TRACE_ACCOUNT_LABELS=0
REQUIRE_ACCOUNT_LABELS=0
KEEP_RUNNING=0

usage() {
	cat <<'USAGE'
Usage: run_lan_helo_chunk_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed to Barony via -datadir=<path>.
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
  --auto-enter-dungeon-repeats <n>
                                Max smoke-driven dungeon transitions (host only).
                                Defaults to --mapgen-samples.
  --force-chunk <0|1>           Enable BARONY_SMOKE_FORCE_HELO_CHUNK.
  --chunk-payload-max <n>       Smoke chunk payload cap (64..900).
  --mapgen-players-override <n> Smoke-only mapgen scaling player count override (1..15).
  --helo-chunk-tx-mode <mode>   HELO chunk send mode: normal, reverse, even-odd,
                                duplicate-first, drop-last, duplicate-conflict-first.
  --network-backend <name>      Backend tag for summary/env (lan|steam|eos; default: lan).
  --strict-adversarial <0|1>    Enable strict adversarial assertions.
  --require-txmode-log <0|1>    Require tx-mode host logs in non-normal tx modes.
  --seed <value>                Optional seed string for host run.
  --require-helo <0|1>          Require HELO chunk/reassembly checks.
  --require-mapgen <0|1>        Require dungeon mapgen summary in host log.
  --mapgen-samples <n>          Required number of mapgen summary lines (default: 1).
  --trace-account-labels <0|1>  Emit smoke logs for resolved lobby account labels (host only).
  --require-account-labels <0|1>
                                Require account-label coverage for remote slots.
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

canonicalize_tx_mode() {
	local mode="$1"
	case "$mode" in
		normal)
			echo "normal"
			;;
		reverse)
			echo "reverse"
			;;
		evenodd|even-odd|even_odd)
			echo "even-odd"
			;;
		duplicate-first|duplicate_first)
			echo "duplicate-first"
			;;
		drop-last|drop_last)
			echo "drop-last"
			;;
		duplicate-conflict-first|duplicate_conflict_first)
			echo "duplicate-conflict-first"
			;;
		*)
			return 1
			;;
	esac
}

is_expected_fail_tx_mode() {
	local mode="$1"
	case "$mode" in
		drop-last|duplicate-conflict-first)
			echo 1
			;;
		*)
			echo 0
			;;
	esac
}

tx_mode_packet_plan_valid() {
	local host_log="$1"
	local mode="$2"
	if [[ "$mode" == "normal" ]]; then
		echo 1
		return
	fi
	if [[ ! -f "$host_log" ]]; then
		echo 0
		return
	fi
	local lines
	lines="$(rg -F "[SMOKE]: HELO chunk tx mode=$mode " "$host_log" || true)"
	if [[ -z "$lines" ]]; then
		echo 0
		return
	fi
	local ok=1
	while IFS= read -r line; do
		[[ -z "$line" ]] && continue
		local packets chunks
		packets="$(echo "$line" | sed -nE 's/.*packets=([0-9]+).*/\1/p')"
		chunks="$(echo "$line" | sed -nE 's/.*chunks=([0-9]+).*/\1/p')"
		if [[ -z "$packets" || -z "$chunks" ]]; then
			ok=0
			break
		fi
		if [[ "$mode" == "reverse" || "$mode" == "even-odd" ]]; then
			if (( packets != chunks )); then
				ok=0
				break
			fi
		elif [[ "$mode" == "duplicate-first" || "$mode" == "duplicate-conflict-first" ]]; then
			if (( packets != chunks + 1 )); then
				ok=0
				break
			fi
		elif [[ "$mode" == "drop-last" ]]; then
			if (( packets + 1 != chunks )); then
				ok=0
				break
			fi
		fi
	done <<< "$lines"
	echo "$ok"
}

collect_chunk_reset_reason_counts() {
	if (($# == 0)); then
		echo ""
		return
	fi
	local -a existing_files=()
	local path
	for path in "$@"; do
		if [[ -f "$path" ]]; then
			existing_files+=("$path")
		fi
	done
	if ((${#existing_files[@]} == 0)); then
		echo ""
		return
	fi
	local all_lines=""
	local file line reason
	for file in "${existing_files[@]}"; do
		while IFS= read -r line; do
			[[ -z "$line" ]] && continue
			reason="$(echo "$line" | sed -nE 's/.*\(([^)]*)\).*/\1/p')"
			if [[ -z "$reason" ]]; then
				reason="unknown"
			fi
			all_lines+="${reason}"$'\n'
		done < <(rg -F "HELO chunk timeout/reset transfer=" "$file" || true)
	done
	if [[ -z "$all_lines" ]]; then
		echo ""
		return
	fi
	local reason_lines
	reason_lines="$(printf '%s' "$all_lines" | sed '/^$/d' | sort | uniq -c | awk '{ count=$1; $1=""; sub(/^ /, ""); printf "%s:%s\n", $0, count }')"
	if [[ -z "$reason_lines" ]]; then
		echo ""
		return
	fi
	echo "$reason_lines" | paste -sd';' -
}

collect_helo_player_slots() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	local slots
	slots="$(rg -o 'sending chunked HELO: player=[0-9]+' "$host_log" \
		| sed -nE 's/.*player=([0-9]+)/\1/p' \
		| sort -n \
		| uniq \
		| paste -sd';' - || true)"
	echo "$slots"
}

collect_missing_helo_player_slots() {
	local host_log="$1"
	local expected_clients="$2"
	if (( expected_clients <= 0 )); then
		echo ""
		return
	fi
	local missing=""
	local player
	for ((player = 1; player <= expected_clients; ++player)); do
		if ! rg -F -q "sending chunked HELO: player=$player " "$host_log"; then
			if [[ -n "$missing" ]]; then
				missing+=";"
			fi
			missing+="$player"
		fi
	done
	echo "$missing"
}

is_helo_player_slot_coverage_ok() {
	local host_log="$1"
	local expected_clients="$2"
	if (( expected_clients <= 0 )); then
		echo 1
		return
	fi
	if [[ ! -f "$host_log" ]]; then
		echo 0
		return
	fi
	local player
	for ((player = 1; player <= expected_clients; ++player)); do
		if ! rg -F -q "sending chunked HELO: player=$player " "$host_log"; then
			echo 0
			return
		fi
	done
	echo 1
}

collect_account_label_slots() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	local slots
	slots="$(rg -o 'lobby account label resolved slot=[0-9]+' "$host_log" \
		| sed -nE 's/.*slot=([0-9]+)/\1/p' \
		| sort -n \
		| uniq \
		| paste -sd';' - || true)"
	echo "$slots"
}

collect_missing_account_label_slots() {
	local host_log="$1"
	local expected_clients="$2"
	if (( expected_clients <= 0 )); then
		echo ""
		return
	fi
	local missing=""
	local slot
	for ((slot = 1; slot <= expected_clients; ++slot)); do
		if ! rg -F -q "lobby account label resolved slot=$slot " "$host_log"; then
			if [[ -n "$missing" ]]; then
				missing+=";"
			fi
			missing+="$slot"
		fi
	done
	echo "$missing"
}

is_account_label_slot_coverage_ok() {
	local host_log="$1"
	local expected_clients="$2"
	if (( expected_clients <= 0 )); then
		echo 1
		return
	fi
	if [[ ! -f "$host_log" ]]; then
		echo 0
		return
	fi
	local slot
	for ((slot = 1; slot <= expected_clients; ++slot)); do
		if ! rg -F -q "lobby account label resolved slot=$slot " "$host_log"; then
			echo 0
			return
		fi
	done
	echo 1
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
		--datadir)
			DATADIR="${2:-}"
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
		--auto-enter-dungeon-repeats)
			AUTO_ENTER_DUNGEON_REPEATS="${2:-}"
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
		--network-backend)
			NETWORK_BACKEND="${2:-}"
			shift 2
			;;
		--strict-adversarial)
			STRICT_ADVERSARIAL="${2:-}"
			shift 2
			;;
		--require-txmode-log)
			REQUIRE_TXMODE_LOG="${2:-}"
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
		--mapgen-samples)
			MAPGEN_SAMPLES="${2:-}"
			shift 2
			;;
		--trace-account-labels)
			TRACE_ACCOUNT_LABELS="${2:-}"
			shift 2
			;;
		--require-account-labels)
			REQUIRE_ACCOUNT_LABELS="${2:-}"
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
if [[ -n "$DATADIR" ]] && [[ ! -d "$DATADIR" ]]; then
	echo "--datadir must reference an existing directory: $DATADIR" >&2
	exit 1
fi
if ! is_uint "$INSTANCES" || (( INSTANCES < 1 || INSTANCES > 15 )); then
	echo "--instances must be 1..15 (got: $INSTANCES)" >&2
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
if [[ -n "$AUTO_ENTER_DUNGEON_REPEATS" ]]; then
	if ! is_uint "$AUTO_ENTER_DUNGEON_REPEATS" || (( AUTO_ENTER_DUNGEON_REPEATS < 1 || AUTO_ENTER_DUNGEON_REPEATS > 256 )); then
		echo "--auto-enter-dungeon-repeats must be 1..256" >&2
		exit 1
	fi
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
	if ! is_uint "$MAPGEN_PLAYERS_OVERRIDE" || (( MAPGEN_PLAYERS_OVERRIDE < 1 || MAPGEN_PLAYERS_OVERRIDE > 15 )); then
		echo "--mapgen-players-override must be 1..15" >&2
		exit 1
	fi
fi
if ! HELO_CHUNK_TX_MODE="$(canonicalize_tx_mode "$HELO_CHUNK_TX_MODE")"; then
	echo "--helo-chunk-tx-mode must be one of: normal, reverse, even-odd, duplicate-first, drop-last, duplicate-conflict-first" >&2
	exit 1
fi
case "$NETWORK_BACKEND" in
	lan|steam|eos)
		;;
	*)
		echo "--network-backend must be one of: lan, steam, eos" >&2
		exit 1
		;;
esac
if [[ "$NETWORK_BACKEND" != "lan" ]]; then
	echo "--network-backend currently only supports lan in this runner" >&2
	exit 1
fi
if ! is_uint "$STRICT_ADVERSARIAL" || (( STRICT_ADVERSARIAL > 1 )); then
	echo "--strict-adversarial must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_TXMODE_LOG" || (( REQUIRE_TXMODE_LOG > 1 )); then
	echo "--require-txmode-log must be 0 or 1" >&2
	exit 1
fi
if [[ -z "$EXPECTED_PLAYERS" ]]; then
	EXPECTED_PLAYERS="$INSTANCES"
fi
if ! is_uint "$EXPECTED_PLAYERS" || (( EXPECTED_PLAYERS < 1 || EXPECTED_PLAYERS > 15 )); then
	echo "--expected-players must be 1..15" >&2
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
rm -rf "$OUTDIR/stdout" "$OUTDIR/instances"
rm -f "$OUTDIR/pids.txt" "$OUTDIR/summary.env"

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
if ! is_uint "$MAPGEN_SAMPLES" || (( MAPGEN_SAMPLES < 1 )); then
	echo "--mapgen-samples must be >= 1" >&2
	exit 1
fi
if ! is_uint "$TRACE_ACCOUNT_LABELS" || (( TRACE_ACCOUNT_LABELS > 1 )); then
	echo "--trace-account-labels must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_ACCOUNT_LABELS" || (( REQUIRE_ACCOUNT_LABELS > 1 )); then
	echo "--require-account-labels must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_ACCOUNT_LABELS )) && (( TRACE_ACCOUNT_LABELS == 0 )); then
	echo "--require-account-labels requires --trace-account-labels 1" >&2
	exit 1
fi
if [[ -z "$AUTO_ENTER_DUNGEON_REPEATS" ]]; then
	AUTO_ENTER_DUNGEON_REPEATS="$MAPGEN_SAMPLES"
fi

LOG_DIR="$OUTDIR/stdout"
INSTANCE_ROOT="$OUTDIR/instances"
PID_FILE="$OUTDIR/pids.txt"
mkdir -p "$LOG_DIR" "$INSTANCE_ROOT"
: > "$PID_FILE"

declare -a PIDS=()
declare -a HOME_DIRS=()
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
	for home_dir in "${HOME_DIRS[@]}"; do
		rm -f "$home_dir/.barony/models.cache" 2>/dev/null || true
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
				"BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS=$AUTO_ENTER_DUNGEON_REPEATS"
			)
		if (( TRACE_ACCOUNT_LABELS )); then
			env_vars+=("BARONY_SMOKE_TRACE_ACCOUNT_LABELS=1")
		fi
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

	local -a app_args=(
		"-windowed"
		"-size=$WINDOW_SIZE"
	)
	if [[ -n "$DATADIR" ]]; then
		app_args+=("-datadir=$DATADIR")
	fi

	env "${env_vars[@]}" "$APP" "${app_args[@]}" >"$stdout_log" 2>&1 &
	local pid="$!"
	PIDS+=("$pid")
	HOME_DIRS+=("$home_dir")
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
EXPECTED_FAIL_TX_MODE="$(is_expected_fail_tx_mode "$HELO_CHUNK_TX_MODE")"
TXMODE_REQUIRED=0
if (( REQUIRE_TXMODE_LOG )) && [[ "$HELO_CHUNK_TX_MODE" != "normal" ]]; then
	TXMODE_REQUIRED=1
fi
STRICT_EXPECTED_FAIL=0
if (( STRICT_ADVERSARIAL && REQUIRE_HELO && EXPECTED_FAIL_TX_MODE )); then
	STRICT_EXPECTED_FAIL=1
fi

result="fail"
deadline=$((SECONDS + TIMEOUT_SECONDS))
host_chunk_lines=0
client_reassembled_lines=0
chunk_reset_lines=0
tx_mode_log_lines=0
tx_mode_packet_plan_ok=0
tx_mode_applied=0
per_client_reassembly_counts=""
all_clients_exact_one=0
all_clients_zero=0
account_label_ok=1

declare -a CLIENT_LOGS=()
for ((i = 2; i <= INSTANCES; ++i)); do
	CLIENT_LOGS+=("$INSTANCE_ROOT/home-${i}/.barony/log.txt")
done

while (( SECONDS < deadline )); do
	host_chunk_lines=$(count_fixed_lines "$HOST_LOG" "sending chunked HELO:")
	if [[ "$HELO_CHUNK_TX_MODE" == "normal" ]]; then
		tx_mode_log_lines=0
		tx_mode_packet_plan_ok=1
		tx_mode_applied=0
	else
		tx_mode_log_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: HELO chunk tx mode=$HELO_CHUNK_TX_MODE")
		tx_mode_packet_plan_ok=$(tx_mode_packet_plan_valid "$HOST_LOG" "$HELO_CHUNK_TX_MODE")
		tx_mode_applied=0
		if (( tx_mode_log_lines > 0 )); then
			tx_mode_applied=1
		fi
	fi

	client_reassembled_lines=0
	chunk_reset_lines=0
	per_client_reassembly_counts=""
	all_clients_exact_one=1
	all_clients_zero=1
	for ((i = 2; i <= INSTANCES; ++i)); do
		client_log="$INSTANCE_ROOT/home-${i}/.barony/log.txt"
		count=$(count_fixed_lines "$client_log" "HELO reassembled:")
		client_reassembled_lines=$((client_reassembled_lines + count))
		reset_count=$(count_fixed_lines "$client_log" "HELO chunk timeout/reset")
		chunk_reset_lines=$((chunk_reset_lines + reset_count))
		if [[ -n "$per_client_reassembly_counts" ]]; then
			per_client_reassembly_counts+=";"
		fi
		per_client_reassembly_counts+="${i}:${count}"
		if (( count != 1 )); then
			all_clients_exact_one=0
		fi
		if (( count != 0 )); then
			all_clients_zero=0
		fi
	done

	mapgen_found=0
	mapgen_count=0
	if [[ -f "$HOST_LOG" ]]; then
		mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")
	fi
	if (( mapgen_count > 0 )); then
		mapgen_found=1
	fi
	game_start_found=$(detect_game_start "$HOST_LOG")

	helo_ok=1
	if (( REQUIRE_HELO )); then
		if (( STRICT_ADVERSARIAL )); then
			if (( EXPECTED_FAIL_TX_MODE )); then
				helo_ok=0
			else
				if (( host_chunk_lines < EXPECTED_CHUNK_LINES || all_clients_exact_one == 0 )); then
					helo_ok=0
				fi
			fi
		else
			if (( host_chunk_lines < EXPECTED_CHUNK_LINES || client_reassembled_lines < EXPECTED_REASSEMBLED_LINES )); then
				helo_ok=0
			fi
		fi
		if (( EXPECTED_CLIENTS > 0 )); then
			helo_player_slot_coverage_ok=$(is_helo_player_slot_coverage_ok "$HOST_LOG" "$EXPECTED_CLIENTS")
			if (( helo_player_slot_coverage_ok == 0 )); then
				helo_ok=0
			fi
		fi
	fi
	txmode_ok=1
	if (( TXMODE_REQUIRED )); then
		if (( tx_mode_log_lines < EXPECTED_CLIENTS || tx_mode_packet_plan_ok == 0 )); then
			txmode_ok=0
		fi
	fi
	mapgen_ok=1
	if (( REQUIRE_MAPGEN )) && (( mapgen_count < MAPGEN_SAMPLES )); then
		mapgen_ok=0
	fi
	account_label_ok=1
	if (( REQUIRE_ACCOUNT_LABELS )) && (( EXPECTED_CLIENTS > 0 )); then
		account_label_ok=$(is_account_label_slot_coverage_ok "$HOST_LOG" "$EXPECTED_CLIENTS")
	fi

	if (( STRICT_EXPECTED_FAIL )); then
		if (( all_clients_zero == 0 )); then
			break
		fi
		if (( chunk_reset_lines > 0 && txmode_ok )); then
			break
		fi
	elif (( helo_ok && mapgen_ok && txmode_ok && account_label_ok )); then
		result="pass"
		break
	fi
	sleep 1
done

game_start_found=$(detect_game_start "$HOST_LOG")
read -r mapgen_found rooms monsters gold items decorations < <(extract_mapgen_metrics "$HOST_LOG")
mapgen_count=0
if [[ -f "$HOST_LOG" ]]; then
	mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")
fi
helo_player_slots="$(collect_helo_player_slots "$HOST_LOG")"
helo_missing_player_slots="$(collect_missing_helo_player_slots "$HOST_LOG" "$EXPECTED_CLIENTS")"
helo_player_slot_coverage_ok="$(is_helo_player_slot_coverage_ok "$HOST_LOG" "$EXPECTED_CLIENTS")"
account_label_slots="$(collect_account_label_slots "$HOST_LOG")"
account_label_missing_slots="$(collect_missing_account_label_slots "$HOST_LOG" "$EXPECTED_CLIENTS")"
account_label_slot_coverage_ok="$(is_account_label_slot_coverage_ok "$HOST_LOG" "$EXPECTED_CLIENTS")"
account_label_lines=0
if [[ -f "$HOST_LOG" ]]; then
	account_label_lines=$(count_fixed_lines "$HOST_LOG" "lobby account label resolved slot=")
fi
chunk_reset_reason_counts=""
if (( EXPECTED_CLIENTS > 0 )); then
	chunk_reset_reason_counts="$(collect_chunk_reset_reason_counts "${CLIENT_LOGS[@]}")"
fi

if (( REQUIRE_ACCOUNT_LABELS )) && (( account_label_slot_coverage_ok == 0 )); then
	result="fail"
fi

SUMMARY_FILE="$OUTDIR/summary.env"
{
	echo "RESULT=$result"
	echo "OUTDIR=$OUTDIR"
	echo "DATADIR=$DATADIR"
	echo "INSTANCES=$INSTANCES"
	echo "EXPECTED_PLAYERS=$EXPECTED_PLAYERS"
	echo "AUTO_ENTER_DUNGEON=$AUTO_ENTER_DUNGEON"
	echo "AUTO_ENTER_DUNGEON_DELAY_SECS=$AUTO_ENTER_DUNGEON_DELAY_SECS"
	echo "AUTO_ENTER_DUNGEON_REPEATS=$AUTO_ENTER_DUNGEON_REPEATS"
	echo "CONNECT_ADDRESS=$CONNECT_ADDRESS"
	echo "NETWORK_BACKEND=$NETWORK_BACKEND"
	echo "FORCE_CHUNK=$FORCE_CHUNK"
	echo "CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
	echo "MAPGEN_PLAYERS_OVERRIDE=$MAPGEN_PLAYERS_OVERRIDE"
	echo "HELO_CHUNK_TX_MODE=$HELO_CHUNK_TX_MODE"
	echo "STRICT_ADVERSARIAL=$STRICT_ADVERSARIAL"
	echo "REQUIRE_TXMODE_LOG=$REQUIRE_TXMODE_LOG"
	echo "EXPECTED_FAIL_TX_MODE=$EXPECTED_FAIL_TX_MODE"
	echo "HOST_CHUNK_LINES=$host_chunk_lines"
	echo "CLIENT_REASSEMBLED_LINES=$client_reassembled_lines"
	echo "PER_CLIENT_REASSEMBLY_COUNTS=$per_client_reassembly_counts"
	echo "CHUNK_RESET_LINES=$chunk_reset_lines"
	echo "CHUNK_RESET_REASON_COUNTS=$chunk_reset_reason_counts"
	echo "TX_MODE_APPLIED=$tx_mode_applied"
	echo "TX_MODE_LOG_LINES=$tx_mode_log_lines"
	echo "TX_MODE_PACKET_PLAN_OK=$tx_mode_packet_plan_ok"
	echo "EXPECTED_CHUNK_LINES=$EXPECTED_CHUNK_LINES"
	echo "EXPECTED_REASSEMBLED_LINES=$EXPECTED_REASSEMBLED_LINES"
	echo "HELO_PLAYER_SLOTS=$helo_player_slots"
	echo "HELO_MISSING_PLAYER_SLOTS=$helo_missing_player_slots"
	echo "HELO_PLAYER_SLOT_COVERAGE_OK=$helo_player_slot_coverage_ok"
	echo "MAPGEN_FOUND=$mapgen_found"
	echo "MAPGEN_COUNT=$mapgen_count"
	echo "MAPGEN_SAMPLES_REQUESTED=$MAPGEN_SAMPLES"
	echo "GAMESTART_FOUND=$game_start_found"
	echo "MAPGEN_ROOMS=$rooms"
	echo "MAPGEN_MONSTERS=$monsters"
	echo "MAPGEN_GOLD=$gold"
	echo "MAPGEN_ITEMS=$items"
	echo "MAPGEN_DECORATIONS=$decorations"
	echo "TRACE_ACCOUNT_LABELS=$TRACE_ACCOUNT_LABELS"
	echo "REQUIRE_ACCOUNT_LABELS=$REQUIRE_ACCOUNT_LABELS"
	echo "ACCOUNT_LABEL_LINES=$account_label_lines"
	echo "ACCOUNT_LABEL_SLOTS=$account_label_slots"
	echo "ACCOUNT_LABEL_MISSING_SLOTS=$account_label_missing_slots"
	echo "ACCOUNT_LABEL_SLOT_COVERAGE_OK=$account_label_slot_coverage_ok"
	echo "HOST_LOG=$HOST_LOG"
	echo "PID_FILE=$PID_FILE"
} > "$SUMMARY_FILE"

log "result=$result chunks=$host_chunk_lines reassembled=$client_reassembled_lines resets=$chunk_reset_lines txmodeApplied=$tx_mode_applied mapgen=$mapgen_found gamestart=$game_start_found"
log "summary=$SUMMARY_FILE"

if [[ "$result" != "pass" ]]; then
	exit 1
fi
