from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

from .common import fail
from .process import run_command
from .summary import parse_summary_values


@dataclass(frozen=True)
class RunnerOrchestrator:
    runner_path: Path
    runner_python: str

    def validate_app_environment(self, app: Path, datadir: Path | None) -> None:
        if not app.is_file():
            fail(f"Barony executable not found: {app}")
        if not app.stat().st_mode & 0o111:
            fail(f"Barony executable is not executable: {app}")
        if datadir and not datadir.is_dir():
            fail(f"--datadir must reference an existing directory: {datadir}")

    def validate_lane_environment(self, app: Path, datadir: Path | None) -> None:
        if not self.runner_path.is_file():
            fail(f"Missing lane runner: {self.runner_path}")
        self.validate_app_environment(app, datadir)

    def build_runner_subcommand_prefix(self, lane: str) -> list[str]:
        return [self.runner_python, str(self.runner_path), lane]

    def build_nested_runner_cmd(
        self,
        *,
        lane: str,
        app: Path,
        datadir: Path | None,
        lane_outdir: Path | None = None,
        lane_args: Sequence[str] = (),
    ) -> list[str]:
        cmd: list[str] = [*self.build_runner_subcommand_prefix(lane), "--app", str(app), *lane_args]
        if datadir:
            cmd.extend(["--datadir", str(datadir)])
        if lane_outdir is not None:
            cmd.extend(["--outdir", str(lane_outdir)])
        return cmd

    def build_helo_lane_base_cmd(
        self,
        *,
        app: Path,
        datadir: Path | None,
        instances: int,
        expected_players: int,
        size: str,
        stagger: int,
        timeout: int,
        lane_outdir: Path,
    ) -> list[str]:
        cmd = [
            *self.build_runner_subcommand_prefix("lan-helo-chunk"),
            "--app",
            str(app),
            "--instances",
            str(instances),
            "--expected-players",
            str(expected_players),
            "--size",
            size,
            "--stagger",
            str(stagger),
            "--timeout",
            str(timeout),
            "--outdir",
            str(lane_outdir),
        ]
        if datadir:
            cmd.extend(["--datadir", str(datadir)])
        return cmd

    def build_mapgen_lane_cmd(
        self,
        *,
        app: Path,
        datadir: Path | None,
        launched_instances: int,
        expected_players: int,
        size: str,
        stagger: int,
        timeout: int,
        lane_outdir: Path,
        auto_start_delay: int,
        auto_enter_dungeon: int,
        auto_enter_dungeon_delay: int,
        force_chunk: int,
        chunk_payload_max: int,
        seed: int,
        require_helo: int,
        start_floor: int,
        mapgen_reload_same_level: int,
        mapgen_reload_seed_base: int,
        auto_enter_dungeon_repeats: int | None = None,
        mapgen_samples: int | None = None,
        mapgen_players_override: int | str | None = None,
        mapgen_control_file: Path | None = None,
    ) -> list[str]:
        cmd = self.build_helo_lane_base_cmd(
            app=app,
            datadir=datadir,
            instances=launched_instances,
            expected_players=expected_players,
            size=size,
            stagger=stagger,
            timeout=timeout,
            lane_outdir=lane_outdir,
        )
        cmd.extend(
            [
                "--auto-start",
                "1",
                "--auto-start-delay",
                str(auto_start_delay),
                "--auto-enter-dungeon",
                str(auto_enter_dungeon),
                "--auto-enter-dungeon-delay",
                str(auto_enter_dungeon_delay),
            ]
        )
        if auto_enter_dungeon_repeats is not None:
            cmd.extend(["--auto-enter-dungeon-repeats", str(auto_enter_dungeon_repeats)])
        cmd.extend(
            [
                "--force-chunk",
                str(force_chunk),
                "--chunk-payload-max",
                str(chunk_payload_max),
                "--seed",
                str(seed),
                "--require-helo",
                str(require_helo),
                "--require-mapgen",
                "1",
                "--mapgen-reload-same-level",
                str(mapgen_reload_same_level),
                "--mapgen-reload-seed-base",
                str(mapgen_reload_seed_base),
                "--start-floor",
                str(start_floor),
            ]
        )
        if mapgen_samples is not None:
            cmd.extend(["--mapgen-samples", str(mapgen_samples)])
        if mapgen_players_override not in (None, ""):
            cmd.extend(["--mapgen-players-override", str(mapgen_players_override)])
        if mapgen_control_file:
            cmd.extend(["--mapgen-control-file", str(mapgen_control_file)])
        return cmd

    def run_helo_child_lane(
        self,
        *,
        app: Path,
        datadir: Path | None,
        instances: int,
        expected_players: int,
        size: str,
        stagger: int,
        timeout: int,
        lane_outdir: Path,
        lane_args: Sequence[str],
        summary_defaults: dict[str, str] | None = None,
        extra_env: dict[str, str] | None = None,
    ) -> tuple[int, str, dict[str, str], Path]:
        cmd = self.build_helo_lane_base_cmd(
            app=app,
            datadir=datadir,
            instances=instances,
            expected_players=expected_players,
            size=size,
            stagger=stagger,
            timeout=timeout,
            lane_outdir=lane_outdir,
        )
        cmd.extend(lane_args)
        rc = run_command(cmd, extra_env)
        child_result = "pass" if rc == 0 else "fail"
        summary_file = lane_outdir / "summary.env"
        values = parse_summary_values(summary_file, summary_defaults or {})
        return rc, child_result, values, summary_file
