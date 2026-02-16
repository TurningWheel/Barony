#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
INSTANCES=5
ROOT_PREFIX="/tmp/barony-inst"
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
PREWARM=1
PREWARM_TIMEOUT=240

usage() {
	cat <<'EOF'
Usage: launch-multi-instance-mac.sh [options]

Options:
  --app <path>               Barony executable path.
  --instances <n>            Number of instances to launch (default: 5).
  --root <prefix>            Prefix for per-instance HOME dirs (default: /tmp/barony-inst).
  --size <WxH>               Window size argument (default: 1280x720).
  --stagger <seconds>        Delay between launches (default: 1).
  --no-prewarm               Skip cache prewarm checks/launches.
  --prewarm-timeout <sec>    Max seconds to wait for each prewarm (default: 240).
  -h, --help                 Show this help.

Examples:
  ./scripts/launch-multi-instance-mac.sh
  ./scripts/launch-multi-instance-mac.sh --instances 5 --root /tmp/barony-inst
  ./scripts/launch-multi-instance-mac.sh --no-prewarm --stagger 2
EOF
}

log() {
	printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

home_for() {
	local idx="$1"
	printf '%s-%s' "$ROOT_PREFIX" "$idx"
}

is_uint() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

wait_for_prewarm_ready() {
	local log_path="$1"
	local timeout_s="$2"
	local end_time=$((SECONDS + timeout_s))

	while (( SECONDS < end_time )); do
		if [[ -f "$log_path" ]]; then
			if rg -q "running main loop\\.|giving keyboard to player 0|loading image './/images/system/title.png'" "$log_path"; then
				return 0
			fi
		fi
		sleep 1
	done

	return 1
}

stop_pid_gracefully() {
	local pid="$1"
	if ! kill -0 "$pid" 2>/dev/null; then
		return 0
	fi

	kill "$pid" 2>/dev/null || true
	for _ in {1..10}; do
		if ! kill -0 "$pid" 2>/dev/null; then
			return 0
		fi
		sleep 1
	done
	kill -9 "$pid" 2>/dev/null || true
}

needs_prewarm() {
	local home="$1"
	local models_cache="$home/.barony/models.cache"
	local books_cache="$home/.barony/books/compiled_books.json"

	[[ ! -s "$models_cache" || ! -s "$books_cache" ]]
}

run_prewarm() {
	local idx="$1"
	local home
	home="$(home_for "$idx")"
	local stdout_log="$home/prewarm.stdout.log"
	local engine_log="$home/.barony/log.txt"

	mkdir -p "$home"
	log "Prewarm $idx: HOME=$home"

	HOME="$home" "$APP" -windowed -size="$WINDOW_SIZE" >"$stdout_log" 2>&1 &
	local pid="$!"

	if wait_for_prewarm_ready "$engine_log" "$PREWARM_TIMEOUT"; then
		log "Prewarm $idx: reached main loop/title."
	else
		log "Prewarm $idx: timeout after ${PREWARM_TIMEOUT}s, continuing."
	fi

	stop_pid_gracefully "$pid"
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
		--root)
			ROOT_PREFIX="${2:-}"
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
		--no-prewarm)
			PREWARM=0
			shift
			;;
		--prewarm-timeout)
			PREWARM_TIMEOUT="${2:-}"
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

if ! is_uint "$INSTANCES" || (( INSTANCES < 1 )); then
	echo "--instances must be a positive integer (got: $INSTANCES)" >&2
	exit 1
fi

if ! is_uint "$STAGGER_SECONDS"; then
	echo "--stagger must be a non-negative integer (got: $STAGGER_SECONDS)" >&2
	exit 1
fi

if ! is_uint "$PREWARM_TIMEOUT" || (( PREWARM_TIMEOUT < 1 )); then
	echo "--prewarm-timeout must be a positive integer (got: $PREWARM_TIMEOUT)" >&2
	exit 1
fi

# Barony changes working directory at startup; keep HOME roots absolute so
# per-instance output/config paths resolve correctly.
if [[ "$ROOT_PREFIX" != /* ]]; then
	ROOT_PREFIX="$PWD/$ROOT_PREFIX"
fi

if (( PREWARM )); then
	log "Checking per-instance caches before launch..."
	for ((i = 1; i <= INSTANCES; i++)); do
		home="$(home_for "$i")"
		mkdir -p "$home"
		if needs_prewarm "$home"; then
			log "Instance $i requires prewarm (missing models/books cache)."
			run_prewarm "$i"
		else
			log "Instance $i cache present, skipping prewarm."
		fi
	done
fi

timestamp="$(date +%Y%m%d-%H%M%S)"
pid_file="${ROOT_PREFIX}-pids-${timestamp}.txt"
touch "$pid_file"

declare -a pids

log "Launching $INSTANCES Barony instances..."
for ((i = 1; i <= INSTANCES; i++)); do
	home="$(home_for "$i")"
	mkdir -p "$home"
	stdout_log="$home/run-${timestamp}.stdout.log"
	HOME="$home" "$APP" -windowed -size="$WINDOW_SIZE" >"$stdout_log" 2>&1 &
	pid="$!"
	pids+=("$pid")
	printf '%s %s %s\n' "$pid" "$home" "$stdout_log" >>"$pid_file"
	log "Instance $i: pid=$pid home=$home log=$stdout_log"
	sleep "$STAGGER_SECONDS"
done

log "All instances launched."
log "PID file: $pid_file"
log "Stop all now: kill ${pids[*]}"
