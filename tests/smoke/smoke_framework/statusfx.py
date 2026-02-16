from __future__ import annotations

import re
from pathlib import Path
from typing import Sequence

from .logscan import file_count_fixed_lines, file_count_matching_lines

STATUSFX_PHASES: tuple[str, ...] = ("init", "create", "update")
STATUSFX_MISMATCH_PATTERNS: dict[str, str] = {
    "init": r"\[SMOKE\]: statusfx queue init slot=[0-9]+ owner=-?[0-9]+ status=mismatch",
    "create": r"\[SMOKE\]: statusfx queue create slot=[0-9]+ owner=-?[0-9]+ status=mismatch",
    "update": r"\[SMOKE\]: statusfx queue update slot=[0-9]+ owner=-?[0-9]+ status=mismatch",
}

def collect_slots_for_lane(log_files: Sequence[Path], lane: str) -> str:
    slot_pattern = re.compile(rf"statusfx queue {re.escape(lane)} slot=([0-9]+)")
    slots: set[int] = set()
    for log_file in log_files:
        if not log_file.is_file():
            continue
        with log_file.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                match = slot_pattern.search(line)
                if match:
                    slots.add(int(match.group(1)))
    if not slots:
        return ""
    return ";".join(str(slot) for slot in sorted(slots))


def collect_missing_slots_for_lane(
    log_files: Sequence[Path],
    lane: str,
    min_slot: int,
    max_slot: int,
) -> str:
    if max_slot < min_slot:
        return ""
    missing: list[str] = []
    for slot in range(min_slot, max_slot + 1):
        needle = f"[SMOKE]: statusfx queue {lane} slot={slot} owner={slot} status=ok"
        found = False
        for log_file in log_files:
            if file_count_fixed_lines(log_file, needle) > 0:
                found = True
                break
        if not found:
            missing.append(str(slot))
    return ";".join(missing)


def count_pattern_in_logs(log_files: Sequence[Path], pattern: str) -> int:
    total = 0
    for log_file in log_files:
        total += file_count_matching_lines(log_file, pattern)
    return total


def collect_statusfx_lane_metrics(log_files: Sequence[Path], instances: int) -> dict[str, str | int]:
    metrics: dict[str, str | int] = {}
    for phase in STATUSFX_PHASES:
        slots = collect_slots_for_lane(log_files, phase)
        missing = collect_missing_slots_for_lane(log_files, phase, 0, instances - 1)
        mismatch = count_pattern_in_logs(log_files, STATUSFX_MISMATCH_PATTERNS[phase])
        metrics[f"{phase}_slots"] = slots
        metrics[f"{phase}_missing_slots"] = missing
        metrics[f"{phase}_slot_coverage_ok"] = 0 if missing else 1
        metrics[f"{phase}_mismatch_lines"] = mismatch
    return metrics
