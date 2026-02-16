# PR 940 Split Plan: 1-15 Players in Reviewable, Mergeable Chunks

## Summary
This plan analyzes `PR #940`, `sayhiben/8p-mod`, and the upstream merge target (`upstream/master`, merge-base `2edf6191b0fbcd0e416cc25ca647c252e04f6a17`).
Current PR size is `66 files`, `+18,301/-1,314`, with three heavy hotspots:
- `src/smoke/SmokeHooksMainMenu.cpp` / `src/smoke/SmokeHooksMapgen.cpp`
- `src/ui/MainMenu.cpp`
- `src/maps.cpp`

The split ordering is **de-risking first**: platform/build baseline, then core multiplayer scaffolding and protocol, then smoke framework/tooling, then mapgen plumbing, then mapgen balance, then default enablement.

## Progress Snapshot (Updated 2026-02-14)
- Legacy smoke shell wrappers under `tests/smoke/` have been removed and replaced by a unified Python CLI entrypoint: `tests/smoke/smoke_runner.py`.
- Smoke CLI surface is argparse-based for both non-mapgen and mapgen lanes, with stdlib-only packaging/bootstrap in place (`tests/smoke/pyproject.toml`, `tests/smoke/.python-version`).
- `smoke_runner.py` is now a thin registration entrypoint; lane execution/parsers were moved into `tests/smoke/smoke_framework/*` modules.
- `lan-helo-chunk` was split into focused modules (`lan_helo_chunk_args`, `lan_helo_chunk_launch`, `lan_helo_chunk_runtime`, `lan_helo_chunk_post`, `lan_helo_chunk_summary`).
- Shared lane result/count helpers now exist in `tests/smoke/smoke_framework/lane_matrix.py` and are adopted across core/lobby/churn lanes.
- Remaining smoke-suite work is no longer shell migration; it is targeted decomposition of the largest lanes (`join-leave-churn`, mapgen lanes), helper-level self-checks/tests, and vestigial-lane culling.

## Execution Status Matrix (Updated 2026-02-14)
- PR1 Platform/build baseline: planned (awaiting extraction branch or upstream-#942 confirmation path).
- PR2 Feature gate + cap scaffolding: planned.
- PR3 Slot mapping refactor: planned.
- PR4 Lobby/join protocol hardening: planned.
- PR5 Status-effect owner encoding: planned.
- PR6 Smoke compile-time framework: planned (scope frozen as code-only, no `tests/smoke/*`).
- PR7 Non-mapgen smoke runner: in progress on branch (argparse + module split complete; extraction cleanup remaining).
- PR8 Mapgen telemetry/integration plumbing: planned and blocked on clean mapgen-only extraction boundary.
- PR9 Mapgen balance tuning: planned and blocked on PR8.
- PR10 Default enablement: planned and blocked on PR1-PR9.

## Immediate Next Milestones
- Finalize PR7 extraction readiness:
  - complete `join-leave-churn` helper decomposition in `churn_statusfx_lane.py`
  - finish vestigial non-mapgen lane/helper audit
  - run final non-mapgen compile/help/lane sanity pack and freeze scope
- Prepare PR8 boundary:
  - isolate mapgen-only hunks across `src/*`, `smoke_runner.py`, and `smoke_framework/*`
  - reject any non-mapgen smoke-lane behavior changes in PR8 candidate diff

## Important API / Interface Changes Across the Series
- CMake options and config macros:
  - `BARONY_SUPER_MULTIPLAYER` in `CMakeLists.txt` + `src/Config.hpp.in`
  - `BARONY_SMOKE_TESTS` in `CMakeLists.txt` + `src/Config.hpp.in`
- Networking protocol:
  - `JOIN` capability byte (len `69 -> 70`) and `HLCN` chunk packet path in `src/net.cpp` and `src/ui/MainMenu.cpp`
  - `lobbyPlayerJoinRequest` signature update in `src/net.hpp`
- New owner encoding helper:
  - `src/status_effect_owner_encoding.hpp`
- Smoke integration CLI:
  - `-smoke-mapgen-integration*` wiring in `src/game.cpp` and implementation in `src/smoke/SmokeHooksMapgen.cpp`

## 1) What "Good PR Size" Means Here
- One PR should own **one concern** and at most **one high-risk hotspot** (`MainMenu.cpp`, `net.cpp`, `maps.cpp`, `SmokeHooksMainMenu.cpp`/`SmokeHooksMapgen.cpp`, or `tests/smoke/smoke_runner.py`).
- Target shape for gameplay/network PRs in this repo: roughly **200-800 changed lines** and **<= 8 files**. For hotspot files, keep hunk count low and isolate to one behavior.
- Keep commit structure consistent per PR:
  1. mechanical/refactor prep
  2. behavior change
  3. tests/docs evidence
- Do not mix build/platform/tooling with gameplay/balance in one PR.
- Minimal validation per PR:
  - Build green for affected targets (`barony`, `editor` when touched)
  - One focused smoke lane that exercises the changed path
  - Manual sanity path in menu/gameplay for touched flow

## 2) Proposed PR Breakdown (Main Deliverable)

### 1. Platform and Build Baseline (PR #942-first)
**Intent / user value**: stabilize cross-platform setup and reduce build friction before gameplay risk.

**Exact scope (in)**:
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

**Exact scope (out)**:
- no `MAXPLAYERS`
- no `maps.cpp`
- no smoke hooks/scripts
- no lobby/gameplay logic

**Key commits**:
- Prefer upstream PR `#942` commits (`91b96212`, `ba792cba`, `7bb1a09b`, `d9b958d8`) instead of reusing mixed `8p-mod` commits (`1f001a25`, `fcfe816e`).

**Dependencies**: none; should merge first.

**Risk level / rollback**: low; revert PR cleanly if platform regressions appear.

**Test plan**:
- Linux and Windows CI build
- macOS local configure/build

**Review notes**:
- Verify CMake link/library deltas and NFD compatibility coverage.

**Cherry-pick / rebase strategy**:
- Base from `upstream/master`; if `#942` merges, drop overlapping `8p-mod` commits during rebase and do not duplicate.

### 2. Super-Multiplayer Feature Gate and 15-Player Cap Scaffolding
**Intent / user value**: add controlled compile-time path for 1-15 without changing defaults yet.

**Exact scope (in)**:
- `CMakeLists.txt` (`BARONY_SUPER_MULTIPLAYER`)
- `src/Config.hpp.in`
- `src/main.hpp` (`MAXPLAYERS` conditional + cap 15)
- `src/main.cpp` static array init cleanup
- `src/game.hpp` packet envelope constant if needed
- `VS/Barony/Barony.vcxproj`
- `xcode/Barony/Barony/Config.hpp`

**Exact scope (out)**:
- no protocol changes
- no lobby behavior
- no smoke tooling
- no mapgen tuning

**Key commits**:
- Selective extraction from `a46cd9a3`, `4288b719`, `0f87d77f`.

**Dependencies**: PR 1.

**Risk level / rollback**: medium; rollback by disabling/reverting flag path.

**Test plan**:
- dual build matrix (`-DBARONY_SUPER_MULTIPLAYER=OFF/ON`) + startup sanity.

**Review notes**:
- Check that splitscreen cap remains 4 via `src/ui/MainMenu.hpp`.

**Cherry-pick / rebase strategy**:
- `git cherry-pick -n` source commits, then commit only the file set above.

### 3. Player-Slot Mapping Refactor (No Behavior Change)
**Intent / user value**: reduce duplication and make slot-theme expansion maintainable.

**Exact scope (in)**:
- `src/player_slot_map.hpp`
- `src/actplayer.cpp`
- `src/interface/interface.cpp`
- `src/interface/playerinventory.cpp`
- `src/items.cpp`
- `src/items.hpp`
- `src/actitem.cpp`
- `src/interface/drawminimap.cpp`
- `src/ui/GameUI.cpp`
- optional tiny text fix in `src/ui/MainMenu.cpp`

**Exact scope (out)**:
- no networking
- no smoke
- no mapgen
- no build scripts

**Key commits**:
- `69013b2f`, `780c0287`, `bce9dd61`, `ae8b98dd` (items-only part), `0e95632b`.

**Dependencies**: PR 2.

**Risk level / rollback**: low.

**Test plan**:
- build + visual sanity for player icon/theme mapping in normal and colorblind modes.

**Review notes**:
- Ensure mapping outputs preserve legacy first slots and deterministic overflow cycling.

**Cherry-pick / rebase strategy**:
- File-scoped cherry-pick from listed commits; exclude unrelated `MainMenu` hunks if any.

### 4. Lobby/Join Protocol Hardening for High Player Counts
**Intent / user value**: robust 15-player join/start flow and reduced HELO fragility.

**Exact scope (in)**:
- `src/net.cpp`
- `src/net.hpp`
- `src/ui/MainMenu.cpp`
- only required support in `src/game.cpp`, `src/interface/drawstatus.cpp`, `src/player.cpp`, `src/entity.cpp`

**Exact scope (out)**:
- no smoke hook framework
- no smoke runner/tooling changes
- no mapgen formulas

**Key commits**:
- Selective extraction from `a46cd9a3`, `780c0287` (ready-sync pieces), `b36471d6`, `4288b719` (net/lobby-only pieces).

**Dependencies**: PR 2 and PR 3.

**Risk level / rollback**: high; rollback by reverting protocol PR while keeping scaffolding.

**Test plan**:
- 4p and 15p lobby join smoke lane
- legacy single-packet fallback lane
- manual ready/start path

**Review notes**:
- Focus on packet compatibility (`JOIN` capability, `HLCN` reassembly, fallback correctness).

**Cherry-pick / rebase strategy**:
- Isolate to net/lobby files; avoid bringing any `tests/smoke/*` here.

### 5. Status-Effect Owner Encoding Hardening (Cap-Safe to 15)
**Intent / user value**: avoid owner misattribution at high slots and preserve compatibility.

**Exact scope (in)**:
- `src/status_effect_owner_encoding.hpp`
- `src/magic/actmagic.cpp`
- `src/magic/castSpell.cpp`
- `src/entity.cpp`
- `src/actplayer.cpp`

**Exact scope (out)**:
- no smoke scripts/docs
- no mapgen
- no build changes

**Key commits**:
- `b975122b`.

**Dependencies**: PR 2.

**Risk level / rollback**: medium-high.

**Test plan**:
- targeted spell/effect ownership checks at 1/8/15 players
- save/reload compatibility lane once smoke framework is available

**Review notes**:
- Verify legacy `EFF_FAST` compatibility and non-player sentinel handling.

**Cherry-pick / rebase strategy**:
- Clean cherry-pick of `b975122b` with docs excluded or minimized.

### 6. Smoke Framework and Compile-Time Gating (Code Only)
**Intent / user value**: add smoke instrumentation architecture without affecting base builds.

**Exact scope (in)**:
- `CMakeLists.txt` (`BARONY_SMOKE_TESTS`)
- `src/CMakeLists.txt`
- `src/Config.hpp.in`
- `src/smoke/SmokeHooks*.cpp`
- `src/smoke/SmokeTestHooks.hpp`
- minimal `#ifdef BARONY_SMOKE_TESTS` callsites in `src/game.cpp`, `src/net.cpp`, `src/ui/MainMenu.cpp`, `src/ui/GameUI.cpp`, `src/scores.cpp`, `src/maps.cpp`

**Exact scope (out)**:
- no `tests/smoke/` runner/tooling changes
- no mapgen balance constants/divisors

**Key commits**:
- Code-only slices from `fc6c6ced`, `4f24a78a`, `0f87d77f`, `14ae1081`, `9ed76e40`, `396263f1`, `b018bfeb`.

**Dependencies**: PR 4 and PR 5.

**Risk level / rollback**: medium.

**Test plan**:
- build ON/OFF (`-DBARONY_SMOKE_TESTS=ON/OFF`)
- verify smoke symbols absent when OFF
- one quick smoke-enabled launch when ON

**Review notes**:
- Review for strict compile gating and minimal core-file intrusion.

**Cherry-pick / rebase strategy**:
- Keep only `src/*` and CMake files; defer all `tests/smoke/*` to next PR.

### 7. Smoke Runner Suite (Non-Mapgen, Python CLI)
**Intent / user value**: reproducible regression lanes for lobby/combat/splitscreen/save/rejoin.

**Exact scope (in)**:
- `tests/smoke/smoke_runner.py` CLI registration/dispatch entrypoint
- `tests/smoke/smoke_framework/` non-mapgen lane modules and shared framework helpers
- `tests/smoke/README.md`
- `tests/smoke/pyproject.toml`
- `tests/smoke/.python-version`
- removal of legacy wrappers:
  - `tests/smoke/run_*_smoke_mac.sh`
  - `tests/smoke/lib/common.sh`

**Exact scope (out)**:
- no gameplay code changes
- no mapgen balance changes

**Key commits**:
- File-scoped extraction from current smoke refactor branch state in `tests/smoke/` (non-mapgen lanes and shared framework modules only).

**Dependencies**: PR 6.

**Risk level / rollback**: low-medium.

**Test plan**:
- `python3 -m py_compile tests/smoke/smoke_runner.py tests/smoke/smoke_framework/*.py`
- argparse sanity for key subcommands:
  - `python3 tests/smoke/smoke_runner.py --help`
  - `python3 tests/smoke/smoke_runner.py <lane> --help`
- run one short 4p lane and one 15p lane via `smoke_runner.py`

**Review notes**:
- Look for exact `key=value` parsing safety, shared helper reuse, and clean lane boundaries.

**Cherry-pick / rebase strategy**:
- Include only non-mapgen hunks in `tests/smoke/smoke_runner.py` and `tests/smoke/smoke_framework/*`, plus smoke README/bootstrap file changes and shell-wrapper deletions.

### 8. Mapgen Telemetry and Integration Plumbing (No Balance Policy Change)
**Intent / user value**: make mapgen tuning measurable and reproducible before gameplay tuning lands.

**Exact scope (in)**:
- `src/smoke/SmokeHooksMapgen.cpp` + `src/smoke/SmokeTestHooks.hpp` (Mapgen summary/integration runner)
- `src/game.cpp` wiring-only for `-smoke-mapgen-integration*`
- smoke-only summary/logging callsites in `src/maps.cpp`
- mapgen tooling:
  - mapgen lane modules in `tests/smoke/smoke_framework/mapgen_lanes.py` (and mapgen-specific helpers)
  - mapgen subcommand registration in `tests/smoke/smoke_runner.py`
  - `tests/smoke/generate_mapgen_heatmap.py`
  - `tests/smoke/generate_smoke_aggregate_report.py`

**Exact scope (out)**:
- no changes to overflow balancing constants/divisors in `maps.cpp`
- no non-mapgen lane behavior changes in `smoke_runner.py` / non-mapgen `smoke_framework/*` modules

**Key commits**:
- Instrumentation-only slices from `f4da9ee9`, `a35e2c7b`, `9ce7ac93`, plus support from earlier smoke commits.

**Dependencies**: PR 6.

**Risk level / rollback**: medium.

**Test plan**:
- integration parity lane (`levels=1,7,16,33`, `players=1..15`) and schema validation in generated CSV

**Review notes**:
- Verify `game.cpp` remains wiring-only and integration logic stays in `src/smoke/SmokeHooksMapgen.cpp`.
- In smoke tooling, review only mapgen-lane hunks (`smoke_runner.py` mapgen registration + mapgen framework modules) for this PR.

**Cherry-pick / rebase strategy**:
- Enforce acceptance check that `maps.cpp` diff is smoke-guard telemetry only; reject any gameplay formula delta in this PR.
- For smoke tooling files, keep only mapgen-related hunks and leave non-mapgen changes in PR7.

### 9. Mapgen Balance Tuning for 5-15 Players (Gameplay Only)
**Intent / user value**: tune large-party mapgen while preserving 1-4 parity.

**Exact scope (in)**:
- primarily `src/maps.cpp`
- optional concise docs update in `AGENTS.md` (`Validation Summary` / `Balancing Lessons and Guardrails`) with summary metrics only

**Exact scope (out)**:
- no CMake/build changes
- no smoke framework changes
- no non-mapgen smoke runner refactors

**Key commits**:
- Gameplay portions from `b7c36be9`, `4288b719`, `9ed76e40`, `f4da9ee9`, `a35e2c7b`, `9ce7ac93`.

**Dependencies**: PR 8.

**Risk level / rollback**: high.

**Test plan**:
- full mapgen gate stack (`runs=2` preflight, `runs=5` volatility, full-lobby confirmation, low-player parity diff)

**Review notes**:
- Validate target ratios and level-1 divergence risk.

**Cherry-pick / rebase strategy**:
- Start from PR 8 branch and apply map-only changes; verify no non-`maps.cpp` code deltas before merge.

### 10. Default Enablement Flip and Release Notes
**Intent / user value**: switch from scaffolded capability to default-on once all gates pass.

**Exact scope (in)**:
- one-line default flip in `CMakeLists.txt` (`BARONY_SUPER_MULTIPLAYER` default)
- concise release-facing notes in `INSTALL.md` and `README.md` if needed

**Exact scope (out)**:
- no logic changes

**Key commits**:
- new tiny commit (do not reuse mixed historical commits)

**Dependencies**: all prior PRs + gate pass.

**Risk level / rollback**: medium.

**Test plan**:
- full CI + 4p/15p smoke sanity

**Review notes**:
- Ensure this PR is only a default switch.

**Cherry-pick / rebase strategy**:
- Create fresh commit on top of merged stack.

## 3) Consolidation and Simplification Opportunities
- Maintain split smoke hook implementation by domain:
  - `SmokeHooksMainMenu.cpp`
  - `SmokeHooksMapgen.cpp`
  - `SmokeHooksCombat.cpp`
  - `SmokeHooksSaveReload.cpp`
  to reduce review blast radius.
- Keep `tests/smoke/smoke_runner.py` thin (registration only) and continue decomposing the largest lane modules:
  - `churn_statusfx_lane.py` (`join-leave-churn` runtime, ready-sync, summary assembly)
  - `mapgen_lanes.py` (parser vs orchestration vs report assembly)
- Keep one canonical parser for `summary.env` key reads and exact token parsing in shared helper functions (already partially consolidated).
- Expand shared lane result/count/row assembly helpers where patterns still repeat (currently partially centralized via `lane_matrix.py`).
- Add a lightweight smoke framework self-check lane (CLI/parser/module wiring) plus a small helper test layer for parser/token edge cases.
- Move shared network lobby constants (`kJoinCapabilityHeloChunkV1`, packet header structs) to a dedicated header (for example `src/net_lobby_protocol.hpp`) to avoid UI/net drift.
- Keep `#ifdef BARONY_SMOKE_TESTS` callsites thin by routing through no-op wrappers; reduce conditional spread in `src/net.cpp` and `src/ui/MainMenu.cpp`.
- In `src/maps.cpp`, convert overflow tuning constants into a small struct/table keyed by overflow band and depth band to make review of coefficient changes explicit.
- Keep smoke tooling Python stdlib-only (no runtime dependency creep) unless a clear value-add justifies new deps.
- Exclude local-only process files from upstream PRs:
  - `AGENTS.md`
  - `styleguide.txt`
  - `HELO_ONLY_CHUNKING_PLAN.md`
- Reduce giant verification-doc churn by storing pass-by-pass data in CSV artifacts and keeping docs summary-only.

## 4) Prerequisites / Gating Tasks
- Merge or explicitly supersede upstream PR `#942` first; otherwise platform diffs will keep re-conflicting.
- Decide rollout default: keep `BARONY_SUPER_MULTIPLAYER` default OFF until PR 10.
- Add CI matrix jobs for `BARONY_SUPER_MULTIPLAYER=OFF/ON` and a smoke compile check with `BARONY_SMOKE_TESTS=ON`.
- Lock mapgen acceptance bands from `AGENTS.md` (`Balancing Lessons and Guardrails`) before PR 9 review starts.
- Pre-agree that mapgen PRs must include artifact links from `tests/smoke/artifacts/` with reproducible command lines.
- Confirm there are no downstream jobs still invoking deleted `tests/smoke/*.sh` wrappers. (Local repo check is already clean; verify CI/docs automation usage.)
- Confirm maintainers want long-form tuning logs in-repo; if not, keep only condensed summaries in docs PRs.
- Before cutting PR8, freeze PR7 scope and land/extract remaining non-mapgen smoke refactors to avoid cross-contamination.

## 5) Paste-Ready Markdown Tracking Plan

### PR Checklist (with link placeholders)
- [ ] PR 1: Platform and build baseline (`#942` aligned) - [link]
- [ ] PR 2: Super-multiplayer feature gate + 15-player scaffolding - [link]
- [ ] PR 3: Player-slot mapping refactor (no behavior change) - [link]
- [ ] PR 4: Lobby/join protocol hardening for high player counts - [link]
- [ ] PR 5: Status-effect owner encoding hardening (15 cap-safe) - [link]
- [ ] PR 6: Smoke framework + compile-time gating (code only) - [link]
- [ ] PR 7: Smoke runner suite (non-mapgen, Python CLI) - [link] (in progress on branch: shell removal + argparse + module split)
- [ ] PR 8: Mapgen telemetry/integration plumbing (no balance change) - [link]
- [ ] PR 9: Mapgen balance tuning (gameplay only, maps-focused) - [link]
- [ ] PR 10: Default enablement flip + release notes - [link]

### Merge Order and Gating Criteria
1. Merge PR 1 when cross-platform CI build passes.
2. Merge PR 2 when ON/OFF flag builds both pass.
3. Merge PR 3 when UI slot theme/asset parity is confirmed.
4. Merge PR 4 after HELO chunk and fallback smoke lane is green.
5. Merge PR 5 after 15p caster-owner correctness checks pass.
6. Merge PR 6 after smoke ON/OFF build gating is verified.
7. Merge PR 7 after argparse smoke lanes pass baseline 4p/15p checks and no `.sh` smoke wrappers remain.
8. Merge PR 8 after integration parity checks are green (`levels=1,7,16,33`).
9. Merge PR 9 only with runs=5 volatility + full-lobby confirmation + low-player parity evidence.
10. Merge PR 10 only when all above are green and maintainers approve default flip.

### Release / Rollout (Protect 1-4 While Landing 1-15)
1. Keep `BARONY_SUPER_MULTIPLAYER` default OFF through PR 9.
2. Continuously run 1-4 baseline smoke/manual checks on every merged PR touching gameplay/network.
3. Enable default ON only in PR 10 after mapgen and networking gates are complete.
4. Keep splitscreen hard cap at 4 throughout (`MAX_SPLITSCREEN` path remains unchanged).

### Mapgen Balance Evaluation Note
- Required metrics and targets (p15 vs p4):
  - rooms `1.62x-1.75x`
  - monsters `1.38x-1.46x`
  - monsters/room `0.82x-0.92x`
  - gold/player `0.70x-0.80x`
  - items/player `0.70x-0.80x`
  - food/player `0.65x-0.78x`
  - decorations `1.85x-2.25x`
  - blocking share `<=45%`
- Required runs:
  - Integration preflight: `levels=1,7,16,33`, `players=1..15`, `runs=2`
  - Volatility gate: same levels/players, `runs=5`
  - Full-lobby confirmation: `simulate-mapgen-players=0`, `players=1..15`, `runs=5`
  - Low-player parity guard: `players=2..4`, `runs=5`, row-level diff check
- Required artifacts per mapgen PR:
  - `mapgen_level_matrix.csv`, aggregate HTML, heatmap HTML, summary env, and before/after ratio table
  - Host log snippets proving wait reason and seed/level validity
  - Short reviewer note with accepted/rejected candidate rationale

## Assumptions and Defaults
- Upstream merge target is `upstream/master`.
- `#942` is treated as upstream prerequisite for platform changes.
- Local-only planning files are excluded from upstream-facing PRs unless maintainers explicitly request them.
- Behavior flag default remains OFF until final enablement PR.
