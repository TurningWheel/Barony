# Multiplayer Expansion Verification Plan (Post-PR 940)

## Summary
This plan turns the current smoke harness into a reliable gate for the 16-player expansion, reruns existing suites with stronger evidence, and closes key automation gaps from PR 940's manual checklist.
Current status from artifacts shows broad LAN smoke success, but two important risks remain:
1. Adversarial HELO fail-modes are not currently proving failure (`drop-last`, `duplicate-conflict-first` mismatches in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/post-refactor-adversarial-v2/adversarial_results.csv`).
2. Coverage is almost entirely LAN/direct-connect; Steam/EOS transport-specific behavior is not validated by current smoke flows.

## 1. Current Suite Disposition and Required Next Actions

| Suite | Current Evidence | Action | Priority |
|---|---|---|---|
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh` | Multiple pass runs at 4p/8p; chunk payload variants; legacy non-chunk path pass | Rerun with stricter assertions and include 16p lane | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_helo_adversarial_smoke_mac.sh` | 4/6 matched; fail-cases unexpectedly pass | Adjust harness/runtime path, then rerun full matrix | Blocker |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh` | Equivalent manual soak exists (`manual-t6-r*`) but not via dedicated soak runner CSV | Run dedicated soak runner for standardized reporting | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh` | Script exists, no recorded run artifacts yet | Run and gate on churn scenarios | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh` | 1 run/player dataset only (`manual-t5`), high noise | Rerun with larger sample size; run simulate + calibration lanes | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/generate_smoke_aggregate_report.py` | Working | Use as single report artifact per test campaign | Medium |

## 2. Immediate Harness Corrections (Before More Reruns)

1. Fix adversarial validity by ensuring TX-mode perturbation executes in the same handshake path exercised by LAN smoke.
2. In `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`, add strict assertions for non-`normal` modes:
- Require host log line with selected tx-mode (`[SMOKE]: HELO chunk tx mode=...`).
- Require expected packet plan count for that mode.
- For expected-fail modes, require at least one chunk reset/error signal and forbid completed reassembly.
3. Add per-client assertions (not only summed counts):
- Every joining client must have exactly one successful HELO reassembly in pass cases.
- No client may reassemble in forced fail cases.
4. Extend `summary.env` schema to include:
- `TX_MODE_APPLIED=0|1`
- `PER_CLIENT_REASSEMBLY_COUNTS=...`
- `CHUNK_RESET_REASON_COUNTS=...`
5. Keep all behavior smoke-only and env-gated in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp` and `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp`.

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

### Phase B: Adversarial Gate (after harness correction)
1. Run full matrix:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_helo_adversarial_smoke_mac.sh --instances 4 --chunk-payload-max 200
```
2. Acceptance: all 6 cases match expected, including fail-modes.

### Phase C: Stability Gate
1. Dedicated soak:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh --runs 20 --instances 8 --force-chunk 1 --chunk-payload-max 200 --auto-enter-dungeon 1 --require-mapgen 1
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh --runs 10 --instances 16 --force-chunk 1 --chunk-payload-max 200 --auto-enter-dungeon 1 --require-mapgen 1 --timeout 480
```
2. Join/leave churn:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh --instances 8 --churn-cycles 5 --churn-count 2 --force-chunk 1 --chunk-payload-max 200
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh --instances 16 --churn-cycles 3 --churn-count 4 --force-chunk 1 --chunk-payload-max 200 --cycle-timeout 360
```

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

## 4. Missing Automated Tests (Driven by PR 940 Checklist)

| PR 940 Area | Current Coverage | New Automated Test |
|---|---|---|
| Lobby player-count warning, `# Players` UX, page focus, page text | Mostly missing | Add `run_lobby_ui_state_smoke_mac.sh` with smoke hooks exporting structured lobby state snapshots per tick |
| Kick dropdown + confirmation correctness across high slots/pages | Missing | Add `run_lobby_kick_target_smoke_mac.sh` that programmatically triggers kick flow and verifies correct target removal |
| Late-join ready-state sync correctness | Partial | Extend churn test to assert ready-state snapshot delivery events in logs per rejoin cycle |
| 5+ direct-connect account label correctness | Missing | Add deterministic log assertion test for names (`Player 5`, etc.) after full lobby join |
| Save/reload 5+ and legacy `players_connected` compatibility | Missing | Add `run_save_reload_compat_smoke_mac.sh` with fixture saves and continue-card state assertions |
| Visual slot mapping (ghost icons, world icons, XP themes, loot bag visuals; normal/colorblind) | Missing | Add `run_visual_slot_mapping_smoke_mac.sh` capturing screenshots and validating expected sprite/theme indices |
| Splitscreen cap behavior (`/splitscreen > 4`) | Missing | Add `run_splitscreen_cap_smoke_mac.sh` asserting clamp and no MAXPLAYERS side effects |
| Steam/EOS transport-specific handshake behavior | Missing | Add backend smoke lane (`lan/steam/eos`) and backend-tagged artifacts; gate Steam first, EOS nightly/manual until creds are automatable |

## 5. Proposed Public Interface / Type Additions

1. Script API additions in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`:
- `--network-backend lan|steam|eos` (default `lan`)
- `--strict-adversarial 0|1` (default `1` for adversarial runner)
- `--require-txmode-log 0|1` (default `0`, set to `1` in adversarial mode)
2. `summary.env` additions:
- `NETWORK_BACKEND`
- `TX_MODE_APPLIED`
- `PER_CLIENT_REASSEMBLY_COUNTS`
- `CHUNK_RESET_REASON_COUNTS`
3. Smoke-only env additions in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp`:
- `BARONY_SMOKE_SCENARIO=<name>`
- `BARONY_SMOKE_EXPORT_STATE_PATH=<path>`
- `BARONY_SMOKE_ASSERT_LEVEL=<0..2>`
4. All new hooks remain dormant unless smoke env vars are set.

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

## 7. Acceptance Gates and Exit Criteria

1. HELO chunking confidence gate:
- All correctness, adversarial, soak, and churn suites pass at required counts.
- No adversarial expectation mismatches.
2. Multiplayer expansion validation gate:
- Automated checks cover each PR 940 checklist cluster at least once (fully automated or semi-automated).
- 16-player join/start/enter-dungeon/churn lanes stable.
3. Scaling gate:
- New mapgen reports show improved trends for loot/gold/items with increasing players.
4. Cleanup gate:
- Remove `/Users/sayhiben/dev/Barony-8p/HELO_ONLY_CHUNKING_PLAN.md` only after HELO confidence gate is green.

## Assumptions and Defaults

1. Default execution environment is macOS with Steam-installed assets; smoke runs use `/Users/sayhiben/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/MacOS/Barony`.
2. Latest built binary is copied into Steam app path before every verification campaign.
3. LAN smoke remains primary fast gate; Steam backend is added to regular validation; EOS backend is added as nightly/manual until credential automation is practical.
4. Smoke tooling remains isolated to `/Users/sayhiben/dev/Barony-8p/src/smoke` and `/Users/sayhiben/dev/Barony-8p/tests/smoke`.
