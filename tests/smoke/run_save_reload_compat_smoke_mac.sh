#!/usr/bin/env bash
set -euo pipefail

APP="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony"
DATADIR=""
WINDOW_SIZE="1280x720"
STAGGER_SECONDS=1
TIMEOUT_SECONDS=540
OUTDIR=""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HELO_RUNNER="$SCRIPT_DIR/run_lan_helo_chunk_smoke_mac.sh"
COMMON_SH="$SCRIPT_DIR/lib/common.sh"
source "$COMMON_SH"

usage() {
	cat <<'USAGE'
Usage: run_save_reload_compat_smoke_mac.sh [options]

Options:
  --app <path>                  Barony executable path.
  --datadir <path>              Optional data directory passed to Barony via -datadir=<path>.
  --size <WxH>                  Window size (default: 1280x720).
  --stagger <sec>               Delay between launches.
  --timeout <sec>               Timeout for the owner-sweep lane (default: 540).
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

read_summary_value() {
	local summary_file="$1"
	local key="$2"
	smoke_summary_get_last "$key" "$summary_file"
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
if ! is_uint "$STAGGER_SECONDS" || ! is_uint "$TIMEOUT_SECONDS"; then
	echo "--stagger and --timeout must be non-negative integers" >&2
	exit 1
fi
if [[ ! -x "$HELO_RUNNER" ]]; then
	echo "Required runner missing or not executable: $HELO_RUNNER" >&2
	exit 1
fi

if [[ -z "$OUTDIR" ]]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	OUTDIR="tests/smoke/artifacts/save-reload-compat-${timestamp}"
fi
if [[ "$OUTDIR" != /* ]]; then
	OUTDIR="$PWD/$OUTDIR"
fi
mkdir -p "$OUTDIR"
rm -f "$OUTDIR/summary.env" "$OUTDIR/save_reload_owner_encoding_results.csv"

LANE_OUTDIR="$OUTDIR/owner-sweep"

log "Artifacts: $OUTDIR"

cmd=(
	"$HELO_RUNNER"
	--app "$APP"
	--instances 1
	--size "$WINDOW_SIZE"
	--stagger "$STAGGER_SECONDS"
	--timeout "$TIMEOUT_SECONDS"
	--force-chunk 1
	--chunk-payload-max 200
	--auto-start 1
	--auto-start-delay 1
	--auto-enter-dungeon 1
	--auto-enter-dungeon-delay 2
	--require-mapgen 1
	--outdir "$LANE_OUTDIR"
)
if [[ -n "$DATADIR" ]]; then
	cmd+=(--datadir "$DATADIR")
fi

BARONY_SMOKE_SAVE_RELOAD_OWNER_SWEEP=1 \
"${cmd[@]}"

lane_summary="$LANE_OUTDIR/summary.env"
host_log="$(read_summary_value "$lane_summary" "HOST_LOG")"
if [[ -z "$host_log" || ! -f "$host_log" ]]; then
	echo "Host log was not found after owner sweep lane: $host_log" >&2
	exit 1
fi

regular_pass_count=0
legacy_pass_count=0
regular_fail_count=0
legacy_fail_count=0

CSV_PATH="$OUTDIR/save_reload_owner_encoding_results.csv"
cat > "$CSV_PATH" <<'CSV'
lane,players_connected,result,artifact
CSV

for players_connected in $(seq 1 15); do
	lane="p${players_connected}"
	result="fail"
	if rg -q "\\[SMOKE\\]: save_reload_owner lane=${lane} players_connected=${players_connected} result=pass" "$host_log"; then
		result="pass"
		regular_pass_count=$((regular_pass_count + 1))
	else
		regular_fail_count=$((regular_fail_count + 1))
	fi
	echo "${lane},${players_connected},${result},${LANE_OUTDIR}" >> "$CSV_PATH"
done

for legacy_lane in legacy-empty legacy-short legacy-long; do
	result="fail"
	if rg -q "\\[SMOKE\\]: save_reload_owner lane=${legacy_lane} .* result=pass" "$host_log"; then
		result="pass"
		legacy_pass_count=$((legacy_pass_count + 1))
	else
		legacy_fail_count=$((legacy_fail_count + 1))
	fi
	echo "${legacy_lane},8,${result},${LANE_OUTDIR}" >> "$CSV_PATH"
done

owner_fail_lines="$(rg -c "\\[SMOKE\\]: save_reload_owner .*status=fail" "$host_log" || echo 0)"
sweep_line="$(rg -F "[SMOKE]: save_reload_owner sweep result=" "$host_log" | tail -n 1 || true)"
sweep_result="$(echo "$sweep_line" | sed -nE 's/.*result=([a-z]+).*/\1/p')"
if [[ -z "$sweep_result" ]]; then
	sweep_result="fail"
fi

overall_result="pass"
if (( regular_fail_count > 0 || legacy_fail_count > 0 || owner_fail_lines > 0 )) || [[ "$sweep_result" != "pass" ]]; then
	overall_result="fail"
fi

{
	echo "RESULT=$overall_result"
	echo "OUTDIR=$OUTDIR"
	echo "LANE_OUTDIR=$LANE_OUTDIR"
	echo "CSV_PATH=$CSV_PATH"
	echo "HOST_LOG=$host_log"
	echo "DATADIR=$DATADIR"
	echo "REGULAR_PASS_COUNT=$regular_pass_count"
	echo "REGULAR_FAIL_COUNT=$regular_fail_count"
	echo "LEGACY_PASS_COUNT=$legacy_pass_count"
	echo "LEGACY_FAIL_COUNT=$legacy_fail_count"
	echo "OWNER_FAIL_LINES=$owner_fail_lines"
	echo "SWEEP_RESULT=$sweep_result"
	echo "SWEEP_LINE=$sweep_line"
} > "$OUTDIR/summary.env"

smoke_prune_models_cache "$LANE_OUTDIR"

log "summary=$OUTDIR/summary.env"
log "csv=$CSV_PATH"

if [[ "$overall_result" != "pass" ]]; then
	echo "Save/reload owner-encoding sweep failed; see $host_log" >&2
	exit 1
fi
