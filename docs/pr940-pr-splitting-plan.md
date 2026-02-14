# PR 940 Split Plan: 1-15 Players in Reviewable, Mergeable Chunks

## Summary
This plan analyzes `PR #940`, `sayhiben/8p-mod`, and the upstream merge target (`upstream/master`, merge-base `2edf6191b0fbcd0e416cc25ca647c252e04f6a17`).
Current PR size is `66 files`, `+18,301/-1,314`, with three heavy hotspots:
- `src/smoke/SmokeTestHooks.cpp`
- `src/ui/MainMenu.cpp`
- `src/maps.cpp`

The split ordering is **de-risking first**: platform/build baseline, then core multiplayer scaffolding and protocol, then smoke framework/tooling, then mapgen plumbing, then mapgen balance, then default enablement.

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
  - `-smoke-mapgen-integration*` wiring in `src/game.cpp` and implementation in `src/smoke/SmokeTestHooks.cpp`

## 1) What "Good PR Size" Means Here
- One PR should own **one concern** and at most **one high-risk hotspot** (`MainMenu.cpp`, `net.cpp`, `maps.cpp`, `SmokeTestHooks.cpp`, or `run_lan_helo_chunk_smoke_mac.sh`).
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
- no shell runners
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
- `src/smoke/SmokeTestHooks.cpp`
- `src/smoke/SmokeTestHooks.hpp`
- minimal `#ifdef BARONY_SMOKE_TESTS` callsites in `src/game.cpp`, `src/net.cpp`, `src/ui/MainMenu.cpp`, `src/ui/GameUI.cpp`, `src/scores.cpp`, `src/maps.cpp`

**Exact scope (out)**:
- no shell runners
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

### 7. Smoke Runner Lanes (Non-Mapgen)
**Intent / user value**: reproducible regression lanes for lobby/combat/splitscreen/save/rejoin.

**Exact scope (in)**:
- non-mapgen scripts and docs in `tests/smoke/` such as:
  - `run_lan_helo_chunk_smoke_mac.sh`
  - `run_helo_adversarial_smoke_mac.sh`
  - `run_lan_helo_soak_mac.sh`
  - `run_lan_join_leave_churn_smoke_mac.sh`
  - `run_lobby_*`
  - `run_remote_combat_slot_bounds_smoke_mac.sh`
  - `run_splitscreen_*`
  - `run_status_effect_queue_init_smoke_mac.sh`
  - `run_save_reload_compat_smoke_mac.sh`
  - `tests/smoke/README.md`

**Exact scope (out)**:
- no gameplay code changes

**Key commits**:
- Script-only slices from `fc6c6ced`, `4f24a78a`, `0f87d77f`, `14ae1081`, `9ed76e40`, `396263f1`, `b018bfeb`.

**Dependencies**: PR 6.

**Risk level / rollback**: low-medium.

**Test plan**:
- `bash -n` on each script
- run one short 4p lane and one 15p lane

**Review notes**:
- Look for duplicated parser logic and exact `key=value` parsing safety.

**Cherry-pick / rebase strategy**:
- Include only `tests/smoke/` files not tied to mapgen matrix sweeps.

### 8. Mapgen Telemetry and Integration Plumbing (No Balance Policy Change)
**Intent / user value**: make mapgen tuning measurable and reproducible before gameplay tuning lands.

**Exact scope (in)**:
- `src/smoke/SmokeTestHooks.cpp` and `.hpp` (Mapgen summary/integration runner)
- `src/game.cpp` wiring-only for `-smoke-mapgen-integration*`
- smoke-only summary/logging callsites in `src/maps.cpp`
- mapgen runners:
  - `tests/smoke/run_mapgen_sweep_mac.sh`
  - `tests/smoke/run_mapgen_level_matrix_mac.sh`
  - `tests/smoke/generate_mapgen_heatmap.py`
  - `tests/smoke/generate_smoke_aggregate_report.py`

**Exact scope (out)**:
- no changes to overflow balancing constants/divisors in `maps.cpp`

**Key commits**:
- Instrumentation-only slices from `f4da9ee9`, `a35e2c7b`, `9ce7ac93`, plus support from earlier smoke commits.

**Dependencies**: PR 6.

**Risk level / rollback**: medium.

**Test plan**:
- integration parity lane (`levels=1,7,16,33`, `players=1..15`) and schema validation in generated CSV

**Review notes**:
- Verify `game.cpp` remains wiring-only and integration logic stays in `SmokeTestHooks`.

**Cherry-pick / rebase strategy**:
- Enforce acceptance check that `maps.cpp` diff is smoke-guard telemetry only; reject any gameplay formula delta in this PR.

### 9. Mapgen Balance Tuning for 5-15 Players (Gameplay Only)
**Intent / user value**: tune large-party mapgen while preserving 1-4 parity.

**Exact scope (in)**:
- primarily `src/maps.cpp`
- optional concise docs update in `docs/extended-multiplayer-balancing-and-tuning-plan.md` and `docs/multiplayer-expansion-verification-plan.md` with summary metrics only

**Exact scope (out)**:
- no CMake/build changes
- no smoke framework changes
- no shell runner logic changes

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
- Split `src/smoke/SmokeTestHooks.cpp` by domain into:
  - `SmokeHooksMainMenu.cpp`
  - `SmokeHooksMapgen.cpp`
  - `SmokeHooksCombat.cpp`
  - `SmokeHooksSaveReload.cpp`
  to reduce review blast radius.
- Extract shared shell helpers from `tests/smoke/run_lan_helo_chunk_smoke_mac.sh` into `tests/smoke/lib/common.sh` (`is_uint`, summary parsing, cache prune, env setup).
- Keep one canonical parser for `summary.env` key reads; currently repeated across many `run_*_smoke_mac.sh`.
- Move shared network lobby constants (`kJoinCapabilityHeloChunkV1`, packet header structs) to a dedicated header (for example `src/net_lobby_protocol.hpp`) to avoid UI/net drift.
- Keep `#ifdef BARONY_SMOKE_TESTS` callsites thin by routing through no-op wrappers; reduce conditional spread in `src/net.cpp` and `src/ui/MainMenu.cpp`.
- In `src/maps.cpp`, convert overflow tuning constants into a small struct/table keyed by overflow band and depth band to make review of coefficient changes explicit.
- Exclude local-only process files from upstream PRs:
  - `AGENTS.md`
  - `styleguide.txt`
  - `HELO_ONLY_CHUNKING_PLAN.md`
- Reduce giant verification-doc churn by storing pass-by-pass data in CSV artifacts and keeping docs summary-only.

## 4) Prerequisites / Gating Tasks
- Merge or explicitly supersede upstream PR `#942` first; otherwise platform diffs will keep re-conflicting.
- Decide rollout default: keep `BARONY_SUPER_MULTIPLAYER` default OFF until PR 10.
- Add CI matrix jobs for `BARONY_SUPER_MULTIPLAYER=OFF/ON` and a smoke compile check with `BARONY_SMOKE_TESTS=ON`.
- Lock mapgen acceptance bands from `docs/extended-multiplayer-balancing-and-tuning-plan.md` before PR 9 review starts.
- Pre-agree that mapgen PRs must include artifact links from `tests/smoke/artifacts/` with reproducible command lines.
- Confirm maintainers want long-form tuning logs in-repo; if not, keep only condensed summaries in docs PRs.

## 5) Paste-Ready Markdown Tracking Plan

### PR Checklist (with link placeholders)
- [ ] PR 1: Platform and build baseline (`#942` aligned) - [link]
- [ ] PR 2: Super-multiplayer feature gate + 15-player scaffolding - [link]
- [ ] PR 3: Player-slot mapping refactor (no behavior change) - [link]
- [ ] PR 4: Lobby/join protocol hardening for high player counts - [link]
- [ ] PR 5: Status-effect owner encoding hardening (15 cap-safe) - [link]
- [ ] PR 6: Smoke framework + compile-time gating (code only) - [link]
- [ ] PR 7: Smoke runner lanes (non-mapgen) - [link]
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
7. Merge PR 7 after smoke lane scripts pass baseline lanes.
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
