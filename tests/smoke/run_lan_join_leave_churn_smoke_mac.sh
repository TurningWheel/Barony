#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
INSTANCES=8
CHURN_CYCLES=2
CHURN_COUNT=2
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
INITIAL_TIMEOUT_SECONDS=180
CYCLE_TIMEOUT_SECONDS=240
SETTLE_SECONDS=5
CHURN_GAP_SECONDS=3
CONNECT_ADDRESS="127.0.0.1:57165"
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
HELO_CHUNK_TX_MODE="normal"
AUTO_READY=0
TRACE_READY_SYNC=0
REQUIRE_READY_SYNC=0
TRACE_JOIN_REJECTS=0
OUTDIR=""
KEEP_RUNNING=0

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AGGREGATE="$SCRIPT_DIR/generate_smoke_aggregate_report.py"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"

usage() {
	cat <<'USAGE'
Usage: run_lan_join_leave_churn_smoke_mac.sh [options]

Options:
  --app <path>                    Barony executable path.
  --datadir <path>                Optional data directory passed to Barony via -datadir=<path>.
  --instances <n>                 Total host+client instances (default: 8, min: 3).
  --churn-cycles <n>              Number of kill/rejoin churn cycles (default: 2).
  --churn-count <n>               Clients churned per cycle (default: 2).
  --size <WxH>                    Window size.
  --stagger <sec>                 Delay between launches.
  --initial-timeout <sec>         Timeout for initial full lobby handshake.
  --cycle-timeout <sec>           Timeout for each churn-cycle rejoin.
  --settle <sec>                  Wait after initial full join before churn.
  --churn-gap <sec>               Delay between kill phase and relaunch phase.
  --connect-address <addr>        Host address for clients.
  --force-chunk <0|1>             BARONY_SMOKE_FORCE_HELO_CHUNK setting.
  --chunk-payload-max <n>         HELO chunk payload cap (64..900).
  --helo-chunk-tx-mode <mode>     HELO tx mode (normal|reverse|even-odd|duplicate-first|drop-last|duplicate-conflict-first).
  --auto-ready <0|1>              Enable BARONY_SMOKE_AUTO_READY on clients.
  --trace-ready-sync <0|1>        Enable host ready-sync trace logs (smoke-only).
  --require-ready-sync <0|1>      Assert ready snapshot queue/send coverage per slot/cycle.
  --trace-join-rejects <0|1>      Enable host smoke trace logs for join reject slot-state snapshots.
  --outdir <path>                 Output directory.
  --keep-running                  Do not kill launched instances on exit.
  -h, --help                      Show this help.
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

count_ready_snapshot_target_lines() {
	local file="$1"
	local mode="$2"
	local target="$3"
	if [[ ! -f "$file" ]]; then
		echo 0
		return
	fi
	local needle=""
	case "$mode" in
		queued)
			needle="[SMOKE]: ready snapshot queued target=$target "
			;;
		sent)
			needle="[SMOKE]: ready snapshot sent target=$target "
			;;
		*)
			echo 0
			return
			;;
	esac
	rg -F -c "$needle" "$file" 2>/dev/null || echo 0
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
		--churn-cycles)
			CHURN_CYCLES="${2:-}"
			shift 2
			;;
		--churn-count)
			CHURN_COUNT="${2:-}"
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
		--initial-timeout)
			INITIAL_TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--cycle-timeout)
			CYCLE_TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--settle)
			SETTLE_SECONDS="${2:-}"
			shift 2
			;;
		--churn-gap)
			CHURN_GAP_SECONDS="${2:-}"
			shift 2
			;;
		--connect-address)
			CONNECT_ADDRESS="${2:-}"
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
		--auto-ready)
			AUTO_READY="${2:-}"
			shift 2
			;;
		--trace-ready-sync)
			TRACE_READY_SYNC="${2:-}"
			shift 2
			;;
		--require-ready-sync)
			REQUIRE_READY_SYNC="${2:-}"
			shift 2
			;;
		--trace-join-rejects)
			TRACE_JOIN_REJECTS="${2:-}"
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
if ! is_uint "$INSTANCES" || (( INSTANCES < 3 || INSTANCES > 15 )); then
	echo "--instances must be 3..15" >&2
	exit 1
fi
if ! is_uint "$CHURN_CYCLES" || (( CHURN_CYCLES < 1 )); then
	echo "--churn-cycles must be >= 1" >&2
	exit 1
fi
if ! is_uint "$CHURN_COUNT" || (( CHURN_COUNT < 1 || CHURN_COUNT >= INSTANCES )); then
	echo "--churn-count must be >= 1 and < instances" >&2
	exit 1
fi
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$INITIAL_TIMEOUT_SECONDS" || ! is_uint "$CYCLE_TIMEOUT_SECONDS" || ! is_uint "$SETTLE_SECONDS" || ! is_uint "$CHURN_GAP_SECONDS"; then
	echo "--stagger, --initial-timeout, --cycle-timeout, --settle and --churn-gap must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$FORCE_CHUNK" || (( FORCE_CHUNK > 1 )); then
	echo "--force-chunk must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_READY" || (( AUTO_READY > 1 )); then
	echo "--auto-ready must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$TRACE_READY_SYNC" || (( TRACE_READY_SYNC > 1 )); then
	echo "--trace-ready-sync must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_READY_SYNC" || (( REQUIRE_READY_SYNC > 1 )); then
	echo "--require-ready-sync must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$TRACE_JOIN_REJECTS" || (( TRACE_JOIN_REJECTS > 1 )); then
	echo "--trace-join-rejects must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_READY_SYNC )) && (( AUTO_READY == 0 )); then
	echo "--require-ready-sync requires --auto-ready 1" >&2
	exit 1
fi
if (( REQUIRE_READY_SYNC )) && (( TRACE_READY_SYNC == 0 )); then
	echo "--require-ready-sync requires --trace-ready-sync 1" >&2
	exit 1
fi
if ! is_uint "$CHUNK_PAYLOAD_MAX" || (( CHUNK_PAYLOAD_MAX < 64 || CHUNK_PAYLOAD_MAX > 900 )); then
	echo "--chunk-payload-max must be 64..900" >&2
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
	OUTDIR="tests/smoke/artifacts/churn-${timestamp}-p${INSTANCES}-c${CHURN_CYCLES}x${CHURN_COUNT}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
LOG_DIR="$OUTDIR/stdout"
INSTANCE_ROOT="$OUTDIR/instances"
rm -rf "$LOG_DIR" "$INSTANCE_ROOT"
rm -f "$OUTDIR/summary.env" "$OUTDIR/churn_cycle_results.csv" "$OUTDIR/ready_sync_results.csv"
mkdir -p "$LOG_DIR" "$INSTANCE_ROOT"

HOST_LOG="$INSTANCE_ROOT/home-1-l1/.barony/log.txt"
CSV_PATH="$OUTDIR/churn_cycle_results.csv"
cat > "$CSV_PATH" <<'CSV'
cycle,required_host_chunk_lines,observed_host_chunk_lines,status
CSV
READY_SYNC_CSV="$OUTDIR/ready_sync_results.csv"
cat > "$READY_SYNC_CSV" <<'CSV'
player,expected_min_queued,observed_queued,expected_min_sent,observed_sent,status
CSV

declare -a SLOT_PIDS
declare -a SLOT_LAUNCH_COUNT
declare -a ALL_PIDS
for ((slot = 0; slot <= INSTANCES; ++slot)); do
	SLOT_PIDS[slot]=0
	SLOT_LAUNCH_COUNT[slot]=0
done

cleanup() {
	if (( KEEP_RUNNING )); then
		log "--keep-running enabled; leaving instances alive"
		return
	fi
	for pid in "${ALL_PIDS[@]}"; do
		kill "$pid" 2>/dev/null || true
	done
	sleep 1
	for pid in "${ALL_PIDS[@]}"; do
		if kill -0 "$pid" 2>/dev/null; then
			kill -9 "$pid" 2>/dev/null || true
		fi
	done
}
trap cleanup EXIT

launch_slot() {
	local slot="$1"
	local role="$2"
	local launch_num=$(( SLOT_LAUNCH_COUNT[slot] + 1 ))
	SLOT_LAUNCH_COUNT[slot]="$launch_num"

	local home_dir="$INSTANCE_ROOT/home-${slot}-l${launch_num}"
	local stdout_log="$LOG_DIR/instance-${slot}-l${launch_num}.stdout.log"
	mkdir -p "$home_dir"

	local -a env_vars=(
		"HOME=$home_dir"
		"BARONY_SMOKE_AUTOPILOT=1"
		"BARONY_SMOKE_CONNECT_DELAY_SECS=2"
		"BARONY_SMOKE_RETRY_DELAY_SECS=3"
		"BARONY_SMOKE_AUTO_READY=$AUTO_READY"
		"BARONY_SMOKE_FORCE_HELO_CHUNK=$FORCE_CHUNK"
		"BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
	)
	if [[ "$role" == "host" ]]; then
		env_vars+=(
			"BARONY_SMOKE_ROLE=host"
			"BARONY_SMOKE_EXPECTED_PLAYERS=$INSTANCES"
			"BARONY_SMOKE_AUTO_START=0"
			"BARONY_SMOKE_AUTO_ENTER_DUNGEON=0"
			"BARONY_SMOKE_HELO_CHUNK_TX_MODE=$HELO_CHUNK_TX_MODE"
		)
		if (( TRACE_READY_SYNC )); then
			env_vars+=("BARONY_SMOKE_TRACE_READY_SYNC=1")
		fi
		if (( TRACE_JOIN_REJECTS )); then
			env_vars+=("BARONY_SMOKE_TRACE_JOIN_REJECTS=1")
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
	SLOT_PIDS[slot]="$pid"
	ALL_PIDS+=("$pid")
	log "launch slot=$slot role=$role launch=$launch_num pid=$pid home=$home_dir"
}

stop_slot() {
	local slot="$1"
	local pid="${SLOT_PIDS[$slot]:-0}"
	if [[ -z "$pid" || "$pid" == "0" ]]; then
		return
	fi
	if ! kill -0 "$pid" 2>/dev/null; then
		SLOT_PIDS[slot]=0
		return
	fi
	log "stop slot=$slot pid=$pid"
	kill "$pid" 2>/dev/null || true
	for _ in {1..10}; do
		if ! kill -0 "$pid" 2>/dev/null; then
			SLOT_PIDS[slot]=0
			return
		fi
		sleep 1
	done
	kill -9 "$pid" 2>/dev/null || true
	SLOT_PIDS[slot]=0
}

wait_for_chunk_target() {
	local target="$1"
	local timeout="$2"
	local label="$3"
	local deadline=$((SECONDS + timeout))
	local count=0
	while (( SECONDS < deadline )); do
		count=$(count_fixed_lines "$HOST_LOG" "sending chunked HELO:")
		if (( count >= target )); then
			log "$label reached chunk target $count/$target" >&2
			echo "$count"
			return 0
		fi
		sleep 1
	done
	log "$label timed out waiting for chunk target: got=$count need=$target" >&2
	echo "$count"
	return 1
}

log "Artifacts: $OUTDIR"

launch_slot 1 host
sleep "$STAGGER_SECONDS"
for ((slot = 2; slot <= INSTANCES; ++slot)); do
	launch_slot "$slot" client
	sleep "$STAGGER_SECONDS"
done

initial_target=$((INSTANCES - 1))
observed_count="$(wait_for_chunk_target "$initial_target" "$INITIAL_TIMEOUT_SECONDS" "initial")"
if [[ -z "$observed_count" ]]; then
	observed_count=0
fi
if (( observed_count < initial_target )); then
	printf '0,%s,%s,fail\n' "$initial_target" "$observed_count" >> "$CSV_PATH"
	echo "Initial lobby did not reach expected HELO chunk target" >&2
	exit 1
fi
printf '0,%s,%s,pass\n' "$initial_target" "$observed_count" >> "$CSV_PATH"

if (( SETTLE_SECONDS > 0 )); then
	log "settling for ${SETTLE_SECONDS}s before churn cycles"
	sleep "$SETTLE_SECONDS"
fi

declare -a CHURN_SLOTS
for ((slot = INSTANCES; slot >= 2 && ${#CHURN_SLOTS[@]} < CHURN_COUNT; --slot)); do
	CHURN_SLOTS+=("$slot")
done

required_target="$initial_target"
failed=0
for ((cycle = 1; cycle <= CHURN_CYCLES; ++cycle)); do
	log "cycle $cycle/$CHURN_CYCLES: churn slots=${CHURN_SLOTS[*]}"
	for slot in "${CHURN_SLOTS[@]}"; do
		stop_slot "$slot"
	done
	if (( CHURN_GAP_SECONDS > 0 )); then
		sleep "$CHURN_GAP_SECONDS"
	fi
	for slot in "${CHURN_SLOTS[@]}"; do
		launch_slot "$slot" client
		sleep "$STAGGER_SECONDS"
	done

	required_target=$((required_target + CHURN_COUNT))
	observed_count="$(wait_for_chunk_target "$required_target" "$CYCLE_TIMEOUT_SECONDS" "cycle-$cycle")"
	if [[ -z "$observed_count" ]]; then
		observed_count=0
	fi
	if (( observed_count < required_target )); then
		printf '%s,%s,%s,fail\n' "$cycle" "$required_target" "$observed_count" >> "$CSV_PATH"
		failed=1
		break
	fi
	printf '%s,%s,%s,pass\n' "$cycle" "$required_target" "$observed_count" >> "$CSV_PATH"
done

final_host_chunk_lines=$(count_fixed_lines "$HOST_LOG" "sending chunked HELO:")
join_fail_lines=$(count_fixed_lines "$HOST_LOG" "Player failed to join lobby")
join_reject_trace_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: lobby join reject code=")
ready_snapshot_expected_total=0
ready_snapshot_queue_lines=0
ready_snapshot_sent_lines=0
ready_sync_fail=0
if (( REQUIRE_READY_SYNC )); then
	ready_snapshot_expected_total="$required_target"
	ready_snapshot_queue_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: ready snapshot queued target=")
	ready_snapshot_sent_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: ready snapshot sent target=")
	if (( ready_snapshot_queue_lines < ready_snapshot_expected_total )); then
		ready_sync_fail=1
	fi
	if (( ready_snapshot_sent_lines < ready_snapshot_expected_total )); then
		ready_sync_fail=1
	fi

	ready_sync_expected_players=$((INSTANCES - 1))
	for ((player = 1; player <= ready_sync_expected_players; ++player)); do
		expected_min=1
		observed_queued="$(count_ready_snapshot_target_lines "$HOST_LOG" queued "$player")"
		observed_sent="$(count_ready_snapshot_target_lines "$HOST_LOG" sent "$player")"
		row_status="pass"
		if (( observed_queued < expected_min || observed_sent < expected_min )); then
			row_status="fail"
			ready_sync_fail=1
		fi
		printf '%s,%s,%s,%s,%s,%s\n' \
			"$player" "$expected_min" "$observed_queued" "$expected_min" "$observed_sent" "$row_status" >> "$READY_SYNC_CSV"
	done

	if (( ready_sync_fail != 0 )); then
		failed=1
	fi
fi
SUMMARY_FILE="$OUTDIR/summary.env"
{
	echo "RESULT=$([[ $failed -eq 0 ]] && echo pass || echo fail)"
	echo "DATADIR=$DATADIR"
	echo "INSTANCES=$INSTANCES"
	echo "CHURN_CYCLES=$CHURN_CYCLES"
	echo "CHURN_COUNT=$CHURN_COUNT"
	echo "FORCE_CHUNK=$FORCE_CHUNK"
	echo "CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
	echo "HELO_CHUNK_TX_MODE=$HELO_CHUNK_TX_MODE"
	echo "AUTO_READY=$AUTO_READY"
	echo "TRACE_READY_SYNC=$TRACE_READY_SYNC"
	echo "REQUIRE_READY_SYNC=$REQUIRE_READY_SYNC"
	echo "TRACE_JOIN_REJECTS=$TRACE_JOIN_REJECTS"
	echo "INITIAL_REQUIRED_HOST_CHUNK_LINES=$initial_target"
	echo "FINAL_REQUIRED_HOST_CHUNK_LINES=$required_target"
	echo "FINAL_HOST_CHUNK_LINES=$final_host_chunk_lines"
	echo "JOIN_FAIL_LINES=$join_fail_lines"
	echo "JOIN_REJECT_TRACE_LINES=$join_reject_trace_lines"
	echo "READY_SNAPSHOT_EXPECTED_TOTAL=$ready_snapshot_expected_total"
	echo "READY_SNAPSHOT_QUEUE_LINES=$ready_snapshot_queue_lines"
	echo "READY_SNAPSHOT_SENT_LINES=$ready_snapshot_sent_lines"
	echo "READY_SYNC_RESULT=$([[ $ready_sync_fail -eq 0 ]] && echo pass || echo fail)"
	echo "CHURN_CSV=$CSV_PATH"
	echo "READY_SYNC_CSV=$READY_SYNC_CSV"
} > "$SUMMARY_FILE"

if command -v python3 >/dev/null 2>&1 && [[ -f "$AGGREGATE" ]]; then
	python3 "$AGGREGATE" --output "$OUTDIR/smoke_aggregate_report.html" --churn-csv "$CSV_PATH"
	log "Aggregate report written to $OUTDIR/smoke_aggregate_report.html"
fi

log "summary=$SUMMARY_FILE"
log "csv=$CSV_PATH"
if (( failed != 0 )); then
	exit 1
fi
