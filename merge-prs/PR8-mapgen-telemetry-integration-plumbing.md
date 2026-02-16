# [PR8] Mapgen Telemetry and Integration Plumbing (No Balance Policy Change)

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Medium
- Status (Updated 2026-02-14): Planned, waiting for clean mapgen-only extraction
- Depends On: PR6 (and PR7 smoke-runner baseline preferred)
- Blocks: PR9 balancing with reproducible evidence

## Progress Snapshot (2026-02-14)
- Mapgen runner orchestration now lives in Python tooling (CLI registration in `tests/smoke/smoke_runner.py`, lane logic in `tests/smoke/smoke_framework/mapgen_lanes.py`), not shell wrappers.
- Report generation tooling is Python-based:
  - `tests/smoke/generate_mapgen_heatmap.py`
  - `tests/smoke/generate_smoke_aggregate_report.py`
- Remaining split requirement: keep PR8 scoped to mapgen-related tooling/lane behavior and smoke C++ integration only.

## Preconditions Before PR8 Extraction
- PR7 non-mapgen scope must be frozen so any residual smoke tooling diffs can be clearly bucketed as mapgen-only.
- Candidate PR8 diff must pass two hard filters:
  - include only mapgen-related `tests/smoke/*` changes
  - include only smoke-telemetry (not balance math) changes in `src/maps.cpp`

## Background
Mapgen tuning must be evidence-driven. Before gameplay coefficients change, we need stable telemetry plus fast, reproducible integration lanes that are clearly separated from gameplay balancing policy.

## What and Why
Land mapgen instrumentation and integration plumbing first so PR9 can be reviewed as pure gameplay/balance policy with measurable before/after outputs.

## Scope
### In Scope
- `src/smoke/SmokeHooksMapgen.cpp`
- `src/smoke/SmokeTestHooks.hpp`
- `src/game.cpp` wiring-only support for `-smoke-mapgen-integration*`
- Smoke-only summary/logging callsites in `src/maps.cpp`
- Mapgen tooling and lane behavior in:
  - mapgen registration portions of `tests/smoke/smoke_runner.py`
  - mapgen lane/module portions of `tests/smoke/smoke_framework/mapgen_lanes.py` (plus mapgen-specific helpers only)
  - `tests/smoke/generate_mapgen_heatmap.py`
  - `tests/smoke/generate_smoke_aggregate_report.py`

### Out of Scope
- Overflow tuning constants/divisors in `src/maps.cpp`
- Gameplay balance policy changes
- Non-mapgen lane behavior changes in `tests/smoke/smoke_runner.py` or non-mapgen `tests/smoke/smoke_framework/*`
- Build-system default flips

## Implementation Instructions
1. Implement mapgen integration parser/validator/runner in `src/smoke/SmokeHooksMapgen.cpp`.
2. Keep `src/game.cpp` limited to option wiring/dispatch (`-smoke-mapgen-integration*`).
3. Add smoke-guarded mapgen summary emission in `src/maps.cpp`; no gameplay formula changes.
4. Keep mapgen-runner behavior deterministic and artifact-oriented (`summary.env`, CSV, report outputs) with mapgen logic in mapgen-specific modules.
5. Ensure per-run HOME isolation and datadir handling are documented and validated.
6. Ensure integration seed root is auto-generated per invocation (no old fixed base-seed dependency).
7. Reject this PR if any `src/maps.cpp` hunk changes gameplay balance math.

## Suggested Commit Structure
1. `SmokeHooksMapgen.cpp` integration runner and schema output.
2. `src/game.cpp` wiring-only CLI support.
3. Mapgen tooling (`smoke_runner.py` mapgen lanes + report generators).
4. Optional mapgen helper module extractions if needed to keep PR8 reviewable (must stay mapgen-only).
5. Smoke-only mapgen telemetry callsites in `src/maps.cpp`.

## Validation Plan
- Smoke-enabled build:
```bash
cmake -S . -B build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build build-mac-smoke -j8 --target barony
```
- Runner sanity:
```bash
python3 -m py_compile tests/smoke/smoke_runner.py tests/smoke/smoke_framework/*.py
python3 tests/smoke/smoke_runner.py mapgen-sweep --help
python3 tests/smoke/smoke_runner.py mapgen-level-matrix --help
```
- Integration preflight (`runs=2`) and volatility lane (`runs=5`) with `levels=1,7,16,33`, `players=1..15`.
- CSV schema validation for required columns.
- Integration parity checks against single-runtime matrix where applicable.

## Acceptance Criteria
- [ ] `-smoke-mapgen-integration*` CLI is wired through `src/game.cpp` and executed by `src/smoke/SmokeHooksMapgen.cpp`.
- [ ] Mapgen lanes generate expected artifact set (`csv`, aggregate HTML, heatmap, summary env).
- [ ] `src/maps.cpp` changes are smoke-guarded telemetry only.
- [ ] Integration lanes pass with reproducible commands and stored artifacts.
- [ ] No gameplay coefficient or balancing logic changes are present.
- [ ] No non-mapgen lane behavior changes are mixed into this PR (`smoke_runner.py` and `smoke_framework/*`).

## Remaining Work Before PR Cut
1. Isolate mapgen-only runner/module hunks from current `tests/smoke/*` branch state.
2. Confirm `src/game.cpp` remains wiring-only for `-smoke-mapgen-integration*`.
3. Re-audit `src/maps.cpp` hunks to verify smoke-guarded telemetry-only changes.
4. Run mapgen command/help sanity and one integration preflight artifact run from the extracted candidate branch.

## Review Focus
- Strict separation: wiring in `game.cpp`, integration logic in `src/smoke/SmokeHooksMapgen.cpp`.
- Mapgen-only scope within smoke tooling (`smoke_runner.py` and `smoke_framework/*`).
- Telemetry schema stability and artifact completeness.
- Absence of gameplay balance deltas.

## Rollback Strategy
Revert PR8 to remove mapgen telemetry/plumbing without touching gameplay balancing policy.

## Extraction Plan (from PR #940 / 8p-mod / current branch)
- Extract instrumentation-only slices from `f4da9ee9`, `a35e2c7b`, `9ce7ac93` plus mapgen-specific smoke tooling hunks.
- Enforce a split check that rejects:
  - non-telemetry `src/maps.cpp` hunks
  - non-mapgen lane behavior changes in `tests/smoke/smoke_runner.py` or non-mapgen `tests/smoke/smoke_framework/*`
