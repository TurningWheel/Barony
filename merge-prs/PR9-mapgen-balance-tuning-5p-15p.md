# [PR9] Mapgen Balance Tuning for 5-15 Players (Gameplay Only)

## Ticket Metadata
- Type: Engineering
- Priority: Critical
- Epic: Multiplayer Expansion 1-15
- Risk: High
- Status (Updated 2026-02-14): Planned, blocked on PR8 extraction/merge
- Depends On: PR8
- Blocks: PR10 default enablement

## Background
The largest gameplay risk is map generation balance at high player counts. PR8 provides instrumentation/plumbing; PR9 now applies policy tuning with strict 1-4 parity and reproducible evidence requirements.

## Progress Snapshot (2026-02-14)
- Smoke tooling prerequisites are in place on branch (Python CLI and framework modules), and shell wrappers are removed.
- PR9 remains intentionally unstarted from an extraction standpoint until PR8 mapgen plumbing is isolated.

## Preconditions Before PR9 Extraction
- PR8 must be extracted/merged (or at minimum frozen) so PR9 can stay gameplay-only.
- PR9 candidate diff must exclude all tooling/build/telemetry-plumbing changes and focus primarily on `src/maps.cpp` balance logic.

## What and Why
Tune mapgen for 5-15 players to improve large-party pacing/economy while preserving 1-4 behavior and maintaining stable progression quality across target floors.

## Scope
### In Scope
- Primary: `src/maps.cpp` gameplay/balance logic
- Optional concise summary updates only:
  - `AGENTS.md` (`Validation Summary` and `Balancing Lessons and Guardrails`)
  - PR-level summary note in the active `merge-prs/PR*.md` ticket

### Out of Scope
- CMake/build-system changes
- Smoke framework architecture changes
- Smoke runner framework/tooling logic changes
- Lobby/protocol changes

## Implementation Instructions
1. Restrict all new balance logic to overflow players only (`connectedPlayers > 4`).
2. Keep 1-4 paths behavior-identical; no drift is permitted.
3. Tune `src/maps.cpp` coefficients with explicit, reviewable constants (table/struct form preferred for clarity).
4. Avoid broad refactors unrelated to coefficient behavior.
5. Run the full gate stack per candidate:
   - Integration preflight (`runs=2`)
   - Volatility gate (`runs=5`)
   - Full-lobby confirmation (`simulate-mapgen-players=0`, `runs=5`)
   - Low-player parity guard (`players=2..4`, `runs=5`) with row-level diff
6. Keep docs summary-only (metrics/outcome/rationale), with raw data in artifacts.

## Suggested Commit Structure
1. Mapgen coefficient/tuning changes in `src/maps.cpp`.
2. Optional concise docs update with final metrics and artifact references.

## Validation Plan
- Build smoke-enabled binary:
```bash
cmake -S . -B build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build build-mac-smoke -j8 --target barony
```
- Required mapgen runs and artifact capture:
  - `levels=1,7,16,33`, `players=1..15`, `runs=2`
  - same levels/players with `runs=5`
  - full-lobby `simulate-mapgen-players=0`, `players=1..15`, `runs=5`
  - low-player guard `players=2..4`, `runs=5` + row-level diff
- Required metrics/targets (`p15 vs p4`):
  - rooms `1.62x-1.75x`
  - monsters `1.38x-1.46x`
  - monsters/room `0.82x-0.92x`
  - gold/player `0.70x-0.80x`
  - items/player `0.70x-0.80x`
  - food/player `0.65x-0.78x`
  - decorations `1.85x-2.25x`
  - blocking share `<=45%`

## Acceptance Criteria
- [ ] Only gameplay balance deltas are included, primarily in `src/maps.cpp`.
- [ ] All overflow logic is gated to players `>4`.
- [ ] Row-level diff proves no 1-4 behavior regression.
- [ ] Required run stack and artifacts are attached (`csv`, aggregate/heatmap HTML, summary env, logs).
- [ ] Reported metrics are within acceptance bands or include explicit maintainer-approved rationale for any exception.

## Review Focus
- 1-4 parity protection.
- Coefficient clarity and behavioral intent.
- Full-lobby divergence risk vs integration-only results.

## Rollback Strategy
Revert PR9 entirely if balance regressions appear; retain PR8 instrumentation for rapid retune/retest.

## Extraction Plan (from PR #940 / 8p-mod)
Start from PR8 branch state and apply map-only gameplay hunks from `b7c36be9`, `4288b719`, `9ed76e40`, `f4da9ee9`, `a35e2c7b`, `9ce7ac93`; verify no non-mapgen tooling/build deltas remain.
