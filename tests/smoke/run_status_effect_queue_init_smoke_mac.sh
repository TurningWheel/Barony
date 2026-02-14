#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
STARTUP_TIMEOUT_SECONDS=540
CYCLE_TIMEOUT_SECONDS=360
OUTDIR=""

STARTUP_PLAYER_COUNTS=(1 5 15)
REJOIN_PLAYER_COUNTS=(5 15)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HELO_RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
CHURN_RUNNER="$SCRIPT_DIR/run_lan_join_leave_churn_smoke_mac.sh"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"
declare -a LOG_FILES=()

usage() {
	cat <<'USAGE'
Usage: run_status_effect_queue_init_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed to Barony via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches.
  --startup-timeout <sec>       Timeout for each startup lane (default: 540).
  --cycle-timeout <sec>         Timeout for each rejoin cycle lane (default: 360).
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

count_pattern_in_logs() {
	local regex="$1"
	shift
	local total=0
	local file count
	for file in "$@"; do
		count="$(rg -c "$regex" "$file" 2>/dev/null || echo 0)"
		if [[ -z "$count" ]]; then
			count=0
		fi
		total=$((total + count))
	done
	echo "$total"
}

collect_slots_for_lane() {
	local lane="$1"
	shift
	if (($# == 0)); then
		echo ""
		return
	fi
	local slots
	slots="$(rg -o "statusfx queue ${lane} slot=[0-9]+" "$@" 2>/dev/null \
		| sed -nE 's/.*slot=([0-9]+)/\1/p' \
		| sort -n \
		| uniq \
		| paste -sd';' - || true)"
	echo "$slots"
}

collect_missing_slots_for_lane() {
	local lane="$1"
	local min_slot="$2"
	local max_slot="$3"
	shift 3
	local -a logs=("$@")
	if (( max_slot < min_slot )); then
		echo ""
		return
	fi
	local missing=""
	local slot file found
	for ((slot = min_slot; slot <= max_slot; ++slot)); do
		found=0
		for file in "${logs[@]}"; do
			if rg -F -q "[SMOKE]: statusfx queue ${lane} slot=${slot} owner=${slot} status=ok" "$file"; then
				found=1
				break
			fi
		done
		if (( found == 0 )); then
			if [[ -n "$missing" ]]; then
				missing+=";"
			fi
			missing+="$slot"
		fi
	done
	echo "$missing"
}

read_summary_value() {
	local summary_file="$1"
	local key="$2"
	smoke_summary_get_last "$key" "$summary_file"
}

prune_models_cache() {
	smoke_prune_models_cache "$1"
}

collect_lane_logs() {
	local lane_outdir="$1"
	LOG_FILES=()
	while IFS= read -r file; do
		[[ -z "$file" ]] && continue
		LOG_FILES+=("$file")
	done < <(find "$lane_outdir/instances" -type f -path "*/.barony/log.txt" | sort)
}

write_csv_row() {
	local line="$1"
	echo "$line" >> "$CSV_PATH"
}

evaluate_startup_lane() {
	local instances="$1"
	local lane_outdir="$2"
	local lane_name="startup-p${instances}"
	local summary_file="$lane_outdir/summary.env"
	local child_result
	child_result="$(read_summary_value "$summary_file" "RESULT")"
	if [[ -z "$child_result" ]]; then
		child_result="fail"
	fi

	collect_lane_logs "$lane_outdir"
	local -a logs=("${LOG_FILES[@]}")
	if ((${#logs[@]} == 0)); then
		echo "No runtime logs found for $lane_name: $lane_outdir" >&2
		return 1
	fi

	local init_slots create_slots update_slots
	init_slots="$(collect_slots_for_lane init "${logs[@]}")"
	create_slots="$(collect_slots_for_lane create "${logs[@]}")"
	update_slots="$(collect_slots_for_lane update "${logs[@]}")"

	local init_missing create_missing update_missing
	init_missing="$(collect_missing_slots_for_lane init 0 $((instances - 1)) "${logs[@]}")"
	create_missing="$(collect_missing_slots_for_lane create 0 $((instances - 1)) "${logs[@]}")"
	update_missing="$(collect_missing_slots_for_lane update 0 $((instances - 1)) "${logs[@]}")"

	local init_ok=1
	local create_ok=1
	local update_ok=1
	if [[ -n "$init_missing" ]]; then
		init_ok=0
	fi
	if [[ -n "$create_missing" ]]; then
		create_ok=0
	fi
	if [[ -n "$update_missing" ]]; then
		update_ok=0
	fi

	local init_mismatch create_mismatch update_mismatch
	init_mismatch="$(count_pattern_in_logs "\\[SMOKE\\]: statusfx queue init slot=[0-9]+ owner=-?[0-9]+ status=mismatch" "${logs[@]}")"
	create_mismatch="$(count_pattern_in_logs "\\[SMOKE\\]: statusfx queue create slot=[0-9]+ owner=-?[0-9]+ status=mismatch" "${logs[@]}")"
	update_mismatch="$(count_pattern_in_logs "\\[SMOKE\\]: statusfx queue update slot=[0-9]+ owner=-?[0-9]+ status=mismatch" "${logs[@]}")"

	local lane_result="pass"
	if [[ "$child_result" != "pass" ]] \
		|| (( init_ok == 0 || create_ok == 0 || update_ok == 0 )) \
		|| (( init_mismatch > 0 || create_mismatch > 0 || update_mismatch > 0 )); then
		lane_result="fail"
	fi

	write_csv_row "${lane_name},${instances},0,0,${lane_result},${child_result},${init_slots},${init_missing},${init_ok},${init_mismatch},${create_slots},${create_missing},${create_ok},${create_mismatch},${update_slots},${update_missing},${update_ok},${update_mismatch},${lane_outdir}"
	if [[ "$lane_result" != "pass" ]]; then
		echo "Status-effect queue startup lane failed: $lane_name" >&2
		return 1
	fi
	return 0
}

evaluate_rejoin_lane() {
	local instances="$1"
	local churn_count="$2"
	local lane_outdir="$3"
	local lane_name="rejoin-p${instances}"
	local summary_file="$lane_outdir/summary.env"
	local child_result
	child_result="$(read_summary_value "$summary_file" "RESULT")"
	if [[ -z "$child_result" ]]; then
		child_result="fail"
	fi

	collect_lane_logs "$lane_outdir"
	local -a logs=("${LOG_FILES[@]}")
	if ((${#logs[@]} == 0)); then
		echo "No runtime logs found for $lane_name: $lane_outdir" >&2
		return 1
	fi

	local init_slots init_missing init_ok=1 init_mismatch
	init_slots="$(collect_slots_for_lane init "${logs[@]}")"
	init_missing="$(collect_missing_slots_for_lane init 0 $((instances - 1)) "${logs[@]}")"
	if [[ -n "$init_missing" ]]; then
		init_ok=0
	fi
	init_mismatch="$(count_pattern_in_logs "\\[SMOKE\\]: statusfx queue init slot=[0-9]+ owner=-?[0-9]+ status=mismatch" "${logs[@]}")"

	local join_fail_lines
	join_fail_lines="$(read_summary_value "$summary_file" "JOIN_FAIL_LINES")"
	if [[ -z "$join_fail_lines" ]]; then
		join_fail_lines=0
	fi
	if (( join_fail_lines > 0 )); then
		log "warning: ${lane_name} observed JOIN_FAIL_LINES=${join_fail_lines} (known intermittent lobby-full retry behavior)"
	fi

	local lane_result="pass"
	if [[ "$child_result" != "pass" ]] || (( init_ok == 0 )) || (( init_mismatch > 0 )); then
		lane_result="fail"
	fi

	write_csv_row "${lane_name},${instances},2,${churn_count},${lane_result},${child_result},${init_slots},${init_missing},${init_ok},${init_mismatch},n/a,n/a,n/a,n/a,n/a,n/a,n/a,n/a,${lane_outdir}"
	if [[ "$lane_result" != "pass" ]]; then
		echo "Status-effect queue rejoin lane failed: $lane_name" >&2
		return 1
	fi
	return 0
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
		--startup-timeout)
			STARTUP_TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--cycle-timeout)
			CYCLE_TIMEOUT_SECONDS="${2:-}"
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
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$STARTUP_TIMEOUT_SECONDS" || ! is_uint "$CYCLE_TIMEOUT_SECONDS"; then
	echo "--stagger, --startup-timeout, and --cycle-timeout must be non-negative integers" >&2
	exit 1
fi
if [[ ! -x "$HELO_RUNNER" ]]; then
	echo "Required runner missing or not executable: $HELO_RUNNER" >&2
	exit 1
fi
if [[ ! -x "$CHURN_RUNNER" ]]; then
	echo "Required runner missing or not executable: $CHURN_RUNNER" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/statusfx-queue-init-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -f "$OUTDIR/summary.env" "$OUTDIR/status_effect_queue_results.csv"

CSV_PATH="$OUTDIR/status_effect_queue_results.csv"
cat > "$CSV_PATH" <<'CSV'
lane,instances,churn_cycles,churn_count,result,child_result,init_slots,init_missing_slots,init_slot_coverage_ok,init_mismatch_lines,create_slots,create_missing_slots,create_slot_coverage_ok,create_mismatch_lines,update_slots,update_missing_slots,update_slot_coverage_ok,update_mismatch_lines,artifact
CSV

log "Artifacts: $OUTDIR"

for instances in "${STARTUP_PLAYER_COUNTS[@]}"; do
	lane_outdir="$OUTDIR/startup-p${instances}"
	log "startup lane instances=${instances}"
	cmd=(
		"$HELO_RUNNER"
		--app "$APP"
		--instances "$instances"
		--size "$WINDOW_SIZE"
		--stagger "$STAGGER_SECONDS"
		--timeout "$STARTUP_TIMEOUT_SECONDS"
		--force-chunk 1
		--chunk-payload-max 200
		--auto-start 1
		--auto-start-delay 2
		--auto-enter-dungeon 1
		--auto-enter-dungeon-delay 3
		--require-mapgen 1
		--outdir "$lane_outdir"
	)
	if [[ -n "$DATADIR" ]]; then
		cmd+=(--datadir "$DATADIR")
	fi
	BARONY_SMOKE_TRACE_STATUS_EFFECT_QUEUE=1 "${cmd[@]}"
	evaluate_startup_lane "$instances" "$lane_outdir"
	prune_models_cache "$lane_outdir"
done

for instances in "${REJOIN_PLAYER_COUNTS[@]}"; do
	lane_outdir="$OUTDIR/rejoin-p${instances}"
	churn_count=2
	if (( instances >= 15 )); then
		churn_count=4
	fi
	log "rejoin lane instances=${instances} churn_count=${churn_count}"
	cmd=(
		"$CHURN_RUNNER"
		--app "$APP"
		--instances "$instances"
		--churn-cycles 2
		--churn-count "$churn_count"
		--size "$WINDOW_SIZE"
		--stagger "$STAGGER_SECONDS"
		--initial-timeout "$CYCLE_TIMEOUT_SECONDS"
		--cycle-timeout "$CYCLE_TIMEOUT_SECONDS"
		--settle 5
		--churn-gap 3
		--force-chunk 1
		--chunk-payload-max 200
		--trace-join-rejects 1
		--outdir "$lane_outdir"
	)
	if [[ -n "$DATADIR" ]]; then
		cmd+=(--datadir "$DATADIR")
	fi
	BARONY_SMOKE_TRACE_STATUS_EFFECT_QUEUE=1 "${cmd[@]}"
	evaluate_rejoin_lane "$instances" "$churn_count" "$lane_outdir"
	prune_models_cache "$lane_outdir"
done

SUMMARY_FILE="$OUTDIR/summary.env"
{
	echo "RESULT=pass"
	echo "OUTDIR=$OUTDIR"
	echo "DATADIR=$DATADIR"
	echo "STARTUP_PLAYER_COUNTS=$(IFS=';'; echo "${STARTUP_PLAYER_COUNTS[*]}")"
	echo "REJOIN_PLAYER_COUNTS=$(IFS=';'; echo "${REJOIN_PLAYER_COUNTS[*]}")"
	echo "STARTUP_TIMEOUT_SECONDS=$STARTUP_TIMEOUT_SECONDS"
	echo "CYCLE_TIMEOUT_SECONDS=$CYCLE_TIMEOUT_SECONDS"
	echo "CSV_PATH=$CSV_PATH"
} > "$SUMMARY_FILE"

log "summary=$SUMMARY_FILE"
log "csv=$CSV_PATH"
