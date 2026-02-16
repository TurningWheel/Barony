from __future__ import annotations

import argparse
import re
from collections import Counter
from pathlib import Path
from typing import Sequence

from .logscan import LogScanCache, file_count_fixed_lines, file_count_matching_lines
from .tokens import last_line_with_prefix, parse_key_value_tokens


def _iter_log_lines(log_file: Path, scan_cache: LogScanCache | None) -> list[str]:
    if scan_cache is not None:
        return scan_cache.lines_for(log_file)
    if not log_file.is_file():
        return []
    with log_file.open("r", encoding="utf-8", errors="replace") as f:
        return [line.rstrip("\n") for line in f]

def canonicalize_lan_tx_mode(mode: str) -> str | None:
    aliases = {
        "normal": "normal",
        "reverse": "reverse",
        "evenodd": "even-odd",
        "even-odd": "even-odd",
        "even_odd": "even-odd",
        "duplicate-first": "duplicate-first",
        "duplicate_first": "duplicate-first",
        "drop-last": "drop-last",
        "drop_last": "drop-last",
        "duplicate-conflict-first": "duplicate-conflict-first",
        "duplicate_conflict_first": "duplicate-conflict-first",
    }
    return aliases.get(mode)


def is_expected_fail_tx_mode(mode: str) -> int:
    return 1 if mode in {"drop-last", "duplicate-conflict-first"} else 0


def extract_smoke_room_key(host_log: Path, backend: str) -> str:
    prefix = f"[SMOKE]: lobby room key backend={backend} key="
    line = last_line_with_prefix(host_log, prefix)
    if not line:
        return ""
    return parse_key_value_tokens(line).get("key", "")


def tx_mode_packet_plan_valid(host_log: Path, mode: str, *, scan_cache: LogScanCache | None = None) -> int:
    if mode == "normal":
        return 1
    if scan_cache is None and not host_log.is_file():
        return 0

    prefix = f"[SMOKE]: HELO chunk tx mode={mode} "
    saw_line = False
    for line in _iter_log_lines(host_log, scan_cache):
        if prefix not in line:
            continue
        saw_line = True
        tokens = parse_key_value_tokens(line)
        try:
            packets = int(tokens.get("packets", ""))
            chunks = int(tokens.get("chunks", ""))
        except ValueError:
            return 0
        if mode in {"reverse", "even-odd"} and packets != chunks:
            return 0
        if mode in {"duplicate-first", "duplicate-conflict-first"} and packets != chunks + 1:
            return 0
        if mode == "drop-last" and packets + 1 != chunks:
            return 0
    return 1 if saw_line else 0


def count_regex_lines(path: Path, pattern: str, *, scan_cache: LogScanCache | None = None) -> int:
    return file_count_matching_lines(path, pattern, scan_cache=scan_cache)


def count_fixed_lines_across_logs(
    needle: str,
    logs: Sequence[Path],
    *,
    scan_cache: LogScanCache | None = None,
) -> int:
    total = 0
    for log_file in logs:
        total += file_count_fixed_lines(log_file, needle, scan_cache=scan_cache)
    return total


def count_regex_lines_across_logs(
    pattern: str,
    logs: Sequence[Path],
    *,
    scan_cache: LogScanCache | None = None,
) -> int:
    total = 0
    for log_file in logs:
        total += file_count_matching_lines(log_file, pattern, scan_cache=scan_cache)
    return total


def collect_remote_combat_event_contexts(logs: Sequence[Path], *, scan_cache: LogScanCache | None = None) -> str:
    contexts: set[str] = set()
    prefix = "[SMOKE]: remote-combat event context="
    for log_file in logs:
        for line in _iter_log_lines(log_file, scan_cache):
            if prefix not in line:
                continue
            context = parse_key_value_tokens(line).get("context", "")
            if context:
                contexts.add(context)
    if not contexts:
        return ""
    return ";".join(sorted(contexts))


def collect_chunk_reset_reason_counts(logs: Sequence[Path], *, scan_cache: LogScanCache | None = None) -> str:
    reason_counts: Counter[str] = Counter()
    reason_re = re.compile(r"\(([^)]*)\)")
    needle = "HELO chunk timeout/reset transfer="
    for log_file in logs:
        for line in _iter_log_lines(log_file, scan_cache):
            if needle not in line:
                continue
            match = reason_re.search(line)
            reason = "unknown"
            if match:
                value = match.group(1).strip()
                if value:
                    reason = value
            reason_counts[reason] += 1
    if not reason_counts:
        return ""
    return ";".join(f"{reason}:{reason_counts[reason]}" for reason in sorted(reason_counts))


def collect_helo_player_slots(host_log: Path, *, scan_cache: LogScanCache | None = None) -> str:
    slots: set[int] = set()
    player_re = re.compile(r"sending chunked HELO: player=([0-9]+)")
    for line in _iter_log_lines(host_log, scan_cache):
        match = player_re.search(line)
        if match:
            slots.add(int(match.group(1)))
    return ";".join(str(slot) for slot in sorted(slots))


def collect_missing_helo_player_slots(
    host_log: Path,
    expected_clients: int,
    *,
    scan_cache: LogScanCache | None = None,
) -> str:
    if expected_clients <= 0:
        return ""
    observed = {int(x) for x in collect_helo_player_slots(host_log, scan_cache=scan_cache).split(";") if x}
    missing = [str(player) for player in range(1, expected_clients + 1) if player not in observed]
    return ";".join(missing)


def is_helo_player_slot_coverage_ok(
    host_log: Path,
    expected_clients: int,
    *,
    scan_cache: LogScanCache | None = None,
) -> int:
    if expected_clients <= 0:
        return 1
    return 1 if not collect_missing_helo_player_slots(host_log, expected_clients, scan_cache=scan_cache) else 0


def collect_account_label_slots(host_log: Path, *, scan_cache: LogScanCache | None = None) -> str:
    slots: set[int] = set()
    slot_re = re.compile(r"lobby account label resolved slot=([0-9]+)")
    for line in _iter_log_lines(host_log, scan_cache):
        match = slot_re.search(line)
        if match:
            slots.add(int(match.group(1)))
    return ";".join(str(slot) for slot in sorted(slots))


def collect_missing_account_label_slots(
    host_log: Path,
    expected_clients: int,
    *,
    scan_cache: LogScanCache | None = None,
) -> str:
    if expected_clients <= 0:
        return ""
    observed = {int(x) for x in collect_account_label_slots(host_log, scan_cache=scan_cache).split(";") if x}
    missing = [str(slot) for slot in range(1, expected_clients + 1) if slot not in observed]
    return ";".join(missing)


def is_account_label_slot_coverage_ok(
    host_log: Path,
    expected_clients: int,
    *,
    scan_cache: LogScanCache | None = None,
) -> int:
    if expected_clients <= 0:
        return 1
    return 1 if not collect_missing_account_label_slots(host_log, expected_clients, scan_cache=scan_cache) else 0


def is_default_slot_lock_snapshot_ok(
    host_log: Path,
    expected_players: int,
    *,
    scan_cache: LogScanCache | None = None,
) -> int:
    prefix = "[SMOKE]: lobby slot-lock snapshot context=lobby-init "
    line = ""
    for value in _iter_log_lines(host_log, scan_cache):
        if prefix in value:
            line = value
            break
    if not line:
        return 0

    tokens = parse_key_value_tokens(line)
    try:
        configured = int(tokens.get("configured", ""))
        free_unlocked = int(tokens.get("free_unlocked", ""))
        free_locked = int(tokens.get("free_locked", ""))
        occupied = int(tokens.get("occupied", ""))
    except ValueError:
        return 0

    expected_unlocked = max(expected_players - 1, 0)
    expected_locked = max((15 - 1) - expected_unlocked, 0)
    if (
        configured == expected_players
        and free_unlocked == expected_unlocked
        and free_locked == expected_locked
        and occupied == 0
    ):
        return 1
    return 0


def collect_player_count_prompt_variants(host_log: Path, *, scan_cache: LogScanCache | None = None) -> str:
    variants: set[str] = set()
    prefix = "[SMOKE]: lobby player-count prompt target="
    for line in _iter_log_lines(host_log, scan_cache):
        if prefix not in line:
            continue
        variant = parse_key_value_tokens(line).get("variant", "")
        if variant:
            variants.add(variant)
    return ";".join(sorted(variants))


def extract_last_player_count_prompt_field(
    host_log: Path,
    key: str,
    *,
    scan_cache: LogScanCache | None = None,
) -> str:
    if scan_cache is not None:
        line = ""
        prefix = "[SMOKE]: lobby player-count prompt target="
        for value in _iter_log_lines(host_log, scan_cache):
            if prefix in value:
                line = value
    else:
        line = last_line_with_prefix(host_log, "[SMOKE]: lobby player-count prompt target=")
    if not line:
        return ""
    return parse_key_value_tokens(line).get(key, "")


def collect_lobby_page_snapshot_metrics(
    host_log: Path,
    *,
    scan_cache: LogScanCache | None = None,
) -> dict[str, int | str]:
    metrics: dict[str, int | str] = {
        "lobby_page_snapshot_lines": 0,
        "lobby_page_unique_count": 0,
        "lobby_page_total_count": 0,
        "lobby_page_visited": "",
        "lobby_focus_mismatch_lines": 0,
        "lobby_cards_misaligned_max": 0,
        "lobby_paperdolls_misaligned_max": 0,
        "lobby_pings_misaligned_max": 0,
        "lobby_warnings_present_lines": 0,
        "lobby_warnings_max_abs_delta": 0,
        "lobby_countdown_present_lines": 0,
        "lobby_countdown_max_abs_delta": 0,
    }
    if scan_cache is None and not host_log.is_file():
        return metrics

    prefix = "[SMOKE]: lobby page snapshot context=visible-page "
    seen_pages: list[int] = []
    seen_pages_set: set[int] = set()
    for line in _iter_log_lines(host_log, scan_cache):
        if prefix not in line:
            continue

        metrics["lobby_page_snapshot_lines"] = int(metrics["lobby_page_snapshot_lines"]) + 1
        tokens = parse_key_value_tokens(line)

        page_text = tokens.get("page", "")
        if "/" in page_text:
            page_item, total_item = page_text.split("/", 1)
            try:
                page = int(page_item)
                page_total = int(total_item)
            except ValueError:
                page = -1
                page_total = 0
            if page >= 0 and page not in seen_pages_set:
                seen_pages.append(page)
                seen_pages_set.add(page)
            if page_total > 0:
                metrics["lobby_page_total_count"] = page_total

        try:
            focus_match = int(tokens.get("focus_page_match", "1"))
        except ValueError:
            focus_match = 1
        if focus_match == 0:
            metrics["lobby_focus_mismatch_lines"] = int(metrics["lobby_focus_mismatch_lines"]) + 1

        for source_key, metric_key in (
            ("cards_misaligned", "lobby_cards_misaligned_max"),
            ("paperdolls_misaligned", "lobby_paperdolls_misaligned_max"),
            ("pings_misaligned", "lobby_pings_misaligned_max"),
        ):
            try:
                value = int(tokens.get(source_key, "0"))
            except ValueError:
                value = 0
            if value > int(metrics[metric_key]):
                metrics[metric_key] = value

        for source_key, present_key, abs_key in (
            ("warnings_center_delta", "lobby_warnings_present_lines", "lobby_warnings_max_abs_delta"),
            ("countdown_center_delta", "lobby_countdown_present_lines", "lobby_countdown_max_abs_delta"),
        ):
            try:
                delta = int(tokens.get(source_key, ""))
            except ValueError:
                continue
            if delta == 9999:
                continue
            metrics[present_key] = int(metrics[present_key]) + 1
            abs_delta = abs(delta)
            if abs_delta > int(metrics[abs_key]):
                metrics[abs_key] = abs_delta

    metrics["lobby_page_unique_count"] = len(seen_pages)
    metrics["lobby_page_visited"] = ";".join(str(page) for page in seen_pages)
    return metrics

def detect_game_start(host_log: Path, *, scan_cache: LogScanCache | None = None) -> int:
    if file_count_fixed_lines(host_log, "Starting game, game seed:", scan_cache=scan_cache) > 0:
        return 1
    return 0


def refresh_optional_assertion_metrics(
    host_log: Path,
    all_logs: Sequence[Path],
    expected_clients: int,
    expected_players: int,
    ns: argparse.Namespace,
    *,
    scan_cache: LogScanCache | None = None,
) -> dict[str, int | str]:
    metrics: dict[str, int | str] = {
        "account_label_ok": 1,
        "account_label_slots": collect_account_label_slots(host_log, scan_cache=scan_cache),
        "account_label_missing_slots": collect_missing_account_label_slots(
            host_log, expected_clients, scan_cache=scan_cache
        ),
        "account_label_slot_coverage_ok": is_account_label_slot_coverage_ok(
            host_log, expected_clients, scan_cache=scan_cache
        ),
        "account_label_lines": file_count_fixed_lines(
            host_log, "lobby account label resolved slot=", scan_cache=scan_cache
        ),
        "auto_kick_ok": 1,
        "auto_kick_ok_lines": 0,
        "auto_kick_fail_lines": 0,
        "auto_kick_result": "disabled",
        "slot_lock_snapshot_lines": 0,
        "default_slot_lock_ok": 1,
        "player_count_prompt_lines": 0,
        "player_count_prompt_variants": "",
        "player_count_prompt_target": "",
        "player_count_prompt_kicked": "",
        "player_count_prompt_variant": "",
        "player_count_copy_ok": 1,
        "lobby_page_state_ok": 1,
        "lobby_page_sweep_ok": 1,
        "lobby_page_snapshot_lines": 0,
        "lobby_page_unique_count": 0,
        "lobby_page_total_count": 0,
        "lobby_page_visited": "",
        "lobby_focus_mismatch_lines": 0,
        "lobby_cards_misaligned_max": 0,
        "lobby_paperdolls_misaligned_max": 0,
        "lobby_pings_misaligned_max": 0,
        "lobby_warnings_present_lines": 0,
        "lobby_warnings_max_abs_delta": 0,
        "lobby_countdown_present_lines": 0,
        "lobby_countdown_max_abs_delta": 0,
        "remote_combat_slot_ok_lines": 0,
        "remote_combat_slot_fail_lines": 0,
        "remote_combat_event_lines": 0,
        "remote_combat_event_contexts": "",
        "remote_combat_pause_action_lines": 0,
        "remote_combat_pause_complete_lines": 0,
        "remote_combat_enemy_bar_action_lines": 0,
        "remote_combat_enemy_complete_lines": 0,
        "remote_combat_slot_bounds_ok": 1,
        "remote_combat_events_ok": 1,
    }

    if ns.require_account_labels and expected_clients > 0 and int(metrics["account_label_slot_coverage_ok"]) == 0:
        metrics["account_label_ok"] = 0

    if ns.auto_kick_target_slot > 0:
        pattern_ok = rf"\[SMOKE\]: auto-kick result target={ns.auto_kick_target_slot} .* status=ok"
        pattern_fail = rf"\[SMOKE\]: auto-kick result target={ns.auto_kick_target_slot} .* status=fail"
        metrics["auto_kick_ok_lines"] = count_regex_lines(host_log, pattern_ok, scan_cache=scan_cache)
        metrics["auto_kick_fail_lines"] = count_regex_lines(host_log, pattern_fail, scan_cache=scan_cache)
        if int(metrics["auto_kick_fail_lines"]) > 0:
            metrics["auto_kick_result"] = "fail"
        elif int(metrics["auto_kick_ok_lines"]) > 0:
            metrics["auto_kick_result"] = "ok"
        else:
            metrics["auto_kick_result"] = "missing"
    if ns.require_auto_kick and (
        int(metrics["auto_kick_ok_lines"]) < 1 or int(metrics["auto_kick_fail_lines"]) > 0
    ):
        metrics["auto_kick_ok"] = 0

    if ns.trace_slot_locks:
        metrics["slot_lock_snapshot_lines"] = file_count_fixed_lines(
            host_log,
            "[SMOKE]: lobby slot-lock snapshot context=lobby-init",
            scan_cache=scan_cache,
        )
    if ns.require_default_slot_locks:
        metrics["default_slot_lock_ok"] = is_default_slot_lock_snapshot_ok(
            host_log,
            expected_players,
            scan_cache=scan_cache,
        )

    if ns.trace_player_count_copy:
        metrics["player_count_prompt_lines"] = file_count_fixed_lines(
            host_log,
            "[SMOKE]: lobby player-count prompt target=",
            scan_cache=scan_cache,
        )
        metrics["player_count_prompt_variants"] = collect_player_count_prompt_variants(
            host_log,
            scan_cache=scan_cache,
        )
        metrics["player_count_prompt_target"] = extract_last_player_count_prompt_field(
            host_log,
            "target",
            scan_cache=scan_cache,
        )
        metrics["player_count_prompt_kicked"] = extract_last_player_count_prompt_field(
            host_log,
            "kicked",
            scan_cache=scan_cache,
        )
        metrics["player_count_prompt_variant"] = extract_last_player_count_prompt_field(
            host_log,
            "variant",
            scan_cache=scan_cache,
        )
    if ns.require_player_count_copy:
        if int(metrics["player_count_prompt_lines"]) < 1:
            metrics["player_count_copy_ok"] = 0
        if (
            ns.expect_player_count_copy_variant
            and str(metrics["player_count_prompt_variant"]) != ns.expect_player_count_copy_variant
        ):
            metrics["player_count_copy_ok"] = 0

    if ns.trace_lobby_page_state:
        page_metrics = collect_lobby_page_snapshot_metrics(host_log, scan_cache=scan_cache)
        metrics.update(page_metrics)
    if ns.require_lobby_page_state:
        if int(metrics["lobby_page_snapshot_lines"]) < 1:
            metrics["lobby_page_state_ok"] = 0
        if (
            int(metrics["lobby_cards_misaligned_max"]) > 0
            or int(metrics["lobby_paperdolls_misaligned_max"]) > 0
            or int(metrics["lobby_pings_misaligned_max"]) > 0
        ):
            metrics["lobby_page_state_ok"] = 0
        if (
            int(metrics["lobby_warnings_present_lines"]) > 0
            and int(metrics["lobby_warnings_max_abs_delta"]) > 2
        ):
            metrics["lobby_page_state_ok"] = 0
        if (
            int(metrics["lobby_countdown_present_lines"]) > 0
            and int(metrics["lobby_countdown_max_abs_delta"]) > 2
        ):
            metrics["lobby_page_state_ok"] = 0
    if ns.require_lobby_page_focus_match and int(metrics["lobby_focus_mismatch_lines"]) > 0:
        metrics["lobby_page_state_ok"] = 0
    if ns.require_lobby_page_sweep:
        if (
            int(metrics["lobby_page_total_count"]) < 1
            or int(metrics["lobby_page_unique_count"]) < int(metrics["lobby_page_total_count"])
        ):
            metrics["lobby_page_sweep_ok"] = 0

    if (
        ns.trace_remote_combat_slot_bounds
        or ns.require_remote_combat_slot_bounds
        or ns.require_remote_combat_events
    ):
        metrics["remote_combat_slot_ok_lines"] = count_regex_lines_across_logs(
            r"\[SMOKE\]: remote-combat slot-check .* status=ok",
            all_logs,
            scan_cache=scan_cache,
        )
        metrics["remote_combat_slot_fail_lines"] = count_regex_lines_across_logs(
            r"\[SMOKE\]: remote-combat slot-check .* status=fail",
            all_logs,
            scan_cache=scan_cache,
        )
        metrics["remote_combat_event_lines"] = count_fixed_lines_across_logs(
            "[SMOKE]: remote-combat event context=",
            all_logs,
            scan_cache=scan_cache,
        )
        metrics["remote_combat_event_contexts"] = collect_remote_combat_event_contexts(
            all_logs,
            scan_cache=scan_cache,
        )
        metrics["remote_combat_pause_action_lines"] = file_count_fixed_lines(
            host_log,
            "[SMOKE]: remote-combat auto-pause action=",
            scan_cache=scan_cache,
        )
        metrics["remote_combat_pause_complete_lines"] = file_count_fixed_lines(
            host_log,
            "[SMOKE]: remote-combat auto-pause complete pulses=",
            scan_cache=scan_cache,
        )
        metrics["remote_combat_enemy_bar_action_lines"] = file_count_fixed_lines(
            host_log,
            "[SMOKE]: remote-combat auto-event action=enemy-bar",
            scan_cache=scan_cache,
        )
        metrics["remote_combat_enemy_complete_lines"] = file_count_fixed_lines(
            host_log,
            "[SMOKE]: remote-combat auto-event complete pulses=",
            scan_cache=scan_cache,
        )

    if ns.require_remote_combat_slot_bounds:
        if int(metrics["remote_combat_slot_ok_lines"]) < 1 or int(metrics["remote_combat_slot_fail_lines"]) > 0:
            metrics["remote_combat_slot_bounds_ok"] = 0

    if ns.require_remote_combat_events:
        if int(metrics["remote_combat_event_lines"]) < 1:
            metrics["remote_combat_events_ok"] = 0
        contexts = str(metrics["remote_combat_event_contexts"])
        if ns.auto_pause_pulses > 0:
            if (
                int(metrics["remote_combat_pause_action_lines"]) < ns.auto_pause_pulses * 2
                or int(metrics["remote_combat_pause_complete_lines"]) < 1
            ):
                metrics["remote_combat_events_ok"] = 0
            if "auto-pause-issued" not in contexts or "auto-unpause-issued" not in contexts:
                metrics["remote_combat_events_ok"] = 0
        if ns.auto_remote_combat_pulses > 0:
            if (
                int(metrics["remote_combat_enemy_bar_action_lines"]) < ns.auto_remote_combat_pulses
                or int(metrics["remote_combat_enemy_complete_lines"]) < 1
            ):
                metrics["remote_combat_events_ok"] = 0
            required_contexts = ("auto-enemy-bar-pulse", "auto-dmgg-pulse", "client-DAMI", "client-DMGG")
            if any(context not in contexts for context in required_contexts):
                metrics["remote_combat_events_ok"] = 0
            if expected_clients > 1 and "client-ENHP" not in contexts:
                metrics["remote_combat_events_ok"] = 0

    return metrics
