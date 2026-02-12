#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
HEATMAP="$SCRIPT_DIR/generate_mapgen_heatmap.py"
AGGREGATE="$SCRIPT_DIR/generate_smoke_aggregate_report.py"

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
MIN_PLAYERS=1
MAX_PLAYERS=15
RUNS_PER_PLAYER=1
BASE_SEED=1000
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=180
AUTO_START_DELAY_SECS=2
AUTO_ENTER_DUNGEON=1
AUTO_ENTER_DUNGEON_DELAY_SECS=3
FORCE_CHUNK=1
CHUNK_PAYLOAD_MAX=200
SIMULATE_MAPGEN_PLAYERS=0
INPROCESS_SIM_BATCH=1
INPROCESS_PLAYER_SWEEP=1
START_FLOOR=0
MAPGEN_RELOAD_SAME_LEVEL=0
MAPGEN_RELOAD_SEED_BASE=0
OUTDIR=""

usage() {
	cat <<'USAGE'
Usage: run_mapgen_sweep_mac.sh [options]

Options:
  --app <path>                 Barony executable path.
  --datadir <path>             Optional data directory passed through to runner via -datadir=<path>.
  --min-players <n>            Start player count (default: 1).
  --max-players <n>            End player count (default: 15).
  --runs-per-player <n>        Runs for each player count (default: 1).
  --base-seed <n>              Base seed value used for deterministic runs.
  --size <WxH>                 Window size for all instances.
  --stagger <sec>              Delay between instance launches.
  --timeout <sec>              Timeout per run.
  --auto-start-delay <sec>     Host auto-start delay after full lobby.
  --auto-enter-dungeon <0|1>   Host forces first dungeon transition after load.
  --auto-enter-dungeon-delay <sec>
                               Delay before forced dungeon entry.
  --force-chunk <0|1>          BARONY_SMOKE_FORCE_HELO_CHUNK setting.
  --chunk-payload-max <n>      Chunk payload cap (64..900).
  --simulate-mapgen-players <0|1>
                               Use one launched instance and simulate mapgen scaling players.
  --inprocess-sim-batch <0|1>  In simulated mode, gather all runs-per-player samples from one runtime.
  --inprocess-player-sweep <0|1>
                               In simulated+batch mode, sweep all player counts in one runtime.
  --start-floor <n>            Smoke-only host start floor (0..99).
  --mapgen-reload-same-level <0|1>
                               Reload same generated level between samples (avoids relaunches in batch mode).
  --mapgen-reload-seed-base <n>
                               Base seed used for same-level reload samples (0 disables forced seed rotation).
  --outdir <path>              Output directory.
  -h, --help                   Show this help.
USAGE
}

is_uint() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

log() {
	printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

read_summary_key() {
	local key="$1"
	local file="$2"
	local line
	line="$(rg -n "^${key}=" "$file" | head -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	echo "${line#*=}"
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
		--min-players)
			MIN_PLAYERS="${2:-}"
			shift 2
			;;
		--max-players)
			MAX_PLAYERS="${2:-}"
			shift 2
			;;
		--runs-per-player)
			RUNS_PER_PLAYER="${2:-}"
			shift 2
			;;
		--base-seed)
			BASE_SEED="${2:-}"
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
		--simulate-mapgen-players)
			SIMULATE_MAPGEN_PLAYERS="${2:-}"
			shift 2
			;;
		--inprocess-sim-batch)
			INPROCESS_SIM_BATCH="${2:-}"
			shift 2
			;;
		--inprocess-player-sweep)
			INPROCESS_PLAYER_SWEEP="${2:-}"
			shift 2
			;;
		--start-floor)
			START_FLOOR="${2:-}"
			shift 2
			;;
		--mapgen-reload-same-level)
			MAPGEN_RELOAD_SAME_LEVEL="${2:-}"
			shift 2
			;;
		--mapgen-reload-seed-base)
			MAPGEN_RELOAD_SEED_BASE="${2:-}"
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
if ! is_uint "$MIN_PLAYERS" || ! is_uint "$MAX_PLAYERS" || ! is_uint "$RUNS_PER_PLAYER"; then
	echo "--min-players, --max-players and --runs-per-player must be positive integers" >&2
	exit 1
fi
if (( MIN_PLAYERS < 1 || MAX_PLAYERS > 15 || MIN_PLAYERS > MAX_PLAYERS )); then
	echo "Player range must satisfy 1 <= min <= max <= 15" >&2
	exit 1
fi
if (( RUNS_PER_PLAYER < 1 )); then
	echo "--runs-per-player must be >= 1" >&2
	exit 1
fi
if ! is_uint "$TIMEOUT_SECONDS" || ! is_uint "$AUTO_START_DELAY_SECS" || ! is_uint "$AUTO_ENTER_DUNGEON_DELAY_SECS" || ! is_uint "$STAGGER_SECONDS"; then
	echo "--timeout, --stagger, --auto-start-delay and --auto-enter-dungeon-delay must be non-negative integers" >&2
	exit 1
fi
if ! is_uint "$AUTO_ENTER_DUNGEON" || (( AUTO_ENTER_DUNGEON > 1 )); then
	echo "--auto-enter-dungeon must be 0 or 1" >&2
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
if ! is_uint "$SIMULATE_MAPGEN_PLAYERS" || (( SIMULATE_MAPGEN_PLAYERS > 1 )); then
	echo "--simulate-mapgen-players must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$INPROCESS_SIM_BATCH" || (( INPROCESS_SIM_BATCH > 1 )); then
	echo "--inprocess-sim-batch must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$INPROCESS_PLAYER_SWEEP" || (( INPROCESS_PLAYER_SWEEP > 1 )); then
	echo "--inprocess-player-sweep must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$START_FLOOR" || (( START_FLOOR > 99 )); then
	echo "--start-floor must be 0..99" >&2
	exit 1
fi
if ! is_uint "$MAPGEN_RELOAD_SAME_LEVEL" || (( MAPGEN_RELOAD_SAME_LEVEL > 1 )); then
	echo "--mapgen-reload-same-level must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$MAPGEN_RELOAD_SEED_BASE"; then
	echo "--mapgen-reload-seed-base must be a non-negative integer" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/mapgen-sweep-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
RUNS_DIR="$OUTDIR/runs"
mkdir -p "$RUNS_DIR"

CSV_PATH="$OUTDIR/mapgen_results.csv"
cat > "$CSV_PATH" <<'CSV'
players,launched_instances,mapgen_players_override,mapgen_players_observed,run,seed,status,start_floor,host_chunk_lines,client_reassembled_lines,mapgen_found,mapgen_level,mapgen_secret,mapgen_seed_observed,rooms,monsters,gold,items,decorations,decorations_blocking,decorations_utility,decorations_traps,decorations_economy,food_items,food_servings,gold_bags,gold_amount,item_stacks,item_units,run_dir,mapgen_wait_reason,mapgen_reload_transition_lines,mapgen_generation_lines,mapgen_generation_unique_seed_count,mapgen_reload_regen_ok
CSV

failures=0
total_runs=0
datadir_args=()
if [[ -n "$DATADIR" ]]; then
	datadir_args=(--datadir "$DATADIR")
fi

log "Writing outputs to $OUTDIR"
if (( SIMULATE_MAPGEN_PLAYERS )); then
	log "Mapgen sweep mode: single-instance simulated player scaling"
	if (( INPROCESS_SIM_BATCH )); then
		log "Mapgen sweep submode: in-process batch collection enabled"
	fi
	if (( INPROCESS_PLAYER_SWEEP )); then
		log "Mapgen sweep submode: in-process single-runtime player sweep enabled"
	fi
fi

parse_mapgen_metrics_lines() {
	local host_log="$1"
	local limit="$2"
	local out_file="$3"
	: > "$out_file"
	if [[ ! -f "$host_log" ]]; then
		echo 0
		return
	fi
	local count=0
	local mapgen_lines_file
	local food_lines_file
	local decoration_lines_file
	local value_lines_file
	mapgen_lines_file="$(mktemp)"
	food_lines_file="$(mktemp)"
	decoration_lines_file="$(mktemp)"
	value_lines_file="$(mktemp)"
	rg -F "successfully generated a dungeon with" "$host_log" | rg 'level=[1-9][0-9]*' > "$mapgen_lines_file" || true
	rg -F "mapgen food summary:" "$host_log" | rg 'level=[1-9][0-9]*' > "$food_lines_file" || true
	rg -F "mapgen decoration summary:" "$host_log" | rg 'level=[1-9][0-9]*' > "$decoration_lines_file" || true
	rg -F "mapgen value summary:" "$host_log" | rg 'level=[1-9][0-9]*' > "$value_lines_file" || true
	local line=""
	local food_line=""
	local decoration_line=""
	local value_line=""
	while IFS= read -r line; do
		if (( count >= limit )); then
			break
		fi
		food_line="$(sed -n "$((count + 1))p" "$food_lines_file" || true)"
		decoration_line="$(sed -n "$((count + 1))p" "$decoration_lines_file" || true)"
		value_line="$(sed -n "$((count + 1))p" "$value_lines_file" || true)"
		if [[ "$line" =~ with[[:space:]]+([0-9]+)[[:space:]]+rooms,[[:space:]]+([0-9]+)[[:space:]]+monsters,[[:space:]]+([0-9]+)[[:space:]]+gold,[[:space:]]+([0-9]+)[[:space:]]+items,[[:space:]]+([0-9]+)[[:space:]]+decorations ]]; then
			local rooms="${BASH_REMATCH[1]}"
			local monsters="${BASH_REMATCH[2]}"
			local gold="${BASH_REMATCH[3]}"
			local items="${BASH_REMATCH[4]}"
			local decorations="${BASH_REMATCH[5]}"
			local decorations_blocking=""
			local decorations_utility=""
			local decorations_traps=""
			local decorations_economy=""
			local food_items=""
			local food_servings=""
			local gold_bags=""
			local gold_amount=""
			local item_stacks=""
			local item_units=""
			local mapgen_level=""
			local mapgen_secret=""
			local mapgen_seed_observed=""
			local mapgen_players_observed=""
			if [[ "$line" =~ level=([0-9]+) ]]; then
				mapgen_level="${BASH_REMATCH[1]}"
			fi
			if [[ "$line" =~ secret=([0-9]+) ]]; then
				mapgen_secret="${BASH_REMATCH[1]}"
			fi
			if [[ "$line" =~ seed=([0-9]+) ]]; then
				mapgen_seed_observed="${BASH_REMATCH[1]}"
			fi
			if [[ "$line" =~ players=([0-9]+) ]]; then
				mapgen_players_observed="${BASH_REMATCH[1]}"
			fi
			if [[ "$food_line" =~ food=([0-9]+) ]]; then
				food_items="${BASH_REMATCH[1]}"
			fi
			if [[ "$food_line" =~ food_servings=([0-9]+) ]]; then
				food_servings="${BASH_REMATCH[1]}"
			fi
			if [[ "$value_line" =~ gold_bags=([0-9]+) ]]; then
				gold_bags="${BASH_REMATCH[1]}"
			fi
			if [[ "$value_line" =~ gold_amount=([0-9]+) ]]; then
				gold_amount="${BASH_REMATCH[1]}"
			fi
			if [[ "$value_line" =~ item_stacks=([0-9]+) ]]; then
				item_stacks="${BASH_REMATCH[1]}"
			fi
			if [[ "$value_line" =~ item_units=([0-9]+) ]]; then
				item_units="${BASH_REMATCH[1]}"
			fi
			if [[ "$decoration_line" =~ blocking=([0-9]+) ]]; then
				decorations_blocking="${BASH_REMATCH[1]}"
			fi
			if [[ "$decoration_line" =~ utility=([0-9]+) ]]; then
				decorations_utility="${BASH_REMATCH[1]}"
			fi
			if [[ "$decoration_line" =~ traps=([0-9]+) ]]; then
				decorations_traps="${BASH_REMATCH[1]}"
			fi
			if [[ "$decoration_line" =~ economy=([0-9]+) ]]; then
				decorations_economy="${BASH_REMATCH[1]}"
			fi
			printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
				"${rooms}" "${monsters}" "${gold}" "${items}" "${decorations}" \
				"${decorations_blocking}" "${decorations_utility}" "${decorations_traps}" "${decorations_economy}" \
				"${food_items}" "${food_servings}" "${gold_bags}" "${gold_amount}" "${item_stacks}" "${item_units}" \
				"${mapgen_level}" "${mapgen_secret}" "${mapgen_seed_observed}" "${mapgen_players_observed}" >> "$out_file"
			count=$((count + 1))
		fi
	done < "$mapgen_lines_file"
	rm -f "$mapgen_lines_file" "$food_lines_file" "$decoration_lines_file" "$value_lines_file"
	echo "$count"
}

write_mapgen_control_file() {
	local control_file="$1"
	local players="$2"
	printf '%s\n' "$players" > "$control_file"
}

use_single_runtime_sweep=0
if (( SIMULATE_MAPGEN_PLAYERS && INPROCESS_SIM_BATCH && INPROCESS_PLAYER_SWEEP && MAPGEN_RELOAD_SAME_LEVEL )); then
	player_span=$((MAX_PLAYERS - MIN_PLAYERS + 1))
	total_samples=$((player_span * RUNS_PER_PLAYER))
	if (( total_samples <= 256 )); then
		use_single_runtime_sweep=1
	else
		log "Single-runtime player sweep disabled: requested samples=${total_samples} exceeds auto-transition cap (256)"
	fi
elif (( SIMULATE_MAPGEN_PLAYERS && INPROCESS_SIM_BATCH && INPROCESS_PLAYER_SWEEP )); then
	log "Single-runtime player sweep requires --mapgen-reload-same-level 1; falling back to per-player batch mode"
fi

if (( use_single_runtime_sweep )); then
	total_runs=$((total_runs + total_samples))
	launched_instances=1
	expected_players=1
	require_helo=0
	run_dir="$RUNS_DIR/p${MIN_PLAYERS}-p${MAX_PLAYERS}-single-runtime"
	mkdir -p "$run_dir"
	control_file="$run_dir/mapgen_players_override.txt"
	write_mapgen_control_file "$control_file" "$MIN_PLAYERS"

	seed_base=$((BASE_SEED + 1))
	batch_transition_repeats="$total_samples"
	reload_seed_base="$MAPGEN_RELOAD_SEED_BASE"
	if (( reload_seed_base == 0 )); then
		reload_seed_base=$((seed_base * 100))
	fi
	single_runtime_timeout_seconds="$TIMEOUT_SECONDS"
	per_sample_timeout_budget=$((AUTO_ENTER_DUNGEON_DELAY_SECS + 9))
	if (( per_sample_timeout_budget < 10 )); then
		per_sample_timeout_budget=10
	fi
	min_single_runtime_timeout=$((120 + total_samples * per_sample_timeout_budget))
	if (( single_runtime_timeout_seconds < min_single_runtime_timeout )); then
		log "Single-runtime sweep timeout auto-bump: requested=${single_runtime_timeout_seconds}s recommended=${min_single_runtime_timeout}s"
		single_runtime_timeout_seconds="$min_single_runtime_timeout"
	fi

	log "Single-runtime sweep: players=${MIN_PLAYERS}..${MAX_PLAYERS} samples=${total_samples} repeats=${batch_transition_repeats} seed=${seed_base} timeout=${single_runtime_timeout_seconds}s"
	"$RUNNER" \
		--app "$APP" \
		"${datadir_args[@]}" \
		--instances "$launched_instances" \
		--size "$WINDOW_SIZE" \
		--stagger "$STAGGER_SECONDS" \
		--timeout "$single_runtime_timeout_seconds" \
		--expected-players "$expected_players" \
		--auto-start 1 \
		--auto-start-delay "$AUTO_START_DELAY_SECS" \
		--auto-enter-dungeon "$AUTO_ENTER_DUNGEON" \
		--auto-enter-dungeon-delay "$AUTO_ENTER_DUNGEON_DELAY_SECS" \
		--auto-enter-dungeon-repeats "$batch_transition_repeats" \
		--force-chunk "$FORCE_CHUNK" \
		--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
		--seed "$seed_base" \
		--require-helo "$require_helo" \
		--require-mapgen 1 \
		--mapgen-samples "$total_samples" \
		--mapgen-players-override "$MIN_PLAYERS" \
		--mapgen-control-file "$control_file" \
		--mapgen-reload-same-level 1 \
		--mapgen-reload-seed-base "$reload_seed_base" \
		--start-floor "$START_FLOOR" \
		--outdir "$run_dir" &
	runner_pid="$!"

	host_log="$run_dir/instances/home-1/.barony/log.txt"
	next_switch_count="$RUNS_PER_PLAYER"
	next_player="$((MIN_PLAYERS + 1))"
	last_written_player="$MIN_PLAYERS"
	while kill -0 "$runner_pid" 2>/dev/null; do
		if [[ -f "$host_log" ]]; then
			mapgen_count_so_far="$(count_fixed_lines "$host_log" "successfully generated a dungeon with")"
			while (( next_player <= MAX_PLAYERS && mapgen_count_so_far >= next_switch_count )); do
				write_mapgen_control_file "$control_file" "$next_player"
				last_written_player="$next_player"
				log "Single-runtime sweep control update: sample=${mapgen_count_so_far} mapgen_players=${next_player}"
				next_player=$((next_player + 1))
				next_switch_count=$((next_switch_count + RUNS_PER_PLAYER))
			done
		fi
		sleep 1
	done
	if wait "$runner_pid"; then
		status="pass"
	else
		status="fail"
	fi
	log "Single-runtime sweep complete: final_control_player=${last_written_player} status=${status}"

	summary="$run_dir/summary.env"
	host_chunk_lines=""
	client_reassembled_lines=""
	mapgen_found=""
	mapgen_wait_reason=""
	mapgen_reload_transition_lines=""
	mapgen_generation_lines=""
	mapgen_generation_unique_seed_count=""
	mapgen_reload_regen_ok=""
	host_log="$run_dir/instances/home-1/.barony/log.txt"
	if [[ -f "$summary" ]]; then
		host_chunk_lines="$(read_summary_key HOST_CHUNK_LINES "$summary")"
		client_reassembled_lines="$(read_summary_key CLIENT_REASSEMBLED_LINES "$summary")"
		mapgen_found="$(read_summary_key MAPGEN_FOUND "$summary")"
		mapgen_wait_reason="$(read_summary_key MAPGEN_WAIT_REASON "$summary")"
		mapgen_reload_transition_lines="$(read_summary_key MAPGEN_RELOAD_TRANSITION_LINES "$summary")"
		mapgen_generation_lines="$(read_summary_key MAPGEN_GENERATION_LINES "$summary")"
		mapgen_generation_unique_seed_count="$(read_summary_key MAPGEN_GENERATION_UNIQUE_SEED_COUNT "$summary")"
		mapgen_reload_regen_ok="$(read_summary_key MAPGEN_RELOAD_REGEN_OK "$summary")"
		host_log_from_summary="$(read_summary_key HOST_LOG "$summary")"
		if [[ -n "$host_log_from_summary" ]]; then
			host_log="$host_log_from_summary"
		fi
	fi

	metrics_file="$run_dir/mapgen_metrics_batch.csv"
	batch_count="$(parse_mapgen_metrics_lines "$host_log" "$total_samples" "$metrics_file")"
	row_failures=0
	sample_index=0
		for ((players = MIN_PLAYERS; players <= MAX_PLAYERS; ++players)); do
			for ((run = 1; run <= RUNS_PER_PLAYER; ++run)); do
				sample_index=$((sample_index + 1))
				seed=$((BASE_SEED + (players - MIN_PLAYERS) * RUNS_PER_PLAYER + run))
				mapgen_seed_observed=""
				rooms=""
				monsters=""
				gold=""
					items=""
					decorations=""
					decorations_blocking=""
					decorations_utility=""
					decorations_traps=""
					decorations_economy=""
					food_items=""
					food_servings=""
					gold_bags=""
					gold_amount=""
					item_stacks=""
					item_units=""
					mapgen_level=""
					mapgen_secret=""
					mapgen_players_observed=""
					row_status="$status"
					if (( sample_index <= batch_count )); then
						line="$(sed -n "${sample_index}p" "$metrics_file" || true)"
						IFS=',' read -r rooms monsters gold items decorations decorations_blocking decorations_utility decorations_traps decorations_economy food_items food_servings gold_bags gold_amount item_stacks item_units mapgen_level mapgen_secret mapgen_seed_observed mapgen_players_observed <<< "$line"
					else
						row_status="fail"
					fi
				if [[ -n "$mapgen_players_observed" ]] && [[ "$mapgen_players_observed" != "$players" ]]; then
					row_status="fail"
				fi
				if [[ -z "$mapgen_players_observed" ]]; then
					mapgen_players_observed="$players"
				fi
				if [[ -z "$mapgen_seed_observed" ]]; then
					mapgen_seed_observed="$seed"
				fi
				if [[ "$row_status" != "pass" ]]; then
					row_failures=1
				fi
				printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
					"$players" "$launched_instances" "$players" "${mapgen_players_observed:-}" \
					"$run" "$seed" "$row_status" "$START_FLOOR" \
					"${host_chunk_lines:-}" "${client_reassembled_lines:-}" "${mapgen_found:-}" \
					"${mapgen_level:-}" "${mapgen_secret:-}" "${mapgen_seed_observed:-}" \
					"${rooms:-}" "${monsters:-}" "${gold:-}" "${items:-}" "${decorations:-}" \
					"${decorations_blocking:-}" "${decorations_utility:-}" "${decorations_traps:-}" "${decorations_economy:-}" \
					"${food_items:-}" "${food_servings:-}" "${gold_bags:-}" "${gold_amount:-}" "${item_stacks:-}" "${item_units:-}" \
					"$run_dir" "${mapgen_wait_reason:-}" "${mapgen_reload_transition_lines:-}" "${mapgen_generation_lines:-}" \
					"${mapgen_generation_unique_seed_count:-}" "${mapgen_reload_regen_ok:-}" >> "$CSV_PATH"
			done
		done
	if [[ "$status" != "pass" ]] || (( batch_count < total_samples )) || (( row_failures > 0 )); then
		failures=$((failures + 1))
	fi
else
	for ((players = MIN_PLAYERS; players <= MAX_PLAYERS; ++players)); do
		if (( SIMULATE_MAPGEN_PLAYERS && INPROCESS_SIM_BATCH )); then
			total_runs=$((total_runs + RUNS_PER_PLAYER))
			run_dir="$RUNS_DIR/p${players}-batch"
			mkdir -p "$run_dir"

			launched_instances=1
			expected_players=1
			require_helo=0
			mapgen_players_override="$players"
			seed_base=$((BASE_SEED + (players - MIN_PLAYERS) * RUNS_PER_PLAYER + 1))
			batch_transition_repeats=$((RUNS_PER_PLAYER * 2))
			if (( MAPGEN_RELOAD_SAME_LEVEL )); then
				batch_transition_repeats="$RUNS_PER_PLAYER"
			elif (( batch_transition_repeats < RUNS_PER_PLAYER + 2 )); then
				batch_transition_repeats=$((RUNS_PER_PLAYER + 2))
			fi
			if (( batch_transition_repeats > 256 )); then
				batch_transition_repeats=256
			fi
			reload_seed_base="$MAPGEN_RELOAD_SEED_BASE"
			if (( MAPGEN_RELOAD_SAME_LEVEL )) && (( reload_seed_base == 0 )); then
				reload_seed_base=$((seed_base * 100))
			fi

			log "Batch run: players=${players} launched=${launched_instances} samples=${RUNS_PER_PLAYER} repeats=${batch_transition_repeats} seed=${seed_base}"
			if "$RUNNER" \
				--app "$APP" \
				"${datadir_args[@]}" \
				--instances "$launched_instances" \
				--size "$WINDOW_SIZE" \
				--stagger "$STAGGER_SECONDS" \
				--timeout "$TIMEOUT_SECONDS" \
				--expected-players "$expected_players" \
				--auto-start 1 \
				--auto-start-delay "$AUTO_START_DELAY_SECS" \
				--auto-enter-dungeon "$AUTO_ENTER_DUNGEON" \
				--auto-enter-dungeon-delay "$AUTO_ENTER_DUNGEON_DELAY_SECS" \
				--auto-enter-dungeon-repeats "$batch_transition_repeats" \
				--force-chunk "$FORCE_CHUNK" \
				--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
				--seed "$seed_base" \
				--require-helo "$require_helo" \
				--require-mapgen 1 \
				--mapgen-samples "$RUNS_PER_PLAYER" \
				--mapgen-players-override "$mapgen_players_override" \
				--mapgen-reload-same-level "$MAPGEN_RELOAD_SAME_LEVEL" \
				--mapgen-reload-seed-base "$reload_seed_base" \
				--start-floor "$START_FLOOR" \
				--outdir "$run_dir"; then
				status="pass"
			else
				status="fail"
			fi

			summary="$run_dir/summary.env"
			host_chunk_lines=""
			client_reassembled_lines=""
			mapgen_found=""
			mapgen_level=""
			mapgen_secret=""
			mapgen_wait_reason=""
			mapgen_reload_transition_lines=""
			mapgen_generation_lines=""
			mapgen_generation_unique_seed_count=""
			mapgen_reload_regen_ok=""
			host_log="$run_dir/instances/home-1/.barony/log.txt"
			if [[ -f "$summary" ]]; then
				host_chunk_lines="$(read_summary_key HOST_CHUNK_LINES "$summary")"
				client_reassembled_lines="$(read_summary_key CLIENT_REASSEMBLED_LINES "$summary")"
				mapgen_found="$(read_summary_key MAPGEN_FOUND "$summary")"
				mapgen_wait_reason="$(read_summary_key MAPGEN_WAIT_REASON "$summary")"
				mapgen_reload_transition_lines="$(read_summary_key MAPGEN_RELOAD_TRANSITION_LINES "$summary")"
				mapgen_generation_lines="$(read_summary_key MAPGEN_GENERATION_LINES "$summary")"
				mapgen_generation_unique_seed_count="$(read_summary_key MAPGEN_GENERATION_UNIQUE_SEED_COUNT "$summary")"
				mapgen_reload_regen_ok="$(read_summary_key MAPGEN_RELOAD_REGEN_OK "$summary")"
				host_log_from_summary="$(read_summary_key HOST_LOG "$summary")"
				if [[ -n "$host_log_from_summary" ]]; then
					host_log="$host_log_from_summary"
				fi
			fi

			metrics_file="$run_dir/mapgen_metrics_batch.csv"
			batch_count="$(parse_mapgen_metrics_lines "$host_log" "$RUNS_PER_PLAYER" "$metrics_file")"
			if [[ "$status" == "pass" ]] && (( batch_count < RUNS_PER_PLAYER )); then
				status="fail"
			fi
			if [[ "$status" != "pass" ]]; then
				failures=$((failures + 1))
			fi

				for ((run = 1; run <= RUNS_PER_PLAYER; ++run)); do
					seed=$((seed_base + run - 1))
					mapgen_seed_observed=""
					mapgen_level=""
					mapgen_secret=""
					rooms=""
					monsters=""
					gold=""
						items=""
						decorations=""
						decorations_blocking=""
						decorations_utility=""
						decorations_traps=""
						decorations_economy=""
						food_items=""
						food_servings=""
						gold_bags=""
						gold_amount=""
						item_stacks=""
						item_units=""
						mapgen_players_observed=""
						row_status="$status"
						if (( run <= batch_count )); then
							line="$(sed -n "${run}p" "$metrics_file" || true)"
							IFS=',' read -r rooms monsters gold items decorations decorations_blocking decorations_utility decorations_traps decorations_economy food_items food_servings gold_bags gold_amount item_stacks item_units mapgen_level mapgen_secret mapgen_seed_observed mapgen_players_observed <<< "$line"
						else
							row_status="fail"
						fi
					if [[ -z "$mapgen_players_observed" ]]; then
						mapgen_players_observed="$mapgen_players_override"
					fi
					if [[ -z "$mapgen_seed_observed" ]]; then
						mapgen_seed_observed="$seed"
					fi
					printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
						"$players" "$launched_instances" "${mapgen_players_override:-}" "${mapgen_players_observed:-}" \
						"$run" "$seed" "$row_status" "$START_FLOOR" \
						"${host_chunk_lines:-}" "${client_reassembled_lines:-}" "${mapgen_found:-}" \
						"${mapgen_level:-}" "${mapgen_secret:-}" "${mapgen_seed_observed:-}" \
						"${rooms:-}" "${monsters:-}" "${gold:-}" "${items:-}" "${decorations:-}" \
						"${decorations_blocking:-}" "${decorations_utility:-}" "${decorations_traps:-}" "${decorations_economy:-}" \
						"${food_items:-}" "${food_servings:-}" "${gold_bags:-}" "${gold_amount:-}" "${item_stacks:-}" "${item_units:-}" \
						"$run_dir" "${mapgen_wait_reason:-}" "${mapgen_reload_transition_lines:-}" "${mapgen_generation_lines:-}" \
						"${mapgen_generation_unique_seed_count:-}" "${mapgen_reload_regen_ok:-}" >> "$CSV_PATH"
				done
			continue
		fi

		for ((run = 1; run <= RUNS_PER_PLAYER; ++run)); do
			total_runs=$((total_runs + 1))
			seed=$((BASE_SEED + (players - MIN_PLAYERS) * RUNS_PER_PLAYER + run))
			run_dir="$RUNS_DIR/p${players}-r${run}"
			mkdir -p "$run_dir"

			launched_instances="$players"
			expected_players="$players"
			mapgen_players_override=""
			require_helo=0
			if (( players > 1 )); then
				require_helo=1
			fi
			if (( SIMULATE_MAPGEN_PLAYERS )); then
				launched_instances=1
				expected_players=1
				require_helo=0
				mapgen_players_override="$players"
			fi

			log "Run ${total_runs}: players=${players} launched=${launched_instances} run=${run} seed=${seed}"
			mapgen_override_args=()
			if [[ -n "$mapgen_players_override" ]]; then
				mapgen_override_args+=(--mapgen-players-override "$mapgen_players_override")
			fi
			if "$RUNNER" \
				--app "$APP" \
				"${datadir_args[@]}" \
				--instances "$launched_instances" \
				--size "$WINDOW_SIZE" \
				--stagger "$STAGGER_SECONDS" \
				--timeout "$TIMEOUT_SECONDS" \
				--expected-players "$expected_players" \
				--auto-start 1 \
				--auto-start-delay "$AUTO_START_DELAY_SECS" \
				--auto-enter-dungeon "$AUTO_ENTER_DUNGEON" \
				--auto-enter-dungeon-delay "$AUTO_ENTER_DUNGEON_DELAY_SECS" \
				--force-chunk "$FORCE_CHUNK" \
				--chunk-payload-max "$CHUNK_PAYLOAD_MAX" \
				--seed "$seed" \
				--require-helo "$require_helo" \
				--require-mapgen 1 \
				--mapgen-reload-same-level "$MAPGEN_RELOAD_SAME_LEVEL" \
				--mapgen-reload-seed-base "$MAPGEN_RELOAD_SEED_BASE" \
				--start-floor "$START_FLOOR" \
				${mapgen_override_args[@]+"${mapgen_override_args[@]}"} \
				--outdir "$run_dir"; then
				status="pass"
			else
				status="fail"
				failures=$((failures + 1))
			fi

			summary="$run_dir/summary.env"
			host_chunk_lines=""
			client_reassembled_lines=""
				mapgen_found=""
				mapgen_level=""
				mapgen_secret=""
				mapgen_seed_observed=""
				rooms=""
				monsters=""
				gold=""
					items=""
					decorations=""
					decorations_blocking=""
					decorations_utility=""
					decorations_traps=""
					decorations_economy=""
					food_items=""
					food_servings=""
					gold_bags=""
					gold_amount=""
					item_stacks=""
					item_units=""
					mapgen_wait_reason=""
				mapgen_reload_transition_lines=""
				mapgen_generation_lines=""
				mapgen_generation_unique_seed_count=""
				mapgen_reload_regen_ok=""
				mapgen_players_observed="${mapgen_players_override:-$players}"
			if [[ -f "$summary" ]]; then
				host_chunk_lines="$(read_summary_key HOST_CHUNK_LINES "$summary")"
				client_reassembled_lines="$(read_summary_key CLIENT_REASSEMBLED_LINES "$summary")"
				mapgen_found="$(read_summary_key MAPGEN_FOUND "$summary")"
				rooms="$(read_summary_key MAPGEN_ROOMS "$summary")"
				monsters="$(read_summary_key MAPGEN_MONSTERS "$summary")"
						gold="$(read_summary_key MAPGEN_GOLD "$summary")"
						items="$(read_summary_key MAPGEN_ITEMS "$summary")"
						decorations="$(read_summary_key MAPGEN_DECORATIONS "$summary")"
						decorations_blocking="$(read_summary_key MAPGEN_DECOR_BLOCKING "$summary")"
						decorations_utility="$(read_summary_key MAPGEN_DECOR_UTILITY "$summary")"
						decorations_traps="$(read_summary_key MAPGEN_DECOR_TRAPS "$summary")"
						decorations_economy="$(read_summary_key MAPGEN_DECOR_ECONOMY "$summary")"
						food_items="$(read_summary_key MAPGEN_FOOD_ITEMS "$summary")"
						food_servings="$(read_summary_key MAPGEN_FOOD_SERVINGS "$summary")"
						gold_bags="$(read_summary_key MAPGEN_GOLD_BAGS "$summary")"
						gold_amount="$(read_summary_key MAPGEN_GOLD_AMOUNT "$summary")"
						item_stacks="$(read_summary_key MAPGEN_ITEM_STACKS "$summary")"
						item_units="$(read_summary_key MAPGEN_ITEM_UNITS "$summary")"
					mapgen_level="$(read_summary_key MAPGEN_LEVEL "$summary")"
					mapgen_secret="$(read_summary_key MAPGEN_SECRET "$summary")"
					mapgen_seed_observed="$(read_summary_key MAPGEN_SEED "$summary")"
					mapgen_wait_reason="$(read_summary_key MAPGEN_WAIT_REASON "$summary")"
					mapgen_reload_transition_lines="$(read_summary_key MAPGEN_RELOAD_TRANSITION_LINES "$summary")"
					mapgen_generation_lines="$(read_summary_key MAPGEN_GENERATION_LINES "$summary")"
					mapgen_generation_unique_seed_count="$(read_summary_key MAPGEN_GENERATION_UNIQUE_SEED_COUNT "$summary")"
					mapgen_reload_regen_ok="$(read_summary_key MAPGEN_RELOAD_REGEN_OK "$summary")"
				fi

				if [[ -z "$mapgen_seed_observed" ]]; then
					mapgen_seed_observed="$seed"
				fi
				printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
					"$players" "$launched_instances" "${mapgen_players_override:-}" "${mapgen_players_observed:-}" \
					"$run" "$seed" "$status" "$START_FLOOR" \
					"${host_chunk_lines:-}" "${client_reassembled_lines:-}" "${mapgen_found:-}" \
					"${mapgen_level:-}" "${mapgen_secret:-}" "${mapgen_seed_observed:-}" \
					"${rooms:-}" "${monsters:-}" "${gold:-}" "${items:-}" "${decorations:-}" \
					"${decorations_blocking:-}" "${decorations_utility:-}" "${decorations_traps:-}" "${decorations_economy:-}" \
					"${food_items:-}" "${food_servings:-}" "${gold_bags:-}" "${gold_amount:-}" "${item_stacks:-}" "${item_units:-}" \
					"$run_dir" "${mapgen_wait_reason:-}" "${mapgen_reload_transition_lines:-}" "${mapgen_generation_lines:-}" \
					"${mapgen_generation_unique_seed_count:-}" "${mapgen_reload_regen_ok:-}" >> "$CSV_PATH"
		done
	done
fi

if command -v python3 >/dev/null 2>&1; then
	python3 "$HEATMAP" --input "$CSV_PATH" --output "$OUTDIR/mapgen_heatmap.html"
	log "Heatmap written to $OUTDIR/mapgen_heatmap.html"
	if [[ -f "$AGGREGATE" ]]; then
		python3 "$AGGREGATE" --output "$OUTDIR/smoke_aggregate_report.html" --mapgen-csv "$CSV_PATH"
		log "Aggregate report written to $OUTDIR/smoke_aggregate_report.html"
	fi
else
	log "python3 not found; skipped heatmap generation"
fi

log "CSV written to $CSV_PATH"
log "Completed $total_runs run(s) with $failures failure(s)"

if (( failures > 0 )); then
	exit 1
fi
