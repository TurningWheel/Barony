from __future__ import annotations

import argparse
import sys
import tempfile
import unittest
from pathlib import Path

SMOKE_ROOT = Path(__file__).resolve().parents[1]
if str(SMOKE_ROOT) not in sys.path:
    sys.path.insert(0, str(SMOKE_ROOT))

from smoke_framework.lane_matrix import compute_lane_result, single_lane_counts, update_lane_counts
from smoke_framework.lane_helpers import (
    build_default_helo_lane_args,
    require_uint_specs,
    run_ns_helo_child_lane,
    single_lane_rollup,
    write_single_lane_result_files,
)
from smoke_framework.logscan import LogScanCache, collect_instance_logs, file_count_fixed_lines
from smoke_framework.mapgen import parse_mapgen_metrics_lines
from smoke_framework.mapgen_schema import resolve_mapgen_metrics_from_rows
from smoke_framework.self_check_lane import run_framework_self_checks
from smoke_framework.summary import parse_summary_key_last
from smoke_framework.tokens import parse_key_value_tokens


class FrameworkHelperTests(unittest.TestCase):
    def test_parse_key_value_tokens_keeps_similar_keys(self) -> None:
        line = "connected=4 over_cap_connected=1 connected_slots=3"
        values = parse_key_value_tokens(line)
        self.assertEqual(values.get("connected"), "4")
        self.assertEqual(values.get("over_cap_connected"), "1")
        self.assertEqual(values.get("connected_slots"), "3")

    def test_lane_matrix_helpers(self) -> None:
        self.assertEqual(compute_lane_result("pass", True, True), "pass")
        self.assertEqual(compute_lane_result("pass", True, False), "fail")
        self.assertEqual(compute_lane_result("fail", True, True), "fail")

        total, passed, failed = update_lane_counts("pass", total_lanes=0, pass_lanes=0, fail_lanes=0)
        self.assertEqual((total, passed, failed), (1, 1, 0))

        total, passed, failed = update_lane_counts(
            "fail",
            total_lanes=total,
            pass_lanes=passed,
            fail_lanes=failed,
        )
        self.assertEqual((total, passed, failed), (2, 1, 1))
        self.assertEqual(single_lane_counts("pass"), (1, 0))
        self.assertEqual(single_lane_counts("fail"), (0, 1))

    def test_framework_self_check_helpers(self) -> None:
        self.assertEqual(run_framework_self_checks(), [])

    def test_lane_helper_validators_and_rollup(self) -> None:
        ns = argparse.Namespace(timeout=3, instances=4)
        require_uint_specs(
            ns,
            (
                ("--timeout", "timeout", None, None),
                ("--instances", "instances", 1, 15),
            ),
        )
        with self.assertRaises(SystemExit):
            require_uint_specs(ns, (("--instances", "instances", 5, 15),))

        args = build_default_helo_lane_args("--foo", "bar", auto_start=1)
        self.assertEqual(
            args[:8],
            [
                "--force-chunk",
                "1",
                "--chunk-payload-max",
                "200",
                "--require-helo",
                "1",
                "--auto-start",
                "1",
            ],
        )
        self.assertEqual(args[-2:], ["--foo", "bar"])

        self.assertEqual(single_lane_rollup("pass"), ("pass", 1, 0))
        self.assertEqual(single_lane_rollup("fail"), ("fail", 0, 1))

    def test_run_ns_helo_child_lane_forwards_namespace(self) -> None:
        ns = argparse.Namespace(
            app=Path("/tmp/fake-app"),
            datadir=None,
            size="1280x720",
            stagger=2,
        )
        captured: dict[str, object] = {}

        def fake_runner(**kwargs: object) -> tuple[int, str, dict[str, str], Path]:
            captured.update(kwargs)
            return 0, "pass", {"RESULT": "pass"}, Path("/tmp/summary.env")

        run_ns_helo_child_lane(
            fake_runner,
            ns,
            instances=4,
            expected_players=4,
            timeout=180,
            lane_outdir=Path("/tmp/out"),
            lane_args=["--auto-start", "1"],
            summary_defaults={"RESULT": "fail"},
            extra_env={"BARONY_SMOKE_X": "1"},
        )
        self.assertEqual(captured["app"], ns.app)
        self.assertEqual(captured["datadir"], ns.datadir)
        self.assertEqual(captured["size"], ns.size)
        self.assertEqual(captured["stagger"], ns.stagger)
        self.assertEqual(captured["instances"], 4)
        self.assertEqual(captured["expected_players"], 4)
        self.assertEqual(captured["timeout"], 180)
        self.assertEqual(captured["lane_outdir"], Path("/tmp/out"))
        self.assertEqual(captured["lane_args"], ["--auto-start", "1"])
        self.assertEqual(captured["summary_defaults"], {"RESULT": "fail"})
        self.assertEqual(captured["extra_env"], {"BARONY_SMOKE_X": "1"})

    def test_collect_instance_logs_supports_current_and_legacy_layouts(self) -> None:
        with tempfile.TemporaryDirectory(prefix="barony-smoke-logscan-") as tmp:
            lane_outdir = Path(tmp) / "lane"
            current = lane_outdir / "instances/home-1/.barony/log.txt"
            legacy = lane_outdir / "instances/home-2-l1/.barony/log.txt"
            current.parent.mkdir(parents=True, exist_ok=True)
            legacy.parent.mkdir(parents=True, exist_ok=True)
            current.write_text("current\n", encoding="utf-8")
            legacy.write_text("legacy\n", encoding="utf-8")

            logs = collect_instance_logs(lane_outdir)
            self.assertEqual(logs, sorted([current, legacy]))

    def test_write_single_lane_result_files_writes_csv_and_summary(self) -> None:
        with tempfile.TemporaryDirectory(prefix="barony-smoke-lane-report-") as tmp:
            root = Path(tmp)
            summary_path = root / "summary.env"
            csv_path = root / "results.csv"
            lane_outdir = root / "lane"
            lane_outdir.mkdir(parents=True, exist_ok=True)

            payload = write_single_lane_result_files(
                summary_path=summary_path,
                csv_path=csv_path,
                csv_header=["lane", "result"],
                csv_row=["lane-a", "pass"],
                lane_result="pass",
                outdir=root,
                app=Path("/tmp/app"),
                datadir=None,
                lane_outdir=lane_outdir,
                summary_extra={"EXTRA_FIELD": "ok"},
            )
            self.assertEqual(payload["RESULT"], "pass")
            self.assertEqual(payload["PASS_LANES"], 1)
            self.assertEqual(payload["FAIL_LANES"], 0)
            self.assertEqual(parse_summary_key_last(summary_path, "RESULT"), "pass")
            self.assertEqual(parse_summary_key_last(summary_path, "EXTRA_FIELD"), "ok")
            rows = csv_path.read_text(encoding="utf-8").strip().splitlines()
            self.assertEqual(rows[0], "lane,result")
            self.assertEqual(rows[1], "lane-a,pass")

    def test_mapgen_metric_resolution_uses_known_order(self) -> None:
        rows = [
            {
                "players": "4",
                "rooms": "100",
                "gold_amount": "1234",
                "decorations": "77",
                "decorations_utility": "5",
            }
        ]
        metrics = resolve_mapgen_metrics_from_rows(rows)
        self.assertEqual(
            metrics,
            ["rooms", "decorations", "gold_amount", "decorations_utility"],
        )

    def test_logscan_cache_refreshes_after_file_change(self) -> None:
        with tempfile.TemporaryDirectory(prefix="barony-smoke-logcache-") as tmp:
            log_path = Path(tmp) / "log.txt"
            log_path.write_text("alpha\nbeta\n", encoding="utf-8")
            cache = LogScanCache()
            self.assertEqual(file_count_fixed_lines(log_path, "alpha", scan_cache=cache), 1)

            log_path.write_text("alpha\nbeta\nalpha\n", encoding="utf-8")
            self.assertEqual(file_count_fixed_lines(log_path, "alpha", scan_cache=cache), 2)

    def test_mapgen_metrics_parser_keeps_event_alignment(self) -> None:
        with tempfile.TemporaryDirectory(prefix="barony-smoke-mapgen-parse-") as tmp:
            host_log = Path(tmp) / "host.log"
            host_log.write_text(
                "\n".join(
                    [
                        "successfully generated a dungeon with 10 rooms, 11 monsters, 12 gold, 13 items, 14 decorations level=7 secret=0 seed=100 players=4",
                        "mapgen value summary: gold_bags=2 gold_amount=200 item_stacks=3 item_units=30 level=7",
                        "successfully generated a dungeon with 20 rooms, 21 monsters, 22 gold, 23 items, 24 decorations level=8 secret=1 seed=200 players=5",
                        "mapgen food summary: food=9 food_servings=18 level=8",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            rows = parse_mapgen_metrics_lines(host_log, 2)
            self.assertEqual(len(rows), 2)
            self.assertEqual(rows[0]["mapgen_seed_observed"], "100")
            self.assertEqual(rows[0]["gold_bags"], "2")
            self.assertEqual(rows[1]["mapgen_seed_observed"], "200")
            self.assertEqual(rows[1]["food_items"], "9")


if __name__ == "__main__":
    unittest.main()
