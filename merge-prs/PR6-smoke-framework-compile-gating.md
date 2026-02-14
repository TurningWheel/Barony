# [PR6] Smoke Framework and Compile-Time Gating (Code Only)

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Medium
- Depends On: PR4, PR5
- Blocks: PR7, PR8, PR9 validation quality

## Background
The expansion needs repeatable smoke instrumentation without polluting base gameplay/runtime paths. Compile-time gating is required so smoke hooks are available for validation builds but absent in normal builds.

## What and Why
Introduce the smoke hook architecture (`SmokeHooks*.cpp` implementations with `SmokeTestHooks.hpp` API) and strict `BARONY_SMOKE_TESTS` compile gating, while keeping gameplay behavior unchanged.

## Scope
### In Scope
- `CMakeLists.txt` (`BARONY_SMOKE_TESTS`)
- `src/CMakeLists.txt`
- `src/Config.hpp.in`
- `src/smoke/SmokeHooksMainMenu.cpp`
- `src/smoke/SmokeHooksGameplay.cpp`
- `src/smoke/SmokeHooksGameUI.cpp`
- `src/smoke/SmokeHooksNet.cpp`
- `src/smoke/SmokeHooksCombat.cpp`
- `src/smoke/SmokeHooksSaveReload.cpp`
- `src/smoke/SmokeHooksMapgen.cpp`
- `src/smoke/SmokeTestHooks.hpp`
- Minimal guarded call sites in:
  - `src/game.cpp`
  - `src/net.cpp`
  - `src/ui/MainMenu.cpp`
  - `src/ui/GameUI.cpp`
  - `src/scores.cpp`
  - `src/maps.cpp`

### Out of Scope
- `tests/smoke/*` shell/python runners
- Any mapgen balance coefficient changes
- Any lobby protocol behavior changes

## Implementation Instructions
1. Add `BARONY_SMOKE_TESTS` option in CMake/config headers, default OFF.
2. Add smoke hook implementation in `src/smoke/SmokeHooks*.cpp` (API in `src/smoke/SmokeTestHooks.hpp`).
3. Add minimal call sites guarded by `#ifdef BARONY_SMOKE_TESTS`; keep intrusion thin.
4. Route behavior through no-op wrappers when smoke is OFF so base paths stay clean.
5. Keep mapgen/gameplay call-site edits instrumentation-only.
6. Explicitly defer runner scripts and mapgen tooling to later PRs.

## Suggested Commit Structure
1. Build/config gating for smoke mode.
2. Add `SmokeHooks*.cpp` code behind `SmokeTestHooks.hpp`.
3. Minimal guarded call-site wiring.

## Validation Plan
- OFF build:
```bash
cmake -S . -B build-smoke-off -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=OFF
cmake --build build-smoke-off -j8 --target barony
```
- ON build:
```bash
cmake -S . -B build-smoke-on -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build build-smoke-on -j8 --target barony
```
- Quick ON launch sanity.
- Optional symbol check: no smoke hook references in OFF binary.

## Acceptance Criteria
- [ ] OFF and ON builds both pass.
- [ ] Smoke hooks are compiled only when `BARONY_SMOKE_TESTS=ON`.
- [ ] Core runtime behavior is unchanged when smoke is OFF.
- [ ] No `tests/smoke/*` runners or mapgen balancing logic are included.
- [ ] Call-site guards are minimal and localized.

## Review Focus
- Compile-time isolation quality.
- Hook ownership boundaries and low core-code intrusion.
- Absence of behavior changes in non-smoke builds.

## Rollback Strategy
Revert PR6 cleanly to remove smoke architecture if needed; no gameplay/protocol contracts should depend on it.

## Extraction Plan (from PR #940 / 8p-mod)
Take code-only slices from `fc6c6ced`, `4f24a78a`, `0f87d77f`, `14ae1081`, `9ed76e40`, `396263f1`, `b018bfeb`; exclude all `tests/smoke/*` files.
