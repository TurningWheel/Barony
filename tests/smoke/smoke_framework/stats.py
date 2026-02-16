from __future__ import annotations

import math
from collections.abc import Iterable, Sequence


def parse_float_or_none(value: str | float | int | None) -> float | None:
    if value is None:
        return None
    try:
        return float(str(value).strip())
    except (TypeError, ValueError):
        return None


def parse_int_or_none(value: str | float | int | None) -> int | None:
    parsed = parse_float_or_none(value)
    if parsed is None:
        return None
    return int(parsed)


def mean(values: Iterable[float]) -> float | None:
    items = list(values)
    if not items:
        return None
    return sum(items) / len(items)


def variance(values: Iterable[float]) -> float | None:
    items = list(values)
    if len(items) < 2:
        return None
    avg = mean(items)
    if avg is None:
        return None
    return sum((value - avg) ** 2 for value in items) / (len(items) - 1)


def stddev(values: Iterable[float]) -> float | None:
    var = variance(values)
    if var is None:
        return None
    return math.sqrt(var)


def covariance(xs: Sequence[float], ys: Sequence[float]) -> float | None:
    if len(xs) != len(ys) or len(xs) < 2:
        return None
    x_avg = mean(xs)
    y_avg = mean(ys)
    if x_avg is None or y_avg is None:
        return None
    return sum((x - x_avg) * (y - y_avg) for x, y in zip(xs, ys)) / (len(xs) - 1)


def linear_slope(xs: Sequence[float], ys: Sequence[float]) -> float | None:
    cov = covariance(xs, ys)
    var_x = variance(xs)
    if cov is None or var_x is None or var_x == 0:
        return None
    return cov / var_x


def correlation(xs: Sequence[float], ys: Sequence[float]) -> float | None:
    cov = covariance(xs, ys)
    x_stddev = stddev(xs)
    y_stddev = stddev(ys)
    if cov is None or x_stddev is None or y_stddev is None or x_stddev == 0 or y_stddev == 0:
        return None
    return cov / (x_stddev * y_stddev)
