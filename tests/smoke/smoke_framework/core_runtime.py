from __future__ import annotations

import sys
from pathlib import Path

from .orchestration import RunnerOrchestrator

SCRIPT_DIR = Path(__file__).resolve().parent.parent
RUNNER = SCRIPT_DIR / "smoke_runner.py"
RUNNER_PYTHON = sys.executable or "python3"
AGGREGATE = SCRIPT_DIR / "generate_smoke_aggregate_report.py"

_ORCH = RunnerOrchestrator(runner_path=RUNNER, runner_python=RUNNER_PYTHON)
validate_lane_environment = _ORCH.validate_lane_environment
run_helo_child_lane = _ORCH.run_helo_child_lane
