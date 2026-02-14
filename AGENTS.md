# Repository Guidelines

## Project Structure & Module Organization
The project is CMake-based with the root `CMakeLists.txt` orchestrating all targets. Core gameplay and editor code lives in `src/`, with major subsystems split into `src/interface/`, `src/magic/`, `src/ui/`, `src/engine/`, and `src/imgui/`. Runtime text and localization assets are in `lang/`. CI helper scripts are in `ci/`. Platform-specific project files are under `VS/`, `VS.2015/`, and `xcode/`. Start with `README.md`, `INSTALL.md`, and `CONTRIBUTING.md` for workflow context.

## Build, Test, and Development Commands
Use out-of-source builds:

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -- -j
```

Common build variants:

- `cmake -DCMAKE_BUILD_TYPE=Release -DFMOD_ENABLED=ON ..` builds release with FMOD.
- `cmake -DFMOD_ENABLED=OFF ..` builds without FMOD.

CI-like Linux scripts (require CI secrets and packaged dependencies):

- `cd ci && ./build-linux_fmod_steam.sh`
- `cd ci && ./build-linux_fmod_steam_eos-barony.sh`

After building, run targets from the build directory (for example `./barony` or `./editor`).

## Local macOS Run (Steam Assets)
For local gameplay/smoke validation on macOS, use the built binary but run it from the Steam app bundle so assets/config paths match normal runtime behavior.

1. Build:

```bash
cmake -S . -B build-mac -G Ninja -DFMOD_ENABLED=OFF
cmake --build build-mac -j8
```

2. Copy the built executable into the Steam install (make a backup first):

```bash
src="$PWD/build-mac/barony.app/Contents/MacOS/barony"
dstdir="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS"
cp "$dstdir/Barony" "$dstdir/Barony.backup-$(date +%Y%m%d-%H%M%S)"
cp "$src" "$dstdir/Barony"
chmod +x "$dstdir/Barony"
```

3. Run from the Steam path:

```bash
"$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony" -windowed -size=1280x720
```

## Coding Style & Naming Conventions
Follow surrounding style in touched files; this codebase mixes legacy C/C++ patterns. Avoid broad formatting-only edits. Tabs are common in older files, while newer edits may use spaces; preserve local consistency. File naming is mostly subsystem-based lowercase (examples: `act*.cpp`, `monster_*.cpp`). Use `PascalCase` for types/classes (for example `Entity`) and uppercase names for macros/constants.

## Testing Guidelines
There is no dedicated unit-test suite in this repository. Required validation is:

- Build success for affected targets (`barony`, `editor` when relevant).
- Manual smoke test of the changed flow (menu/load/gameplay/editor path you touched).
- Keep GitHub Actions Linux build checks green for PRs.
- For multiplayer-expansion work, update `/Users/sayhiben/dev/Barony-8p/docs/multiplayer-expansion-verification-plan.md` inline as progress happens (checklist state + artifact paths + notable caveats).

## Commit & Pull Request Guidelines
Create a topic branch per change. For bugfix work, target `master` (per `README.md`). Keep commits focused and message subjects short, imperative, and specific (recent history includes messages like `update hash` and `fix one who knocks achievement when parrying`). In PRs, include: what changed, why, test steps/results, and linked issues. Add screenshots for visible UI/editor changes.

## Security & Configuration Tips
Do not commit secrets or environment tokens. CI scripts rely on `DEPENDENCIES_ZIP_KEY` and `DEPENDENCIES_ZIP_IV`; provide them via environment variables only.

## Codex Sandbox / Permissions
When running in Codex with sandboxing, ask for sandbox breakout/escalation permission before running commands that may require access outside the sandbox or external services.

- Common examples: `git ...`, `gh ...`, Steam app binary runs, and other commands that touch restricted paths/resources.
- If a command is blocked by sandboxing, rerun with escalation rather than changing the intended workflow.
- If launches fail with `Abort trap: 6` during smoke runs, treat it as a likely sandbox restriction signal and rerun with escalation.

## Multiplayer Expansion (PR 940) Working Notes
- Expansion target is `MAXPLAYERS=15` (not 16). Preserve nibble-packed ownership assumptions unless a deliberate encoding refactor is planned.
- Keep smoke instrumentation isolated to `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp` and `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp` with minimal call sites in gameplay/UI/network files.
- Keep headless mapgen integration plumbing (`-smoke-mapgen-integration*` parsing/validation/runner) in `SmokeTestHooks`; `src/game.cpp` should stay wiring-only for those options.
- Avoid adding ad-hoc smoke utility logic directly in core gameplay files; prefer hook APIs in `SmokeTestHooks` and keep base-game paths clean.
- Preferred local validation path is local build binary + Steam assets datadir (`--app .../build-mac/.../barony --datadir .../Barony.app/Contents/Resources`) instead of replacing the Steam executable.
- After long or high-instance smoke runs, clean generated cache bloat (especially `models.cache` under smoke artifact homes) while preserving logs/artifacts needed for debugging.
- If host performance degrades during smoke campaigns, check for lingering `run_mapgen_sweep_mac.sh`, `run_lan_helo_chunk_smoke_mac.sh`, and `barony` processes; terminate stale runs before launching new lanes.
- Known intermittent issue: churn/rejoin can show transient `lobby full` / join retries (`error code 16`). Track with artifacts and summaries, and avoid conflating it with unrelated feature-lane pass/fail unless assertions require it.
- Add and maintain compile-time gating for smoke hooks/call sites so smoke instrumentation compiles or executes only when a dedicated smoke-test flag is enabled.
- Smoke validation requires a smoke-enabled build (`-DBARONY_SMOKE_TESTS=ON`); if expected `[SMOKE]` logs are missing, verify generated config/build mode and rebuild the smoke target before rerunning tests.
- Fresh per-instance smoke homes can stall in intro/title flow; ensure smoke homes are pre-seeded with profile data (`skipintro=true`, `mods=[]`, and compiled books cache) so autopilot reaches lobby/gameplay deterministically.
- Local splitscreen is a legacy path and should stay hard-capped at 4 players; retain dedicated smoke coverage for `/splitscreen > 4` clamp behavior and over-cap leakage checks.
- When parsing smoke status lines with similarly named keys (for example `connected` vs `over_cap_connected`), parse exact `key=value` tokens to avoid false negatives and lane hangs.
- During style/contribution cleanup, treat `#ifdef BARONY_SMOKE_TESTS` guards around smoke-hook callsites as an acceptable and idiomatic exception.
- Preferred balancing loop for mapgen tuning: hook-owned in-process integration preflight (`levels=1,7,16,33`, fixed seed) -> single-runtime matrix confirmation -> runs=5 volatility gate -> full-lobby confirmation.

### Validation Summary (2026-02-12)
- Overall expansion status is near-finish: core LAN networking validation is green (HELO correctness, adversarial fail modes, soak/churn, high-slot regression lanes).
- Completed/green lanes include: save/reload owner-encoding sweep (`1..15`), lobby regression lanes (kick-target, slot-lock/copy, page navigation), remote-combat slot bounds, local splitscreen baseline, and `/splitscreen > 4` cap clamp.
- Steam backend handshake was validated for host-room/key flow; local same-account multi-instance joins remain a known Steam limitation.
- EOS handshake coverage is still open and should be treated as a gating item for full backend sign-off.
- Known intermittent issue remains: churn/rejoin can hit transient `lobby full` / `error code 16` retries before recovery; track with artifacts and do not conflate with unrelated lane failures.
- Smoke compile/runtime gating is in place (`BARONY_SMOKE_TESTS`), and the preferred local lane path is local build binary + Steam `--datadir` assets.

### Balancing Lessons and Guardrails
- Hard rule: preserve `1..4p` gameplay parity; all new mapgen balancing logic must be overflow-only (`connectedPlayers > 4`).
- Use sweep confidence policy consistently: `runs=3` for directional iteration, `runs=5` for volatility gate/promotion decisions.
- Promotion requires both simulated and full-lobby confirmation: integration/single-runtime can pass while full-lobby diverges (observed in pass15g level-1 comparison), so do not skip full-lobby gates.
- Keep mapgen tuning explainable and measurable via telemetry (`rooms`, `monsters`, `gold`, `items`, `decorations`, food metrics, value metrics, seed/regeneration diagnostics).
- Current target bands for `p15 vs p4` reviews:
  - rooms `1.62x-1.75x`
  - monsters `1.38x-1.46x`
  - monsters/room `0.82x-0.92x`
  - gold/player `0.70x-0.80x`
  - items/player `0.70x-0.80x`
  - food/player `0.65x-0.78x`
  - decorations `1.85x-2.25x`, blocking share `<= 45%`
- Maintain integration ownership boundaries: integration parser/validator/runner belong in `SmokeTestHooks`; `src/game.cpp` remains wiring-only.
- Keep operational hygiene between long runs: prune generated `models.cache`, and terminate stale `run_mapgen_*`, `run_lan_helo_*`, and `barony` processes before relaunch.

### Technical Commands and Config Reference
- Smoke-enabled build (required for `[SMOKE]` hooks/logs):
```bash
cmake -S . -B build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build build-mac-smoke -j8 --target barony
```
- Preferred mapgen tuning loop commands:
  - Fast in-process integration preflight (`runs=2`):
```bash
USER_HOME="$HOME"
OUT="tests/smoke/artifacts/mapgen-integration-preflight-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT/home"
HOME="$OUT/home" build-mac-smoke/barony.app/Contents/MacOS/barony \
  -windowed -size=1280x720 -nosound \
  -datadir="$USER_HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -smoke-mapgen-integration \
  -smoke-mapgen-integration-csv="$OUT/mapgen_level_matrix.csv" \
  -smoke-mapgen-integration-levels=1,7,16,33 \
  -smoke-mapgen-integration-min-players=1 \
  -smoke-mapgen-integration-max-players=15 \
  -smoke-mapgen-integration-runs=2
```
  - Volatility gate (`runs=5`): same command with `-smoke-mapgen-integration-runs=5`.
  - Low-player parity guard (`2..4p`): same command with `-smoke-mapgen-integration-min-players=2`, `-smoke-mapgen-integration-max-players=4`, `-smoke-mapgen-integration-runs=5`.
  - Scripted single-runtime matrix lane (`simulate-mapgen-players=1`, `runs=2/5`):
```bash
OUT="tests/smoke/artifacts/mapgen-level-matrix-passNN-$(date +%Y%m%d-%H%M%S)"
tests/smoke/run_mapgen_level_matrix_mac.sh \
  --app "build-mac-smoke/barony.app/Contents/MacOS/barony" \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --levels "1,7,16,33" \
  --min-players 1 --max-players 15 --runs-per-player 2 \
  --simulate-mapgen-players 1 --inprocess-sim-batch 1 --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --outdir "$OUT"
```
  - Full-lobby confirmation (`simulate-mapgen-players=0`):
```bash
tests/smoke/run_mapgen_sweep_mac.sh \
  --min-players 1 --max-players 15 --runs-per-player 5 \
  --simulate-mapgen-players 0 --auto-enter-dungeon 1 \
  --outdir "tests/smoke/artifacts/mapgen-full-posttune-$(date +%Y%m%d-%H%M%S)"
```
- Additional validation lane commands:
  - Same-level mapgen regeneration sanity lane (procedural floor):
```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --instances 1 --auto-start 1 --auto-start-delay 0 \
  --auto-enter-dungeon 1 --auto-enter-dungeon-delay 3 \
  --mapgen-samples 3 --require-mapgen 1 \
  --mapgen-reload-same-level 1 --mapgen-reload-seed-base 100100 \
  --start-floor 1 \
  --outdir "tests/smoke/artifacts/reload-procedural-verify-$(date +%Y%m%d-%H%M%S)"
```
  - Churn/rejoin retry investigation (`error code 16`):
```bash
tests/smoke/run_lan_join_leave_churn_smoke_mac.sh \
  --instances 8 --churn-cycles 3 --churn-count 2 \
  --force-chunk 1 --chunk-payload-max 200 \
  --auto-ready 1 --trace-ready-sync 1 --require-ready-sync 1 \
  --trace-join-rejects 1 \
  --outdir "tests/smoke/artifacts/churn-retry-investigation-$(date +%Y%m%d-%H%M%S)"
```
  - Steam/EOS handshake lanes:
```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --network-backend steam --instances 2 \
  --force-chunk 1 --chunk-payload-max 200 --timeout 360 \
  --outdir "tests/smoke/artifacts/steam-handshake-multiacct-$(date +%Y%m%d-%H%M%S)"

tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --network-backend eos --instances 2 \
  --force-chunk 1 --chunk-payload-max 200 --timeout 360 \
  --outdir "tests/smoke/artifacts/eos-handshake-$(date +%Y%m%d-%H%M%S)"
```
- Technical config/guardrails:
  - Use procedural floors for balancing sweeps (`1,7,16,33`); fixed/story floors may report `MAPGEN_WAIT_REASON=reload-complete-no-mapgen-samples`.
  - Keep smoke-run homes isolated (`HOME="$OUT/home"`) to avoid cross-run config/data leakage.
  - Integration seed root is now auto-generated per invocation; do not rely on the removed `-smoke-mapgen-integration-base-seed` override.
  - Maintain compile-time/runtime gating with `BARONY_SMOKE_TESTS`; keep non-smoke gameplay paths clean.
  - Keep integration parser/validator/runner in `src/smoke/SmokeTestHooks.*`; keep `src/game.cpp` wiring-only for `-smoke-mapgen-integration*`.
- Post-run hygiene commands:
```bash
find tests/smoke/artifacts -type f -name models.cache -delete
ps -Ao pid,ppid,etime,command | rg "run_mapgen_level_matrix_mac.sh|run_mapgen_sweep_mac.sh|run_lan_helo_chunk_smoke_mac.sh|barony.app/Contents/MacOS/barony"
```
