# Extended Multiplayer Mapgen Balancing and Tuning Plan (5p-15p, 1p-4p Locked)

## Summary
This plan defines a repeatable tuning workflow for multiplayer map generation from 5 to 15 players while preserving current 1-4 player balance exactly.

Baseline artifact for current state:
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass10-survival-guard-runs3-20260211-195102`

Primary conclusions from baseline:
- Total rewards scale strongly at high player counts.
- Per-player rewards still fall too far by 15p.
- Rooms and total monsters are broadly in a reasonable range.
- Decoration totals are acceptable, but blocking-share should be controlled.
- Map regeneration validity is healthy (100% target-level and unique-seed rates).

## Emphatic Constraint (Do Not Regress Legacy Balance)
1. Do not change gameplay balance for 1-4 players.
2. Any new balancing logic must be gated to overflow players only (`connectedPlayers > 4`).
3. Keep 1-4 loot/monster legacy divisor behavior unchanged.
4. Keep non-overflow food behavior unchanged.
5. Maintain splitscreen cap behavior at 4 players.

## Current Stage
- Stage: `Pass15h12 fast-lane convergence` (integration volatility aggregates are near-target; final full-lobby promotion confirmation is still pending).
- Next stage: `Pass15h12 full-lobby confirmation + parity re-gate` (complete `simulate-mapgen-players=0` runs=5 confirmation, then decide promote/retune).

## Latest Iteration Notes (February 12, 2026)
- Fixed single-runtime sweep harness timeout behavior for long `runs=5` lanes:
  - `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh` now auto-bumps timeout in single-runtime player-sweep mode based on sample count.
  - `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh` now emits `MAPGEN_WAIT_REASON=timeout-before-mapgen-samples` when a run exits on timeout before required mapgen samples.
- Pass11d volatility matrix now completes successfully after timeout fix:
  - Artifact: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass11d-runs5-timeoutfix-20260211-213415`
  - Coverage: levels `1,7,16,33`, players `1..15`, `runs=5`, all 300 rows passing.
  - `p15 vs p4` totals: rooms `1.545x`, monsters `1.382x`, gold `2.171x`, items `2.473x`, food `2.873x`, decorations `2.286x`.
  - `p15 vs p4` per-player: gold `0.579x`, items `0.659x`, food `0.766x`.
  - Blocking-share at `p15`: `23.7%`.
- Pass12a exploratory retune (overflow rooms/economy) was rejected and reverted:
  - Artifact: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass12a-sanity-runs2-20260211-215341`
  - Regressions: rooms overshoot (`1.832x`), monsters undershoot (`1.299x`), and food per-player overshoot (`0.902x`).
- Pass12b/12c/12d follow-up economy passes were evaluated, then rejected for promotion:
  - Artifacts:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass12b-econfood-runs2-20260211-221343`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass12c-econfood-runs2-20260211-222214`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass12d-econfood-runs2-20260211-223048`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass12c-econfood-runs5-20260211-223919`
  - Key findings:
    - Pass12b (`runs=2`) over-scaled monsters (`1.569x`) while still missing gold/player uplift.
    - Pass12c (`runs=2`) looked directionally good (`gold/player 0.665x`, `items/player 0.717x`) but failed volatility gate at `runs=5`.
    - Pass12c (`runs=5`) regressed to under-scaled monsters (`1.239x`) and under-scaled food/player (`0.469x`), despite better gold/player (`0.648x`) and items/player (`0.678x`).
    - Pass12d (`runs=2`) over-scaled rooms (`1.832x`) and under-scaled items/player (`0.626x`) and food/player (`0.618x`).
- Pass13 narrow slot-capacity/economy pass (`runs=2`) was also rejected:
  - Artifact: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass13-slot-econ-runs2-20260211-230136`
  - Result shape: `p15 vs p4` totals rooms `1.527x`, monsters `1.576x`, gold `2.381x`, items `2.557x`, food `2.444x`, decorations `2.346x`.
  - Per-player at `p15`: gold `0.635x`, items `0.682x`, food `0.652x`.
  - Primary regression: monster density moved from pass11d `0.894x` to `1.033x` (above target band), while gold/items per-player still remained below target.
- Value-telemetry lane is now implemented end-to-end for mapgen runs:
  - `src/maps.cpp` now emits `mapgen value summary` with `gold_bags`, `gold_amount`, `item_stacks`, `item_units`.
  - Smoke parsers/CSVs/reports now carry those fields through:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_level_matrix_mac.sh`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/generate_mapgen_heatmap.py`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/generate_smoke_aggregate_report.py`
  - Validation artifact:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-value-telemetry-matrix-smoke-20260211-232404`
- Pass14 value-tuning sequence was run end-to-end:
  - Baseline value capture (`runs=2`):
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass14-baseline-value-runs2-20260211-233124`
    - `gold_amount/player` at `p15` vs `p4`: `3.462x` (severe overshoot), while count lanes stayed near prior pass11d behavior.
  - Pass14a (scope value bonus to ambient generated bags only, `runs=2`):
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass14a-value-goldscope-runs2-20260211-234139`
    - `gold_amount/player` reduced to `1.296x`.
  - Pass14b (aggressive bag-value compression, `runs=2`) and deterministic follow-up pass14c:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass14b-value-goldcompress-runs2-20260211-235029`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass14c-value-goldcompress-deterministic-runs2-20260211-235840`
    - Pass14c preserved non-value metrics exactly vs baseline and reduced `gold_amount/player` to `0.737x` in the runs=2 lane.
  - Pass14c volatility gate (`runs=5`):
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass14c-value-goldcompress-deterministic-runs5-20260212-000635`
    - All 300 rows passed.
    - Aggregate `p15 vs p4`: rooms `1.545x`, monsters `1.382x`, gold `2.171x`, items `2.473x`, food `3.315x`, decorations `2.286x`.
    - Aggregate per-player: gold `0.579x`, items `0.659x`, food `0.884x`, gold value (`gold_amount/player`) `0.896x`.
    - Level spread on `gold_amount/player` remained uneven (`L1 0.673x`, `L7 1.247x`, `L16 2.007x`, `L33 0.687x`), indicating remaining depth-mix value volatility.
- Pass15a depth-normalized overflow generated-gold value smoothing is now implemented and validated in fast integration lanes:
  - Code path: `/Users/sayhiben/dev/Barony-8p/src/maps.cpp` (`getOverflowGeneratedGoldValueScaleByDepth`, applied only when `overflowPlayers > 0` and only to generated base gold-bag value).
  - Artifacts:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15a-runs2-20260212-120325`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15a-runs5-20260212-120325`
  - `runs=2` lane (`120/120` pass rows) was directionally useful but noisy by depth.
  - `runs=5` lane (`300/300` pass rows) held stable and kept non-value count lanes aligned with pass14c aggregate shape.
  - `gold_amount/player` (`p15 vs p4`) improved depth spread from pass14c (`L1 0.673x`, `L7 1.247x`, `L16 2.007x`, `L33 0.687x`) to pass15a (`L1 0.726x`, `L7 1.056x`, `L16 1.282x`, `L33 0.744x`).
  - Aggregate `gold_amount/player` moved from `0.896x` (pass14c runs=5 matrix) to `0.838x` (pass15a runs=5 integration), with the large mid-floor value spike substantially reduced.
- Pass15b-pass15f follow-up count/value passes were run in fast integration lanes (`runs=2` + `runs=5`) and used to converge on a post-pass15a candidate:
  - Artifacts:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15b-runs2-20260212-121348`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15b-runs5-20260212-121348`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15c-runs2-20260212-121611`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15c-runs5-20260212-121611`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15d-runs2-20260212-121759`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15d-runs5-20260212-121759`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15e-runs2-20260212-121937`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15e-runs5-20260212-121938`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15f-runs2-20260212-122112`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15f-runs5-20260212-122112`
  - Key findings:
    - pass15b/pass15c over-expanded rooms (`1.800x`) and under-ran monster density (`0.703x-0.715x`), despite directional economy gains.
    - pass15d restored room shape (`1.545x`) but kept monsters below target (`1.303x`).
    - pass15e restored monster total (`1.403x`) but reintroduced deeper-floor value spikes (`L16 gold_amount/player 1.583x`).
    - pass15f reached near-target count lanes (`gold/player 0.688x`, `items/player 0.699x`, `food/player 0.723x`, `monsters 1.387x`) but overshot value lane (`gold_amount/player 1.070x`, `L7/L16 1.467x/1.612x`).
- Pass15g is the current integrated candidate (count shape from pass15f + value-only depth-band correction):
  - Artifacts:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15g-runs2-20260212-122229`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15g-runs5-20260212-122229`
  - `runs=5` (`300/300` pass rows) result shape:
    - `p15 vs p4`: rooms `1.545x`, monsters `1.387x`, monsters/room `0.897x`, gold/player `0.688x`, items/player `0.699x`, food/player `0.723x`, decorations `2.100x`, blocking-share `19.0%`.
    - `gold_amount/player` improved vs pass15f from `1.070x` to `1.021x`, with level spread improved at `L7/L16` from `1.467x/1.612x` to `1.274x/1.405x`.
- Low-player guard validation after pass15g:
  - Focused artifact (`players=2..4`, `runs=5`): `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15g-runs5-p2to4-20260212-122642` (`60/60` pass rows).
  - Row-level parity check on full runs confirms no `1..4p` regressions between pass15a and pass15g (`diffs=0`, `missing=0`) using:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15a-runs5-20260212-120325`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15g-runs5-20260212-122229`
- Full-lobby confirmation after pass15g:
  - Artifact: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-posttune-pass15g-20260212-132721`
  - Coverage: level `1`, players `1..15`, `runs=5`, all `75/75` rows passing (`mapgen_wait_reason=none` on all rows).
  - Full-lobby `p15 vs p4`: rooms `1.530x`, monsters `1.595x`, monsters/room `1.042x`, gold/player `0.610x`, items/player `0.714x`, food/player `0.610x`, decorations `3.545x`, blocking-share `46.2%`, `gold_amount/player=1.544x`.
  - Integration comparison at level `1` (same candidate): rooms stayed aligned (`1.530x` vs `1.523x`) and items/player stayed close (`0.714x` vs `0.723x`), but full-lobby diverged on monsters/room (`1.042x` vs `0.873x`), decorations (`3.545x` vs `1.667x`), food/player (`0.610x` vs `0.785x`), and `gold_amount/player` (`1.544x` vs `0.919x`).
- Decision: hold promotion on pass15g; keep it as baseline and run targeted retune focused on full-lobby level-1 divergence, then re-gate with integration + full-lobby.
- Pass15h fast-lane retune resumed with unpinned high-sample volatility aggregates (`4` invocations x `runs=10`, `2400` rows each):
  - Artifacts:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15h8-volatility-agg-20260212-172054`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15h9-volatility-agg-20260212-172508`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15h10-volatility-agg-20260212-172916`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15h11-volatility-agg-20260212-173303`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-pass15h12-volatility-agg-20260212-173635`
  - Shape evolution:
    - pass15h8 remained room-heavy (`rooms 1.795x`) with low density (`monsters/room 0.781x`) and value overshoot (`gold_amount/player 1.086x`).
    - pass15h9 corrected value but over-tightened rooms (`1.497x`) and raised density (`0.965x`).
    - pass15h10/pass15h11 improved global totals but still had level-1 pressure or item-floor regressions.
    - pass15h12 is the current integration candidate:
      - aggregate `p15 vs p4`: rooms `1.721x`, monsters `1.415x`, monsters/room `0.813x`, gold/player `0.733x`, items/player `0.734x`, food/player `0.766x`, decorations `1.812x`, `gold_amount/player 0.923x`.
      - level-1 `p15 vs p4`: rooms `1.582x`, monsters/room `0.889x`, decorations `1.925x`, `gold_amount/player 1.111x`.
- Full-lobby rerun on pass15h12 was started for promotion confidence, then intentionally stopped early to continue fast retune iteration:
  - Partial artifact: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-posttune-pass15h12-20260212-173948`
  - Completed sample count before stop: `8/75` rows (all `pass`, all `mapgen_wait_reason=none`).
- Smoke-only in-process headless integration runner is now implemented for rapid mapgen iteration:
  - Integration ownership is now hook-local: parser/validator/runner + CSV synthesis live in `src/smoke/SmokeTestHooks.cpp` and `src/smoke/SmokeTestHooks.hpp`.
  - `src/game.cpp` is intentionally limited to thin CLI wiring (`parseIntegrationOptionArg`, `validateIntegrationOptions`, `runIntegrationMatrix`).
  - CSV schema matches matrix outputs (`mapgen_level_matrix.csv`) and reuses smoke mapgen player override control-file behavior.
  - Integration seeding is now auto-generated per invocation; the temporary deterministic `-smoke-mapgen-integration-base-seed` override used for method-parity validation has been removed from commands and parser.
- Integration matrix run completed on current harness:
  - Artifact: `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-matrix2-20260212-005916`
  - Coverage: levels `1,7,16,25,33`, players `1..15`, `runs=2`, all 150 rows passing.
  - Level 25 no longer fails due missing `players` token in generation summary; integration rows now default observed players to requested override for pass/fail gating and mark `mapgen_generation_lines=0` when generation-summary players are absent.
- Comparability update (shared procedural matrix now aligned):
  - Fresh parity rerun artifacts:
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-parity-refresh-20260212-023902`
    - `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-method-parity-refresh-20260212-023933`
  - Scope: levels `1,7,16,33`, players `1..15`, `runs=1`, `base_seed=1000`.
  - Result: row counts `60` vs `60`, normalized compare (`run_dir` excluded) shows no differences (`normalized_row_mismatch=0`, `missing_keys=0`).
  - Current interpretation: in-process integration is now comparable for these procedural floors and can be used for rapid balance preflight; re-run parity checks whenever integration harness state/seed handling changes.

## Recommended Next Direction (Pass15h12 Follow-up)
1. Keep pass15h12 helper values as the current candidate baseline and complete full-lobby confirmation (`simulate-mapgen-players=0`, `runs=5`, `players=1..15`) end-to-end.
2. Re-run low-player guard lane (`players=2..4`, `runs=5`) and row-level parity compare to confirm no `1..4p` drift from accepted baseline.
3. If full-lobby still inflates level-1 density/decor/value, apply level-1-only overflow trims (forced-monster anchors / room spread) before any global coefficient change.
4. Preserve seedless volatility discipline: keep integration runs unpinned and rely on averaged aggregates for decisions.
5. Keep cache/process hygiene between lanes (`models.cache` cleanup + stale process checks) before launching the next full-lobby gate.

## Relevant Code and Tunables

### Core mapgen tuning code
- `/Users/sayhiben/dev/Barony-8p/src/maps.cpp`

Key overflow helper functions (all 5p+ only):
- `getOverflowLootToMonsterRerollDivisor()`
- `getOverflowRoomSelectionTrials()`
- `getOverflowBonusEntityRolls()`
- `getOverflowForcedMonsterSpawns()`
- `getOverflowForcedGoldSpawns()`
- `getOverflowForcedLootSpawns()`
- `getOverflowLootGoldRollDivisor()`
- `getOverflowForcedDecorationSpawns()`
- `getOverflowDecorationObstacleBudget()`

Key mapgen telemetry emitted by mapgen:
- Primary mapgen line: rooms/monsters/gold/items/decorations
- Decoration subtype line: `blocking/utility/traps/economy`
- Food telemetry line: `food` and `food_servings`

### Smoke hooks and runtime controls
- `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.cpp`
- `/Users/sayhiben/dev/Barony-8p/src/smoke/SmokeTestHooks.hpp`
- `/Users/sayhiben/dev/Barony-8p/src/game.cpp` (wiring-only callsite for integration CLI)

Important mapgen smoke controls:
- `BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS`
- `BARONY_SMOKE_MAPGEN_CONTROL_FILE`
- `BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL`
- `BARONY_SMOKE_MAPGEN_RELOAD_SEED_BASE`
- `BARONY_SMOKE_MAPGEN_PREVENT_DEATH`
- `BARONY_SMOKE_MAPGEN_HP_FLOOR`
- `BARONY_SMOKE_START_FLOOR`

### Runner scripts and reports
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_lan_helo_chunk_smoke_mac.sh`
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh`
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_level_matrix_mac.sh`
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/generate_mapgen_heatmap.py`
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/generate_smoke_aggregate_report.py`
- `/Users/sayhiben/dev/Barony-8p/tests/smoke/README.md`

## Facets, Gameplay Impact, and Target Intent

### 1. Rooms
Impact:
- More rooms reduce collision pressure and friendly-fire congestion.
- Excessive rooms can dilute pacing.

Intent:
- Maintain moderate growth for 5-15p.
- Keep high-player rooms around ~1.5x-1.75x p4, not 2x+.

### 2. Monsters (Total)
Impact:
- Main XP pressure source.
- Over-scaling drives chaos and wipe probability.

Intent:
- Keep monsters clearly increasing with player count.
- Avoid density spikes that force unavoidable pileups.

### 3. Monster Density (Monsters per Room)
Impact:
- Direct proxy for collision/friendly-fire pressure.

Intent:
- Keep monsters/room around a stable band at high player counts.
- Accept slight tapering at 12-15p if total monsters still scale.

### 4. Gold
Impact:
- Core progression resource for mixed-skill groups.

Intent:
- At higher player counts, progression support should outpace risk growth modestly.
- Raise high-player per-player gold floor.

### 5. Items
Impact:
- Build progression, survivability, and player agency.

Intent:
- Raise high-player per-player items floor.
- Keep total scaling robust while avoiding runaway abundance.

### 6. Food Servings
Impact:
- Attrition pacing and run continuity.

Intent:
- Prevent high-party starvation pressure.
- Keep food supplemental, but ensure a stable per-player floor.

### 7. Decorations (Total + Subtype Mix)
Impact:
- Affects readability, movement friction, hazards, and utility landmarks.

Intent:
- Keep ambiance growth, but avoid over-blocking shared traversal lanes.
- Favor non-blocking or mixed utility growth over pure blocking growth.

## Recommended Ratio Targets (anchored to p4)

### Global 15p targets relative to p4
- Rooms total: `1.62x-1.75x`
- Monsters total: `1.38x-1.46x`
- Monsters/room: `0.82x-0.92x` of p4 density
- Gold/player: `0.70x-0.80x`
- Items/player: `0.70x-0.80x`
- Food/player: `0.65x-0.78x`
- Decorations total: `1.85x-2.25x`
- Blocking decoration share: `<= 45%`

### Progression principle
- For 5-15p, reward growth should exceed risk growth by roughly `10-25%` on total progression outputs.
- This preserves accessibility for mixed-skill groups without trivializing difficulty.

## Current Baseline Insights (Pass10)
From `/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-pass10-survival-guard-runs3-20260211-195102`:
- `p4 -> p15` totals:
  - rooms `+63.7%`
  - monsters `+33.8%`
  - gold `+110.9%`
  - items `+116.4%`
  - food servings `+27.3%`
  - decorations `+131.3%`
- Remaining per-player gaps at `p15` vs `p4`:
  - gold/player `~0.56x`
  - items/player `~0.58x`
  - food/player `~0.34x`
- Decoration subtype means (`p4` -> `p15`):
  - total `4.25 -> 9.83`
  - blocking `1.83 -> 4.83`
  - utility `1.58 -> 2.83`
  - traps `1.58 -> 3.83`
  - economy-linked `0.50 -> 1.33`

## Tuning Process (Iteration Workflow)

### Sweep confidence policy
- Use `runs=3` for fast formula iteration while exploring direction.
- Use `runs=5` for volatility/gating sweeps and promotion decisions.
- If any key metric is near a threshold (about within 10% of a target band), escalate that lane to `runs=5` before accepting.

### Step 1: Edit overflow-only tunables
Touch only overflow helper logic and overflow branches in:
- `/Users/sayhiben/dev/Barony-8p/src/maps.cpp`

### Step 2: Build smoke binary
```bash
cmake -S /Users/sayhiben/dev/Barony-8p -B /Users/sayhiben/dev/Barony-8p/build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build /Users/sayhiben/dev/Barony-8p/build-mac-smoke -j8 --target barony
```

### Step 3: Run fast in-process integration preflight (`runs=2`)
```bash
USER_HOME="$HOME"
OUT="/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-integration-preflight-passNN-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT/home"
HOME="$OUT/home" /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  -windowed -size=1280x720 -nosound \
  -datadir="$USER_HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -smoke-mapgen-integration \
  -smoke-mapgen-integration-csv="$OUT/mapgen_level_matrix.csv" \
  -smoke-mapgen-integration-levels=1,7,16,33 \
  -smoke-mapgen-integration-min-players=1 \
  -smoke-mapgen-integration-max-players=15 \
  -smoke-mapgen-integration-runs=2
```

### Step 4: Run fast sanity matrix (`runs=2`)
```bash
OUT="/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-passNN-sanity-$(date +%Y%m%d-%H%M%S)"
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_level_matrix_mac.sh \
  --app "/Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony" \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --levels "1,7,16,33" \
  --min-players 1 --max-players 15 --runs-per-player 2 \
  --simulate-mapgen-players 1 --inprocess-sim-batch 1 --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --outdir "$OUT"
```

### Step 5: Run required volatility matrix (`runs=5`)
```bash
OUT="/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-passNN-runs5-$(date +%Y%m%d-%H%M%S)"
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_level_matrix_mac.sh \
  --app "/Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony" \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --levels "1,7,16,33" \
  --min-players 1 --max-players 15 --runs-per-player 5 \
  --simulate-mapgen-players 1 --inprocess-sim-batch 1 --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --outdir "$OUT"
```

### Step 6: Full-lobby confirmation (`simulate-mapgen-players=0`, promotion confidence)
```bash
OUT="/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-posttune-passNN-$(date +%Y%m%d-%H%M%S)"
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh \
  --app "/Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony" \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --min-players 1 --max-players 15 --runs-per-player 5 \
  --simulate-mapgen-players 0 \
  --auto-enter-dungeon 1 \
  --outdir "$OUT"
```

### Step 7: Hygiene and stale process cleanup
```bash
find /Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts -type f -name models.cache -delete
ps -Ao pid,ppid,etime,command | rg "run_mapgen_level_matrix_mac.sh|run_mapgen_sweep_mac.sh|run_lan_helo_chunk_smoke_mac.sh|barony.app/Contents/MacOS/barony"
```

## Pass11 Starting Targets (Recommended)
1. Increase per-player gold floor (5-15p).
2. Increase per-player item floor (5-15p).
3. Increase per-player food floor (5-15p).
4. Reduce blocking-share drift in decorations.
5. Keep rooms and monster totals near current trajectory unless density exits target range.

## Additional Opportunities (Next)
1. Add derived telemetry columns for easier balance gating:
- monsters_per_room
- gold_per_player
- items_per_player
- food_servings_per_player
- decor_blocking_share

2. Add loot-quality distribution telemetry (not just item count).
3. Add spawn clustering metric for monster/player collision pressure.
4. Add automated 1-4p parity check script for every tuning pass.

## Acceptance Criteria for Promotion
1. 1-4p parity preserved.
2. Regeneration/match metrics remain 100%.
3. 15p per-player economy enters target band (or materially closer with documented rationale).
4. Monster density stays in acceptable high-player band.
5. Full-lobby confirmation run preserves trend direction from simulated matrix.

## Documentation Process for Each Pass
For each new pass:
1. Record artifact paths and summary deltas.
2. Update stage and checklist status in:
- `/Users/sayhiben/dev/Barony-8p/docs/multiplayer-expansion-verification-plan.md`
3. Keep only useful logs; prune cache bloat.
