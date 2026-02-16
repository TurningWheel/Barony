from __future__ import annotations

from pathlib import Path


def parse_key_value_tokens(line: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for token in line.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        values[key] = value
    return values


def last_line_with_prefix(path: Path, prefix: str) -> str:
    if not path.exists():
        return ""
    last = ""
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if prefix in line:
                last = line
    return last


def last_prefixed_metric_str(path: Path, prefix: str, key: str, default: str = "") -> str:
    line = last_line_with_prefix(path, prefix)
    if line == "":
        return default
    return parse_key_value_tokens(line).get(key, default)


def last_prefixed_metric_int(path: Path, prefix: str, key: str, default: int = 0) -> int:
    value = last_prefixed_metric_str(path, prefix, key, str(default))
    try:
        return int(value)
    except (TypeError, ValueError):
        return default
