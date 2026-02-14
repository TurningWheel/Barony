# [PR8] Mapgen Telemetry and Integration Plumbing (No Balance Policy Change)

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Medium
- Depends On: PR6
- Blocks: PR9 balancing with reproducible evidence

## Background
Mapgen tuning must be evidence-driven. Before gameplay coefficients change, the project needs stable telemetry and an integration runner path that is fast, reproducible, and isolated from core gameplay logic.

## What and Why
Land mapgen instrumentation and integration plumbing first so PR9 can be reviewed as pure gameplay/balance policy with measurable before/after outputs.

## Scope
### In Scope
- `src/smoke/SmokeHooksMapgen.cpp`
- `src/smoke/SmokeTestHooks.hpp`
- `src/game.cpp` wiring-only support for `-smoke-mapgen-integration*`
- Smoke-only summary/logging callsites in `src/maps.cpp`
- Mapgen smoke tooling:
  - `tests/smoke/run_mapgen_sweep_mac.sh`
  - `tests/smoke/run_mapgen_level_matrix_mac.sh`
  - `tests/smoke/generate_mapgen_heatmap.py`
  - `tests/smoke/generate_smoke_aggregate_report.py`

### Out of Scope
- Overflow tuning constants/divisors in `src/maps.cpp`
- Gameplay balance policy changes
- Build-system default flips

## Implementation Instructions
1. Implement integration parser/validator/runner in `src/smoke/SmokeHooksMapgen.cpp` only.
2. Keep `src/game.cpp` limited to CLI option wiring/dispatch (`-smoke-mapgen-integration*`).
3. Add smoke-guarded mapgen summary emission in `src/maps.cpp` with no gameplay formula changes.
4. Add runner scripts and report tooling for matrix/full-lobby artifact generation.
5. Ensure per-run HOME isolation and datadir support are documented and used.
6. Ensure integration seed root is auto-generated per invocation (no old fixed base-seed dependency).
7. Reject this PR if `src/maps.cpp` includes any non-telemetry balancing delta.

## Suggested Commit Structure
1. `SmokeHooksMapgen.cpp` integration runner and schema output.
2. `src/game.cpp` wiring-only CLI support.
3. Mapgen runner scripts/report generators.
4. Smoke-only telemetry callsites in `src/maps.cpp`.

## Validation Plan
- Smoke-enabled build:
```bash
cmake -S . -B build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build build-mac-smoke -j8 --target barony
```
- Integration preflight (`runs=2`) and volatility lane (`runs=5`) with `levels=1,7,16,33`, `players=1..15`.
- CSV schema validation for required columns.
- Integration parity checks against single-runtime matrix where applicable.

## Acceptance Criteria
- [ ] `-smoke-mapgen-integration*` CLI is wired through `src/game.cpp` and executed by `src/smoke/SmokeHooksMapgen.cpp`.
- [ ] Mapgen runners generate expected artifact set (`csv`, aggregate HTML, heatmap, summary env).
- [ ] `src/maps.cpp` changes are smoke-guarded telemetry only.
- [ ] Integration lanes pass with reproducible commands and stored artifacts.
- [ ] No gameplay coefficient or balancing logic changes are present.

## Review Focus
- Strict separation: wiring in `game.cpp`, logic in `src/smoke/SmokeHooksMapgen.cpp`.
- Telemetry schema stability and artifact completeness.
- No accidental balance changes.

## Rollback Strategy
Revert PR8 to remove telemetry/plumbing without touching gameplay balancing policy.

## Extraction Plan (from PR #940 / 8p-mod)
Extract instrumentation-only slices from `f4da9ee9`, `a35e2c7b`, `9ce7ac93` plus supporting smoke commits; enforce a diff check that rejects non-telemetry `maps.cpp` hunks.
