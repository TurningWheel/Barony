from __future__ import annotations

from collections.abc import Iterable, Mapping, Sequence

MAPGEN_BASE_METRICS: tuple[str, ...] = (
    "rooms",
    "monsters",
    "gold",
    "items",
    "food_servings",
    "decorations",
)

MAPGEN_OPTIONAL_METRICS: tuple[str, ...] = (
    "gold_bags",
    "gold_amount",
    "item_stacks",
    "item_units",
    "decorations_blocking",
    "decorations_utility",
    "decorations_traps",
    "decorations_economy",
)

MAPGEN_SCALING_METRICS: tuple[str, ...] = (
    *MAPGEN_BASE_METRICS,
    *MAPGEN_OPTIONAL_METRICS,
)

MAPGEN_TREND_METRICS: tuple[str, ...] = (
    "rooms",
    "monsters",
    "gold",
    "gold_bags",
    "gold_amount",
    "items",
    "item_stacks",
    "item_units",
    "food_servings",
    "decorations",
    "decorations_blocking",
    "decorations_utility",
    "decorations_traps",
    "decorations_economy",
)


def resolve_mapgen_metrics(keys: Iterable[str], *, preferred: Sequence[str] = MAPGEN_SCALING_METRICS) -> list[str]:
    key_set = set(keys)
    return [metric for metric in preferred if metric in key_set]


def resolve_mapgen_metrics_from_rows(
    rows: Sequence[Mapping[str, str]],
    *,
    preferred: Sequence[str] = MAPGEN_SCALING_METRICS,
    default: Sequence[str] = MAPGEN_BASE_METRICS,
) -> list[str]:
    if not rows:
        return list(default)
    keys: set[str] = set()
    for row in rows:
        keys.update(row.keys())
    return resolve_mapgen_metrics(keys, preferred=preferred)
