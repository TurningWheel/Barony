#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=300
KICK_DELAY_SECONDS=2
MIN_PLAYERS=2
MAX_PLAYERS=15
OUTDIR=""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HELO_RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"

usage() {
	cat <<'USAGE'
Usage: run_lobby_kick_target_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed to Barony via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches (default: 1).
  --timeout <sec>               Timeout per player-count lane (default: 300).
  --kick-delay <sec>            Delay before host auto-kick after full lobby (default: 2).
  --min-players <n>             Minimum lobby size (default: 2).
  --max-players <n>             Maximum lobby size (default: 15).
  --outdir <path>               Artifact directory.
  -h, --help                    Show this help.
USAGE
}

is_uint() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

log() {
	printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

read_summary_value() {
	local summary_file="$1"
	local key="$2"
	if [[ ! -f "$summary_file" ]]; then
		echo ""
		return
	fi
	sed -n "s/^${key}=//p" "$summary_file" | tail -n 1
}

prune_models_cache() {
	local lane_outdir="$1"
	if [[ ! -d "$lane_outdir/instances" ]]; then
		return
	fi
	find "$lane_outdir/instances" -type f -name models.cache -delete 2>/dev/null || true
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
		--kick-delay)
			KICK_DELAY_SECONDS="${2:-}"
			shift 2
			;;
		--min-players)
			MIN_PLAYERS="${2:-}"
			shift 2
			;;
		--max-players)
			MAX_PLAYERS="${2:-}"
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
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$KICK_DELAY_SECONDS"; then
	echo "--stagger, --timeout and --kick-delay must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$MIN_PLAYERS" || ! is_uint "$MAX_PLAYERS"; then
	echo "--min-players and --max-players must be integers" >&2
	exit 1
fi
if (( MIN_PLAYERS < 2 || MIN_PLAYERS > 15 )); then
	echo "--min-players must be 2..15" >&2
	exit 1
fi
if (( MAX_PLAYERS < 2 || MAX_PLAYERS > 15 )); then
	echo "--max-players must be 2..15" >&2
	exit 1
fi
if (( MIN_PLAYERS > MAX_PLAYERS )); then
	echo "--min-players cannot be greater than --max-players" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/lobby-kick-target-${timestamp}-p${MIN_PLAYERS}to${MAX_PLAYERS}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -rf "$OUTDIR"/p*
rm -f "$OUTDIR/kick_target_results.csv" "$OUTDIR/summary.env"

CSV_PATH="$OUTDIR/kick_target_results.csv"
{
	echo "lane,instances,target_slot,result,child_result,auto_kick_result,auto_kick_ok_lines,auto_kick_fail_lines,host_chunk_lines,client_reassembled_lines,outdir"
} > "$CSV_PATH"

total_lanes=0
pass_lanes=0
fail_lanes=0

for ((instances = MIN_PLAYERS; instances <= MAX_PLAYERS; ++instances)); do
	target_slot=$((instances - 1))
	lane_name="p${instances}-kick-slot${target_slot}"
	lane_outdir="$OUTDIR/$lane_name"

	log "Running lane $lane_name"
	cmd=(
		"$HELO_RUNNER"
		--app "$APP"
		--instances "$instances"
		--expected-players "$instances"
		--size "$WINDOW_SIZE"
		--stagger "$STAGGER_SECONDS"
		--timeout "$TIMEOUT_SECONDS"
		--force-chunk 1
		--chunk-payload-max 200
		--require-helo 1
		--auto-start 0
		--auto-kick-target-slot "$target_slot"
		--auto-kick-delay "$KICK_DELAY_SECONDS"
		--require-auto-kick 1
		--outdir "$lane_outdir"
	)
	if [[ -n "$DATADIR" ]]; then
		cmd+=(--datadir "$DATADIR")
	fi

	if "${cmd[@]}"; then
		child_result="pass"
	else
		child_result="fail"
	fi

	summary_file="$lane_outdir/summary.env"
	auto_kick_result="$(read_summary_value "$summary_file" "AUTO_KICK_RESULT")"
	auto_kick_ok_lines="$(read_summary_value "$summary_file" "AUTO_KICK_OK_LINES")"
	auto_kick_fail_lines="$(read_summary_value "$summary_file" "AUTO_KICK_FAIL_LINES")"
	host_chunk_lines="$(read_summary_value "$summary_file" "HOST_CHUNK_LINES")"
	client_reassembled_lines="$(read_summary_value "$summary_file" "CLIENT_REASSEMBLED_LINES")"

	if [[ -z "$auto_kick_result" ]]; then
		auto_kick_result="missing"
	fi
	if [[ -z "$auto_kick_ok_lines" ]]; then
		auto_kick_ok_lines=0
	fi
	if [[ -z "$auto_kick_fail_lines" ]]; then
		auto_kick_fail_lines=0
	fi
	if [[ -z "$host_chunk_lines" ]]; then
		host_chunk_lines=0
	fi
	if [[ -z "$client_reassembled_lines" ]]; then
		client_reassembled_lines=0
	fi

	lane_result="pass"
	if [[ "$child_result" != "pass" || "$auto_kick_result" != "ok" || "$auto_kick_fail_lines" != "0" ]]; then
		lane_result="fail"
	fi

	echo "${lane_name},${instances},${target_slot},${lane_result},${child_result},${auto_kick_result},${auto_kick_ok_lines},${auto_kick_fail_lines},${host_chunk_lines},${client_reassembled_lines},${lane_outdir}" >> "$CSV_PATH"

	total_lanes=$((total_lanes + 1))
	if [[ "$lane_result" == "pass" ]]; then
		pass_lanes=$((pass_lanes + 1))
	else
		fail_lanes=$((fail_lanes + 1))
	fi

	prune_models_cache "$lane_outdir"
done

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
	echo "MIN_PLAYERS=$MIN_PLAYERS"
	echo "MAX_PLAYERS=$MAX_PLAYERS"
	echo "TIMEOUT_SECONDS=$TIMEOUT_SECONDS"
	echo "KICK_DELAY_SECONDS=$KICK_DELAY_SECONDS"
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
