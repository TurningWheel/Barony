from __future__ import annotations

from pathlib import Path


def parse_summary_key(path: Path, key: str) -> str:
    if not path.exists():
        return ""
    prefix = f"{key}="
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith(prefix):
                return line[len(prefix) :]
    return ""


def parse_summary_key_last(path: Path, key: str) -> str:
    if not path.exists():
        return ""
    prefix = f"{key}="
    last = ""
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith(prefix):
                last = line[len(prefix) :]
    return last


def parse_summary_values(path: Path, defaults: dict[str, str]) -> dict[str, str]:
    values: dict[str, str] = {}
    for key, default in defaults.items():
        value = parse_summary_key_last(path, key)
        if value == "":
            value = default
        values[key] = value
    return values


def write_summary_env(path: Path, items: dict[str, str | int | Path]) -> None:
    with path.open("w", encoding="utf-8") as f:
        for key, value in items.items():
            f.write(f"{key}={value}\n")
