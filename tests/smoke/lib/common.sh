#!/usr/bin/env bash

set -euo pipefail

smoke_is_uint() {
	[[ "$1" =~ ^[0-9]+$ ]]
}

smoke_log() {
	printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

smoke_summary_get() {
	local key="$1"
	local file="$2"
	if [[ ! -f "$file" ]]; then
		echo ""
		return
	fi
	awk -v key="$key" '
		index($0, key "=") == 1 {
			sub("^[^=]*=", "", $0)
			print
			exit
		}
	' "$file"
}

smoke_summary_get_last() {
	local key="$1"
	local file="$2"
	if [[ ! -f "$file" ]]; then
		echo ""
		return
	fi
	awk -v key="$key" '
		index($0, key "=") == 1 {
			value = $0
			sub("^[^=]*=", "", value)
			last = value
		}
		END { print last }
	' "$file"
}

smoke_prune_models_cache() {
	local lane_outdir="$1"
	if [[ ! -d "$lane_outdir/instances" ]]; then
		return
	fi
	find "$lane_outdir/instances" -type f -name models.cache -delete 2>/dev/null || true
}

smoke_count_fixed_lines() {
	local file="$1"
	local needle="$2"
	if [[ ! -f "$file" ]]; then
		echo 0
		return
	fi
	rg -F -c "$needle" "$file" 2>/dev/null || echo 0
}
