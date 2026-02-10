# Multiplayer Expansion Verification Plan (Post-PR 940)

## Summary
This plan turns the current smoke harness into a reliable gate for the up-to-15-player expansion, reruns existing suites with stronger evidence, and closes key automation gaps from PR 940's manual checklist.
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
- Observed transient churn rejoin retries (`sending error code 16 to client` / `Player failed to join lobby`) in 8p ready-sync runs before eventual recovery; latest 16p ready-sync run remained clean, but the intermittent 8p issue is now re-confirmed and tracked.
- Added HELO player slot coverage assertions/fields (`HELO_PLAYER_SLOTS`, `HELO_MISSING_PLAYER_SLOTS`, `HELO_PLAYER_SLOT_COVERAGE_OK`) in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`; validated in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-slot-coverage-20260209-213156-p8`.
- Added account-label coverage assertions/fields (`ACCOUNT_LABEL_LINES`, `ACCOUNT_LABEL_SLOTS`, `ACCOUNT_LABEL_MISSING_SLOTS`, `ACCOUNT_LABEL_SLOT_COVERAGE_OK`) in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`; validated at 8p and 16p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223536-p8-r5` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223827-p16-r1`.
- Added `--datadir <path>` passthrough to `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_helo_adversarial_smoke_mac.sh`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh`, and `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh` so local build binaries can run against Steam `Contents/Resources` without replacing the Steam executable.
- Hardened smoke reruns against stale artifact reuse by clearing per-run volatile state (`instances/`, `stdout/`, summary/pid/csv files) before launch in the core runners.
- Re-ran 8p ready-sync churn lane on the build+datadir launch path and reproduced rejoin retry bursts (`error code 16` / `Player failed to join lobby`) before eventual recovery: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-224150-p8-c3x2-postdatadir` (`JOIN_FAIL_LINES=12`, lane still pass).
- Added smoke-only join-reject slot-state traces (`BARONY_SMOKE_TRACE_JOIN_REJECTS`) in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp` with a minimal call site in `/Users/sayhiben/dev/Barony-8p/src/net.cpp`, and runner option `--trace-join-rejects` in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh`.
- Captured join-reject diagnostics in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-225142-p8-c2x2-joinreject-trace`: repeated `code=16` snapshots report `free_unlocked=0`, `free_locked=8`, `occupied=7`, `states=OOOOOOOLLLLLLLL` before eventual rejoin success.
- Developer warning confirmed: multiple spell/effect paths pack caster player IDs into 4-bit fields (`& 0xF` / high nibble), which cannot safely represent player slot 16 plus non-player sentinel; this is now tracked as a dedicated compatibility task (examples in `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp` and decode paths in `/Users/sayhiben/dev/Barony-8p/src/entity.cpp`, `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`).
- Decision update (February 10, 2026): keep existing nibble bit-packing unchanged and cap `MAXPLAYERS` to `15` (`BARONY_SUPER_MULTIPLAYER`) to avoid 16th-slot/sentinel collisions. Existing 16p artifacts remain useful historical stress evidence, but forward gating now targets 15p max.
- Post-cap baseline check passed at 15p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/cap15-baseline-p15-escalated` (`RESULT=pass`, `HOST_CHUNK_LINES=14`, `CLIENT_REASSEMBLED_LINES=14`, `CHUNK_RESET_LINES=0`).
- Completed focused code audit of player-related bit/nibble packing after the 15p cap change:
  - ✅ Safe at 15: core high-nibble caster encodings for `EFF_NIMBLENESS`, `EFF_GREATER_MIGHT`, `EFF_COUNSEL`, `EFF_STURDINESS`, `EFF_MAXIMISE`, `EFF_MINIMISE` (encode in `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp`, decode in `/Users/sayhiben/dev/Barony-8p/src/entity.cpp` and `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`).
  - ✅ Resolved: `EFF_FAST` caster attribution now uses a high-nibble owner-id encode (`(player + 1) << 4`, low nibble clear) with dual-format decode support for backward compatibility (new owner-id format plus legacy bitmask values) in `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp` and `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`.
  - ✅ Resolved: `EFF_DIVINE_FIRE` ownership consume path now matches owner-id semantics (`==`) instead of bitwise mask checks in `/Users/sayhiben/dev/Barony-8p/src/entity.cpp`.
  - ✅ Resolved: `EFF_SIGIL`/`EFF_SANCTUARY` now encode non-player ownership explicitly as high-nibble `0` sentinel (no overflow dependency), with player-owner nibble packing centralized in `/Users/sayhiben/dev/Barony-8p/src/magic/actmagic.cpp`.
- Added compile-time guardrails for `EFF_FAST` owner encoding/decoding (`MAXPLAYERS <= 15`) in `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp` and `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`.
- Added compile-time guardrail for packed owner encoding in `/Users/sayhiben/dev/Barony-8p/src/magic/actmagic.cpp` (`MAXPLAYERS <= 15`).
- Centralized packed status-effect owner encode/decode helpers in `/Users/sayhiben/dev/Barony-8p/src/status_effect_owner_encoding.hpp` and switched `EFF_FAST`, `EFF_DIVINE_FIRE`, `EFF_SIGIL`, and `EFF_SANCTUARY` paths to use the shared codec in `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp`, `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`, `/Users/sayhiben/dev/Barony-8p/src/magic/actmagic.cpp`, and `/Users/sayhiben/dev/Barony-8p/src/entity.cpp`.
- Applied the shared codec to the remaining core owner-nibble status-effect sites (`EFF_NIMBLENESS`, `EFF_GREATER_MIGHT`, `EFF_COUNSEL`, `EFF_STURDINESS`, `EFF_MAXIMISE`, `EFF_MINIMISE`) so raw `>> 4`/`<< 4` owner packing logic is removed from those paths in `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp`, `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`, and `/Users/sayhiben/dev/Barony-8p/src/entity.cpp`.

### Active Checklist (Updated February 10, 2026)
- [x] Phase A correctness gate (4p/8p/15p + payload edges + legacy + transition/mapgen lane)
- [x] Post-cap 15p HELO baseline rerun (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/cap15-baseline-p15-escalated`)
- [x] Phase B adversarial gate (6/6 expectations matched)
- [x] Phase C churn 8p lane (`--instances 8 --churn-cycles 5 --churn-count 2`)
- [x] Historical 16p churn evidence captured pre-cap (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4`); forward churn gate now uses 15p.
- [x] Phase C soak 8p lane (`--runs 10 --instances 8`; 12/12 completed runs passed in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`)
- [x] Historical 16p soak evidence captured pre-cap (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`); forward soak gate now uses 15p.
- [x] Phase D simulated mapgen baseline with in-process batching (`--simulate-mapgen-players 1 --inprocess-sim-batch 1 --runs-per-player 3`)
- [x] Phase D high-volume simulated mapgen (`--simulate-mapgen-players 1 --inprocess-sim-batch 1 --runs-per-player 12`; pass in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch12-fixed`)
- [x] Phase D full-lobby calibration mapgen (`--simulate-mapgen-players 0 --runs-per-player 3`; completed dataset in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-complete`)
- [x] Section 4 late-join ready-state snapshot assertions in churn lane (`--auto-ready 1 --trace-ready-sync 1 --require-ready-sync 1`; passes at 8p and historical 16p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4`)
- [x] Section 4 direct-connect high-slot assignment coverage (`HELO_PLAYER_SLOT_COVERAGE_OK=1`; pass in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-slot-coverage-20260209-213156-p8`)
- [x] Section 4 direct-connect account label coverage (`ACCOUNT_LABEL_SLOT_COVERAGE_OK=1`; passes at 8p and historical 16p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223536-p8-r5` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223827-p16-r1`)
- [ ] Investigate intermittent churn rejoin retries (`error code 16` bursts): reproduced in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-224150-p8-c3x2-postdatadir` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-225142-p8-c2x2-joinreject-trace`; trace suggests a transient slot-lock window (`free_unlocked=0`, `free_locked=8`) during rejoin attempts
- [x] Adopt explicit 15-player cap to preserve existing nibble-packed caster/player encodings (`0xF` masks) without refactoring bit operations
- [x] Audit player/caster bit/nibble encoding paths for 15-player safety (spell effects, voice metadata, loot bag keying)
- [x] Confirm core high-nibble caster ownership paths are safe at 15 (`EFF_NIMBLENESS`, `EFF_GREATER_MIGHT`, `EFF_COUNSEL`, `EFF_STURDINESS`, `EFF_MAXIMISE`, `EFF_MINIMISE`)
- [x] Confirm full-byte `player+1` ownership paths are safe at 15 (`EFF_CONFUSED`, `EFF_TABOO`, `EFF_PINPOINT`, `EFF_PENANCE`, `EFF_CURSE_FLESH`)
- [x] Fix `EFF_FAST` caster attribution encoding for slots 8-15 (migrated to high-nibble owner-id encode with legacy bitmask decode fallback for backward compatibility)
- [x] Resolve `EFF_DIVINE_FIRE` high-nibble ownership semantics (consume path now uses owner-id equality)
- [x] Harden `EFF_SIGIL`/`EFF_SANCTUARY` non-player sentinel encoding so it does not depend on overflow side effects
- [ ] Add `GameUI` status-effect queue initialization lane for 1p/5p/15p startup and late-join/rejoin safety
- [ ] Add automation for default slot-lock behavior and occupied-slot count-reduction kick copy permutations (1-player/2-player/multi-player wording)
- [ ] Add automation for lobby page navigation and alignment checks on keyboard/controller (focus, card placement, paperdolls, ping frames, warnings, countdown alignment while paging)
- [ ] Add remote-combat invalid-slot regression lane (pause/unpause, enemy HP bars, combat interactions with remote players)
- [ ] Add baseline local 4-player splitscreen lane (spawn/join/control/pause/hud integrity) to confirm legacy splitscreen behavior still works
- [ ] Add targeted overflow pacing lane for hunger/appraisal at 3p/4p/5p/8p/12p/15p (legacy-vs-overflow behavior tracking)
- [ ] Steam backend handshake lane
- [ ] EOS backend handshake lane
- [ ] Remaining PR-940 checklist automation scripts in Section 4

## 1. Current Suite Disposition and Required Next Actions

| Suite | Current Evidence | Action | Priority |
|---|---|---|---|
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh` | ✅ Pass at 4p/8p/16p + payload 64/900 + legacy path + dungeon transition/mapgen (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173914-p4`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173234-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173331-p16`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173555-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173647-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173741-p8`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-20260209-173813-p8`) plus account-label coverage passes at 8p/16p (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223536-p8-r5`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223827-p16-r1`) | Keep as correctness gate; rerun as needed after networking changes | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_helo_adversarial_smoke_mac.sh` | ✅ 6/6 matched strict expectations (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/helo-adversarial-20260209-172722/adversarial_results.csv`) | Keep strict mode as default in adversarial runs | Blocker cleared |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh` | ✅ 8p soak passed beyond target (12 completed pass runs in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`) and 16p soak passed to agreed cutoff (6 completed pass runs in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`) | Keep periodic soak as regression guard; no immediate blocker open here | High |
| `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh` | ✅ 8p churn lane passed (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-174003-p8-c5x2/churn_cycle_results.csv`) and 16p churn lane passed (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4/churn_cycle_results.csv`); ✅ ready-sync assertion mode validated at both 8p and 16p (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2/ready_sync_results.csv`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4/ready_sync_results.csv`); ⚠️ intermittent rejoin retries reproduced in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-224150-p8-c3x2-postdatadir` and traced in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-225142-p8-c2x2-joinreject-trace` (`JOIN_FAIL_LINES=9`, `JOIN_REJECT_TRACE_LINES=9`) | Keep as mandatory churn regression lane; investigate and reduce retry bursts | High |
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
3. 15p baseline pass:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh --instances 15 --force-chunk 1 --chunk-payload-max 200 --timeout 480
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
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_soak_mac.sh --runs 10 --instances 15 --force-chunk 1 --chunk-payload-max 200 --auto-enter-dungeon 1 --require-mapgen 1 --timeout 480
```
2. Join/leave churn:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh --instances 8 --churn-cycles 5 --churn-count 2 --force-chunk 1 --chunk-payload-max 200
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_join_leave_churn_smoke_mac.sh --instances 15 --churn-cycles 3 --churn-count 4 --force-chunk 1 --chunk-payload-max 200 --cycle-timeout 360
```
- Status (February 10, 2026): ✅ Complete for current Phase C scope. 8p soak met and exceeded the current 10-run target with 12/12 completed pass runs (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-185835-p8-n20`), 16p soak met the agreed cutoff with 6/6 completed pass runs after escalation (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/soak-20260209-191250-p16-n10`), and churn passed at both 8p and 16p (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-174003-p8-c5x2`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-20260209-192405-p16-c3x4`).

### Phase D: Mapgen/Scaling Data Collection
1. Fast high-volume simulated sweep:
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh --min-players 1 --max-players 15 --runs-per-player 12 --simulate-mapgen-players 1 --stagger 0 --auto-start-delay 0 --auto-enter-dungeon 1 --outdir /Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2
```
2. Full-lobby calibration (real multiplayer joins):
```bash
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh --min-players 1 --max-players 15 --runs-per-player 3 --simulate-mapgen-players 0 --auto-enter-dungeon 1 --outdir /Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2
```
3. Compare slopes/correlation from generated report and use as baseline for gameplay scaling iteration.
- Status (February 10, 2026): ✅ Complete for current Phase D data collection scope. Completed simulated sweeps at 3 and 12 samples/player (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch3`, `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-sim-v2-batch12-fixed`) and completed full-lobby calibration at 3 runs/player (`/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-v2-complete`, 48/48 pass rows; assembled from stable+tail campaigns after disk-pressure remediation).

## 4. Missing Automated Tests (Driven by PR 940 Checklist)

| PR 940 Area | Current Coverage | New Automated Test |
|---|---|---|
| Lobby player-count warning, `# Players` UX, page focus, page text | Mostly missing | Add `run_lobby_ui_state_smoke_mac.sh` with smoke hooks exporting structured lobby state snapshots per tick |
| Kick dropdown + confirmation correctness across high slots/pages | Missing | Add `run_lobby_kick_target_smoke_mac.sh` that programmatically triggers kick flow and verifies correct target removal |
| Default slot-lock behavior + occupied-slot count-reduction copy variants | Missing | Add `run_lobby_slot_lock_and_kick_copy_smoke_mac.sh` asserting lock defaults and confirmation text for 1-player/2-player/multi-player removal cases |
| Late-join ready-state sync correctness | Partial | ✅ Extended churn test with `--require-ready-sync`; lanes passing at 8p and 16p in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/churn-ready-sync-20260209-213532-p16-c3x4` (monitor intermittent retry bursts) |
| Lobby page navigation alignment (keyboard/controller, focus, paperdolls, ping/countdown while paging) | Missing | Add `run_lobby_page_navigation_smoke_mac.sh` with scripted paging inputs and per-tick UI snapshot assertions |
| 5+ direct-connect account label correctness | ✅ Automated in HELO lane at 8p/16p (`ACCOUNT_LABEL_SLOT_COVERAGE_OK=1`) | ✅ Implemented with `--trace-account-labels 1 --require-account-labels 1` in `run_lan_helo_chunk_smoke_mac.sh`; evidence in `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223536-p8-r5` and `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/account-label-coverage-20260209-223827-p16-r1` |
| Status-effect caster ownership encoding near player cap | ✅ Updated: audited high-risk paths now use consistent owner-id semantics (`EFF_FAST`, `EFF_DIVINE_FIRE`) and explicit non-player sentinel handling (`EFF_SIGIL`, `EFF_SANCTUARY`) with packed-owner compile-time guards | Keep `MAXPLAYERS<=15`; add targeted status-effect queue smoke coverage at 1p/5p/15p |
| `GameUI` status-effect queue initialization at high player counts | Missing | Add `run_status_effect_queue_init_smoke_mac.sh` to assert queue initialization/index assignment safety at 1p/5p/15p, including late-join/rejoin |
| Save/reload 5+ and legacy `players_connected` compatibility | Missing | Add `run_save_reload_compat_smoke_mac.sh` with fixture saves and continue-card state assertions |
| Visual slot mapping (ghost icons, world icons, XP themes, loot bag visuals; normal/colorblind) | Missing | Add `run_visual_slot_mapping_smoke_mac.sh` capturing screenshots and validating expected sprite/theme indices |
| Runtime combat/UI slot safety (pause/unpause, enemy HP bars, remote combat interactions) | Missing | Add `run_remote_combat_slot_bounds_smoke_mac.sh` that exercises remote combat HUD and verifies no invalid-slot reads/writes |
| Baseline local splitscreen functionality (4 players) | Missing | Add `run_splitscreen_baseline_smoke_mac.sh` to verify 4-player local split-screen setup, controller assignment, HUD/camera, pause flow, and first-floor transition behavior |
| Splitscreen cap behavior (`/splitscreen > 4`) | Missing | Add `run_splitscreen_cap_smoke_mac.sh` asserting clamp and no MAXPLAYERS side effects |
| Steam/EOS transport-specific handshake behavior | Missing | Add backend smoke lane (`lan/steam/eos`) and backend-tagged artifacts; gate Steam first, EOS nightly/manual until creds are automatable |

## 5. Proposed Public Interface / Type Additions

1. Script API additions in `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`:
- `--network-backend lan|steam|eos` (default `lan`)
- `--strict-adversarial 0|1` (default `1` for adversarial runner)
- `--require-txmode-log 0|1` (default `0`, set to `1` in adversarial mode)
   - Status (February 10, 2026): ⚠️ Partial. Backend tagging schema is implemented, but runner currently enforces `lan` execution only.
2. Script execution additions across smoke runners:
- `--datadir <path>` passthrough to launch local builds against Steam assets (`-datadir=<path>`).
   - Status (February 10, 2026): ✅ Implemented in HELO/soak/adversarial/mapgen/churn runners.
3. `summary.env` additions:
- `NETWORK_BACKEND`
- `TX_MODE_APPLIED`
- `PER_CLIENT_REASSEMBLY_COUNTS`
- `CHUNK_RESET_REASON_COUNTS`
   - Status (February 10, 2026): ✅ Implemented.
4. Smoke-only env additions in `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp`:
- `BARONY_SMOKE_SCENARIO=<name>`
- `BARONY_SMOKE_EXPORT_STATE_PATH=<path>`
- `BARONY_SMOKE_ASSERT_LEVEL=<0..2>`
   - Status (February 10, 2026): ⚠️ Not implemented yet. Additional env implemented: `BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS=<n>` for in-runtime mapgen batch sampling, `BARONY_SMOKE_TRACE_READY_SYNC=0|1` for ready snapshot diagnostics, and `BARONY_SMOKE_TRACE_JOIN_REJECTS=0|1` for slot-state join-reject diagnostics.
5. All new hooks remain dormant unless smoke env vars are set.
   - Status (February 10, 2026): ✅ True for implemented hooks.

## 6. Scaling Validation and Gameplay Tuning Loop (1-15 players)

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
5. Add a dedicated overflow pacing lane (3p/4p/5p/8p/12p/15p) for hunger/appraisal checks to ensure 3p/4p behavior remains stable while >4 scaling remains bounded.
   - Status (February 10, 2026): Baseline dataset refreshed with new batch sweep path; no gameplay tuning commits applied yet in this pass.

## 7. Acceptance Gates and Exit Criteria

1. HELO chunking confidence gate:
- All correctness, adversarial, soak, and churn suites pass at required counts.
- No adversarial expectation mismatches.
   - Current status (February 10, 2026): ✅ Green for current lane targets: correctness + adversarial + soak/churn lanes (including historical 16p stress evidence) are passing.
2. Multiplayer expansion validation gate:
- Automated checks cover each PR 940 checklist cluster at least once (fully automated or semi-automated).
- 15-player join/start/enter-dungeon/churn lanes stable.
   - Current status (February 10, 2026): ⚠️ Not complete. Core HELO/lobby-join/churn/mapgen lanes are green, and checklist automation coverage improved (ready-sync churn assertions + high-slot assignment assertions + account-label coverage assertions), but multiple PR-940 UI/save/splitscreen checks and Steam/EOS backend lanes remain open.
3. Scaling gate:
- New mapgen reports show improved trends for loot/gold/items with increasing players.
   - Current status (February 10, 2026): ⚠️ Data collection baseline is now complete (simulated and full-lobby), but tuning loop/trend targets are still open.
4. Cleanup gate:
- Remove `/Users/sayhiben/dev/Barony-8p/HELO_ONLY_CHUNKING_PLAN.md` only after HELO confidence gate is green.
   - Current status (February 10, 2026): Not ready.

## 8. Bit/Nibble Encoding Safety Audit (4-15 players)

This section tracks player/caster identity encodings that rely on `0xF`, high nibble fields, or compact bit-packing so the 15-player cap remains safe and maintainable.

### Confirmed Safe at `MAXPLAYERS=15`

1. Core high-nibble caster ownership encodings are safe and consistent with current decode logic:
- `EFF_NIMBLENESS`, `EFF_GREATER_MIGHT`, `EFF_COUNSEL`, `EFF_STURDINESS`, `EFF_MAXIMISE`, `EFF_MINIMISE`.
- Encode examples: `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp:1899`, `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp:2941`, `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp:3025`.
- Decode examples: `/Users/sayhiben/dev/Barony-8p/src/entity.cpp:2600`, `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp:11521`.
2. Full-byte `player+1` ownership fields are safe with cap 15:
- `EFF_CONFUSED`, `EFF_TABOO`, `EFF_PINPOINT`, `EFF_PENANCE`, `EFF_CURSE_FLESH`.
- Examples: `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp:4563`, `/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp:5816`, `/Users/sayhiben/dev/Barony-8p/src/entity.cpp:18544`.
3. Loot bag owner nibble keying remains safe for slots `0..14`:
- `/Users/sayhiben/dev/Barony-8p/src/stat.cpp:1695`, `/Users/sayhiben/dev/Barony-8p/src/items.cpp:7639`.

### Follow-Up Tasks

1. ✅ `EFF_DIVINE_FIRE` ownership semantics were aligned:
- Consume path now compares decoded owner id with `(1 + player)` using equality in `/Users/sayhiben/dev/Barony-8p/src/entity.cpp`.
2. ✅ `EFF_SIGIL` / `EFF_SANCTUARY` non-player sentinel no longer depends on overflow:
- Encode now preserves low nibble strength and explicitly emits high nibble `0` for non-player casters in `/Users/sayhiben/dev/Barony-8p/src/magic/actmagic.cpp`.
- Player ownership nibble packing was centralized with an explicit `MAXPLAYERS <= 15` compile-time guard in `/Users/sayhiben/dev/Barony-8p/src/magic/actmagic.cpp`.

### Guardrails

1. Keep `MAXPLAYERS` capped at 15 unless these encodings are migrated.
2. ✅ Compile-time guard(s) now exist near packed-player encode/decode sites (`/Users/sayhiben/dev/Barony-8p/src/magic/castSpell.cpp`, `/Users/sayhiben/dev/Barony-8p/src/actplayer.cpp`, `/Users/sayhiben/dev/Barony-8p/src/magic/actmagic.cpp`) to prevent accidental cap increases without format updates.
3. Prefer new smoke assertions/hook traces in `/Users/sayhiben/dev/Barony-8p/src/smoke` and `/Users/sayhiben/dev/Barony-8p/tests/smoke` for any fixes here to keep core gameplay files minimally touched.

## 9. Assumptions and Defaults

1. Default execution environment is macOS with Steam-installed assets.
2. Preferred launch path for smoke in this pass is local build binary + Steam assets datadir (`--app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony --datadir /Users/sayhiben/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources`) to avoid replacing the Steam app executable.
3. LAN smoke remains primary fast gate; Steam backend is added to regular validation; EOS backend is added as nightly/manual until credential automation is practical.
4. Smoke tooling remains isolated to `/Users/sayhiben/dev/Barony-8p/src/smoke` and `/Users/sayhiben/dev/Barony-8p/tests/smoke`.
