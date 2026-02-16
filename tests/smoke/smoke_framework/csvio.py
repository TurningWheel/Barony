from __future__ import annotations

import csv
from pathlib import Path
from typing import Sequence


def write_csv_header(csv_path: Path, header: Sequence[str]) -> None:
    with csv_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(list(header))


def append_csv_row(csv_path: Path, row: Sequence[str | int | float | Path]) -> None:
    with csv_path.open("a", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow([str(item) if isinstance(item, Path) else item for item in row])
