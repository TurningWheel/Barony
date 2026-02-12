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
MAPGEN_CONTROL_FILE=""
MAPGEN_RELOAD_SAME_LEVEL=0
MAPGEN_RELOAD_SEED_BASE=0
START_FLOOR=0
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
AUTO_KICK_TARGET_SLOT=0
AUTO_KICK_DELAY_SECS=2
REQUIRE_AUTO_KICK=0
TRACE_SLOT_LOCKS=0
REQUIRE_DEFAULT_SLOT_LOCKS=0
AUTO_PLAYER_COUNT_TARGET=0
AUTO_PLAYER_COUNT_DELAY_SECS=2
TRACE_PLAYER_COUNT_COPY=0
REQUIRE_PLAYER_COUNT_COPY=0
EXPECT_PLAYER_COUNT_COPY_VARIANT=""
TRACE_LOBBY_PAGE_STATE=0
REQUIRE_LOBBY_PAGE_STATE=0
REQUIRE_LOBBY_PAGE_FOCUS_MATCH=0
AUTO_LOBBY_PAGE_SWEEP=0
AUTO_LOBBY_PAGE_DELAY_SECS=2
REQUIRE_LOBBY_PAGE_SWEEP=0
TRACE_REMOTE_COMBAT_SLOT_BOUNDS=0
REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS=0
REQUIRE_REMOTE_COMBAT_EVENTS=0
AUTO_PAUSE_PULSES=0
AUTO_PAUSE_DELAY_SECS=2
AUTO_PAUSE_HOLD_SECS=1
AUTO_REMOTE_COMBAT_PULSES=0
AUTO_REMOTE_COMBAT_DELAY_SECS=2
KEEP_RUNNING=0
SEED_CONFIG_PATH="$HOME/.barony/config/config.json"
SEED_BOOKS_PATH="$HOME/.barony/books/compiled_books.json"

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
  --connect-address <addr>      Client connect target. LAN uses host:port; online uses lobby key (for example S1234).
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
  --mapgen-control-file <path>  Optional file read at mapgen-time for dynamic player override.
                                If present and valid (1..15), this overrides --mapgen-players-override.
  --mapgen-reload-same-level <0|1>
                                Smoke-only gameplay autopilot reloads the same dungeon level between samples.
  --mapgen-reload-seed-base <n> Base seed used for same-level reload samples (0 disables forced seed rotation).
  --start-floor <n>            Smoke-only host start floor (0..99, default: 0).
  --helo-chunk-tx-mode <mode>   HELO chunk send mode: normal, reverse, even-odd,
                                duplicate-first, drop-last, duplicate-conflict-first.
  --network-backend <name>      Network backend to execute (lan|steam|eos; default: lan).
  --strict-adversarial <0|1>    Enable strict adversarial assertions.
  --require-txmode-log <0|1>    Require tx-mode host logs in non-normal tx modes.
  --seed <value>                Optional seed string for host run.
  --require-helo <0|1>          Require HELO chunk/reassembly checks.
  --require-mapgen <0|1>        Require dungeon mapgen summary in host log.
  --mapgen-samples <n>          Required number of mapgen summary lines (default: 1).
  --trace-account-labels <0|1>  Emit smoke logs for resolved lobby account labels (host only).
  --require-account-labels <0|1>
                                Require account-label coverage for remote slots.
  --auto-kick-target-slot <n>   Host smoke autopilot kicks this player slot (1..14, 0 disables).
  --auto-kick-delay <sec>       Delay after full lobby before auto-kick (default: 2).
  --require-auto-kick <0|1>     Require smoke auto-kick verification before pass.
  --trace-slot-locks <0|1>      Emit smoke slot-lock snapshots during lobby initialization.
  --require-default-slot-locks <0|1>
                                Require default host slot-lock snapshot assertions.
  --auto-player-count-target <n>
                                Host smoke autopilot requests this lobby player-count target (2..15, 0 disables).
  --auto-player-count-delay <sec>
                                Delay after full lobby before host requests player-count change.
  --trace-player-count-copy <0|1>
                                Emit smoke logs for occupied-slot count-reduction kick prompt copy.
  --require-player-count-copy <0|1>
                                Require player-count prompt trace before pass.
  --expect-player-count-copy-variant <name>
                                Expected prompt variant when requiring player-count copy:
                                single, double, multi, warning-only, none.
  --trace-lobby-page-state <0|1>
                                Emit smoke logs for lobby page/focus/alignment snapshots while paging.
  --require-lobby-page-state <0|1>
                                Require lobby page alignment snapshot assertions before pass.
  --require-lobby-page-focus-match <0|1>
                                Require focused widget page match for traced lobby snapshots.
  --auto-lobby-page-sweep <0|1> Host smoke autopilot sweeps visible lobby pages after full lobby.
  --auto-lobby-page-delay <sec> Delay between host auto page changes (default: 2).
  --require-lobby-page-sweep <0|1>
                                Require full visible-page sweep coverage before pass.
  --trace-remote-combat-slot-bounds <0|1>
                                Emit smoke logs for remote-combat slot bound checks/events.
  --require-remote-combat-slot-bounds <0|1>
                                Require zero remote-combat slot-check failures and at least one success.
  --require-remote-combat-events <0|1>
                                Require remote-combat event coverage (pause/unpause and enemy-bar pulses).
  --auto-pause-pulses <n>       Host smoke autopilot issues pause/unpause pulses (0 disables).
  --auto-pause-delay <sec>      Delay before each pause pulse (default: 2).
  --auto-pause-hold <sec>       Pause hold duration before unpause (default: 1).
  --auto-remote-combat-pulses <n>
                                Host smoke autopilot triggers enemy-bar combat pulses (0 disables).
  --auto-remote-combat-delay <sec>
                                Delay between host enemy-bar combat pulses (default: 2).
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

seed_smoke_home_profile() {
	local home_dir="$1"
	local seed_root="$home_dir/.barony"
	local config_dest="$seed_root/config/config.json"
	local wrote_config=0

	if [[ -f "$SEED_CONFIG_PATH" ]]; then
		mkdir -p "$seed_root/config"
		if command -v jq >/dev/null 2>&1; then
			if ! jq '.skipintro = true | .mods = []' "$SEED_CONFIG_PATH" > "$config_dest" 2>/dev/null; then
				cp "$SEED_CONFIG_PATH" "$config_dest"
			fi
		else
			cp "$SEED_CONFIG_PATH" "$config_dest"
		fi
		wrote_config=1
	fi

	if (( wrote_config == 0 )); then
		mkdir -p "$seed_root/config"
		cat > "$config_dest" <<'JSON'
{"skipintro":true,"mods":[]}
JSON
	fi

	if [[ -f "$SEED_BOOKS_PATH" ]]; then
		mkdir -p "$seed_root/books"
		cp "$SEED_BOOKS_PATH" "$seed_root/books/compiled_books.json"
	fi
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

count_regex_lines() {
	local file="$1"
	local pattern="$2"
	if [[ ! -f "$file" ]]; then
		echo 0
		return
	fi
	rg -c "$pattern" "$file" 2>/dev/null || echo 0
}

count_fixed_lines_across_logs() {
	local needle="$1"
	shift
	local total=0
	local file
	for file in "$@"; do
		if [[ ! -f "$file" ]]; then
			continue
		fi
		local count
		count="$(rg -F -c "$needle" "$file" 2>/dev/null || echo 0)"
		total=$((total + count))
	done
	echo "$total"
}

count_regex_lines_across_logs() {
	local pattern="$1"
	shift
	local total=0
	local file
	for file in "$@"; do
		if [[ ! -f "$file" ]]; then
			continue
		fi
		local count
		count="$(rg -c "$pattern" "$file" 2>/dev/null || echo 0)"
		total=$((total + count))
	done
	echo "$total"
}

extract_smoke_room_key() {
	local host_log="$1"
	local backend="$2"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	local line
	line="$(rg -F "[SMOKE]: lobby room key backend=${backend} key=" "$host_log" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	echo "$line" | sed -nE 's/.* key=([^ ]+).*/\1/p'
}

collect_remote_combat_event_contexts() {
	if (($# == 0)); then
		echo ""
		return
	fi
	local contexts=""
	local file
	for file in "$@"; do
		if [[ ! -f "$file" ]]; then
			continue
		fi
		local line
		while IFS= read -r line; do
			[[ -z "$line" ]] && continue
			local context
			context="$(echo "$line" | sed -nE 's/.*context=([^ ]+).*/\1/p')"
			if [[ -z "$context" ]]; then
				continue
			fi
			contexts+="${context}"$'\n'
		done < <(rg -F "[SMOKE]: remote-combat event context=" "$file" || true)
	done
	if [[ -z "$contexts" ]]; then
		echo ""
		return
	fi
	printf '%s' "$contexts" | sed '/^$/d' | sort -u | paste -sd';' -
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

is_default_slot_lock_snapshot_ok() {
	local host_log="$1"
	local expected_players="$2"
	if [[ ! -f "$host_log" ]]; then
		echo 0
		return
	fi
	local line
	line="$(rg -F "[SMOKE]: lobby slot-lock snapshot context=lobby-init " "$host_log" | head -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo 0
		return
	fi
	local configured free_unlocked free_locked occupied
	configured="$(echo "$line" | sed -nE 's/.*configured=([0-9]+).*/\1/p')"
	free_unlocked="$(echo "$line" | sed -nE 's/.*free_unlocked=([0-9]+).*/\1/p')"
	free_locked="$(echo "$line" | sed -nE 's/.*free_locked=([0-9]+).*/\1/p')"
	occupied="$(echo "$line" | sed -nE 's/.*occupied=([0-9]+).*/\1/p')"
	if [[ -z "$configured" || -z "$free_unlocked" || -z "$free_locked" || -z "$occupied" ]]; then
		echo 0
		return
	fi
	local expected_unlocked=$(( expected_players > 0 ? expected_players - 1 : 0 ))
	local max_remote_slots=$(( 15 - 1 ))
	local expected_locked=$(( max_remote_slots - expected_unlocked ))
	if (( expected_locked < 0 )); then
		expected_locked=0
	fi
	if (( configured == expected_players && free_unlocked == expected_unlocked && free_locked == expected_locked && occupied == 0 )); then
		echo 1
	else
		echo 0
	fi
}

collect_player_count_prompt_variants() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	local variants
	variants="$(rg -o 'lobby player-count prompt target=[0-9]+ kicked=[0-9]+ variant=[a-z-]+' "$host_log" \
		| sed -nE 's/.*variant=([a-z-]+).*/\1/p' \
		| sort -u \
		| paste -sd';' - || true)"
	echo "$variants"
}

extract_last_player_count_prompt_field() {
	local host_log="$1"
	local key="$2"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	local line
	line="$(rg -F "[SMOKE]: lobby player-count prompt target=" "$host_log" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo ""
		return
	fi
	case "$key" in
		target)
			echo "$line" | sed -nE 's/.*target=([0-9]+).*/\1/p'
			;;
		kicked)
			echo "$line" | sed -nE 's/.*kicked=([0-9]+).*/\1/p'
			;;
		variant)
			echo "$line" | sed -nE 's/.*variant=([a-z-]+).*/\1/p'
			;;
		*)
			echo ""
			;;
	esac
}

collect_lobby_page_snapshot_metrics() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo "0|0|0||0|0|0|0|0|0|0|0"
		return
	fi

	local lines
	lines="$(rg -F "[SMOKE]: lobby page snapshot context=visible-page " "$host_log" || true)"
	if [[ -z "$lines" ]]; then
		echo "0|0|0||0|0|0|0|0|0|0|0"
		return
	fi

	local snapshot_lines=0
	local pages_seen=""
	local page_count_total=0
	local focus_mismatch_lines=0
	local cards_misaligned_max=0
	local paperdolls_misaligned_max=0
	local pings_misaligned_max=0
	local warnings_present_lines=0
	local warnings_max_abs_delta=0
	local countdown_present_lines=0
	local countdown_max_abs_delta=0

	local line
	while IFS= read -r line; do
		[[ -z "$line" ]] && continue
		snapshot_lines=$((snapshot_lines + 1))

		local page page_total
		page="$(echo "$line" | sed -nE 's/.*page=([0-9]+)\/([0-9]+).*/\1/p')"
		page_total="$(echo "$line" | sed -nE 's/.*page=([0-9]+)\/([0-9]+).*/\2/p')"
		if [[ -n "$page_total" ]]; then
			page_count_total="$page_total"
		fi
		if [[ -n "$page" ]]; then
			if [[ ";$pages_seen;" != *";$page;"* ]]; then
				if [[ -n "$pages_seen" ]]; then
					pages_seen+=";"
				fi
				pages_seen+="$page"
			fi
		fi

		local focus_match cards_misaligned paperdolls_misaligned pings_misaligned
		focus_match="$(echo "$line" | sed -nE 's/.*focus_page_match=([0-9]+).*/\1/p')"
		cards_misaligned="$(echo "$line" | sed -nE 's/.*cards_misaligned=([0-9]+).*/\1/p')"
		paperdolls_misaligned="$(echo "$line" | sed -nE 's/.*paperdolls_misaligned=([0-9]+).*/\1/p')"
		pings_misaligned="$(echo "$line" | sed -nE 's/.*pings_misaligned=([0-9]+).*/\1/p')"

		if [[ "$focus_match" == "0" ]]; then
			focus_mismatch_lines=$((focus_mismatch_lines + 1))
		fi
		if [[ -n "$cards_misaligned" ]] && (( cards_misaligned > cards_misaligned_max )); then
			cards_misaligned_max="$cards_misaligned"
		fi
		if [[ -n "$paperdolls_misaligned" ]] && (( paperdolls_misaligned > paperdolls_misaligned_max )); then
			paperdolls_misaligned_max="$paperdolls_misaligned"
		fi
		if [[ -n "$pings_misaligned" ]] && (( pings_misaligned > pings_misaligned_max )); then
			pings_misaligned_max="$pings_misaligned"
		fi

		local warnings_delta countdown_delta
		warnings_delta="$(echo "$line" | sed -nE 's/.*warnings_center_delta=(-?[0-9]+).*/\1/p')"
		countdown_delta="$(echo "$line" | sed -nE 's/.*countdown_center_delta=(-?[0-9]+).*/\1/p')"

		if [[ -n "$warnings_delta" ]] && (( warnings_delta != 9999 )); then
			warnings_present_lines=$((warnings_present_lines + 1))
			local abs_warning="$warnings_delta"
			if (( abs_warning < 0 )); then
				abs_warning=$(( -abs_warning ))
			fi
			if (( abs_warning > warnings_max_abs_delta )); then
				warnings_max_abs_delta="$abs_warning"
			fi
		fi
		if [[ -n "$countdown_delta" ]] && (( countdown_delta != 9999 )); then
			countdown_present_lines=$((countdown_present_lines + 1))
			local abs_countdown="$countdown_delta"
			if (( abs_countdown < 0 )); then
				abs_countdown=$(( -abs_countdown ))
			fi
			if (( abs_countdown > countdown_max_abs_delta )); then
				countdown_max_abs_delta="$abs_countdown"
			fi
		fi
	done <<< "$lines"

	local unique_pages_count=0
	if [[ -n "$pages_seen" ]]; then
		unique_pages_count="$(echo "$pages_seen" | awk -F';' '{print NF}')"
	fi

	echo "${snapshot_lines}|${unique_pages_count}|${page_count_total}|${pages_seen}|${focus_mismatch_lines}|${cards_misaligned_max}|${paperdolls_misaligned_max}|${pings_misaligned_max}|${warnings_present_lines}|${warnings_max_abs_delta}|${countdown_present_lines}|${countdown_max_abs_delta}"
}

extract_mapgen_metrics() {
	local host_log="$1"
	local line
	local food_line
	local decoration_line
	line="$(rg -F "successfully generated a dungeon with" "$host_log" | tail -n 1 || true)"
	food_line="$(rg -F "mapgen food summary:" "$host_log" | tail -n 1 || true)"
	decoration_line="$(rg -F "mapgen decoration summary:" "$host_log" | tail -n 1 || true)"
	if [[ -z "$line" ]]; then
		echo "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
		return
	fi
	if [[ "$line" =~ with[[:space:]]+([0-9]+)[[:space:]]+rooms,[[:space:]]+([0-9]+)[[:space:]]+monsters,[[:space:]]+([0-9]+)[[:space:]]+gold,[[:space:]]+([0-9]+)[[:space:]]+items,[[:space:]]+([0-9]+)[[:space:]]+decorations ]]; then
		local rooms="${BASH_REMATCH[1]}"
		local monsters="${BASH_REMATCH[2]}"
		local gold="${BASH_REMATCH[3]}"
		local items="${BASH_REMATCH[4]}"
		local decorations="${BASH_REMATCH[5]}"
		local decor_blocking=0
		local decor_utility=0
		local decor_traps=0
		local decor_economy=0
		local food_items=0
		local food_servings=0
		local mapgen_level=0
		local mapgen_secret=0
		local mapgen_seed=0
		if [[ "$line" =~ level=([0-9]+) ]]; then
			mapgen_level="${BASH_REMATCH[1]}"
		fi
		if [[ "$line" =~ secret=([0-9]+) ]]; then
			mapgen_secret="${BASH_REMATCH[1]}"
		fi
		if [[ "$food_line" =~ food=([0-9]+) ]]; then
			food_items="${BASH_REMATCH[1]}"
		fi
		if [[ "$food_line" =~ food_servings=([0-9]+) ]]; then
			food_servings="${BASH_REMATCH[1]}"
		fi
		if [[ "$decoration_line" =~ blocking=([0-9]+) ]]; then
			decor_blocking="${BASH_REMATCH[1]}"
		fi
		if [[ "$decoration_line" =~ utility=([0-9]+) ]]; then
			decor_utility="${BASH_REMATCH[1]}"
		fi
		if [[ "$decoration_line" =~ traps=([0-9]+) ]]; then
			decor_traps="${BASH_REMATCH[1]}"
		fi
		if [[ "$decoration_line" =~ economy=([0-9]+) ]]; then
			decor_economy="${BASH_REMATCH[1]}"
		fi
		if [[ "$line" =~ seed=([0-9]+) ]]; then
			mapgen_seed="${BASH_REMATCH[1]}"
		fi
		echo "1 ${rooms} ${monsters} ${gold} ${items} ${decorations} ${decor_blocking} ${decor_utility} ${decor_traps} ${decor_economy} ${food_items} ${food_servings} ${mapgen_level} ${mapgen_secret} ${mapgen_seed}"
	else
		echo "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
	fi
}

collect_mapgen_generation_seeds() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	rg -F "generating a dungeon from level set '" "$host_log" \
		| sed -nE 's/.*\(seed ([0-9]+)\).*/\1/p' \
		| paste -sd';' - || true
}

collect_reload_transition_seeds() {
	local host_log="$1"
	if [[ ! -f "$host_log" ]]; then
		echo ""
		return
	fi
	rg -F "[SMOKE]: auto-reloading dungeon level transition" "$host_log" \
		| sed -nE 's/.* seed=([0-9]+).*/\1/p' \
		| paste -sd';' - || true
}

count_list_values() {
	local values="$1"
	if [[ -z "$values" ]]; then
		echo 0
		return
	fi
	echo "$values" | tr ';' '\n' | awk 'NF { ++count } END { print count + 0 }'
}

count_unique_list_values() {
	local values="$1"
	if [[ -z "$values" ]]; then
		echo 0
		return
	fi
	echo "$values" | tr ';' '\n' | awk 'NF { seen[$0] = 1 } END { print length(seen) + 0 }'
}

count_seed_matches() {
	local expected_seeds="$1"
	local observed_seeds="$2"
	if [[ -z "$expected_seeds" || -z "$observed_seeds" ]]; then
		echo 0
		return
	fi
	awk -v expected="$expected_seeds" -v observed="$observed_seeds" '
	BEGIN {
		n_observed = split(observed, observed_list, ";");
		for (i = 1; i <= n_observed; ++i) {
			if (length(observed_list[i]) > 0) {
				seen[observed_list[i]] = 1;
			}
		}
		n_expected = split(expected, expected_list, ";");
		matches = 0;
		for (i = 1; i <= n_expected; ++i) {
			if (length(expected_list[i]) > 0 && seen[expected_list[i]]) {
				++matches;
			}
		}
		print matches + 0;
	}'
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
		--mapgen-control-file)
			MAPGEN_CONTROL_FILE="${2:-}"
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
		--start-floor)
			START_FLOOR="${2:-}"
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
		--auto-kick-target-slot)
			AUTO_KICK_TARGET_SLOT="${2:-}"
			shift 2
			;;
		--auto-kick-delay)
			AUTO_KICK_DELAY_SECS="${2:-}"
			shift 2
			;;
		--require-auto-kick)
			REQUIRE_AUTO_KICK="${2:-}"
			shift 2
			;;
		--trace-slot-locks)
			TRACE_SLOT_LOCKS="${2:-}"
			shift 2
			;;
		--require-default-slot-locks)
			REQUIRE_DEFAULT_SLOT_LOCKS="${2:-}"
			shift 2
			;;
		--auto-player-count-target)
			AUTO_PLAYER_COUNT_TARGET="${2:-}"
			shift 2
			;;
		--auto-player-count-delay)
			AUTO_PLAYER_COUNT_DELAY_SECS="${2:-}"
			shift 2
			;;
		--trace-player-count-copy)
			TRACE_PLAYER_COUNT_COPY="${2:-}"
			shift 2
			;;
		--require-player-count-copy)
			REQUIRE_PLAYER_COUNT_COPY="${2:-}"
			shift 2
			;;
		--expect-player-count-copy-variant)
			EXPECT_PLAYER_COUNT_COPY_VARIANT="${2:-}"
			shift 2
			;;
		--trace-lobby-page-state)
			TRACE_LOBBY_PAGE_STATE="${2:-}"
			shift 2
			;;
		--require-lobby-page-state)
			REQUIRE_LOBBY_PAGE_STATE="${2:-}"
			shift 2
			;;
		--require-lobby-page-focus-match)
			REQUIRE_LOBBY_PAGE_FOCUS_MATCH="${2:-}"
			shift 2
			;;
		--auto-lobby-page-sweep)
			AUTO_LOBBY_PAGE_SWEEP="${2:-}"
			shift 2
			;;
		--auto-lobby-page-delay)
			AUTO_LOBBY_PAGE_DELAY_SECS="${2:-}"
			shift 2
			;;
		--require-lobby-page-sweep)
			REQUIRE_LOBBY_PAGE_SWEEP="${2:-}"
			shift 2
			;;
		--trace-remote-combat-slot-bounds)
			TRACE_REMOTE_COMBAT_SLOT_BOUNDS="${2:-}"
			shift 2
			;;
		--require-remote-combat-slot-bounds)
			REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS="${2:-}"
			shift 2
			;;
		--require-remote-combat-events)
			REQUIRE_REMOTE_COMBAT_EVENTS="${2:-}"
			shift 2
			;;
		--auto-pause-pulses)
			AUTO_PAUSE_PULSES="${2:-}"
			shift 2
			;;
		--auto-pause-delay)
			AUTO_PAUSE_DELAY_SECS="${2:-}"
			shift 2
			;;
		--auto-pause-hold)
			AUTO_PAUSE_HOLD_SECS="${2:-}"
			shift 2
			;;
		--auto-remote-combat-pulses)
			AUTO_REMOTE_COMBAT_PULSES="${2:-}"
			shift 2
			;;
		--auto-remote-combat-delay)
			AUTO_REMOTE_COMBAT_DELAY_SECS="${2:-}"
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
if [[ -n "$MAPGEN_CONTROL_FILE" ]]; then
	MAPGEN_CONTROL_FILE="${MAPGEN_CONTROL_FILE/#\~/$HOME}"
fi
if ! is_uint "$MAPGEN_RELOAD_SAME_LEVEL" || (( MAPGEN_RELOAD_SAME_LEVEL > 1 )); then
	echo "--mapgen-reload-same-level must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$MAPGEN_RELOAD_SEED_BASE"; then
	echo "--mapgen-reload-seed-base must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$START_FLOOR" || (( START_FLOOR > 99 )); then
	echo "--start-floor must be 0..99" >&2
	exit 1
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
if ! is_uint "$AUTO_KICK_TARGET_SLOT" || (( AUTO_KICK_TARGET_SLOT < 0 || AUTO_KICK_TARGET_SLOT > 14 )); then
	echo "--auto-kick-target-slot must be 0..14" >&2
	exit 1
fi
if ! is_uint "$AUTO_KICK_DELAY_SECS"; then
	echo "--auto-kick-delay must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_AUTO_KICK" || (( REQUIRE_AUTO_KICK > 1 )); then
	echo "--require-auto-kick must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_AUTO_KICK )) && (( AUTO_KICK_TARGET_SLOT == 0 )); then
	echo "--require-auto-kick requires --auto-kick-target-slot > 0" >&2
	exit 1
fi
if (( AUTO_KICK_TARGET_SLOT > 0 )) && (( AUTO_KICK_TARGET_SLOT >= EXPECTED_PLAYERS )); then
	echo "--auto-kick-target-slot must be less than --expected-players" >&2
	exit 1
fi
if ! is_uint "$TRACE_SLOT_LOCKS" || (( TRACE_SLOT_LOCKS > 1 )); then
	echo "--trace-slot-locks must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_DEFAULT_SLOT_LOCKS" || (( REQUIRE_DEFAULT_SLOT_LOCKS > 1 )); then
	echo "--require-default-slot-locks must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_DEFAULT_SLOT_LOCKS )) && (( TRACE_SLOT_LOCKS == 0 )); then
	echo "--require-default-slot-locks requires --trace-slot-locks 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_PLAYER_COUNT_TARGET" || (( AUTO_PLAYER_COUNT_TARGET < 0 || AUTO_PLAYER_COUNT_TARGET > 15 )); then
	echo "--auto-player-count-target must be 0..15" >&2
	exit 1
fi
if (( AUTO_PLAYER_COUNT_TARGET > 0 )) && (( AUTO_PLAYER_COUNT_TARGET < 2 )); then
	echo "--auto-player-count-target must be 2..15 when enabled" >&2
	exit 1
fi
if ! is_uint "$AUTO_PLAYER_COUNT_DELAY_SECS"; then
	echo "--auto-player-count-delay must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$TRACE_PLAYER_COUNT_COPY" || (( TRACE_PLAYER_COUNT_COPY > 1 )); then
	echo "--trace-player-count-copy must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_PLAYER_COUNT_COPY" || (( REQUIRE_PLAYER_COUNT_COPY > 1 )); then
	echo "--require-player-count-copy must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_PLAYER_COUNT_COPY )) && (( TRACE_PLAYER_COUNT_COPY == 0 )); then
	echo "--require-player-count-copy requires --trace-player-count-copy 1" >&2
	exit 1
fi
if (( REQUIRE_PLAYER_COUNT_COPY )) && (( AUTO_PLAYER_COUNT_TARGET == 0 )); then
	echo "--require-player-count-copy requires --auto-player-count-target > 0" >&2
	exit 1
fi
if [[ -n "$EXPECT_PLAYER_COUNT_COPY_VARIANT" ]]; then
	case "$EXPECT_PLAYER_COUNT_COPY_VARIANT" in
		single|double|multi|warning-only|none)
			;;
		*)
			echo "--expect-player-count-copy-variant must be one of: single, double, multi, warning-only, none" >&2
			exit 1
			;;
	esac
	if (( TRACE_PLAYER_COUNT_COPY == 0 )); then
		echo "--expect-player-count-copy-variant requires --trace-player-count-copy 1" >&2
		exit 1
	fi
	if (( AUTO_PLAYER_COUNT_TARGET == 0 )); then
		echo "--expect-player-count-copy-variant requires --auto-player-count-target > 0" >&2
		exit 1
	fi
fi
if ! is_uint "$TRACE_LOBBY_PAGE_STATE" || (( TRACE_LOBBY_PAGE_STATE > 1 )); then
	echo "--trace-lobby-page-state must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_LOBBY_PAGE_STATE" || (( REQUIRE_LOBBY_PAGE_STATE > 1 )); then
	echo "--require-lobby-page-state must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_LOBBY_PAGE_FOCUS_MATCH" || (( REQUIRE_LOBBY_PAGE_FOCUS_MATCH > 1 )); then
	echo "--require-lobby-page-focus-match must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_LOBBY_PAGE_STATE )) && (( TRACE_LOBBY_PAGE_STATE == 0 )); then
	echo "--require-lobby-page-state requires --trace-lobby-page-state 1" >&2
	exit 1
fi
if (( REQUIRE_LOBBY_PAGE_FOCUS_MATCH )) && (( TRACE_LOBBY_PAGE_STATE == 0 )); then
	echo "--require-lobby-page-focus-match requires --trace-lobby-page-state 1" >&2
	exit 1
fi
if (( REQUIRE_LOBBY_PAGE_FOCUS_MATCH )) && (( REQUIRE_LOBBY_PAGE_STATE == 0 )); then
	echo "--require-lobby-page-focus-match requires --require-lobby-page-state 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_LOBBY_PAGE_SWEEP" || (( AUTO_LOBBY_PAGE_SWEEP > 1 )); then
	echo "--auto-lobby-page-sweep must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_LOBBY_PAGE_DELAY_SECS"; then
	echo "--auto-lobby-page-delay must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_LOBBY_PAGE_SWEEP" || (( REQUIRE_LOBBY_PAGE_SWEEP > 1 )); then
	echo "--require-lobby-page-sweep must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_LOBBY_PAGE_SWEEP )) && (( TRACE_LOBBY_PAGE_STATE == 0 )); then
	echo "--require-lobby-page-sweep requires --trace-lobby-page-state 1" >&2
	exit 1
fi
if (( REQUIRE_LOBBY_PAGE_SWEEP )) && (( AUTO_LOBBY_PAGE_SWEEP == 0 )); then
	echo "--require-lobby-page-sweep requires --auto-lobby-page-sweep 1" >&2
	exit 1
fi
if ! is_uint "$TRACE_REMOTE_COMBAT_SLOT_BOUNDS" || (( TRACE_REMOTE_COMBAT_SLOT_BOUNDS > 1 )); then
	echo "--trace-remote-combat-slot-bounds must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS" || (( REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS > 1 )); then
	echo "--require-remote-combat-slot-bounds must be 0 or 1" >&2
	exit 1
fi
if ! is_uint "$REQUIRE_REMOTE_COMBAT_EVENTS" || (( REQUIRE_REMOTE_COMBAT_EVENTS > 1 )); then
	echo "--require-remote-combat-events must be 0 or 1" >&2
	exit 1
fi
if (( REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS || REQUIRE_REMOTE_COMBAT_EVENTS )) && (( TRACE_REMOTE_COMBAT_SLOT_BOUNDS == 0 )); then
	echo "--require-remote-combat-slot-bounds/--require-remote-combat-events require --trace-remote-combat-slot-bounds 1" >&2
	exit 1
fi
if ! is_uint "$AUTO_PAUSE_PULSES" || (( AUTO_PAUSE_PULSES < 0 || AUTO_PAUSE_PULSES > 64 )); then
	echo "--auto-pause-pulses must be 0..64" >&2
	exit 1
fi
if ! is_uint "$AUTO_PAUSE_DELAY_SECS"; then
	echo "--auto-pause-delay must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$AUTO_PAUSE_HOLD_SECS"; then
	echo "--auto-pause-hold must be a non-negative integer" >&2
	exit 1
fi
if ! is_uint "$AUTO_REMOTE_COMBAT_PULSES" || (( AUTO_REMOTE_COMBAT_PULSES < 0 || AUTO_REMOTE_COMBAT_PULSES > 64 )); then
	echo "--auto-remote-combat-pulses must be 0..64" >&2
	exit 1
fi
if ! is_uint "$AUTO_REMOTE_COMBAT_DELAY_SECS"; then
	echo "--auto-remote-combat-delay must be a non-negative integer" >&2
	exit 1
fi
if (( REQUIRE_REMOTE_COMBAT_EVENTS )) && (( AUTO_PAUSE_PULSES == 0 && AUTO_REMOTE_COMBAT_PULSES == 0 )); then
	echo "--require-remote-combat-events requires at least one of --auto-pause-pulses or --auto-remote-combat-pulses" >&2
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
	seed_smoke_home_profile "$home_dir"

	local -a env_vars=()
	if [[ "$NETWORK_BACKEND" == "lan" ]]; then
		env_vars+=("HOME=$home_dir")
	fi
	env_vars+=(
		"BARONY_SMOKE_AUTOPILOT=1"
		"BARONY_SMOKE_NETWORK_BACKEND=$NETWORK_BACKEND"
		"BARONY_SMOKE_CONNECT_DELAY_SECS=2"
		"BARONY_SMOKE_RETRY_DELAY_SECS=3"
		"BARONY_SMOKE_FORCE_HELO_CHUNK=$FORCE_CHUNK"
		"BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
		"BARONY_SMOKE_HELO_CHUNK_TX_MODE=$HELO_CHUNK_TX_MODE"
	)
	if (( TRACE_REMOTE_COMBAT_SLOT_BOUNDS )); then
		env_vars+=("BARONY_SMOKE_TRACE_REMOTE_COMBAT_SLOT_BOUNDS=1")
	fi

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
		if (( AUTO_KICK_TARGET_SLOT > 0 )); then
			env_vars+=(
				"BARONY_SMOKE_AUTO_KICK_TARGET_SLOT=$AUTO_KICK_TARGET_SLOT"
				"BARONY_SMOKE_AUTO_KICK_DELAY_SECS=$AUTO_KICK_DELAY_SECS"
			)
		fi
		if (( TRACE_ACCOUNT_LABELS )); then
			env_vars+=("BARONY_SMOKE_TRACE_ACCOUNT_LABELS=1")
		fi
		if (( TRACE_SLOT_LOCKS )); then
			env_vars+=("BARONY_SMOKE_TRACE_SLOT_LOCKS=1")
		fi
		if (( TRACE_PLAYER_COUNT_COPY )); then
			env_vars+=("BARONY_SMOKE_TRACE_PLAYER_COUNT_COPY=1")
		fi
		if (( TRACE_LOBBY_PAGE_STATE )); then
			env_vars+=("BARONY_SMOKE_TRACE_LOBBY_PAGE_STATE=1")
		fi
		if (( AUTO_PLAYER_COUNT_TARGET > 0 )); then
			env_vars+=(
				"BARONY_SMOKE_AUTO_PLAYER_COUNT_TARGET=$AUTO_PLAYER_COUNT_TARGET"
				"BARONY_SMOKE_AUTO_PLAYER_COUNT_DELAY_SECS=$AUTO_PLAYER_COUNT_DELAY_SECS"
			)
		fi
		if (( AUTO_LOBBY_PAGE_SWEEP )); then
			env_vars+=(
				"BARONY_SMOKE_AUTO_LOBBY_PAGE_SWEEP=1"
				"BARONY_SMOKE_AUTO_LOBBY_PAGE_DELAY_SECS=$AUTO_LOBBY_PAGE_DELAY_SECS"
			)
		fi
		if (( AUTO_PAUSE_PULSES > 0 )); then
			env_vars+=(
				"BARONY_SMOKE_AUTO_PAUSE_PULSES=$AUTO_PAUSE_PULSES"
				"BARONY_SMOKE_AUTO_PAUSE_DELAY_SECS=$AUTO_PAUSE_DELAY_SECS"
				"BARONY_SMOKE_AUTO_PAUSE_HOLD_SECS=$AUTO_PAUSE_HOLD_SECS"
			)
		fi
			if (( AUTO_REMOTE_COMBAT_PULSES > 0 )); then
				env_vars+=(
					"BARONY_SMOKE_AUTO_REMOTE_COMBAT_PULSES=$AUTO_REMOTE_COMBAT_PULSES"
					"BARONY_SMOKE_AUTO_REMOTE_COMBAT_DELAY_SECS=$AUTO_REMOTE_COMBAT_DELAY_SECS"
				)
			fi
			if [[ -n "$MAPGEN_PLAYERS_OVERRIDE" ]]; then
				env_vars+=("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS=$MAPGEN_PLAYERS_OVERRIDE")
			fi
			if [[ -n "$MAPGEN_CONTROL_FILE" ]]; then
				env_vars+=("BARONY_SMOKE_MAPGEN_CONTROL_FILE=$MAPGEN_CONTROL_FILE")
			fi
			if (( MAPGEN_RELOAD_SAME_LEVEL )); then
				env_vars+=("BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL=1")
				if (( MAPGEN_RELOAD_SEED_BASE > 0 )); then
					env_vars+=("BARONY_SMOKE_MAPGEN_RELOAD_SEED_BASE=$MAPGEN_RELOAD_SEED_BASE")
				fi
			fi
			if (( START_FLOOR > 0 )); then
				env_vars+=("BARONY_SMOKE_START_FLOOR=$START_FLOOR")
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
HOST_LOG="$INSTANCE_ROOT/home-1/.barony/log.txt"
HOST_STDOUT_LOG="$LOG_DIR/instance-1.stdout.log"
if [[ "$NETWORK_BACKEND" != "lan" ]]; then
	HOST_LOG="$HOST_STDOUT_LOG"
fi
backend_room_key=""
backend_room_key_found=1
backend_launch_blocked=0

if [[ "$NETWORK_BACKEND" == "lan" ]]; then
	for ((i = 1; i <= INSTANCES; ++i)); do
		if (( i == 1 )); then
			launch_instance "$i" "host"
		else
			launch_instance "$i" "client"
		fi
		sleep "$STAGGER_SECONDS"
	done
else
	launch_instance "1" "host"
	sleep "$STAGGER_SECONDS"
	room_key_wait_timeout="$TIMEOUT_SECONDS"
	if (( room_key_wait_timeout > 180 )); then
		room_key_wait_timeout=180
	fi
	log "Waiting up to ${room_key_wait_timeout}s for ${NETWORK_BACKEND} room key"
	room_key_deadline=$((SECONDS + room_key_wait_timeout))
	while (( SECONDS < room_key_deadline )); do
		backend_room_key="$(extract_smoke_room_key "$HOST_LOG" "$NETWORK_BACKEND")"
		if [[ -n "$backend_room_key" ]]; then
			break
		fi
		sleep 1
	done
	if [[ -z "$backend_room_key" ]]; then
		backend_room_key_found=0
		backend_launch_blocked=1
		log "Failed to capture ${NETWORK_BACKEND} room key from host log"
	else
		CONNECT_ADDRESS="$backend_room_key"
		log "Using ${NETWORK_BACKEND} room key ${CONNECT_ADDRESS} for client joins"
		for ((i = 2; i <= INSTANCES; ++i)); do
			launch_instance "$i" "client"
			sleep "$STAGGER_SECONDS"
		done
	fi
fi

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
auto_kick_ok=1
auto_kick_ok_lines=0
auto_kick_fail_lines=0
slot_lock_snapshot_lines=0
default_slot_lock_ok=1
player_count_prompt_lines=0
player_count_prompt_variants=""
player_count_prompt_target=""
player_count_prompt_kicked=""
player_count_prompt_variant=""
player_count_copy_ok=1
lobby_page_snapshot_lines=0
lobby_page_unique_count=0
lobby_page_total_count=0
lobby_page_visited=""
lobby_focus_mismatch_lines=0
lobby_cards_misaligned_max=0
lobby_paperdolls_misaligned_max=0
lobby_pings_misaligned_max=0
lobby_warnings_present_lines=0
lobby_warnings_max_abs_delta=0
lobby_countdown_present_lines=0
lobby_countdown_max_abs_delta=0
lobby_page_state_ok=1
lobby_page_sweep_ok=1
remote_combat_slot_ok_lines=0
remote_combat_slot_fail_lines=0
remote_combat_event_lines=0
remote_combat_event_contexts=""
remote_combat_pause_action_lines=0
remote_combat_pause_complete_lines=0
remote_combat_enemy_bar_action_lines=0
remote_combat_enemy_complete_lines=0
remote_combat_slot_bounds_ok=1
remote_combat_events_ok=1
mapgen_wait_reason="none"
mapgen_reload_complete_tick=0
reload_transition_lines=0

declare -a CLIENT_LOGS=()
for ((i = 2; i <= INSTANCES; ++i)); do
	if [[ "$NETWORK_BACKEND" == "lan" ]]; then
		CLIENT_LOGS+=("$INSTANCE_ROOT/home-${i}/.barony/log.txt")
	else
		CLIENT_LOGS+=("$LOG_DIR/instance-${i}.stdout.log")
	fi
done
declare -a ALL_LOGS=("$HOST_LOG")
if (( INSTANCES > 1 )); then
	for client_log in "${CLIENT_LOGS[@]}"; do
		ALL_LOGS+=("$client_log")
	done
fi

if (( backend_launch_blocked )); then
	log "Skipping handshake wait loop: backend client launch prerequisites were not met"
else
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
	reload_transition_lines=0
	if (( MAPGEN_RELOAD_SAME_LEVEL )); then
		reload_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: auto-reloading dungeon level transition")
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
	if (( REQUIRE_MAPGEN && AUTO_ENTER_DUNGEON && MAPGEN_RELOAD_SAME_LEVEL && AUTO_ENTER_DUNGEON_REPEATS > 0 )); then
		if (( reload_transition_lines >= AUTO_ENTER_DUNGEON_REPEATS && mapgen_count < MAPGEN_SAMPLES )); then
			if (( mapgen_reload_complete_tick == 0 )); then
				mapgen_reload_complete_tick=$SECONDS
			elif (( SECONDS - mapgen_reload_complete_tick >= 5 )); then
				mapgen_wait_reason="reload-complete-no-mapgen-samples"
				break
			fi
		else
			mapgen_reload_complete_tick=0
		fi
	fi
	account_label_ok=1
	if (( REQUIRE_ACCOUNT_LABELS )) && (( EXPECTED_CLIENTS > 0 )); then
		account_label_ok=$(is_account_label_slot_coverage_ok "$HOST_LOG" "$EXPECTED_CLIENTS")
	fi
	auto_kick_ok=1
	if (( AUTO_KICK_TARGET_SLOT > 0 )); then
		auto_kick_ok_lines=$(count_regex_lines "$HOST_LOG" "\\[SMOKE\\]: auto-kick result target=${AUTO_KICK_TARGET_SLOT} .* status=ok")
		auto_kick_fail_lines=$(count_regex_lines "$HOST_LOG" "\\[SMOKE\\]: auto-kick result target=${AUTO_KICK_TARGET_SLOT} .* status=fail")
	fi
	if (( REQUIRE_AUTO_KICK )); then
		if (( auto_kick_ok_lines < 1 || auto_kick_fail_lines > 0 )); then
			auto_kick_ok=0
		fi
	fi
	default_slot_lock_ok=1
	if (( TRACE_SLOT_LOCKS )); then
		slot_lock_snapshot_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: lobby slot-lock snapshot context=lobby-init")
	fi
	if (( REQUIRE_DEFAULT_SLOT_LOCKS )); then
		default_slot_lock_ok=$(is_default_slot_lock_snapshot_ok "$HOST_LOG" "$EXPECTED_PLAYERS")
	fi
	player_count_copy_ok=1
	if (( TRACE_PLAYER_COUNT_COPY )); then
		player_count_prompt_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: lobby player-count prompt target=")
		player_count_prompt_variants="$(collect_player_count_prompt_variants "$HOST_LOG")"
		player_count_prompt_target="$(extract_last_player_count_prompt_field "$HOST_LOG" target)"
		player_count_prompt_kicked="$(extract_last_player_count_prompt_field "$HOST_LOG" kicked)"
		player_count_prompt_variant="$(extract_last_player_count_prompt_field "$HOST_LOG" variant)"
	fi
	if (( REQUIRE_PLAYER_COUNT_COPY )); then
		if (( player_count_prompt_lines < 1 )); then
			player_count_copy_ok=0
		fi
		if [[ -n "$EXPECT_PLAYER_COUNT_COPY_VARIANT" ]] && [[ "$player_count_prompt_variant" != "$EXPECT_PLAYER_COUNT_COPY_VARIANT" ]]; then
			player_count_copy_ok=0
		fi
	fi
	lobby_page_state_ok=1
	lobby_page_sweep_ok=1
	if (( TRACE_LOBBY_PAGE_STATE )); then
		local_page_metrics="$(collect_lobby_page_snapshot_metrics "$HOST_LOG")"
		IFS='|' read -r lobby_page_snapshot_lines lobby_page_unique_count lobby_page_total_count lobby_page_visited \
			lobby_focus_mismatch_lines lobby_cards_misaligned_max lobby_paperdolls_misaligned_max \
			lobby_pings_misaligned_max lobby_warnings_present_lines lobby_warnings_max_abs_delta \
			lobby_countdown_present_lines lobby_countdown_max_abs_delta <<< "$local_page_metrics"
	fi
	if (( REQUIRE_LOBBY_PAGE_STATE )); then
		if (( lobby_page_snapshot_lines < 1 )); then
			lobby_page_state_ok=0
		fi
		if (( lobby_cards_misaligned_max > 0 || lobby_paperdolls_misaligned_max > 0 || lobby_pings_misaligned_max > 0 )); then
			lobby_page_state_ok=0
		fi
		if (( lobby_warnings_present_lines > 0 && lobby_warnings_max_abs_delta > 2 )); then
			lobby_page_state_ok=0
		fi
		if (( lobby_countdown_present_lines > 0 && lobby_countdown_max_abs_delta > 2 )); then
			lobby_page_state_ok=0
		fi
	fi
	if (( REQUIRE_LOBBY_PAGE_FOCUS_MATCH )) && (( lobby_focus_mismatch_lines > 0 )); then
		lobby_page_state_ok=0
	fi
	if (( REQUIRE_LOBBY_PAGE_SWEEP )); then
		if (( lobby_page_total_count < 1 || lobby_page_unique_count < lobby_page_total_count )); then
			lobby_page_sweep_ok=0
		fi
	fi
	remote_combat_slot_bounds_ok=1
	remote_combat_events_ok=1
	if (( TRACE_REMOTE_COMBAT_SLOT_BOUNDS || REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS || REQUIRE_REMOTE_COMBAT_EVENTS )); then
		remote_combat_slot_ok_lines=$(count_regex_lines_across_logs "\\[SMOKE\\]: remote-combat slot-check .* status=ok" "${ALL_LOGS[@]}")
		remote_combat_slot_fail_lines=$(count_regex_lines_across_logs "\\[SMOKE\\]: remote-combat slot-check .* status=fail" "${ALL_LOGS[@]}")
		remote_combat_event_lines=$(count_fixed_lines_across_logs "[SMOKE]: remote-combat event context=" "${ALL_LOGS[@]}")
		remote_combat_event_contexts="$(collect_remote_combat_event_contexts "${ALL_LOGS[@]}")"
		remote_combat_pause_action_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-pause action=")
		remote_combat_pause_complete_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-pause complete pulses=")
		remote_combat_enemy_bar_action_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-event action=enemy-bar")
		remote_combat_enemy_complete_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-event complete pulses=")
	fi
	if (( REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS )); then
		if (( remote_combat_slot_ok_lines < 1 || remote_combat_slot_fail_lines > 0 )); then
			remote_combat_slot_bounds_ok=0
		fi
	fi
	if (( REQUIRE_REMOTE_COMBAT_EVENTS )); then
		if (( remote_combat_event_lines < 1 )); then
			remote_combat_events_ok=0
		fi
		if (( AUTO_PAUSE_PULSES > 0 )); then
			if (( remote_combat_pause_action_lines < AUTO_PAUSE_PULSES * 2 || remote_combat_pause_complete_lines < 1 )); then
				remote_combat_events_ok=0
			fi
			if [[ "$remote_combat_event_contexts" != *"auto-pause-issued"* || "$remote_combat_event_contexts" != *"auto-unpause-issued"* ]]; then
				remote_combat_events_ok=0
			fi
		fi
		if (( AUTO_REMOTE_COMBAT_PULSES > 0 )); then
			if (( remote_combat_enemy_bar_action_lines < AUTO_REMOTE_COMBAT_PULSES || remote_combat_enemy_complete_lines < 1 )); then
				remote_combat_events_ok=0
			fi
			if [[ "$remote_combat_event_contexts" != *"auto-enemy-bar-pulse"* || "$remote_combat_event_contexts" != *"auto-dmgg-pulse"* || "$remote_combat_event_contexts" != *"client-DAMI"* || "$remote_combat_event_contexts" != *"client-DMGG"* ]]; then
				remote_combat_events_ok=0
			fi
			if (( EXPECTED_CLIENTS > 1 )) && [[ "$remote_combat_event_contexts" != *"client-ENHP"* ]]; then
				remote_combat_events_ok=0
			fi
		fi
	fi

	if (( STRICT_EXPECTED_FAIL )); then
		if (( all_clients_zero == 0 )); then
			break
		fi
		if (( chunk_reset_lines > 0 && txmode_ok )); then
			break
		fi
	elif (( helo_ok && mapgen_ok && txmode_ok && account_label_ok && auto_kick_ok && default_slot_lock_ok && player_count_copy_ok && lobby_page_state_ok && lobby_page_sweep_ok && remote_combat_slot_bounds_ok && remote_combat_events_ok )); then
		result="pass"
		break
	fi
	sleep 1
done
fi

game_start_found=$(detect_game_start "$HOST_LOG")
read -r mapgen_found rooms monsters gold items decorations decor_blocking decor_utility decor_traps decor_economy food_items food_servings mapgen_level mapgen_secret mapgen_seed < <(extract_mapgen_metrics "$HOST_LOG")
mapgen_count=0
if [[ -f "$HOST_LOG" ]]; then
	mapgen_count=$(count_fixed_lines "$HOST_LOG" "successfully generated a dungeon with")
fi
reload_transition_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: auto-reloading dungeon level transition")
reload_transition_seeds="$(collect_reload_transition_seeds "$HOST_LOG")"
reload_transition_seed_count="$(count_list_values "$reload_transition_seeds")"
mapgen_generation_count=$(count_fixed_lines "$HOST_LOG" "generating a dungeon from level set '")
mapgen_generation_seeds="$(collect_mapgen_generation_seeds "$HOST_LOG")"
mapgen_generation_seed_count="$(count_list_values "$mapgen_generation_seeds")"
mapgen_generation_unique_seed_count="$(count_unique_list_values "$mapgen_generation_seeds")"
mapgen_reload_seed_match_count="$(count_seed_matches "$reload_transition_seeds" "$mapgen_generation_seeds")"
mapgen_reload_regen_ok=1
if (( MAPGEN_RELOAD_SAME_LEVEL && reload_transition_lines > 0 )); then
	if (( MAPGEN_RELOAD_SEED_BASE > 0 )); then
		if (( mapgen_reload_seed_match_count < reload_transition_lines )); then
			mapgen_reload_regen_ok=0
		fi
	else
		if (( mapgen_generation_count < reload_transition_lines )); then
			mapgen_reload_regen_ok=0
		fi
	fi
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
if (( AUTO_KICK_TARGET_SLOT > 0 )); then
	auto_kick_ok_lines=$(count_regex_lines "$HOST_LOG" "\\[SMOKE\\]: auto-kick result target=${AUTO_KICK_TARGET_SLOT} .* status=ok")
	auto_kick_fail_lines=$(count_regex_lines "$HOST_LOG" "\\[SMOKE\\]: auto-kick result target=${AUTO_KICK_TARGET_SLOT} .* status=fail")
fi
auto_kick_result="disabled"
if (( AUTO_KICK_TARGET_SLOT > 0 )); then
	if (( auto_kick_fail_lines > 0 )); then
		auto_kick_result="fail"
	elif (( auto_kick_ok_lines > 0 )); then
		auto_kick_result="ok"
	else
		auto_kick_result="missing"
	fi
fi
slot_lock_snapshot_lines=0
if (( TRACE_SLOT_LOCKS )); then
	slot_lock_snapshot_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: lobby slot-lock snapshot context=lobby-init")
fi
default_slot_lock_ok=1
if (( REQUIRE_DEFAULT_SLOT_LOCKS )); then
	default_slot_lock_ok="$(is_default_slot_lock_snapshot_ok "$HOST_LOG" "$EXPECTED_PLAYERS")"
fi
player_count_prompt_lines=0
player_count_prompt_variants=""
player_count_prompt_target=""
player_count_prompt_kicked=""
player_count_prompt_variant=""
if (( TRACE_PLAYER_COUNT_COPY )); then
	player_count_prompt_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: lobby player-count prompt target=")
	player_count_prompt_variants="$(collect_player_count_prompt_variants "$HOST_LOG")"
	player_count_prompt_target="$(extract_last_player_count_prompt_field "$HOST_LOG" target)"
	player_count_prompt_kicked="$(extract_last_player_count_prompt_field "$HOST_LOG" kicked)"
	player_count_prompt_variant="$(extract_last_player_count_prompt_field "$HOST_LOG" variant)"
fi
player_count_copy_ok=1
if (( REQUIRE_PLAYER_COUNT_COPY )); then
	if (( player_count_prompt_lines < 1 )); then
		player_count_copy_ok=0
	fi
	if [[ -n "$EXPECT_PLAYER_COUNT_COPY_VARIANT" ]] && [[ "$player_count_prompt_variant" != "$EXPECT_PLAYER_COUNT_COPY_VARIANT" ]]; then
		player_count_copy_ok=0
	fi
fi
lobby_page_state_ok=1
lobby_page_sweep_ok=1
lobby_page_snapshot_lines=0
lobby_page_unique_count=0
lobby_page_total_count=0
lobby_page_visited=""
lobby_focus_mismatch_lines=0
lobby_cards_misaligned_max=0
lobby_paperdolls_misaligned_max=0
lobby_pings_misaligned_max=0
lobby_warnings_present_lines=0
lobby_warnings_max_abs_delta=0
lobby_countdown_present_lines=0
lobby_countdown_max_abs_delta=0
if (( TRACE_LOBBY_PAGE_STATE )); then
	page_metrics="$(collect_lobby_page_snapshot_metrics "$HOST_LOG")"
	IFS='|' read -r lobby_page_snapshot_lines lobby_page_unique_count lobby_page_total_count lobby_page_visited \
		lobby_focus_mismatch_lines lobby_cards_misaligned_max lobby_paperdolls_misaligned_max \
		lobby_pings_misaligned_max lobby_warnings_present_lines lobby_warnings_max_abs_delta \
		lobby_countdown_present_lines lobby_countdown_max_abs_delta <<< "$page_metrics"
fi
if (( REQUIRE_LOBBY_PAGE_STATE )); then
	if (( lobby_page_snapshot_lines < 1 )); then
		lobby_page_state_ok=0
	fi
	if (( lobby_cards_misaligned_max > 0 || lobby_paperdolls_misaligned_max > 0 || lobby_pings_misaligned_max > 0 )); then
		lobby_page_state_ok=0
	fi
	if (( lobby_warnings_present_lines > 0 && lobby_warnings_max_abs_delta > 2 )); then
		lobby_page_state_ok=0
	fi
	if (( lobby_countdown_present_lines > 0 && lobby_countdown_max_abs_delta > 2 )); then
		lobby_page_state_ok=0
	fi
fi
if (( REQUIRE_LOBBY_PAGE_FOCUS_MATCH )) && (( lobby_focus_mismatch_lines > 0 )); then
	lobby_page_state_ok=0
fi
if (( REQUIRE_LOBBY_PAGE_SWEEP )); then
	if (( lobby_page_total_count < 1 || lobby_page_unique_count < lobby_page_total_count )); then
		lobby_page_sweep_ok=0
	fi
fi
remote_combat_slot_ok_lines=0
remote_combat_slot_fail_lines=0
remote_combat_event_lines=0
remote_combat_event_contexts=""
remote_combat_pause_action_lines=0
remote_combat_pause_complete_lines=0
remote_combat_enemy_bar_action_lines=0
remote_combat_enemy_complete_lines=0
if (( TRACE_REMOTE_COMBAT_SLOT_BOUNDS || REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS || REQUIRE_REMOTE_COMBAT_EVENTS )); then
	remote_combat_slot_ok_lines=$(count_regex_lines_across_logs "\\[SMOKE\\]: remote-combat slot-check .* status=ok" "${ALL_LOGS[@]}")
	remote_combat_slot_fail_lines=$(count_regex_lines_across_logs "\\[SMOKE\\]: remote-combat slot-check .* status=fail" "${ALL_LOGS[@]}")
	remote_combat_event_lines=$(count_fixed_lines_across_logs "[SMOKE]: remote-combat event context=" "${ALL_LOGS[@]}")
	remote_combat_event_contexts="$(collect_remote_combat_event_contexts "${ALL_LOGS[@]}")"
	remote_combat_pause_action_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-pause action=")
	remote_combat_pause_complete_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-pause complete pulses=")
	remote_combat_enemy_bar_action_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-event action=enemy-bar")
	remote_combat_enemy_complete_lines=$(count_fixed_lines "$HOST_LOG" "[SMOKE]: remote-combat auto-event complete pulses=")
fi
remote_combat_slot_bounds_ok=1
if (( REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS )); then
	if (( remote_combat_slot_ok_lines < 1 || remote_combat_slot_fail_lines > 0 )); then
		remote_combat_slot_bounds_ok=0
	fi
fi
remote_combat_events_ok=1
if (( REQUIRE_REMOTE_COMBAT_EVENTS )); then
	if (( remote_combat_event_lines < 1 )); then
		remote_combat_events_ok=0
	fi
	if (( AUTO_PAUSE_PULSES > 0 )); then
		if (( remote_combat_pause_action_lines < AUTO_PAUSE_PULSES * 2 || remote_combat_pause_complete_lines < 1 )); then
			remote_combat_events_ok=0
		fi
		if [[ "$remote_combat_event_contexts" != *"auto-pause-issued"* || "$remote_combat_event_contexts" != *"auto-unpause-issued"* ]]; then
			remote_combat_events_ok=0
		fi
	fi
	if (( AUTO_REMOTE_COMBAT_PULSES > 0 )); then
		if (( remote_combat_enemy_bar_action_lines < AUTO_REMOTE_COMBAT_PULSES || remote_combat_enemy_complete_lines < 1 )); then
			remote_combat_events_ok=0
		fi
		if [[ "$remote_combat_event_contexts" != *"auto-enemy-bar-pulse"* || "$remote_combat_event_contexts" != *"auto-dmgg-pulse"* || "$remote_combat_event_contexts" != *"client-DAMI"* || "$remote_combat_event_contexts" != *"client-DMGG"* ]]; then
			remote_combat_events_ok=0
		fi
		if (( EXPECTED_CLIENTS > 1 )) && [[ "$remote_combat_event_contexts" != *"client-ENHP"* ]]; then
			remote_combat_events_ok=0
		fi
	fi
fi

if (( REQUIRE_ACCOUNT_LABELS )) && (( account_label_slot_coverage_ok == 0 )); then
	result="fail"
fi
if (( REQUIRE_AUTO_KICK )) && [[ "$auto_kick_result" != "ok" ]]; then
	result="fail"
fi
if (( REQUIRE_DEFAULT_SLOT_LOCKS )) && (( default_slot_lock_ok == 0 )); then
	result="fail"
fi
if (( REQUIRE_PLAYER_COUNT_COPY )) && (( player_count_copy_ok == 0 )); then
	result="fail"
fi
if (( REQUIRE_LOBBY_PAGE_STATE )) && (( lobby_page_state_ok == 0 )); then
	result="fail"
fi
if (( REQUIRE_LOBBY_PAGE_SWEEP )) && (( lobby_page_sweep_ok == 0 )); then
	result="fail"
fi
if (( REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS )) && (( remote_combat_slot_bounds_ok == 0 )); then
	result="fail"
fi
if (( REQUIRE_REMOTE_COMBAT_EVENTS )) && (( remote_combat_events_ok == 0 )); then
	result="fail"
fi
if [[ "$mapgen_wait_reason" != "none" ]]; then
	result="fail"
fi
if (( REQUIRE_MAPGEN && MAPGEN_RELOAD_SAME_LEVEL && mapgen_reload_regen_ok == 0 )); then
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
	echo "BACKEND_ROOM_KEY=$backend_room_key"
	echo "BACKEND_ROOM_KEY_FOUND=$backend_room_key_found"
	echo "BACKEND_LAUNCH_BLOCKED=$backend_launch_blocked"
	echo "FORCE_CHUNK=$FORCE_CHUNK"
	echo "CHUNK_PAYLOAD_MAX=$CHUNK_PAYLOAD_MAX"
	echo "MAPGEN_PLAYERS_OVERRIDE=$MAPGEN_PLAYERS_OVERRIDE"
	echo "MAPGEN_CONTROL_FILE=$MAPGEN_CONTROL_FILE"
	echo "MAPGEN_RELOAD_SAME_LEVEL=$MAPGEN_RELOAD_SAME_LEVEL"
	echo "MAPGEN_RELOAD_SEED_BASE=$MAPGEN_RELOAD_SEED_BASE"
	echo "START_FLOOR=$START_FLOOR"
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
	echo "MAPGEN_WAIT_REASON=$mapgen_wait_reason"
	echo "GAMESTART_FOUND=$game_start_found"
	echo "MAPGEN_ROOMS=$rooms"
	echo "MAPGEN_MONSTERS=$monsters"
	echo "MAPGEN_GOLD=$gold"
	echo "MAPGEN_ITEMS=$items"
	echo "MAPGEN_DECORATIONS=$decorations"
	echo "MAPGEN_DECOR_BLOCKING=$decor_blocking"
	echo "MAPGEN_DECOR_UTILITY=$decor_utility"
	echo "MAPGEN_DECOR_TRAPS=$decor_traps"
	echo "MAPGEN_DECOR_ECONOMY=$decor_economy"
	echo "MAPGEN_FOOD_ITEMS=$food_items"
	echo "MAPGEN_FOOD_SERVINGS=$food_servings"
	echo "MAPGEN_LEVEL=$mapgen_level"
	echo "MAPGEN_SECRET=$mapgen_secret"
	echo "MAPGEN_SEED=$mapgen_seed"
	echo "MAPGEN_RELOAD_TRANSITION_LINES=$reload_transition_lines"
	echo "MAPGEN_RELOAD_TRANSITION_SEEDS=$reload_transition_seeds"
	echo "MAPGEN_RELOAD_TRANSITION_SEED_COUNT=$reload_transition_seed_count"
	echo "MAPGEN_GENERATION_LINES=$mapgen_generation_count"
	echo "MAPGEN_GENERATION_SEEDS=$mapgen_generation_seeds"
	echo "MAPGEN_GENERATION_SEED_COUNT=$mapgen_generation_seed_count"
	echo "MAPGEN_GENERATION_UNIQUE_SEED_COUNT=$mapgen_generation_unique_seed_count"
	echo "MAPGEN_RELOAD_SEED_MATCH_COUNT=$mapgen_reload_seed_match_count"
	echo "MAPGEN_RELOAD_REGEN_OK=$mapgen_reload_regen_ok"
	echo "TRACE_ACCOUNT_LABELS=$TRACE_ACCOUNT_LABELS"
	echo "REQUIRE_ACCOUNT_LABELS=$REQUIRE_ACCOUNT_LABELS"
	echo "ACCOUNT_LABEL_LINES=$account_label_lines"
	echo "ACCOUNT_LABEL_SLOTS=$account_label_slots"
	echo "ACCOUNT_LABEL_MISSING_SLOTS=$account_label_missing_slots"
	echo "ACCOUNT_LABEL_SLOT_COVERAGE_OK=$account_label_slot_coverage_ok"
	echo "AUTO_KICK_TARGET_SLOT=$AUTO_KICK_TARGET_SLOT"
	echo "AUTO_KICK_DELAY_SECS=$AUTO_KICK_DELAY_SECS"
	echo "REQUIRE_AUTO_KICK=$REQUIRE_AUTO_KICK"
	echo "AUTO_KICK_RESULT=$auto_kick_result"
	echo "AUTO_KICK_OK_LINES=$auto_kick_ok_lines"
	echo "AUTO_KICK_FAIL_LINES=$auto_kick_fail_lines"
	echo "TRACE_SLOT_LOCKS=$TRACE_SLOT_LOCKS"
	echo "REQUIRE_DEFAULT_SLOT_LOCKS=$REQUIRE_DEFAULT_SLOT_LOCKS"
	echo "SLOT_LOCK_SNAPSHOT_LINES=$slot_lock_snapshot_lines"
	echo "DEFAULT_SLOT_LOCK_OK=$default_slot_lock_ok"
	echo "AUTO_PLAYER_COUNT_TARGET=$AUTO_PLAYER_COUNT_TARGET"
	echo "AUTO_PLAYER_COUNT_DELAY_SECS=$AUTO_PLAYER_COUNT_DELAY_SECS"
	echo "TRACE_PLAYER_COUNT_COPY=$TRACE_PLAYER_COUNT_COPY"
	echo "REQUIRE_PLAYER_COUNT_COPY=$REQUIRE_PLAYER_COUNT_COPY"
	echo "EXPECT_PLAYER_COUNT_COPY_VARIANT=$EXPECT_PLAYER_COUNT_COPY_VARIANT"
	echo "PLAYER_COUNT_PROMPT_LINES=$player_count_prompt_lines"
	echo "PLAYER_COUNT_PROMPT_VARIANTS=$player_count_prompt_variants"
	echo "PLAYER_COUNT_PROMPT_TARGET=$player_count_prompt_target"
	echo "PLAYER_COUNT_PROMPT_KICKED=$player_count_prompt_kicked"
	echo "PLAYER_COUNT_PROMPT_VARIANT=$player_count_prompt_variant"
	echo "PLAYER_COUNT_COPY_OK=$player_count_copy_ok"
	echo "TRACE_LOBBY_PAGE_STATE=$TRACE_LOBBY_PAGE_STATE"
	echo "REQUIRE_LOBBY_PAGE_STATE=$REQUIRE_LOBBY_PAGE_STATE"
	echo "REQUIRE_LOBBY_PAGE_FOCUS_MATCH=$REQUIRE_LOBBY_PAGE_FOCUS_MATCH"
	echo "AUTO_LOBBY_PAGE_SWEEP=$AUTO_LOBBY_PAGE_SWEEP"
	echo "AUTO_LOBBY_PAGE_DELAY_SECS=$AUTO_LOBBY_PAGE_DELAY_SECS"
	echo "REQUIRE_LOBBY_PAGE_SWEEP=$REQUIRE_LOBBY_PAGE_SWEEP"
	echo "LOBBY_PAGE_SNAPSHOT_LINES=$lobby_page_snapshot_lines"
	echo "LOBBY_PAGE_UNIQUE_COUNT=$lobby_page_unique_count"
	echo "LOBBY_PAGE_TOTAL_COUNT=$lobby_page_total_count"
	echo "LOBBY_PAGE_VISITED=$lobby_page_visited"
	echo "LOBBY_FOCUS_MISMATCH_LINES=$lobby_focus_mismatch_lines"
	echo "LOBBY_CARDS_MISALIGNED_MAX=$lobby_cards_misaligned_max"
	echo "LOBBY_PAPERDOLLS_MISALIGNED_MAX=$lobby_paperdolls_misaligned_max"
	echo "LOBBY_PINGS_MISALIGNED_MAX=$lobby_pings_misaligned_max"
	echo "LOBBY_WARNINGS_PRESENT_LINES=$lobby_warnings_present_lines"
	echo "LOBBY_WARNINGS_MAX_ABS_DELTA=$lobby_warnings_max_abs_delta"
	echo "LOBBY_COUNTDOWN_PRESENT_LINES=$lobby_countdown_present_lines"
	echo "LOBBY_COUNTDOWN_MAX_ABS_DELTA=$lobby_countdown_max_abs_delta"
	echo "LOBBY_PAGE_STATE_OK=$lobby_page_state_ok"
	echo "LOBBY_PAGE_SWEEP_OK=$lobby_page_sweep_ok"
	echo "TRACE_REMOTE_COMBAT_SLOT_BOUNDS=$TRACE_REMOTE_COMBAT_SLOT_BOUNDS"
	echo "REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS=$REQUIRE_REMOTE_COMBAT_SLOT_BOUNDS"
	echo "REQUIRE_REMOTE_COMBAT_EVENTS=$REQUIRE_REMOTE_COMBAT_EVENTS"
	echo "AUTO_PAUSE_PULSES=$AUTO_PAUSE_PULSES"
	echo "AUTO_PAUSE_DELAY_SECS=$AUTO_PAUSE_DELAY_SECS"
	echo "AUTO_PAUSE_HOLD_SECS=$AUTO_PAUSE_HOLD_SECS"
	echo "AUTO_REMOTE_COMBAT_PULSES=$AUTO_REMOTE_COMBAT_PULSES"
	echo "AUTO_REMOTE_COMBAT_DELAY_SECS=$AUTO_REMOTE_COMBAT_DELAY_SECS"
	echo "REMOTE_COMBAT_SLOT_OK_LINES=$remote_combat_slot_ok_lines"
	echo "REMOTE_COMBAT_SLOT_FAIL_LINES=$remote_combat_slot_fail_lines"
	echo "REMOTE_COMBAT_EVENT_LINES=$remote_combat_event_lines"
	echo "REMOTE_COMBAT_EVENT_CONTEXTS=$remote_combat_event_contexts"
	echo "REMOTE_COMBAT_AUTO_PAUSE_ACTION_LINES=$remote_combat_pause_action_lines"
	echo "REMOTE_COMBAT_AUTO_PAUSE_COMPLETE_LINES=$remote_combat_pause_complete_lines"
	echo "REMOTE_COMBAT_AUTO_ENEMY_BAR_LINES=$remote_combat_enemy_bar_action_lines"
	echo "REMOTE_COMBAT_AUTO_ENEMY_COMPLETE_LINES=$remote_combat_enemy_complete_lines"
	echo "REMOTE_COMBAT_SLOT_BOUNDS_OK=$remote_combat_slot_bounds_ok"
	echo "REMOTE_COMBAT_EVENTS_OK=$remote_combat_events_ok"
	echo "HOST_LOG=$HOST_LOG"
	echo "PID_FILE=$PID_FILE"
} > "$SUMMARY_FILE"

log "result=$result backend=$NETWORK_BACKEND roomKeyFound=$backend_room_key_found launchBlocked=$backend_launch_blocked chunks=$host_chunk_lines reassembled=$client_reassembled_lines resets=$chunk_reset_lines txmodeApplied=$tx_mode_applied mapgen=$mapgen_found mapgenWait=$mapgen_wait_reason mapgenReloadRegenOk=$mapgen_reload_regen_ok gamestart=$game_start_found autoKick=$auto_kick_result slotLockOk=$default_slot_lock_ok playerCountCopyOk=$player_count_copy_ok lobbyPageStateOk=$lobby_page_state_ok lobbyPageSweepOk=$lobby_page_sweep_ok remoteSlotOk=$remote_combat_slot_bounds_ok remoteEventsOk=$remote_combat_events_ok remoteSlotFail=$remote_combat_slot_fail_lines"
log "summary=$SUMMARY_FILE"

if [[ "$result" != "pass" ]]; then
	exit 1
fi
