from __future__ import annotations

import re
from pathlib import Path


class LogScanCache:
    def __init__(self) -> None:
        self._lines_by_path: dict[Path, tuple[tuple[int, int], list[str]]] = {}
        self._fixed_count_cache: dict[tuple[Path, tuple[int, int], str], int] = {}
        self._regex_count_cache: dict[tuple[Path, tuple[int, int], str], int] = {}
        self._regex_last_cache: dict[tuple[Path, tuple[int, int], str], str] = {}
        self._regex_cache: dict[str, re.Pattern[str]] = {}

    def _signature(self, path: Path) -> tuple[int, int]:
        if not path.exists():
            return (-1, -1)
        stat = path.stat()
        return (int(stat.st_mtime_ns), int(stat.st_size))

    def lines_for(self, path: Path) -> list[str]:
        signature = self._signature(path)
        cached = self._lines_by_path.get(path)
        if cached is not None and cached[0] == signature:
            return cached[1]
        if signature == (-1, -1):
            lines: list[str] = []
        else:
            with path.open("r", encoding="utf-8", errors="replace") as f:
                lines = [line.rstrip("\n") for line in f]
        self._lines_by_path[path] = (signature, lines)
        return lines

    def _key(self, path: Path, token: str) -> tuple[Path, tuple[int, int], str]:
        signature = self._signature(path)
        self.lines_for(path)
        return (path, signature, token)

    def _regex(self, pattern: str) -> re.Pattern[str]:
        regex = self._regex_cache.get(pattern)
        if regex is None:
            regex = re.compile(pattern)
            self._regex_cache[pattern] = regex
        return regex

    def count_fixed_lines(self, path: Path, needle: str) -> int:
        key = self._key(path, needle)
        cached = self._fixed_count_cache.get(key)
        if cached is not None:
            return cached
        count = sum(1 for line in self.lines_for(path) if needle in line)
        self._fixed_count_cache[key] = count
        return count

    def count_matching_lines(self, path: Path, pattern: str) -> int:
        key = self._key(path, pattern)
        cached = self._regex_count_cache.get(key)
        if cached is not None:
            return cached
        regex = self._regex(pattern)
        count = sum(1 for line in self.lines_for(path) if regex.search(line))
        self._regex_count_cache[key] = count
        return count

    def last_matching_line(self, path: Path, pattern: str) -> str:
        key = self._key(path, pattern)
        cached = self._regex_last_cache.get(key)
        if cached is not None:
            return cached
        regex = self._regex(pattern)
        last = ""
        for line in self.lines_for(path):
            if regex.search(line):
                last = line
        self._regex_last_cache[key] = last
        return last


def file_last_matching_line(path: Path, pattern: str, *, scan_cache: LogScanCache | None = None) -> str:
    if scan_cache is not None:
        return scan_cache.last_matching_line(path, pattern)
    if not path.exists():
        return ""
    regex = re.compile(pattern)
    last = ""
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if regex.search(line):
                last = line.rstrip("\n")
    return last


def file_count_matching_lines(path: Path, pattern: str, *, scan_cache: LogScanCache | None = None) -> int:
    if scan_cache is not None:
        return scan_cache.count_matching_lines(path, pattern)
    if not path.exists():
        return 0
    regex = re.compile(pattern)
    count = 0
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if regex.search(line):
                count += 1
    return count


def file_count_fixed_lines(path: Path, needle: str, *, scan_cache: LogScanCache | None = None) -> int:
    if scan_cache is not None:
        return scan_cache.count_fixed_lines(path, needle)
    if not path.exists():
        return 0
    count = 0
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if needle in line:
                count += 1
    return count


def collect_instance_logs(lane_outdir: Path) -> list[Path]:
    instances_dir = lane_outdir / "instances"
    if not instances_dir.is_dir():
        return []
    logs = set(instances_dir.glob("home-*/.barony/log.txt"))
    # Keep compatibility with any legacy per-lane home suffix directories.
    logs.update(instances_dir.glob("home-*-l*/.barony/log.txt"))
    return sorted(logs)
