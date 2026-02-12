# Multiplayer Expansion Verification Plan (PR 940, Near-Finish)

Last updated: February 12, 2026  
Target: `MAXPLAYERS=15`

## 1. Status Snapshot
- Overall status: mostly complete.
- Core networking validation is green: LAN HELO correctness, adversarial strict fail-modes, soak/churn, and high-slot regression lanes.
- Steam backend handshake is validated for host-room creation/key capture, but local same-account multi-instance joins are limited by Steam account/session rules.
- EOS backend handshake coverage is still pending.
- Scaling work has advanced to overflow/mapgen tuning pass 10 (structural + decoration telemetry + sweep stability hardening). Latest full simulated volatility matrix (`1/7/16/33`, `1..15p`, runs=3) keeps core metrics positively sloped across all levels (`rooms/monsters/gold/items/food/decorations` all `4/4` positive-slope levels) with rooms in target range (`mean high_vs_low_pct +58.6`) and economy totals strengthened (`gold mean high_vs_low_pct +125.2`, `items +74.5`).
- Current balancing stage is `Pass10 complete / Pass11 planning complete`: implementation focus is now per-player economy/food uplift for `>4p` while preserving strict `<=4p` parity.
- Regeneration/level-target diagnostics are stable in the runs=3 matrix (`target_level_match_rate_pct=100`, `observed_seed_unique_rate_pct=100`, `reload_unique_seed_rate_pct=100`), confirming that same-level reload sweeps regenerate unique maps rather than reusing a prior map.
- Targeted economy bump experiment on `levels=1,16` (runs=3) produced mixed deep-floor outcomes and was reverted; pass8 is now the current simulated tuning baseline while preserving pass5 behavior for `1..4p`.
- Follow-up pass6/pass6b overflow experiments (runs=3 full matrix) successfully reduced extreme food inflation and raised high-party gold slopes, but over-softened monster scaling on deeper levels (`level16/33 monster high_vs_low_pct` dropping toward `+10..+22`), so these formulas were not accepted as baseline.
- Pass7 combined pass (runs=3) improved economy/food shape but still left deep-floor monster pressure weak (`level16/33 monster high_vs_low_pct` around `+11..+23`).
- Pass8 structural tuning applied overflow bonuses on explicit gen-byte maps (entity roll bonus + forced monster/gold/loot additions under `>4p` overflow), which restored deep-floor monster pressure without regressing 1-4p baselines.
- Pass8 retains deterministic 1-4p parity with pass5 baseline in the matrix lane (row-for-row identical for `rooms/monsters/gold/items/decorations/food_servings` at players `1..4`).
- Pass10 aggregate `p4 -> p15` mean deltas: rooms `+63.7%`, monsters `+33.8%`, gold `+110.9%`, items `+116.4%`, food servings `+27.3%`, decorations `+131.3%`; monster density fell from `1.059` to `0.865` monsters/room, reducing high-party clumping risk while preserving positive monster slope.
- Remaining pass10 economy gap is per-player availability: `gold/player` drops from `5.917` (`4p`) to `3.328` (`15p`), `items/player` from `3.562` to `2.056`, and `food/player` from `1.979` to `0.672`.
- Mapgen sweep stability is now hardened against host death stalls during long same-level reload lanes (`BARONY_SMOKE_MAPGEN_PREVENT_DEATH`, default-on when `BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL=1`), with pass10 level-33 lane completing all 45 samples without takeover.
- Same-level mapgen sampling now has explicit validation guardrails: procedural reload regeneration is seed-verified, and non-procedural floors fail fast with a clear wait reason instead of timing out.
- Smoke mapgen harness now supports dynamic runtime player override control (`BARONY_SMOKE_MAPGEN_CONTROL_FILE`) so simulated sweeps can step player count in a single process without relaunching between player-count lanes.
- Matrix reporting now includes cross-level aggregate outputs (`mapgen_level_overall.csv`, `mapgen_level_overall.md`, and HTML aggregate section) in addition to per-level trends.
- Mapgen CSV/report telemetry now includes observed generation seeds and food availability (`mapgen_seed_observed`, `food_items`, `food_servings`) plus explicit regeneration-diversity rates (`observed_seed_unique_rate_pct`, `reload_unique_seed_rate_pct`).
- Known intermittent: 8p churn rejoin retries (`error code 16`) can occur transiently and then recover.
- Extended balancing playbook is now captured in `/Users/sayhiben/dev/Barony-8p/docs/extended-multiplayer-balancing-and-tuning-plan.md` (process, tunables, commands, ratio targets, and gameplay rationale).
- Sweep confidence policy: keep `runs-per-player=3` for fast directional iteration, but require `runs-per-player=5` for volatility/gating and promotion baselines.
- Latest integration commit on `8p-mod`: `f4da9ee9` (`mapgen: add pass10 balancing docs and sweep stability telemetry`).

## 2. Open Checklist
- [ ] Re-run post-tuning full-lobby calibration (`--simulate-mapgen-players 0`) to confirm mapgen tuning under real join/load timing.
- [x] Add same-level reload mapgen verification + fail-fast diagnostics for non-procedural floors (`MAPGEN_WAIT_REASON`, reload/generation seed evidence in summaries/CSV).
- [x] Add single-runtime simulated player-count sweep mode with observed override tracing (`mapgen_players_observed`) to reduce relaunch cost during balancing.
- [x] Add cross-level matrix aggregate summary outputs for balancing diagnostics.
- [x] Add observed-seed + food telemetry to mapgen sweep outputs and aggregate reporting (`mapgen_seed_observed`, `food_items`, `food_servings`, regeneration-diversity rates).
- [x] Add volatility-aware balancing sweep policy (`runs=3` fast iteration, `runs=5` gating/promotion).
- [x] Harden same-level mapgen sweep stability against host-death stalls in long reload lanes (smoke-only survival guard).
- [ ] Rebalance post-pass10 economy/loot/food per-player pacing for `>4p` (totals are positive, but per-player availability still drops noticeably at high party counts).
- [x] Re-tune post-pass6b monster pressure for deeper levels (`16/33`) while preserving reduced 5p clumping and reduced food inflation (validated in pass8 simulated matrix).
- [ ] Investigate intermittent churn rejoin retries (`error code 16`) and reduce/resolve transient slot-lock windows.
- [ ] Add targeted overflow pacing lane for hunger/appraisal at `3p/4p/5p/8p/12p/15p`.
- [ ] Validate Steam backend multi-account join handshake (requires separate Steam account/session or another machine).
- [ ] Validate EOS backend handshake lane.
- [ ] Close remaining PR-940 automation gap: visual slot mapping lane (normal + colorblind).

## 3. Completed Gates (Condensed)
- [x] Phase A correctness gate (4p/8p/15p + payload edges + legacy + transition/mapgen).
- [x] Phase B adversarial gate (strict matrix, expected fail-modes).
- [x] Phase C stability gate (soak + churn, including ready-sync assertions).
- [x] 15-player cap and nibble/owner-encoding hardening with compile-time guardrails.
- [x] Save/reload owner-encoding compatibility sweep (`players_connected=1..15` + legacy fixtures).
- [x] Lobby regression lanes: kick-target matrix, slot-lock/copy matrix, page navigation sweep.
- [x] Runtime slot-safety lane: remote combat invalid-slot bounds.
- [x] Local legacy lanes: splitscreen baseline and `/splitscreen > 4` cap clamp.
- [x] Smoke compile/runtime gating (`BARONY_SMOKE_TESTS`) and datadir launch-path support.
- [x] Steam handshake lane (host room-key validation) with same-account local limitation captured.
- [x] PR 940 style/contribution compliance pass for touched C++ changes.

## 4. Canonical Evidence Artifacts
- LAN/adversarial:
  - `tests/smoke/artifacts/cap15-baseline-p15-escalated`
  - `tests/smoke/artifacts/helo-adversarial-20260209-172722`
- Stability:
  - `tests/smoke/artifacts/soak-20260209-185835-p8-n20`
  - `tests/smoke/artifacts/churn-ready-sync-20260209-212709-p8-c3x2-r2`
- Retry diagnostics:
  - `tests/smoke/artifacts/churn-ready-sync-20260209-224150-p8-c3x2-postdatadir`
  - `tests/smoke/artifacts/churn-ready-sync-20260209-225142-p8-c2x2-joinreject-trace`
- Mapgen/scaling:
  - `tests/smoke/artifacts/mapgen-sim-balance-revisit-20260210-r4`
  - `tests/smoke/artifacts/mapgen-full-v2-complete` (pre-tuning full-lobby reference)
  - `tests/smoke/artifacts/reload-static-floor-fastfail-20260211` (non-procedural floor fast-fail repro)
  - `tests/smoke/artifacts/reload-procedural-regen-verify-20260211` (procedural floor reload regeneration verification)
  - `tests/smoke/artifacts/mapgen-sweep-static-fastfail-smoke-20260211` (sweep-level fast-fail proof)
  - `tests/smoke/artifacts/mapgen-sweep-procedural-regen3-smoke-20260211` (batch sweep with reload regeneration evidence)
  - `tests/smoke/artifacts/mapgen-level-matrix-floor16-fixcheck-20260211` (matrix floor selection fix check, level-match 100%)
  - `tests/smoke/artifacts/mapgen-sweep-level16-tuned2-20260211-123239` (pass2 level-16 runs=2 validation after overflow tuning update)
  - `tests/smoke/artifacts/mapgen-level-matrix-procedural-tuned-pass2-runs1-20260211-124032` (pass2 full procedural matrix snapshot)
  - `tests/smoke/artifacts/mapgen-sweep-level7-pass2-runs2-20260211-130609` (pass2 level-7 runs=2 confirmation; gold weak-positive)
  - `tests/smoke/artifacts/mapgen-sweep-level7-pass3-runs2-20260211-131456` (pass3 level-7 runs=2 confirmation; gold strengthened)
  - `tests/smoke/artifacts/mapgen-sweep-level16-pass3-runs2-20260211-132303` (pass3 level-16 runs=2 confirmation; gold/items strengthened)
  - `tests/smoke/artifacts/mapgen-level-matrix-procedural-tuned-pass3-runs1-20260211-133108` (latest pass3 full procedural matrix snapshot)
  - `tests/smoke/artifacts/mapgen-sweep-single-runtime-smoketest-20260211-161629` (single-runtime simulated player sweep proof, observed override mapping)
  - `tests/smoke/artifacts/mapgen-level-matrix-single-runtime-smoketest-20260211-161845-fix` (matrix fast-path proof, cross-level aggregate outputs enabled)
  - `tests/smoke/artifacts/mapgen-level-matrix-fast-balance-pass4b-20260211-164339` (pass4 full matrix smoke with seed/food telemetry and regeneration-diversity reporting)
  - `tests/smoke/artifacts/mapgen-levels1-16-roomfinal-pass4d-runs2-20260211-165800` (pass4 targeted runs=2 confirmation for room/economy on levels 1 and 16)
  - `tests/smoke/artifacts/mapgen-level-matrix-final-pass4d-runs1-20260211-170235` (latest pass4 full 4-level snapshot on current formulas)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass5-econtrim-runs3-20260211-171330` (current pass5 full 4-level runs=3 baseline; regeneration and level-target rates all 100%)
  - `tests/smoke/artifacts/mapgen-levels1-16-pass5b-econup-runs3-20260211-172559` (targeted economy bump A/B on levels 1+16; mixed deep-floor outcome, reverted)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass6-targeted-runs3-20260211-173807` (pass6 full runs=3: strong food inflation correction + higher gold/item trend, but monster scaling softened too much)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass6b-targeted-runs3-20260211-175024` (pass6b follow-up runs=3: stronger high-party gold with moderated food inflation; deep-floor monster slope still weak, needs another tuning pass)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass7-combined-runs3-20260211-183626` (combined one-pass reroll/monster/gold retune; economy and food remained improved, but deep-floor monster pressure still under target)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass8-structural-runs3-20260211-185102` (overflow-on-explicit-range structural fix; deep-floor monster pressure recovered, regeneration diagnostics remain 100% match/unique, 1-4p parity preserved)
  - `tests/smoke/artifacts/mapgen-survival-verify-20260211-195012` (targeted level-33 verification of smoke survival guard + regeneration checks)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass10-survival-guard-runs3-20260211-195102` (latest full runs=3 matrix with decoration subtype telemetry and death-stall hardening; all levels pass with regeneration diagnostics at 100%)
- Steam:
  - `tests/smoke/artifacts/steam-handshake-lane-20260210-205340-p1`
  - `tests/smoke/artifacts/steam-handshake-lane-20260210-205400-p2-local-limit`
- Save/reload:
  - `tests/smoke/artifacts/save-reload-compat-pipeline`

## 5. Next-Run Commands

### 5.1 Post-tuning full-lobby calibration
```bash
tests/smoke/run_mapgen_sweep_mac.sh \
  --min-players 1 --max-players 15 --runs-per-player 5 \
  --simulate-mapgen-players 0 --auto-enter-dungeon 1 \
  --outdir "tests/smoke/artifacts/mapgen-full-posttune-$(date +%Y%m%d-%H%M%S)"
```

### 5.2 Churn retry investigation (error code 16)
```bash
tests/smoke/run_lan_join_leave_churn_smoke_mac.sh \
  --instances 8 --churn-cycles 3 --churn-count 2 \
  --force-chunk 1 --chunk-payload-max 200 \
  --auto-ready 1 --trace-ready-sync 1 --require-ready-sync 1 \
  --trace-join-rejects 1 \
  --outdir "tests/smoke/artifacts/churn-retry-investigation-$(date +%Y%m%d-%H%M%S)"
```

### 5.3 Same-level mapgen regeneration sanity lane (procedural floor)
```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --instances 1 --auto-start 1 --auto-start-delay 0 \
  --auto-enter-dungeon 1 --auto-enter-dungeon-delay 3 \
  --mapgen-samples 3 --require-mapgen 1 \
  --mapgen-reload-same-level 1 --mapgen-reload-seed-base 100100 \
  --start-floor 1 \
  --outdir "tests/smoke/artifacts/reload-procedural-verify-$(date +%Y%m%d-%H%M%S)"
```

### 5.4 Steam multi-account handshake confirmation
```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --network-backend steam --instances 2 \
  --force-chunk 1 --chunk-payload-max 200 --timeout 360 \
  --outdir "tests/smoke/artifacts/steam-handshake-multiacct-$(date +%Y%m%d-%H%M%S)"
```

### 5.5 EOS handshake lane
```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --network-backend eos --instances 2 \
  --force-chunk 1 --chunk-payload-max 200 --timeout 360 \
  --outdir "tests/smoke/artifacts/eos-handshake-$(date +%Y%m%d-%H%M%S)"
```

### 5.6 Volatility-gating simulated mapgen matrix (single-runtime player sweeps)
```bash
tests/smoke/run_mapgen_level_matrix_mac.sh \
  --levels 1,7,16,33 \
  --min-players 1 --max-players 15 --runs-per-player 5 \
  --simulate-mapgen-players 1 \
  --inprocess-sim-batch 1 --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --outdir "tests/smoke/artifacts/mapgen-level-matrix-fast-$(date +%Y%m%d-%H%M%S)"
```

## 6. Build/Runtime Preconditions
- Build smoke-enabled binaries for validation lanes:
```bash
cmake -S . -B build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build build-mac-smoke -j8 --target barony
```
- Prefer local build binary + Steam asset datadir (`--app ... --datadir ...`) instead of replacing the Steam executable.
- For mapgen balancing, prefer procedural floors (for example `1`, `16`, `25`, `33`); fixed/story floors can intentionally fail fast with `MAPGEN_WAIT_REASON=reload-complete-no-mapgen-samples`.
- Keep smoke hooks isolated to `src/smoke/SmokeTestHooks.cpp` and `src/smoke/SmokeTestHooks.hpp` with minimal call sites.

## 7. Cache Hygiene
After long runs, prune cache bloat while preserving logs/artifacts:
```bash
find tests/smoke/artifacts -type f -name models.cache -delete
```

## 8. Exit Criteria
- All open checklist items in Section 2 are complete.
- Post-tuning full-lobby mapgen run is green and consistent with simulated trend improvements.
- Churn retry behavior is either resolved or bounded with clear acceptance thresholds.
- Steam multi-account and EOS handshake lanes are validated and documented.
