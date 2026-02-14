# Multiplayer Expansion Verification Plan (PR 940, Near-Finish)

Last updated: February 12, 2026  
Target: `MAXPLAYERS=15`

## 1. Status Snapshot
- Overall status: mostly complete.
- Core networking validation is green: LAN HELO correctness, adversarial strict fail-modes, soak/churn, and high-slot regression lanes.
- Steam backend handshake is validated for host-room creation/key capture, but local same-account multi-instance joins are limited by Steam account/session rules.
- EOS backend handshake coverage is still pending.
- Scaling work has advanced through pass11d volatility validation (`levels=1/7/16/33`, `players=1..15`, `runs=5`) with all 300 matrix rows passing after harness timeout fixes.
- Current balancing stage is `Pass15h12 pre-promotion convergence`: fast integration volatility lanes are near target, and a fresh full-lobby confirmation on the new candidate is pending completion.
- Regeneration/level-target diagnostics are stable in the runs=3 matrix (`target_level_match_rate_pct=100`, `observed_seed_unique_rate_pct=100`, `reload_unique_seed_rate_pct=100`), confirming that same-level reload sweeps regenerate unique maps rather than reusing a prior map.
- Targeted economy bump experiment on `levels=1,16` (runs=3) produced mixed deep-floor outcomes and was reverted; pass8 is now the current simulated tuning baseline while preserving pass5 behavior for `1..4p`.
- Follow-up pass6/pass6b overflow experiments (runs=3 full matrix) successfully reduced extreme food inflation and raised high-party gold slopes, but over-softened monster scaling on deeper levels (`level16/33 monster high_vs_low_pct` dropping toward `+10..+22`), so these formulas were not accepted as baseline.
- Pass7 combined pass (runs=3) improved economy/food shape but still left deep-floor monster pressure weak (`level16/33 monster high_vs_low_pct` around `+11..+23`).
- Pass8 structural tuning applied overflow bonuses on explicit gen-byte maps (entity roll bonus + forced monster/gold/loot additions under `>4p` overflow), which restored deep-floor monster pressure without regressing 1-4p baselines.
- Pass8 retains deterministic 1-4p parity with pass5 baseline in the matrix lane (row-for-row identical for `rooms/monsters/gold/items/decorations/food_servings` at players `1..4`).
- Pass10 aggregate `p4 -> p15` mean deltas: rooms `+63.7%`, monsters `+33.8%`, gold `+110.9%`, items `+116.4%`, food servings `+27.3%`, decorations `+131.3%`; monster density fell from `1.059` to `0.865` monsters/room, reducing high-party clumping risk while preserving positive monster slope.
- Remaining pass10 economy gap is per-player availability: `gold/player` drops from `5.917` (`4p`) to `3.328` (`15p`), `items/player` from `3.562` to `2.056`, and `food/player` from `1.979` to `0.672`.
- Mapgen sweep stability is now hardened against host death stalls during long same-level reload lanes (`BARONY_SMOKE_MAPGEN_PREVENT_DEATH`, default-on when `BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL=1`), with pass10 level-33 lane completing all 45 samples without takeover.
- Long single-runtime player sweeps are now hardened against silent timeout truncation: mapgen sweep auto-bumps timeout for large sample counts, and timeout exits now surface `MAPGEN_WAIT_REASON=timeout-before-mapgen-samples`.
- Same-level mapgen sampling now has explicit validation guardrails: procedural reload regeneration is seed-verified, and non-procedural floors fail fast with a clear wait reason instead of timing out.
- Smoke mapgen harness now supports dynamic runtime player override control (`BARONY_SMOKE_MAPGEN_CONTROL_FILE`) so simulated sweeps can step player count in a single process without relaunching between player-count lanes.
- Matrix reporting now includes cross-level aggregate outputs (`mapgen_level_overall.csv`, `mapgen_level_overall.md`, and HTML aggregate section) in addition to per-level trends.
- Mapgen CSV/report telemetry now includes observed generation seeds and food availability (`mapgen_seed_observed`, `food_items`, `food_servings`) plus explicit regeneration-diversity rates (`observed_seed_unique_rate_pct`, `reload_unique_seed_rate_pct`).
- Current pass11d `runs=5` aggregate (`p15` vs `p4`): rooms `1.545x`, monsters `1.382x`, gold `2.171x`, items `2.473x`, food `2.873x`, decorations `2.286x`; per-player ratios are gold `0.579x`, items `0.659x`, food `0.766x`.
- Pass12a exploratory retune was run and rejected (`runs=2`) due regressions (rooms overshoot, monster under-scaling, food per-player overshoot), then reverted.
- Pass12b/12c/12d follow-up economy iterations were also evaluated; pass12c was advanced to `runs=5` and then rejected due volatility-gate regressions.
- Latest pass12c `runs=5` aggregate (`p15` vs `p4`): rooms `1.545x`, monsters `1.239x`, gold `2.430x`, items `2.541x`, food `1.758x`, decorations `2.186x`; per-player ratios are gold `0.648x`, items `0.678x`, food `0.469x`.
- Pass13 narrow slot-capacity/economy candidate (`runs=2`) was evaluated and rejected:
  - Artifact: `tests/smoke/artifacts/mapgen-level-matrix-pass13-slot-econ-runs2-20260211-230136`
  - `p15` vs `p4`: rooms `1.527x`, monsters `1.576x`, gold `2.381x`, items `2.557x`, food `2.444x`, decorations `2.346x`; per-player ratios gold `0.635x`, items `0.682x`, food `0.652x`.
  - Regression reason: monster density overshot target (`1.033x` of p4 vs target `0.82x-0.92x`) while economy/player still missed target floors.
- Mapgen value telemetry is now wired into smoke outputs for economy-focused tuning:
  - New fields: `gold_bags`, `gold_amount`, `item_stacks`, `item_units`.
  - Flow validated through single-runtime matrix smoke:
    - `tests/smoke/artifacts/mapgen-value-telemetry-matrix-smoke-20260211-232404`
- Pass14 value-lane tuning sequence is complete (`runs=2` exploration + `runs=5` gate):
  - Baseline value capture: `tests/smoke/artifacts/mapgen-level-matrix-pass14-baseline-value-runs2-20260211-233124`
    - `gold_amount/player` (`p15` vs `p4`): `3.462x` (clear runaway value scaling).
  - Candidate sequence:
    - `tests/smoke/artifacts/mapgen-level-matrix-pass14a-value-goldscope-runs2-20260211-234139`
    - `tests/smoke/artifacts/mapgen-level-matrix-pass14b-value-goldcompress-runs2-20260211-235029`
    - `tests/smoke/artifacts/mapgen-level-matrix-pass14c-value-goldcompress-deterministic-runs2-20260211-235840`
    - `tests/smoke/artifacts/mapgen-level-matrix-pass14c-value-goldcompress-deterministic-runs5-20260212-000635`
  - Pass14c runs=2 preserved non-value metrics and reduced `gold_amount/player` to `0.737x`.
  - Pass14c runs=5 (300/300 pass rows) aggregate `p15` vs `p4`:
    - totals: rooms `1.545x`, monsters `1.382x`, gold `2.171x`, items `2.473x`, food `3.315x`, decorations `2.286x`
    - per-player: gold `0.579x`, items `0.659x`, food `0.884x`, gold value (`gold_amount/player`) `0.896x`
    - depth spread caveat: `gold_amount/player` by level remained uneven (`L1 0.673x`, `L7 1.247x`, `L16 2.007x`, `L33 0.687x`).
- Pass15a depth-normalized generated-gold value smoothing is now implemented and validated in fast in-process integration lanes:
  - Artifacts:
    - `tests/smoke/artifacts/mapgen-integration-pass15a-runs2-20260212-120325`
    - `tests/smoke/artifacts/mapgen-integration-pass15a-runs5-20260212-120325`
  - Coverage:
    - runs=2: `120/120` pass rows (`levels=1,7,16,33`, `players=1..15`).
    - runs=5: `300/300` pass rows (`levels=1,7,16,33`, `players=1..15`).
  - pass15a runs=5 preserved non-value aggregate shape (`rooms/monsters/gold/items/food/decorations`) while materially reducing depth-band value volatility.
  - `gold_amount/player` (`p15` vs `p4`) depth spread moved from pass14c (`L1 0.673x`, `L7 1.247x`, `L16 2.007x`, `L33 0.687x`) to pass15a (`L1 0.726x`, `L7 1.056x`, `L16 1.282x`, `L33 0.744x`).
  - aggregate `gold_amount/player` (`p15` vs `p4`) is now `0.838x` in pass15a runs=5 integration.
- Pass15b-pass15f follow-up fast integration iterations (`runs=2` probes + `runs=5` gates) were evaluated and used to converge on a post-pass15a candidate:
  - pass15b/pass15c over-expanded rooms (`1.800x`) and reduced monster density (`0.703x-0.715x`).
  - pass15d restored room shape (`1.545x`) but monsters remained below target (`1.303x`).
  - pass15e restored monster total (`1.403x`) but worsened deep-floor value spread (`L16 gold_amount/player 1.583x`).
  - pass15f brought count-lane metrics close to target (`gold/player 0.688x`, `items/player 0.699x`, `food/player 0.723x`, monsters `1.387x`) but over-shot value (`gold_amount/player 1.070x`, `L7/L16 1.467x/1.612x`).
- Pass15g is the selected pre-promotion candidate (pass15f count shape plus value-only depth-band correction):
  - Artifacts:
    - `tests/smoke/artifacts/mapgen-integration-pass15g-runs2-20260212-122229`
    - `tests/smoke/artifacts/mapgen-integration-pass15g-runs5-20260212-122229`
  - runs=5 (`300/300` pass rows) `p15` vs `p4`:
    - rooms `1.545x`, monsters `1.387x`, monsters/room `0.897x`, gold/player `0.688x`, items/player `0.699x`, food/player `0.723x`, decorations `2.100x`, blocking-share `19.0%`
    - `gold_amount/player` improved vs pass15f from `1.070x` to `1.021x`; depth spread also improved at `L7/L16` from `1.467x/1.612x` to `1.274x/1.405x`.
- Low-player guard status after pass15g:
  - Focused integration lane: `tests/smoke/artifacts/mapgen-integration-pass15g-runs5-p2to4-20260212-122642` (`players=2..4`, `runs=5`, `60/60` pass rows).
  - Row-level full-run parity check confirms no `1..4p` regressions between pass15a and pass15g (`diffs=0`, `missing=0`).
- Full-lobby confirmation on pass15g is complete:
  - Artifact: `tests/smoke/artifacts/mapgen-full-posttune-pass15g-20260212-132721`
  - Coverage: level `1`, players `1..15`, `runs=5`, `75/75` pass rows; `mapgen_wait_reason=none` on all rows.
  - Full-lobby `p15 vs p4`: rooms `1.530x`, monsters `1.595x`, monsters/room `1.042x`, gold/player `0.610x`, items/player `0.714x`, food/player `0.610x`, decorations `3.545x`, blocking-share `46.2%`, `gold_amount/player=1.544x`.
  - Comparison against pass15g integration level-1 trend shows alignment on rooms/items but divergence on monsters/room, decorations, food/player, and `gold_amount/player`; promotion is therefore held pending retune.
- Pass15h fast-lane retune iterations (`h8..h12`) completed using unpinned high-sample integration volatility aggregates (`4` invocations x `runs=10`, `2400` rows each):
  - `tests/smoke/artifacts/mapgen-integration-pass15h8-volatility-agg-20260212-172054` (room/value overshoot)
  - `tests/smoke/artifacts/mapgen-integration-pass15h9-volatility-agg-20260212-172508` (room under-run / density overcorrection)
  - `tests/smoke/artifacts/mapgen-integration-pass15h10-volatility-agg-20260212-172916` (global shape improved, level-1 density still high)
  - `tests/smoke/artifacts/mapgen-integration-pass15h11-volatility-agg-20260212-173303` (level-1 shape improved, item floor still weak)
  - `tests/smoke/artifacts/mapgen-integration-pass15h12-volatility-agg-20260212-173635` (current candidate)
    - `p15 vs p4`: rooms `1.721x`, monsters `1.415x`, monsters/room `0.813x`, gold/player `0.733x`, items/player `0.734x`, food/player `0.766x`, decorations `1.812x`, `gold_amount/player=0.923x`.
    - level-1 `p15 vs p4`: rooms `1.582x`, monsters/room `0.889x`, decorations `1.925x`, `gold_amount/player=1.111x`.
- Pass15h12 full-lobby confirmation lane was started and intentionally interrupted after early health checks to continue fast-loop tuning:
  - Partial artifact: `tests/smoke/artifacts/mapgen-full-posttune-pass15h12-20260212-173948`
  - Completed rows before stop: `8/75`, all `pass`, all `mapgen_wait_reason=none`.
- Smoke-only headless integration matrix mode is now available in the game binary (`-smoke-mapgen-integration` family) for fast in-process sweeps without launcher relaunch overhead.
  - Integration parser/validator/runner + CSV row synthesis are now owned by `src/smoke/SmokeTestHooks.cpp`/`src/smoke/SmokeTestHooks.hpp`; `src/game.cpp` remains a thin wiring callsite only.
  - Integration now auto-selects a per-run seed root; the temporary `-smoke-mapgen-integration-base-seed` override used during parity bring-up has been removed.
- Latest integration artifacts:
  - `tests/smoke/artifacts/mapgen-integration-matrix2-20260212-005916` (`levels=1,7,16,25,33`, `players=1..15`, `runs=2`, `150/150` pass rows).
  - `tests/smoke/artifacts/mapgen-integration-parity-refresh-20260212-023902` (integration rerun used for direct parity comparison).
  - `tests/smoke/artifacts/mapgen-method-parity-refresh-20260212-023933` (single-runtime rerun used for direct parity comparison).
- Current integration-vs-single-runtime parity state (shared procedural floors):
  - Scope: levels `1,7,16,33`, players `1..15`, `runs=1`, `base_seed=1000`.
  - Row counts: `60` vs `60`; normalized compare (excluding `run_dir`) is exact (`normalized_row_mismatch=0`, `missing_keys=0`).
  - Interpretation: integration mode is now suitable as the primary fast preflight/volatility lane for these floors; retain full-lobby (`simulate-mapgen-players=0`) lane for promotion confirmation.
- Current in-tree mapgen tuning is pass15h12 (latest overflow count/value retune candidate).
- Known intermittent: 8p churn rejoin retries (`error code 16`) can occur transiently and then recover.
- Extended balancing playbook is now captured in `/Users/sayhiben/dev/Barony-8p/docs/extended-multiplayer-balancing-and-tuning-plan.md` (process, tunables, commands, ratio targets, and gameplay rationale).
- Sweep confidence policy: keep `runs-per-player=3` for fast directional iteration, but require `runs-per-player=5` for volatility/gating and promotion baselines.
- Latest tuning branch state includes hook-owned integration plumbing plus parity refresh artifacts (`mapgen-integration-parity-refresh-20260212-023902`, `mapgen-method-parity-refresh-20260212-023933`).

## 2. Open Checklist
- [x] Re-run post-tuning full-lobby calibration (`--simulate-mapgen-players 0`) to confirm mapgen tuning under real join/load timing (`tests/smoke/artifacts/mapgen-full-posttune-pass15g-20260212-132721`, `75/75` pass rows; trend divergence documented for follow-up retune).
- [x] Add same-level reload mapgen verification + fail-fast diagnostics for non-procedural floors (`MAPGEN_WAIT_REASON`, reload/generation seed evidence in summaries/CSV).
- [x] Add single-runtime simulated player-count sweep mode with observed override tracing (`mapgen_players_observed`) to reduce relaunch cost during balancing.
- [x] Add cross-level matrix aggregate summary outputs for balancing diagnostics.
- [x] Add observed-seed + food telemetry to mapgen sweep outputs and aggregate reporting (`mapgen_seed_observed`, `food_items`, `food_servings`, regeneration-diversity rates).
- [x] Add volatility-aware balancing sweep policy (`runs=3` fast iteration, `runs=5` gating/promotion).
- [x] Harden same-level mapgen sweep stability against host-death stalls in long reload lanes (smoke-only survival guard).
- [x] Harden single-runtime mapgen player-sweep timeout behavior and emit explicit timeout wait reasons (`timeout-before-mapgen-samples`).
- [x] Add mapgen economy-value telemetry (`gold_bags`, `gold_amount`, `item_stacks`, `item_units`) to smoke summaries/CSVs/reports.
- [x] Complete pass14 value-lane tuning cycle (`runs=2` exploration + deterministic candidate + `runs=5` volatility gate).
- [x] Complete pass15a depth-normalized overflow gold-value smoothing (`runs=2` fast integration probe + `runs=5` fast integration gate).
- [x] Complete pass15b-pass15g fast integration retune sequence and select pass15g as pre-promotion candidate (`runs=2` directional + `runs=5` gating).
- [x] Re-validate low-player invariants after overflow retune (`1..4` row-level parity check plus focused `2..4` runs=5 integration lane).
- [x] Add smoke-only in-process mapgen integration lane (`-smoke-mapgen-integration`) for fast structural matrix sweeps.
- [x] Bring in-process integration metric parity in line with single-runtime reload matrix outputs on procedural floors (`1,7,16,33`) before using integration lane for balance sign-off (`tests/smoke/artifacts/mapgen-integration-parity-refresh-20260212-023902`, `tests/smoke/artifacts/mapgen-method-parity-refresh-20260212-023933`).
- [x] Move integration smoke parsing/execution logic out of `src/game.cpp` into `src/smoke/SmokeTestHooks.cpp`/`src/smoke/SmokeTestHooks.hpp`, keeping `game.cpp` wiring-only.
- [ ] Rebalance post-pass10 economy/loot/food per-player pacing for `>4p` (pass15h12 integration candidate is near-target; full-lobby runs=5 confirmation and final divergence check are still required before promotion).
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
  - `tests/smoke/artifacts/mapgen-level-matrix-pass11d-runs5-20260211-211839` (first pass11d runs=5 attempt; deterministic truncation around sample ~52 exposed single-runtime timeout limitation)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass11d-runs5-timeoutfix-20260211-213415` (pass11d runs=5 completion after timeout hardening; all levels pass, regeneration diagnostics remain 100%)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass12a-sanity-runs2-20260211-215341` (exploratory pass12a runs=2; rejected and reverted due rooms/monster/food regressions)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass12b-econfood-runs2-20260211-221343` (pass12b runs=2; items/player improved, but monsters over-scaled and gold/player remained weak)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass12c-econfood-runs2-20260211-222214` (pass12c runs=2; strongest directional candidate before volatility gate)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass12d-econfood-runs2-20260211-223048` (pass12d runs=2; room overshoot and per-player item/food regressions)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass12c-econfood-runs5-20260211-223919` (pass12c runs=5 volatility gate; rejected after monster and food under-scaling)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass13-slot-econ-runs2-20260211-230136` (pass13 runs=2; rejected due monster-density overshoot with sub-target gold/items per-player)
  - `tests/smoke/artifacts/mapgen-value-telemetry-matrix-smoke-20260211-232404` (value-telemetry lane verification: new economy fields present in summary, sweep CSV, matrix CSV, and reports)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass14-baseline-value-runs2-20260211-233124` (pass14 baseline value read with new telemetry; identified runaway gold_amount/player)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass14a-value-goldscope-runs2-20260211-234139` (pass14a: scoped overflow bonus to ambient generated bags only)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass14b-value-goldcompress-runs2-20260211-235029` (pass14b: aggressive compression candidate; rejected due extra RNG perturbation)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass14c-value-goldcompress-deterministic-runs2-20260211-235840` (pass14c deterministic candidate; non-value lanes unchanged, gold_amount/player normalized in runs=2)
  - `tests/smoke/artifacts/mapgen-level-matrix-pass14c-value-goldcompress-deterministic-runs5-20260212-000635` (pass14c volatility gate; 300/300 pass rows, depth-band value variance still present)
  - `tests/smoke/artifacts/mapgen-integration-pass15a-runs2-20260212-120325` (pass15a fast integration probe, 120/120 pass rows)
  - `tests/smoke/artifacts/mapgen-integration-pass15a-runs5-20260212-120325` (pass15a fast integration volatility gate, 300/300 pass rows, depth-band value variance reduced)
  - `tests/smoke/artifacts/mapgen-integration-pass15b-runs2-20260212-121348` (pass15b exploratory count/value retune, runs=2)
  - `tests/smoke/artifacts/mapgen-integration-pass15b-runs5-20260212-121348` (pass15b volatility gate; rejected due room overshoot + monster density under-run)
  - `tests/smoke/artifacts/mapgen-integration-pass15c-runs2-20260212-121611` (pass15c exploratory adjustment, runs=2)
  - `tests/smoke/artifacts/mapgen-integration-pass15c-runs5-20260212-121611` (pass15c volatility gate; rejected due room overshoot + monster density under-run)
  - `tests/smoke/artifacts/mapgen-integration-pass15d-runs2-20260212-121759` (pass15d exploratory adjustment, runs=2)
  - `tests/smoke/artifacts/mapgen-integration-pass15d-runs5-20260212-121759` (pass15d volatility gate; rooms restored, monster total still under target)
  - `tests/smoke/artifacts/mapgen-integration-pass15e-runs2-20260212-121937` (pass15e exploratory adjustment, runs=2)
  - `tests/smoke/artifacts/mapgen-integration-pass15e-runs5-20260212-121938` (pass15e volatility gate; monster total restored, deep-floor value spread worsened)
  - `tests/smoke/artifacts/mapgen-integration-pass15f-runs2-20260212-122112` (pass15f exploratory near-target count lane, runs=2)
  - `tests/smoke/artifacts/mapgen-integration-pass15f-runs5-20260212-122112` (pass15f volatility gate; value overshoot)
  - `tests/smoke/artifacts/mapgen-integration-pass15g-runs2-20260212-122229` (pass15g candidate probe; value-only correction on pass15f count lane)
  - `tests/smoke/artifacts/mapgen-integration-pass15g-runs5-20260212-122229` (pass15g candidate volatility gate; 300/300 pass rows)
  - `tests/smoke/artifacts/mapgen-integration-pass15g-runs5-p2to4-20260212-122642` (focused low-player guard lane, `players=2..4`, 60/60 pass rows)
  - `tests/smoke/artifacts/mapgen-full-posttune-pass15g-20260212-132721` (full-lobby pass15g confirmation, `players=1..15`, `runs=5`, `75/75` pass rows; level-1 trend divergence evidence)
  - `tests/smoke/artifacts/mapgen-integration-pass15h8-volatility-agg-20260212-172054` (h8 unpinned volatility aggregate, `2400/2400` pass rows; room/value overshoot)
  - `tests/smoke/artifacts/mapgen-integration-pass15h9-volatility-agg-20260212-172508` (h9 unpinned volatility aggregate, `2400/2400` pass rows; room under-run / density overcorrection)
  - `tests/smoke/artifacts/mapgen-integration-pass15h10-volatility-agg-20260212-172916` (h10 unpinned volatility aggregate, `2400/2400` pass rows; global shape improved, level-1 density still elevated)
  - `tests/smoke/artifacts/mapgen-integration-pass15h11-volatility-agg-20260212-173303` (h11 unpinned volatility aggregate, `2400/2400` pass rows; item floor still weak)
  - `tests/smoke/artifacts/mapgen-integration-pass15h12-volatility-agg-20260212-173635` (h12 unpinned volatility aggregate, `2400/2400` pass rows; current near-target candidate)
  - `tests/smoke/artifacts/mapgen-full-posttune-pass15h12-20260212-173948` (partial full-lobby pass15h12 confirmation lane, intentionally interrupted after `8/75` clean samples to continue tuning loop)
  - `tests/smoke/artifacts/mapgen-integration-matrix2-20260212-005916` (headless in-process integration matrix; structural lane pass and level-25 summary-token hardening)
  - `tests/smoke/artifacts/mapgen-integration-parity-refresh-20260212-023902` (integration parity refresh lane, levels `1/7/16/33`, players `1..15`, runs `1`)
  - `tests/smoke/artifacts/mapgen-method-parity-refresh-20260212-023933` (single-runtime parity refresh lane paired with integration artifact above)
- Steam:
  - `tests/smoke/artifacts/steam-handshake-lane-20260210-205340-p1`
  - `tests/smoke/artifacts/steam-handshake-lane-20260210-205400-p2-local-limit`
- Save/reload:
  - `tests/smoke/artifacts/save-reload-compat-pipeline`

## 5. Next-Run Commands

### 5.1 Post-retune full-lobby recalibration
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

### 5.6 Volatility-gating integration matrix (preferred fast lane)
```bash
USER_HOME="$HOME"
OUT="tests/smoke/artifacts/mapgen-integration-runs5-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT/home"
HOME="$OUT/home" build-mac-smoke/barony.app/Contents/MacOS/barony \
  -windowed -size=1280x720 -nosound \
  -datadir="$USER_HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -smoke-mapgen-integration \
  -smoke-mapgen-integration-csv="$OUT/mapgen_level_matrix.csv" \
  -smoke-mapgen-integration-levels=1,7,16,33 \
  -smoke-mapgen-integration-min-players=1 \
  -smoke-mapgen-integration-max-players=15 \
  -smoke-mapgen-integration-runs=5
```

### 5.7 Fast in-process integration preflight (structural lane)
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

### 5.8 Low-player parity guard lane (`2..4p`)
```bash
USER_HOME="$HOME"
OUT="tests/smoke/artifacts/mapgen-integration-p2to4-runs5-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT/home"
HOME="$OUT/home" build-mac-smoke/barony.app/Contents/MacOS/barony \
  -windowed -size=1280x720 -nosound \
  -datadir="$USER_HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -smoke-mapgen-integration \
  -smoke-mapgen-integration-csv="$OUT/mapgen_level_matrix.csv" \
  -smoke-mapgen-integration-levels=1,7,16,33 \
  -smoke-mapgen-integration-min-players=2 \
  -smoke-mapgen-integration-max-players=4 \
  -smoke-mapgen-integration-runs=5
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
- Keep integration smoke logic hook-local (`src/smoke/SmokeTestHooks.*`); do not reintroduce parser/runner utility logic into `src/game.cpp`.

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
