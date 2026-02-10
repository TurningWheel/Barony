# Multiplayer Expansion Verification Plan (Post-PR 940)

## Summary
This plan turns the current smoke harness into a reliable gate for the 16-player expansion, reruns existing suites with stronger evidence, and closes key automation gaps from PR 940's manual checklist.
Current status from artifacts now shows broad LAN smoke success with strict adversarial gating enabled.
1. ✅ Adversarial HELO fail-modes now prove failure with strict assertions (`drop-last`, `duplicate-conflict-first`) in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-adversarial-20260209-172722/adversarial_results.csv`.
2. ⚠️ Coverage is still almost entirely LAN/direct-connect; Steam/EOS transport-specific behavior remains unvalidated.
3. ⚠️ LAN stability lanes and Phase D data collection lanes are green, but Steam/EOS transport-specific behavior and checklist-specific automation coverage remain unvalidated.

### Progress Notes (Updated February 10, 2026)
- Added strict adversarial HELO assertions and per-client reassembly accounting to `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`.
- Added richer smoke outputs (`NETWORK_BACKEND`, `TX_MODE_APPLIED`, `PER_CLIENT_REASSEMBLY_COUNTS`, `CHUNK_RESET_REASON_COUNTS`, `MAPGEN_COUNT`, `HOST_LOG`) and wired adversarial CSV/report consumption.
- Kept smoke perturbation behavior encapsulated in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp`/`.hpp` with minimal call sites in `/Users/sayhiben/dev/Barony-8p/src/net.cpp` and `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp`.
- Added in-runtime repeated dungeon transitions for mapgen sampling via `BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS` and in-process simulated sweep batching in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh`.
- Completed 8p soak beyond the current target (12 completed runs, all pass) in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`.
- Observed `Abort trap: 6` when launching the 16p soak in sandbox mode; rerunning with escalation resolved launches.
- Completed 16p soak to the agreed cutoff (6 completed runs, all pass) in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`.
- Completed 16p churn lane with all cycles passing in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4/churn_cycle_results.csv`.
- Fixed an in-process mapgen batch hang by decoupling dungeon transition repeats from required mapgen samples:
  - Added runner option `--auto-enter-dungeon-repeats` in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`.
  - Updated `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh` to set repeat headroom in batch mode.
- Completed high-volume simulated mapgen sweep (1..16, 12 samples/player) with 192/192 pass rows in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch12-fixed/mapgen_results.csv`.
- During full-lobby calibration at high player counts, runs stalled due disk exhaustion (`models.cache` growth under per-instance smoke homes).
- Cleaned `models.cache` files in smoke artifacts and recovered free space, then updated `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh` cleanup to auto-prune per-instance `models.cache`.
- Completed full-lobby calibration using stable timing and tail reruns:
  - Stable campaign partial (`1..13` and `14` partial): `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-stable`.
  - Tail rerun (`14..16`, 9/9 pass): `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-tail1416/mapgen_results.csv`.
  - Combined complete dataset (`1..16`, 3 runs/player, 48/48 pass): `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-complete/mapgen_results.csv`.
- Added ready-state snapshot trace hooks gated by `BARONY_SMOKE_TRACE_READY_SYNC` in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp`/`.hpp`, with minimal call sites in `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp`.
- Extended `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh` with `--auto-ready`, `--trace-ready-sync`, and `--require-ready-sync`, plus `ready_sync_results.csv` and `READY_SNAPSHOT_*` summary fields.
- Completed ready-sync churn validation lane at 8p (`3` cycles, `2` churned clients/cycle) with ready-sync assertions passing in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2`.
- Completed ready-sync churn validation lane at 16p (`3` cycles, `4` churned clients/cycle) with ready-sync assertions passing and no join failures in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4`.
- Observed transient churn rejoin retries (`sending error code 16 to client` / `Player failed to join lobby`) in one 8p ready-sync run before eventual recovery; not reproduced in the latest 16p ready-sync run, but still tracked as an intermittent follow-up.
- Added HELO player slot coverage assertions/fields (`HELO_PLAYER_SLOTS`, `HELO_MISSING_PLAYER_SLOTS`, `HELO_PLAYER_SLOT_COVERAGE_OK`) in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`; validated in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-slot-coverage-20260209-213156-p8`.

### Active Checklist (Updated February 10, 2026)
- [x] Phase A correctness gate (4p/8p/16p + payload edges + legacy + transition/mapgen lane)
- [x] Phase B adversarial gate (6/6 expectations matched)
- [x] Phase C churn 8p lane (`--instances 8 --churn-cycles 5 --churn-count 2`)
- [x] Phase C churn 16p lane (`--instances 16 --churn-cycles 3 --churn-count 4 --cycle-timeout 360`; passed in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4`)
- [x] Phase C soak 8p lane (`--runs 10 --instances 8`; 12/12 completed runs passed in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`)
- [x] Phase C soak 16p lane (`--runs 10 --instances 16 --timeout 480`; cutoff accepted at 6/6 completed pass runs in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`)
- [x] Phase D simulated mapgen baseline with in-process batching (`--simulate-mapgen-players 1 --inprocess-sim-batch 1 --runs-per-player 3`)
- [x] Phase D high-volume simulated mapgen (`--simulate-mapgen-players 1 --inprocess-sim-batch 1 --runs-per-player 12`; pass in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch12-fixed`)
- [x] Phase D full-lobby calibration mapgen (`--simulate-mapgen-players 0 --runs-per-player 3`; completed dataset in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-complete`)
- [x] Section 4 late-join ready-state snapshot assertions in churn lane (`--auto-ready 1 --trace-ready-sync 1 --require-ready-sync 1`; passes at 8p and 16p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4`)
- [x] Section 4 direct-connect high-slot assignment coverage (`HELO_PLAYER_SLOT_COVERAGE_OK=1`; pass in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-slot-coverage-20260209-213156-p8`)
- [ ] Investigate intermittent churn rejoin retries (`error code 16` bursts) seen in one 8p ready-sync run (not reproduced in latest 16p ready-sync run)
- [ ] Steam backend handshake lane
- [ ] EOS backend handshake lane
- [ ] Remaining PR-940 checklist automation scripts in Section 4

## 1. Current Suite Disposition and Required Next Actions

| Suite | Current Evidence | Action | Priority |
|---|---|---|---|
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh` | ✅ Pass at 4p/8p/16p + payload 64/900 + legacy path + dungeon transition/mapgen (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173914-p4`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173234-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173331-p16`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173555-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173647-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173741-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173813-p8`) | Keep as correctness gate; rerun as needed after networking changes | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_helo_adversarial_smoke_mac.sh` | ✅ 6/6 matched strict expectations (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-adversarial-20260209-172722/adversarial_results.csv`) | Keep strict mode as default in adversarial runs | Blocker cleared |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh` | ✅ 8p soak passed beyond target (12 completed pass runs in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`) and 16p soak passed to agreed cutoff (6 completed pass runs in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`) | Keep periodic soak as regression guard; no immediate blocker open here | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh` | ✅ 8p churn lane passed (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-174003-p8-c5x2/churn_cycle_results.csv`) and 16p churn lane passed (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4/churn_cycle_results.csv`); ✅ ready-sync assertion mode validated at both 8p and 16p (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2/ready_sync_results.csv`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4/ready_sync_results.csv`) | Keep as mandatory churn regression lane; monitor intermittent rejoin retry bursts | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh` | ✅ Simulated batch path validated at 3 and 12 samples/player (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch3/mapgen_results.csv`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch12-fixed/mapgen_results.csv`) and full-lobby calibration completed (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-complete/mapgen_results.csv`, 48/48 pass rows) | Use complete dataset for scaling/tuning loop | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/generate_smoke_aggregate_report.py` | ✅ Updated for extended adversarial CSV schema and report still generates | Use as single report artifact per campaign | Medium |

## 2. Immediate Harness Corrections (Before More Reruns)

1. Fix adversarial validity by ensuring TX-mode perturbation executes in the same handshake path exercised by LAN smoke.
   - Status (February 10, 2026): ✅ Complete. TX-mode plan logic now routes through smoke hooks and is used by both LAN/direct-connect and P2P chunk send paths.
2. In `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`, add strict assertions for non-`normal` modes:
- Require host log line with selected tx-mode (`[SMOKE]: HELO chunk tx mode=...`).
- Require expected packet plan count for that mode.
- For expected-fail modes, require at least one chunk reset/error signal and forbid completed reassembly.
   - Status (February 10, 2026): ✅ Complete (`--strict-adversarial`, `--require-txmode-log`, tx plan validation).
3. Add per-client assertions (not only summed counts):
- Every joining client must have exactly one successful HELO reassembly in pass cases.
- No client may reassemble in forced fail cases.
   - Status (February 10, 2026): ✅ Complete (`PER_CLIENT_REASSEMBLY_COUNTS` emitted and asserted in strict mode).
4. Extend `summary.env` schema to include:
- `TX_MODE_APPLIED=0|1`
- `PER_CLIENT_REASSEMBLY_COUNTS=...`
- `CHUNK_RESET_REASON_COUNTS=...`
   - Status (February 10, 2026): ✅ Complete (plus `NETWORK_BACKEND`, `MAPGEN_COUNT`, `MAPGEN_SAMPLES_REQUESTED`, `HOST_LOG`).
5. Keep all behavior smoke-only and env-gated in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp` and `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp`.
   - Status (February 10, 2026): ✅ Complete for new tx-mode plan and repeated auto-enter behavior; game source call sites remain minimal.

## 3. Rerun Plan for Existing Suites

### Phase A: Correctness Gate (must pass before further feature work)
1. 4p baseline pass:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 4 --force-chunk 1 --chunk-payload-max 200 --timeout 240
```
2. 8p baseline pass:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 8 --force-chunk 1 --chunk-payload-max 200 --timeout 300
```
3. 16p baseline pass:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 16 --force-chunk 1 --chunk-payload-max 200 --timeout 480
```
4. Payload edges:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 8 --force-chunk 1 --chunk-payload-max 64 --timeout 300
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 8 --force-chunk 1 --chunk-payload-max 900 --timeout 300
```
5. Legacy HELO path:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 8 --force-chunk 0 --require-helo 0 --timeout 300
```
6. First dungeon transition + mapgen check:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 8 --force-chunk 1 --chunk-payload-max 200 --auto-start 1 --auto-enter-dungeon 1 --require-mapgen 1 --timeout 360
```
- Status (February 10, 2026): ✅ Complete. All six lanes above passed; see artifact directories listed in Section 1 table.

### Phase B: Adversarial Gate (after harness correction)
1. Run full matrix:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_helo_adversarial_smoke_mac.sh --instances 4 --chunk-payload-max 200
```
2. Acceptance: all 6 cases match expected, including fail-modes.
- Status (February 10, 2026): ✅ Complete. `drop-last` and `duplicate-conflict-first` now fail for expected reasons; strict matrix is green.

### Phase C: Stability Gate
1. Dedicated soak:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh --runs 10 --instances 8 --force-chunk 1 --chunk-payload-max 200 --auto-enter-dungeon 1 --require-mapgen 1
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh --runs 10 --instances 16 --force-chunk 1 --chunk-payload-max 200 --auto-enter-dungeon 1 --require-mapgen 1 --timeout 480
```
2. Join/leave churn:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh --instances 8 --churn-cycles 5 --churn-count 2 --force-chunk 1 --chunk-payload-max 200
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh --instances 16 --churn-cycles 3 --churn-count 4 --force-chunk 1 --chunk-payload-max 200 --cycle-timeout 360
```
- Status (February 10, 2026): ✅ Complete for current Phase C scope. 8p soak met and exceeded the current 10-run target with 12/12 completed pass runs (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`), 16p soak met the agreed cutoff with 6/6 completed pass runs after escalation (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`), and churn passed at both 8p and 16p (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-174003-p8-c5x2`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4`).

### Phase D: Mapgen/Scaling Data Collection
1. Fast high-volume simulated sweep:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh --min-players 1 --max-players 16 --runs-per-player 12 --simulate-mapgen-players 1 --stagger 0 --auto-start-delay 0 --auto-enter-dungeon 1 --outdir /Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2
```
2. Full-lobby calibration (real multiplayer joins):
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh --min-players 1 --max-players 16 --runs-per-player 3 --simulate-mapgen-players 0 --auto-enter-dungeon 1 --outdir /Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2
```
3. Compare slopes/correlation from generated report and use as baseline for gameplay scaling iteration.
- Status (February 10, 2026): ✅ Complete for current Phase D data collection scope. Completed simulated sweeps at 3 and 12 samples/player (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch3`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch12-fixed`) and completed full-lobby calibration at 3 runs/player (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-complete`, 48/48 pass rows; assembled from stable+tail campaigns after disk-pressure remediation).

## 4. Missing Automated Tests (Driven by PR 940 Checklist)

| PR 940 Area | Current Coverage | New Automated Test |
|---|---|---|
| Lobby player-count warning, `# Players` UX, page focus, page text | Mostly missing | Add `run_lobby_ui_state_smoke_mac.sh` with smoke hooks exporting structured lobby state snapshots per tick |
| Kick dropdown + confirmation correctness across high slots/pages | Missing | Add `run_lobby_kick_target_smoke_mac.sh` that programmatically triggers kick flow and verifies correct target removal |
| Late-join ready-state sync correctness | Partial | ✅ Extended churn test with `--require-ready-sync`; lanes passing at 8p and 16p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4` (monitor intermittent retry bursts) |
| 5+ direct-connect account label correctness | Partial | ✅ Added deterministic HELO player-slot coverage assertions in `run_lan_helo_chunk_smoke_mac.sh` (high-slot assignment coverage); explicit UI label text automation still pending |
| Save/reload 5+ and legacy `players_connected` compatibility | Missing | Add `run_save_reload_compat_smoke_mac.sh` with fixture saves and continue-card state assertions |
| Visual slot mapping (ghost icons, world icons, XP themes, loot bag visuals; normal/colorblind) | Missing | Add `run_visual_slot_mapping_smoke_mac.sh` capturing screenshots and validating expected sprite/theme indices |
| Splitscreen cap behavior (`/splitscreen > 4`) | Missing | Add `run_splitscreen_cap_smoke_mac.sh` asserting clamp and no MAXPLAYERS side effects |
| Steam/EOS transport-specific handshake behavior | Missing | Add backend smoke lane (`lan/steam/eos`) and backend-tagged artifacts; gate Steam first, EOS nightly/manual until creds are automatable |

## 5. Proposed Public Interface / Type Additions

1. Script API additions in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`:
- `--network-backend lan|steam|eos` (default `lan`)
- `--strict-adversarial 0|1` (default `1` for adversarial runner)
- `--require-txmode-log 0|1` (default `0`, set to `1` in adversarial mode)
   - Status (February 10, 2026): ⚠️ Partial. Argument/schema implemented, but runner currently enforces `lan` execution only.
2. `summary.env` additions:
- `NETWORK_BACKEND`
- `TX_MODE_APPLIED`
- `PER_CLIENT_REASSEMBLY_COUNTS`
- `CHUNK_RESET_REASON_COUNTS`
   - Status (February 10, 2026): ✅ Implemented.
3. Smoke-only env additions in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp`:
- `BARONY_SMOKE_SCENARIO=<name>`
- `BARONY_SMOKE_EXPORT_STATE_PATH=<path>`
- `BARONY_SMOKE_ASSERT_LEVEL=<0..2>`
   - Status (February 10, 2026): ⚠️ Not implemented yet. Additional env implemented: `BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS=<n>` for in-runtime mapgen batch sampling and `BARONY_SMOKE_TRACE_READY_SYNC=0|1` for ready snapshot diagnostics.
4. All new hooks remain dormant unless smoke env vars are set.
   - Status (February 10, 2026): ✅ True for implemented hooks.

## 6. Scaling Validation and Gameplay Tuning Loop (1-16 players)

1. Use new mapgen datasets to detect non-scaling metrics automatically.
2. Define quantitative targets:
- `monsters` slope positive and stable.
- `gold` and `items` non-negative trend vs players.
- No severe regressions in `rooms` and `decorations`.
3. Prioritize tuning points currently likely suppressing scaling in `/Users/sayhiben/dev/Barony-8p/src/maps.cpp`:
- Loot-to-monster reroll aggressiveness (`getOverflowLootToMonsterRerollDivisor` and call site around line 4778).
- Food and item-stack overflow rules around lines 7542 and 7727.
- Gold bonus calculations around lines 6249, 6262, and 7848.
4. Rerun Phase D after each scaling change and compare against previous aggregate report.
   - Status (February 10, 2026): Baseline dataset refreshed with new batch sweep path; no gameplay tuning commits applied yet in this pass.

## 7. Acceptance Gates and Exit Criteria

1. HELO chunking confidence gate:
- All correctness, adversarial, soak, and churn suites pass at required counts.
- No adversarial expectation mismatches.
   - Current status (February 10, 2026): ✅ Green for current lane targets: correctness + adversarial + soak (8p/16p cutoff) + churn (8p/16p) are all passing.
2. Multiplayer expansion validation gate:
- Automated checks cover each PR 940 checklist cluster at least once (fully automated or semi-automated).
- 16-player join/start/enter-dungeon/churn lanes stable.
   - Current status (February 10, 2026): ⚠️ Not complete. Core HELO/lobby-join/churn/mapgen lanes are green, and checklist automation coverage improved (ready-sync churn assertions + high-slot assignment assertions), but multiple PR-940 UI/save/splitscreen checks and Steam/EOS backend lanes remain open.
3. Scaling gate:
- New mapgen reports show improved trends for loot/gold/items with increasing players.
   - Current status (February 10, 2026): ⚠️ Data collection baseline is now complete (simulated and full-lobby), but tuning loop/trend targets are still open.
4. Cleanup gate:
- Remove `/Users/sayhiben/dev/Barony-8p/HELO_ONLY_CHUNKING_PLAN.md` only after HELO confidence gate is green.
   - Current status (February 10, 2026): Not ready.

## Assumptions and Defaults

1. Default execution environment is macOS with Steam-installed assets; smoke runs use `/Users/sayhiben/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony`.
2. Latest built binary is copied into Steam app path before every verification campaign.
3. LAN smoke remains primary fast gate; Steam backend is added to regular validation; EOS backend is added as nightly/manual until credential automation is practical.
4. Smoke tooling remains isolated to `/Users/sayhiben/dev/Barony-8p/src/smoke` and `/Users/sayhiben/dev/Barony-8p/tests/smoke`.
