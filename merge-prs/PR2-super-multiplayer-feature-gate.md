# [PR2] Super-Multiplayer Feature Gate and 15-Player Scaffolding

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Medium
- Depends On: PR1
- Blocks: PR3, PR4, PR5, PR10

## Background
The project needs a controlled way to compile/run 1-15 player support incrementally without changing default behavior during stabilization. A compile-time gate avoids forcing unfinished networking/mapgen paths into default builds.

## What and Why
Introduce `BARONY_SUPER_MULTIPLAYER` and `MAXPLAYERS=15` scaffolding behind a default-OFF switch. This enables later PRs to build and test the 15-player path while preserving 1-4 production behavior.

## Scope
### In Scope
- `CMakeLists.txt` (`BARONY_SUPER_MULTIPLAYER`)
- `src/Config.hpp.in`
- `src/main.hpp` (conditional `MAXPLAYERS` cap `15`)
- `src/main.cpp` (static array init cleanup for cap-safe compile)
- `src/game.hpp` (packet envelope constant only if required for compile)
- `VS/Barony/Barony.vcxproj`
- `xcode/Barony/Barony/Config.hpp`

### Out of Scope
- Protocol or packet behavior changes
- Lobby readiness/start flow changes
- Smoke framework/scripts
- Mapgen tuning or telemetry

## Implementation Instructions
1. Add `BARONY_SUPER_MULTIPLAYER` in `CMakeLists.txt`, default `OFF`.
2. Thread the option through `src/Config.hpp.in` and generated platform config headers.
3. In `src/main.hpp`, set `MAXPLAYERS` to `15` only when the flag is enabled; keep legacy cap when disabled.
4. Update static arrays and related initialization in `src/main.cpp` so both OFF/ON builds are clean.
5. Keep local splitscreen cap unchanged at `4` (`MAX_SPLITSCREEN` path remains legacy behavior).
6. Do not include protocol, smoke, or mapgen behavior deltas in this PR.

## Suggested Commit Structure
1. Build/config plumbing for `BARONY_SUPER_MULTIPLAYER`.
2. `MAXPLAYERS` compile-time scaffolding and array/init cleanup.
3. VS/Xcode generated config parity updates.

## Validation Plan
- Build matrix:
```bash
cmake -S . -B build-pr2-off -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SUPER_MULTIPLAYER=OFF
cmake --build build-pr2-off -j8 --target barony

cmake -S . -B build-pr2-on -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SUPER_MULTIPLAYER=ON
cmake --build build-pr2-on -j8 --target barony
```
- Startup sanity (both variants) to main menu.

## Acceptance Criteria
- [ ] Both OFF and ON variants compile successfully.
- [ ] Default remains OFF.
- [ ] No protocol/lobby/mapgen/smoke script behavior changes are included.
- [ ] Splitscreen cap remains hard-limited to 4.
- [ ] Config headers are consistent across CMake/VS/Xcode paths.

## Review Focus
- Compile-time gating correctness and defaults.
- No behavior leakage when OFF.
- No accidental API/protocol changes.

## Rollback Strategy
Disable or revert the flag path; this PR is isolated from gameplay state and should revert cleanly.

## Extraction Plan (from PR #940 / 8p-mod)
Perform selective extraction from `a46cd9a3`, `4288b719`, `0f87d77f` using `git cherry-pick -n`, then stage only the scoped files for this PR.
