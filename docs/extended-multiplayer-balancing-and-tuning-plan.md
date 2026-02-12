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
- Stage: `Pass10 complete` (structural overflow tuning + decoration telemetry + survival-guarded long sweeps).
- Next stage: `Pass11 targeted economy/food/decor composition tuning` with runs=3 volatility sweep and full-lobby confirmation.

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

### Step 1: Edit overflow-only tunables
Touch only overflow helper logic and overflow branches in:
- `/Users/sayhiben/dev/Barony-8p/src/maps.cpp`

### Step 2: Build smoke binary
```bash
cmake -S /Users/sayhiben/dev/Barony-8p -B /Users/sayhiben/dev/Barony-8p/build-mac-smoke -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SMOKE_TESTS=ON
cmake --build /Users/sayhiben/dev/Barony-8p/build-mac-smoke -j8 --target barony
```

### Step 3: Run fast sanity matrix (`runs=2`)
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

### Step 4: Run required volatility matrix (`runs=3`)
```bash
OUT="/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-level-matrix-passNN-runs3-$(date +%Y%m%d-%H%M%S)"
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_level_matrix_mac.sh \
  --app "/Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony" \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --levels "1,7,16,33" \
  --min-players 1 --max-players 15 --runs-per-player 3 \
  --simulate-mapgen-players 1 --inprocess-sim-batch 1 --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --outdir "$OUT"
```

### Step 5: Full-lobby confirmation (`simulate-mapgen-players=0`)
```bash
OUT="/Users/sayhiben/dev/Barony-8p/tests/smoke/artifacts/mapgen-full-posttune-passNN-$(date +%Y%m%d-%H%M%S)"
/Users/sayhiben/dev/Barony-8p/tests/smoke/run_mapgen_sweep_mac.sh \
  --app "/Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony" \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --min-players 1 --max-players 15 --runs-per-player 3 \
  --simulate-mapgen-players 0 \
  --auto-enter-dungeon 1 \
  --outdir "$OUT"
```

### Step 6: Hygiene and stale process cleanup
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
