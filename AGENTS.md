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
- Avoid adding ad-hoc smoke utility logic directly in core gameplay files; prefer hook APIs in `SmokeTestHooks` and keep base-game paths clean.
- Preferred local validation path is local build binary + Steam assets datadir (`--app .../build-mac/.../barony --datadir .../Barony.app/Contents/Resources`) instead of replacing the Steam executable.
- After long or high-instance smoke runs, clean generated cache bloat (especially `models.cache` under smoke artifact homes) while preserving logs/artifacts needed for debugging.
- Known intermittent issue: churn/rejoin can show transient `lobby full` / join retries (`error code 16`). Track with artifacts and summaries, and avoid conflating it with unrelated feature-lane pass/fail unless assertions require it.
- Add and maintain compile-time gating for smoke hooks/call sites so smoke instrumentation compiles or executes only when a dedicated smoke-test flag is enabled.
