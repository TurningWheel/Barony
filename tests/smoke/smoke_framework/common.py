from __future__ import annotations

import datetime as dt


def log(message: str) -> None:
    now = dt.datetime.now().strftime("%H:%M:%S")
    print(f"[{now}] {message}")


def fail(message: str) -> None:
    raise SystemExit(message)


def require_uint(name: str, value: int, minimum: int | None = None, maximum: int | None = None) -> None:
    if value < 0:
        fail(f"{name} must be a non-negative integer")
    if minimum is not None and value < minimum:
        fail(f"{name} must be >= {minimum}")
    if maximum is not None and value > maximum:
        fail(f"{name} must be <= {maximum}")
