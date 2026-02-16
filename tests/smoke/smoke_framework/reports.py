from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Sequence

from .common import log


def find_python3() -> str | None:
    from shutil import which

    return which("python3")


def run_optional_aggregate(aggregate_script: Path, out_html: Path, args: Sequence[str]) -> bool:
    python3 = find_python3()
    if python3 is None:
        return False
    if not aggregate_script.is_file():
        return False
    subprocess.run([python3, str(aggregate_script), "--output", str(out_html), *args], check=False)
    log(f"Aggregate report written to {out_html}")
    return True
