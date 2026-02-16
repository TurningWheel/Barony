from __future__ import annotations

import argparse
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

from .common import fail, log, require_uint
from .csvio import append_csv_row, write_csv_header
from .fs import normalize_outdir, reset_paths
from .helo_metrics import canonicalize_lan_tx_mode
from .lane_status import pass_fail, result_from_failures
from .logscan import file_count_fixed_lines
from .process import launch_local_instance, terminate_process_group
from .reports import run_optional_aggregate
from .summary import write_summary_env

AGGREGATE = Path(__file__).resolve().parent.parent / "generate_smoke_aggregate_report.py"


@dataclass(frozen=True)
class ChurnArtifacts:
    outdir: Path
    stdout_dir: Path
    instance_root: Path
    summary_path: Path
    churn_csv_path: Path
    ready_csv_path: Path
    host_log: Path


@dataclass(frozen=True)
class ReadySyncStats:
    expected_total: int
    queue_lines: int
    sent_lines: int
    failed: int


class SlotLifecycle:
    def __init__(self, ns: argparse.Namespace, artifacts: ChurnArtifacts) -> None:
        self.ns = ns
        self.artifacts = artifacts
        self.slot_procs: dict[int, subprocess.Popen[bytes] | None] = {
            slot: None for slot in range(0, ns.instances + 1)
        }
        self.slot_launch_count: dict[int, int] = {slot: 0 for slot in range(0, ns.instances + 1)}
        self.all_procs: list[subprocess.Popen[bytes]] = []

    def launch_slot(self, slot: int, role: str) -> None:
        launch_num = self.slot_launch_count[slot] + 1
        self.slot_launch_count[slot] = launch_num
        home_dir = self.artifacts.instance_root / f"home-{slot}-l{launch_num}"
        stdout_log = self.artifacts.stdout_dir / f"instance-{slot}-l{launch_num}.stdout.log"
        home_dir.mkdir(parents=True, exist_ok=True)

        env = {
            "BARONY_SMOKE_AUTOPILOT": "1",
            "BARONY_SMOKE_CONNECT_DELAY_SECS": "2",
            "BARONY_SMOKE_RETRY_DELAY_SECS": "3",
            "BARONY_SMOKE_AUTO_READY": str(self.ns.auto_ready),
            "BARONY_SMOKE_FORCE_HELO_CHUNK": str(self.ns.force_chunk),
            "BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX": str(self.ns.chunk_payload_max),
        }
        if role == "host":
            env.update(
                {
                    "BARONY_SMOKE_ROLE": "host",
                    "BARONY_SMOKE_EXPECTED_PLAYERS": str(self.ns.instances),
                    "BARONY_SMOKE_AUTO_START": "0",
                    "BARONY_SMOKE_AUTO_ENTER_DUNGEON": "0",
                    "BARONY_SMOKE_HELO_CHUNK_TX_MODE": self.ns.helo_chunk_tx_mode,
                }
            )
            if self.ns.trace_ready_sync:
                env["BARONY_SMOKE_TRACE_READY_SYNC"] = "1"
            if self.ns.trace_join_rejects:
                env["BARONY_SMOKE_TRACE_JOIN_REJECTS"] = "1"
        else:
            env.update(
                {
                    "BARONY_SMOKE_ROLE": "client",
                    "BARONY_SMOKE_CONNECT_ADDRESS": self.ns.connect_address,
                }
            )

        proc = launch_local_instance(
            app=self.ns.app,
            datadir=self.ns.datadir,
            size=self.ns.size,
            home_dir=home_dir,
            stdout_log=stdout_log,
            extra_env=env,
            set_home=True,
        )
        self.slot_procs[slot] = proc
        self.all_procs.append(proc)
        log(f"launch slot={slot} role={role} launch={launch_num} pid={proc.pid} home={home_dir}")

    def stop_slot(self, slot: int) -> None:
        proc = self.slot_procs.get(slot)
        if proc is None:
            return
        if proc.poll() is not None:
            self.slot_procs[slot] = None
            return
        log(f"stop slot={slot} pid={proc.pid}")
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                pass
        self.slot_procs[slot] = None

    def cleanup(self) -> None:
        terminate_process_group(
            self.all_procs,
            keep_running=self.ns.keep_running,
            keep_running_message="--keep-running enabled; leaving instances alive",
            logger=log,
            grace_seconds=1.0,
        )


def validate_join_leave_churn_args(ns: argparse.Namespace) -> None:
    require_uint("--instances", ns.instances, minimum=3, maximum=15)
    require_uint("--churn-cycles", ns.churn_cycles, minimum=1)
    require_uint("--churn-count", ns.churn_count, minimum=1, maximum=14)
    if ns.churn_count >= ns.instances:
        fail("--churn-count must be >= 1 and < instances")
    require_uint("--stagger", ns.stagger)
    require_uint("--initial-timeout", ns.initial_timeout)
    require_uint("--cycle-timeout", ns.cycle_timeout)
    require_uint("--settle", ns.settle)
    require_uint("--churn-gap", ns.churn_gap)
    require_uint("--force-chunk", ns.force_chunk, minimum=0, maximum=1)
    require_uint("--chunk-payload-max", ns.chunk_payload_max, minimum=64, maximum=900)
    require_uint("--auto-ready", ns.auto_ready, minimum=0, maximum=1)
    require_uint("--trace-ready-sync", ns.trace_ready_sync, minimum=0, maximum=1)
    require_uint("--require-ready-sync", ns.require_ready_sync, minimum=0, maximum=1)
    require_uint("--trace-join-rejects", ns.trace_join_rejects, minimum=0, maximum=1)
    if ns.require_ready_sync and not ns.auto_ready:
        fail("--require-ready-sync requires --auto-ready 1")
    if ns.require_ready_sync and not ns.trace_ready_sync:
        fail("--require-ready-sync requires --trace-ready-sync 1")
    normalized_mode = canonicalize_lan_tx_mode(ns.helo_chunk_tx_mode)
    if normalized_mode is None:
        fail(
            "--helo-chunk-tx-mode must be one of: normal, reverse, even-odd, "
            "duplicate-first, drop-last, duplicate-conflict-first"
        )
    ns.helo_chunk_tx_mode = normalized_mode


def prepare_churn_artifacts(ns: argparse.Namespace) -> ChurnArtifacts:
    outdir = normalize_outdir(
        ns.outdir,
        f"churn-p{ns.instances}-c{ns.churn_cycles}x{ns.churn_count}",
    )
    stdout_dir = outdir / "stdout"
    instance_root = outdir / "instances"
    summary_path = outdir / "summary.env"
    churn_csv_path = outdir / "churn_cycle_results.csv"
    ready_csv_path = outdir / "ready_sync_results.csv"
    reset_paths(stdout_dir, instance_root, summary_path, churn_csv_path, ready_csv_path)
    stdout_dir.mkdir(parents=True, exist_ok=True)
    instance_root.mkdir(parents=True, exist_ok=True)

    write_csv_header(
        churn_csv_path,
        ["cycle", "required_host_chunk_lines", "observed_host_chunk_lines", "status"],
    )
    write_csv_header(
        ready_csv_path,
        [
            "player",
            "expected_min_queued",
            "observed_queued",
            "expected_min_sent",
            "observed_sent",
            "status",
        ],
    )

    return ChurnArtifacts(
        outdir=outdir,
        stdout_dir=stdout_dir,
        instance_root=instance_root,
        summary_path=summary_path,
        churn_csv_path=churn_csv_path,
        ready_csv_path=ready_csv_path,
        host_log=instance_root / "home-1-l1/.barony/log.txt",
    )


def wait_for_chunk_target(host_log: Path, target: int, timeout_seconds: int, label: str) -> tuple[int, bool]:
    deadline = time.monotonic() + timeout_seconds
    count = 0
    while time.monotonic() < deadline:
        count = file_count_fixed_lines(host_log, "sending chunked HELO:")
        if count >= target:
            log(f"{label} reached chunk target {count}/{target}")
            return count, True
        time.sleep(1)
    log(f"{label} timed out waiting for chunk target: got={count} need={target}")
    return count, False


def append_cycle_result(
    churn_csv_path: Path, cycle: int, required_target: int, observed_count: int, reached: bool
) -> None:
    append_csv_row(
        churn_csv_path,
        [cycle, required_target, observed_count, pass_fail(reached)],
    )


def build_churn_slots(instances: int, churn_count: int) -> list[int]:
    slots: list[int] = []
    for slot in range(instances, 1, -1):
        if len(slots) >= churn_count:
            break
        slots.append(slot)
    return slots


def run_churn_cycles(
    lifecycle: SlotLifecycle,
    ns: argparse.Namespace,
    artifacts: ChurnArtifacts,
    *,
    churn_slots: list[int],
    initial_required_target: int,
) -> tuple[int, bool]:
    required_target = initial_required_target
    for cycle in range(1, ns.churn_cycles + 1):
        log(f"cycle {cycle}/{ns.churn_cycles}: churn slots={' '.join(str(s) for s in churn_slots)}")
        for slot in churn_slots:
            lifecycle.stop_slot(slot)
        if ns.churn_gap > 0:
            time.sleep(ns.churn_gap)
        for slot in churn_slots:
            lifecycle.launch_slot(slot, "client")
            if ns.stagger > 0:
                time.sleep(ns.stagger)

        required_target += ns.churn_count
        observed_count, reached = wait_for_chunk_target(
            artifacts.host_log, required_target, ns.cycle_timeout, f"cycle-{cycle}"
        )
        append_cycle_result(artifacts.churn_csv_path, cycle, required_target, observed_count, reached)
        if not reached:
            return required_target, False
    return required_target, True


def collect_ready_sync_stats(
    ns: argparse.Namespace, artifacts: ChurnArtifacts, required_target: int
) -> ReadySyncStats:
    if not ns.require_ready_sync:
        return ReadySyncStats(expected_total=0, queue_lines=0, sent_lines=0, failed=0)

    expected_total = required_target
    queue_lines = file_count_fixed_lines(artifacts.host_log, "[SMOKE]: ready snapshot queued target=")
    sent_lines = file_count_fixed_lines(artifacts.host_log, "[SMOKE]: ready snapshot sent target=")
    failed = 0
    if queue_lines < expected_total or sent_lines < expected_total:
        failed = 1

    for player in range(1, ns.instances):
        expected_min = 1
        observed_queued = file_count_fixed_lines(
            artifacts.host_log, f"[SMOKE]: ready snapshot queued target={player} "
        )
        observed_sent = file_count_fixed_lines(
            artifacts.host_log, f"[SMOKE]: ready snapshot sent target={player} "
        )
        row_ok = observed_queued >= expected_min and observed_sent >= expected_min
        if not row_ok:
            failed = 1
        append_csv_row(
            artifacts.ready_csv_path,
            [
                player,
                expected_min,
                observed_queued,
                expected_min,
                observed_sent,
                pass_fail(row_ok),
            ],
        )

    return ReadySyncStats(
        expected_total=expected_total,
        queue_lines=queue_lines,
        sent_lines=sent_lines,
        failed=failed,
    )


def build_join_leave_churn_summary(
    ns: argparse.Namespace,
    artifacts: ChurnArtifacts,
    *,
    result: str,
    initial_target: int,
    required_target: int,
    final_host_chunk_lines: int,
    join_fail_lines: int,
    join_reject_trace_lines: int,
    ready_stats: ReadySyncStats,
) -> dict[str, str | int | Path]:
    return {
        "RESULT": result,
        "DATADIR": ns.datadir or "",
        "INSTANCES": ns.instances,
        "CHURN_CYCLES": ns.churn_cycles,
        "CHURN_COUNT": ns.churn_count,
        "FORCE_CHUNK": ns.force_chunk,
        "CHUNK_PAYLOAD_MAX": ns.chunk_payload_max,
        "HELO_CHUNK_TX_MODE": ns.helo_chunk_tx_mode,
        "AUTO_READY": ns.auto_ready,
        "TRACE_READY_SYNC": ns.trace_ready_sync,
        "REQUIRE_READY_SYNC": ns.require_ready_sync,
        "TRACE_JOIN_REJECTS": ns.trace_join_rejects,
        "INITIAL_REQUIRED_HOST_CHUNK_LINES": initial_target,
        "FINAL_REQUIRED_HOST_CHUNK_LINES": required_target,
        "FINAL_HOST_CHUNK_LINES": final_host_chunk_lines,
        "JOIN_FAIL_LINES": join_fail_lines,
        "JOIN_REJECT_TRACE_LINES": join_reject_trace_lines,
        "READY_SNAPSHOT_EXPECTED_TOTAL": ready_stats.expected_total,
        "READY_SNAPSHOT_QUEUE_LINES": ready_stats.queue_lines,
        "READY_SNAPSHOT_SENT_LINES": ready_stats.sent_lines,
        "READY_SYNC_RESULT": result_from_failures(ready_stats.failed),
        "CHURN_CSV": artifacts.churn_csv_path,
        "READY_SYNC_CSV": artifacts.ready_csv_path,
    }


def cmd_join_leave_churn(ns: argparse.Namespace) -> int:
    validate_join_leave_churn_args(ns)
    artifacts = prepare_churn_artifacts(ns)
    lifecycle = SlotLifecycle(ns, artifacts)

    log(f"Artifacts: {artifacts.outdir}")
    failures = 0
    initial_target = ns.instances - 1
    required_target = initial_target

    try:
        lifecycle.launch_slot(1, "host")
        if ns.stagger > 0:
            time.sleep(ns.stagger)
        for slot in range(2, ns.instances + 1):
            lifecycle.launch_slot(slot, "client")
            if ns.stagger > 0:
                time.sleep(ns.stagger)

        observed_count, reached = wait_for_chunk_target(
            artifacts.host_log, initial_target, ns.initial_timeout, "initial"
        )
        append_cycle_result(artifacts.churn_csv_path, 0, initial_target, observed_count, reached)
        if not reached:
            fail("Initial lobby did not reach expected HELO chunk target")

        if ns.settle > 0:
            log(f"settling for {ns.settle}s before churn cycles")
            time.sleep(ns.settle)

        churn_slots = build_churn_slots(ns.instances, ns.churn_count)
        required_target, all_cycles_reached = run_churn_cycles(
            lifecycle,
            ns,
            artifacts,
            churn_slots=churn_slots,
            initial_required_target=initial_target,
        )
        if not all_cycles_reached:
            failures += 1

        final_host_chunk_lines = file_count_fixed_lines(artifacts.host_log, "sending chunked HELO:")
        join_fail_lines = file_count_fixed_lines(artifacts.host_log, "Player failed to join lobby")
        join_reject_trace_lines = file_count_fixed_lines(
            artifacts.host_log, "[SMOKE]: lobby join reject code="
        )
        ready_stats = collect_ready_sync_stats(ns, artifacts, required_target)
        if ready_stats.failed:
            failures += 1

        result = result_from_failures(failures)
        write_summary_env(
            artifacts.summary_path,
            build_join_leave_churn_summary(
                ns,
                artifacts,
                result=result,
                initial_target=initial_target,
                required_target=required_target,
                final_host_chunk_lines=final_host_chunk_lines,
                join_fail_lines=join_fail_lines,
                join_reject_trace_lines=join_reject_trace_lines,
                ready_stats=ready_stats,
            ),
        )
        run_optional_aggregate(
            AGGREGATE,
            artifacts.outdir / "smoke_aggregate_report.html",
            ["--churn-csv", str(artifacts.churn_csv_path)],
        )
        log(
            f"result={result} required={required_target} observed={final_host_chunk_lines} "
            f"joinFail={join_fail_lines} readySyncFail={ready_stats.failed} "
            f"readyQueued={ready_stats.queue_lines}/{ready_stats.expected_total} "
            f"readySent={ready_stats.sent_lines}/{ready_stats.expected_total}"
        )
        log(f"summary={artifacts.summary_path}")
        log(f"csv={artifacts.churn_csv_path}")
        return 1 if result != "pass" else 0
    finally:
        lifecycle.cleanup()
