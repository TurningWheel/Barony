from __future__ import annotations

import argparse
import tempfile
from pathlib import Path

from .csvio import append_csv_row, write_csv_header
from .lane_matrix import compute_lane_result, single_lane_counts, update_lane_counts
from .lane_status import pass_fail
from .summary import parse_summary_key_last, write_summary_env
from .tokens import parse_key_value_tokens


def run_framework_self_checks() -> list[str]:
    errors: list[str] = []

    kv = parse_key_value_tokens("connected=4 over_cap_connected=1")
    if kv.get("connected") != "4":
        errors.append("tokens.parse_key_value_tokens did not keep connected key value")
    if kv.get("over_cap_connected") != "1":
        errors.append("tokens.parse_key_value_tokens did not keep over_cap_connected key value")

    if compute_lane_result("pass", True, True) != "pass":
        errors.append("lane_matrix.compute_lane_result pass path failed")
    if compute_lane_result("pass", True, False) != "fail":
        errors.append("lane_matrix.compute_lane_result fail-check path failed")
    total, passed, failed = update_lane_counts("pass", total_lanes=0, pass_lanes=0, fail_lanes=0)
    if (total, passed, failed) != (1, 1, 0):
        errors.append("lane_matrix.update_lane_counts pass path failed")
    pass_count, fail_count = single_lane_counts("fail")
    if (pass_count, fail_count) != (0, 1):
        errors.append("lane_matrix.single_lane_counts fail path failed")

    with tempfile.TemporaryDirectory(prefix="barony-smoke-self-check-") as tmp:
        base = Path(tmp)
        summary_path = base / "summary.env"
        csv_path = base / "rows.csv"

        write_summary_env(summary_path, {"RESULT": "pass", "COUNT": 2})
        if parse_summary_key_last(summary_path, "RESULT") != "pass":
            errors.append("summary.parse_summary_key_last RESULT mismatch")
        if parse_summary_key_last(summary_path, "COUNT") != "2":
            errors.append("summary.parse_summary_key_last COUNT mismatch")

        write_csv_header(csv_path, ["k", "v", "status"])
        append_csv_row(csv_path, ["a", 1, pass_fail(True)])
        lines = csv_path.read_text(encoding="utf-8").strip().splitlines()
        if len(lines) != 2:
            errors.append("csvio write/append row count mismatch")

    return errors


def cmd_framework_self_check(_ns: argparse.Namespace) -> int:
    errors = run_framework_self_checks()
    if errors:
        for issue in errors:
            print(f"[SMOKE] self-check fail: {issue}")
        print(f"[SMOKE] result=fail checks={len(errors)}")
        return 1
    print("[SMOKE] result=pass checks=6")
    return 0


def register_framework_self_check_parser(
    sub: argparse._SubParsersAction[argparse.ArgumentParser],
) -> None:
    parser = sub.add_parser(
        "framework-self-check",
        help="Run lightweight smoke framework parser/helper self-checks",
    )
    parser.set_defaults(handler=cmd_framework_self_check)
