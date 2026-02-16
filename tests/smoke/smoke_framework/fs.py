from __future__ import annotations

import datetime as dt
import shutil
from pathlib import Path


def normalize_outdir(outdir: str | None, fallback_prefix: str) -> Path:
    if outdir:
        path = Path(outdir)
    else:
        stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
        path = Path(f"tests/smoke/artifacts/{fallback_prefix}-{stamp}")
    if not path.is_absolute():
        path = Path.cwd() / path
    path.mkdir(parents=True, exist_ok=True)
    return path


def reset_paths(*paths: Path) -> None:
    for path in paths:
        if path.is_dir():
            shutil.rmtree(path, ignore_errors=True)
        elif path.exists():
            path.unlink()


def prune_models_cache(lane_outdir: Path) -> None:
    cache_root = lane_outdir / "instances"
    if not cache_root.is_dir():
        cache_root = lane_outdir
    if not cache_root.is_dir():
        return
    for cache in cache_root.rglob("models.cache"):
        try:
            cache.unlink()
        except OSError:
            pass
