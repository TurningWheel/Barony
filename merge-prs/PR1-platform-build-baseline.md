# [PR1] Platform and Build Baseline (#942-first)

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Low
- Status (Updated 2026-02-14): Planned, not yet isolated for review
- Depends On: None
- Blocks: PR2+

## Background
PR #940 mixes platform/build deltas with gameplay, networking, smoke tooling, and mapgen balance work. The upstream prerequisite PR #942 already contains a cleaner slice of cross-platform/build fixes that should land first to reduce rebase conflict and keep later PRs focused on behavior.

## What and Why
Deliver a stable cross-platform baseline before taking multiplayer risk. This lowers CI churn and avoids noisy conflicts while extracting gameplay/networking slices from `sayhiben/8p-mod`.

## Scope
### In Scope
- `CMakeLists.txt`
- `cmake/Modules/FindSDL2.cmake`
- `INSTALL.md`
- `setup_barony_vs2022_x64.ps1`
- `src/main.hpp`
- `src/init_game.cpp`
- `src/light.cpp`
- `src/steam_shared.cpp`
- `src/engine/audio/sound.hpp`
- `.gitignore`

### Out of Scope
- Any `MAXPLAYERS` behavior change
- Any `src/maps.cpp` tuning or telemetry
- Any smoke hooks/scripts
- Any lobby/join gameplay logic

## Implementation Instructions
1. Branch from `upstream/master`.
2. Prefer upstream PR #942 commit lineage (`91b96212`, `ba792cba`, `7bb1a09b`, `d9b958d8`) over mixed 8p-mod commits.
3. If #942 is already merged upstream, make this PR a no-op or a minimal superseding delta only for unresolved platform/build issues.
4. Keep changes constrained to the file list above; do not bring multiplayer code paths.
5. Validate CMake/module behavior on Linux, Windows, and macOS.

## Suggested Commit Structure
1. Build system and dependency resolution updates.
2. Platform compatibility source adjustments.
3. Install/setup doc updates.

## Validation Plan
- CI: Linux and Windows builds pass.
- Local macOS:
```bash
cmake -S . -B build-mac-pr1 -G Ninja -DFMOD_ENABLED=OFF
cmake --build build-mac-pr1 -j8 --target barony
```

## Acceptance Criteria
- [ ] Cross-platform CI is green for this PR.
- [ ] macOS configure/build succeeds locally.
- [ ] PR diff contains no gameplay/networking/mapgen changes.
- [ ] No `tests/smoke/*` files are added or modified.
- [ ] If #942 is merged, this PR cleanly supersedes only missing pieces.

## Review Focus
- CMake linking/library detection correctness.
- SDL2 and NFD compatibility handling.
- No hidden behavior changes.

## Rollback Strategy
Revert this PR fully if platform regressions appear; no downstream data migration or protocol dependency is introduced.

## Extraction Plan (from PR #940 / 8p-mod)
Use file-scoped cherry-pick/reconstruction from #942-aligned commits, avoiding mixed commits (`1f001a25`, `fcfe816e`) that include unrelated multiplayer concerns.
