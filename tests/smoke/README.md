# Smoke Test Harness (macOS)

This folder contains blackbox-oriented smoke scripts for LAN lobby automation, HELO chunking checks, and map generation sweeps.

All main runners support `--app <path>` and optional `--datadir <path>` so you can run a locally built binary against a specific asset directory.

## Files

- `run_lan_helo_chunk_smoke_mac.sh`
  - Launches 1 host + N-1 clients as isolated instances.
  - Uses env-driven autopilot hooks in game code to host/join automatically.
  - Verifies chunked HELO send/reassembly by parsing per-instance logs.
  - Optionally auto-starts gameplay and can force a smoke-only transition from the starting area into dungeon floor 1.
  - Optionally waits for dungeon map generation completion.
  - Supports smoke-only HELO tx adversarial modes (reordering/dup/drop tests).
  - Supports strict adversarial assertions and backend tagging in `summary.env`.
  - Supports explicit transition budget via `--auto-enter-dungeon-repeats` (defaults to `--mapgen-samples`).
  - Supports same-level mapgen reload sampling (`--mapgen-reload-same-level 1`) with optional per-sample seed rotation (`--mapgen-reload-seed-base <n>`).
  - Supports dynamic mapgen player override control via `--mapgen-control-file <path>` for in-process player-count tuning.
  - In mapgen-required same-level reload mode, fails fast with `MAPGEN_WAIT_REASON=reload-complete-no-mapgen-samples` when the selected floor is non-procedural.
  - Emits mapgen regeneration evidence fields in `summary.env` (`MAPGEN_RELOAD_TRANSITION_*`, `MAPGEN_GENERATION_*`, `MAPGEN_RELOAD_REGEN_OK`).
  - Seeds smoke homes with `skipintro=true` automatically (writes a minimal config when no seed config exists) to avoid intro/title startup stalls.
  - Optionally traces/asserts lobby account label coverage for remote slots (`--trace-account-labels 1 --require-account-labels 1`).

- `run_lan_helo_soak_mac.sh`
  - Repeats LAN HELO smoke runs (default 10x).
  - Emits `soak_results.csv` and an aggregate HTML report.

- `run_helo_adversarial_smoke_mac.sh`
  - Runs a matrix of HELO tx modes:
    - expected pass: `normal`, `reverse`, `even-odd`, `duplicate-first`
    - expected fail: `drop-last`, `duplicate-conflict-first`
  - Emits `adversarial_results.csv` and an aggregate HTML report.

- `run_lan_join_leave_churn_smoke_mac.sh`
  - Launches a full lobby, then repeatedly kills/relaunches selected clients.
  - Asserts rejoin progress by requiring increasing host HELO chunk counts.
  - Optionally enables and asserts ready-state snapshot sync coverage (`--auto-ready 1 --trace-ready-sync 1 --require-ready-sync 1`).
  - Optionally traces join-reject slot-state snapshots (`--trace-join-rejects 1`) to debug transient `error code 16` retries.
  - Emits per-cycle churn CSV and an aggregate HTML report.

- `run_status_effect_queue_init_smoke_mac.sh`
  - Runs startup lanes at 1p/5p/15p with auto-start + dungeon entry.
  - Runs late-join/rejoin churn lanes at 5p/15p.
  - Enables smoke-only status-effect queue tracing and asserts slot-owner safety (startup: `init`/`create`/`update`, rejoin: `init`) with no mismatches.
  - Emits `status_effect_queue_results.csv` and a run summary.

- `run_lobby_slot_lock_and_kick_copy_smoke_mac.sh`
  - Runs a lobby matrix for default slot-lock behavior and occupied-slot player-count reduction kick-copy variants.
  - Asserts default host slot-lock snapshots and prompt copy variants for `single`/`double`/`multi` occupied-player reductions.
  - Emits `slot_lock_kick_copy_results.csv` and a run summary.

- `run_lobby_page_navigation_smoke_mac.sh`
  - Runs a full-lobby page navigation lane with smoke-driven page sweep.
  - Asserts page/alignment snapshots for card placement, paperdolls, ping frames, and centered warning/countdown overlays when present.
  - Optionally enforces focus-page matching (`--require-focus-match 1`).
  - Emits `page_navigation_results.csv` and a run summary.

- `run_remote_combat_slot_bounds_smoke_mac.sh`
  - Runs a full-lobby remote-combat lane with smoke-driven pause/unpause pulses, enemy-bar combat pulses, and visible damage-gib pulses.
  - Asserts remote-combat slot bounds (`REMOTE_COMBAT_SLOT_FAIL_LINES=0`) and required event coverage contexts.
  - Emits `remote_combat_results.csv` and a run summary.

- `run_splitscreen_baseline_smoke_mac.sh`
  - Runs a local 4-player splitscreen baseline lane in a single smoke instance.
  - Uses local-lobby autopilot to ready 4 slots, verifies baseline camera/HUD/local-slot state, runs pause/unpause pulses, and forces first-floor transition.
  - Emits `splitscreen_results.csv` and a run summary.

- `run_splitscreen_cap_smoke_mac.sh`
  - Runs a local splitscreen cap lane in a single smoke instance.
  - Issues `/enablecheats` + `/splitscreen <target>` (default `15`) and asserts hard clamp behavior at 4 local players with no over-cap slot activation side effects.
  - Emits `splitscreen_cap_results.csv` and a run summary.

- `run_mapgen_sweep_mac.sh`
  - Runs repeated sessions for player counts in a range (default `1..15`).
  - Writes aggregate CSV with map generation metrics.
  - Generates a simple HTML heatmap via `generate_mapgen_heatmap.py`.
  - Supports a fast single-instance mode that simulates mapgen scaling player counts.
  - Supports smoke-only start-floor control via `--start-floor <n>` for same-floor cross-player comparisons.
  - Supports in-process same-level sample collection (`--inprocess-sim-batch 1 --mapgen-reload-same-level 1`) to avoid relaunching between samples.
  - Supports in-process single-runtime player sweeps (`--inprocess-player-sweep 1`) that step mapgen player overrides across all requested player counts without relaunching.
  - CSV includes mapgen wait/reload regeneration diagnostics (`mapgen_wait_reason`, `mapgen_reload_transition_lines`, `mapgen_generation_lines`, `mapgen_generation_unique_seed_count`, `mapgen_reload_regen_ok`).
  - CSV now also records both intended and observed scaling players (`mapgen_players_override`, `mapgen_players_observed`) for sweep-control verification.
  - CSV now records observed generation seed and food metrics (`mapgen_seed_observed`, `food_items`, `food_servings`) for regeneration and hunger-scaling analysis.

- `run_mapgen_level_matrix_mac.sh`
  - Runs multiple per-floor mapgen sweeps and keeps each floor in its own artifact/report directory.
  - In same-level reload mode, maps each requested `--levels` floor directly to `--start-floor=<level>` so level labels and observed floor IDs stay aligned.
  - Defaults to in-process same-level sampling (no relaunch between samples) for faster per-floor campaigns.
  - Emits a combined matrix CSV plus per-floor trend summary (`mapgen_level_trends.csv`) and cross-level aggregate summaries (`mapgen_level_overall.csv`, `mapgen_level_overall.md`).
  - Per-floor trends now distinguish target-floor matching from regeneration diversity (`target_level_match_rate_pct`, `observed_seed_unique_rate_pct`, `reload_unique_seed_rate_pct`).
  - Emits an HTML aggregate report for matrix data (`mapgen_level_matrix_aggregate_report.html`).

- `generate_mapgen_heatmap.py`
  - Converts the CSV output into a colorized HTML table.

- `generate_smoke_aggregate_report.py`
  - Produces one HTML summary from mapgen/soak/adversarial/churn CSVs.
  - Also supports matrix-level mapgen aggregation via `--mapgen-matrix-csv`.

## Quick Start

Run HELO chunking smoke for 4 players:

```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --instances 4 \
  --force-chunk 1 \
  --chunk-payload-max 200
```

Run with local build binary + Steam asset directory (avoids replacing Steam app executable):

```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --instances 4 \
  --force-chunk 1 \
  --chunk-payload-max 200
```

Run HELO smoke with account-label coverage assertions:

```bash
tests/smoke/run_lan_helo_chunk_smoke_mac.sh \
  --instances 8 \
  --force-chunk 1 \
  --chunk-payload-max 200 \
  --trace-account-labels 1 \
  --require-account-labels 1
```

Run mapgen sweep for players `1..15`, one run each:

```bash
tests/smoke/run_mapgen_sweep_mac.sh \
  --min-players 1 \
  --max-players 15 \
  --runs-per-player 1 \
  --auto-enter-dungeon 1 \
  --force-chunk 1 \
  --chunk-payload-max 200

# Faster single-instance mapgen sweep (simulated mapgen players 1..15):
tests/smoke/run_mapgen_sweep_mac.sh \
  --min-players 1 \
  --max-players 15 \
  --runs-per-player 8 \
  --simulate-mapgen-players 1 \
  --inprocess-sim-batch 1 \
  --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --stagger 0 \
  --auto-start-delay 0 \
  --auto-enter-dungeon 1

# Per-floor matrix sweep (default levels: 1,7,16,25,33):
tests/smoke/run_mapgen_level_matrix_mac.sh \
  --runs-per-player 2 \
  --simulate-mapgen-players 1 \
  --inprocess-sim-batch 1 \
  --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --levels 1,7,16,25,33
```

In `--simulate-mapgen-players 1` mode, `--inprocess-sim-batch 1` runs all samples for a given player count in one runtime by using repeated smoke-driven dungeon transitions.
With `--inprocess-player-sweep 1` and same-level reload enabled, the sweep can also keep one runtime alive while stepping mapgen player overrides across the full player-count range.
Choose procedural floors for mapgen balancing lanes; fixed/story floors will fail fast with `mapgen_wait_reason=reload-complete-no-mapgen-samples`.
The sweep now sets extra transition headroom automatically so sparse/no-generate floors do not stall sample collection.

Run a 10x HELO soak:

```bash
tests/smoke/run_lan_helo_soak_mac.sh \
  --runs 10 \
  --instances 8 \
  --force-chunk 1 \
  --chunk-payload-max 200
```

Run adversarial HELO matrix:

```bash
tests/smoke/run_helo_adversarial_smoke_mac.sh \
  --instances 4 \
  --chunk-payload-max 200
```

Run join/leave churn smoke:

```bash
tests/smoke/run_lan_join_leave_churn_smoke_mac.sh \
  --instances 8 \
  --churn-cycles 2 \
  --churn-count 2
```

Run churn with ready-state snapshot assertions:

```bash
tests/smoke/run_lan_join_leave_churn_smoke_mac.sh \
  --instances 8 \
  --churn-cycles 2 \
  --churn-count 2 \
  --auto-ready 1 \
  --trace-ready-sync 1 \
  --require-ready-sync 1
```

Run status-effect queue initialization + rejoin safety lane:

```bash
tests/smoke/run_status_effect_queue_init_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run default slot-lock + kick-copy matrix lane:

```bash
tests/smoke/run_lobby_slot_lock_and_kick_copy_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run lobby page navigation/alignment lane:

```bash
tests/smoke/run_lobby_page_navigation_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run remote-combat slot-bounds lane:

```bash
tests/smoke/run_remote_combat_slot_bounds_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run local 4-player splitscreen baseline lane:

```bash
tests/smoke/run_splitscreen_baseline_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run local splitscreen cap lane (`/splitscreen 15` should clamp to 4):

```bash
tests/smoke/run_splitscreen_cap_smoke_mac.sh \
  --app /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

## Artifact Layout

Both scripts write to `tests/smoke/artifacts/...` by default.

Each run includes:

- `summary.env`: key-value summary (pass/fail, counts, mapgen metrics)
  - Mapgen reload diagnostics include: `MAPGEN_WAIT_REASON`, `MAPGEN_RELOAD_TRANSITION_LINES`, `MAPGEN_RELOAD_TRANSITION_SEEDS`, `MAPGEN_GENERATION_LINES`, `MAPGEN_GENERATION_SEEDS`, `MAPGEN_RELOAD_REGEN_OK`.
  - Includes additional HELO fields: `NETWORK_BACKEND`, `TX_MODE_APPLIED`,
    `PER_CLIENT_REASSEMBLY_COUNTS`, `CHUNK_RESET_REASON_COUNTS`,
    `HELO_PLAYER_SLOTS`, `HELO_PLAYER_SLOT_COVERAGE_OK`,
    `ACCOUNT_LABEL_SLOTS`, `ACCOUNT_LABEL_SLOT_COVERAGE_OK`
- Churn ready-sync mode adds `READY_SNAPSHOT_*` fields and `READY_SYNC_CSV`; join-reject tracing adds `JOIN_REJECT_TRACE_LINES`
- Status-effect queue lane adds `status_effect_queue_results.csv` with per-lane slot coverage/mismatch results (`init/create/update`).
- Lobby slot-lock/copy lane adds `slot_lock_kick_copy_results.csv` with per-lane slot-lock and prompt-variant assertions.
- Lobby page navigation lane adds `page_navigation_results.csv` with per-lane page/alignment assertions and focus mismatch diagnostics.
- Remote-combat lane adds `remote_combat_results.csv` with per-lane slot-bound/event assertions and context coverage.
- Local splitscreen baseline lane adds `splitscreen_results.csv` with local lobby readiness, baseline camera/HUD/pause checks, and transition assertions.
- Local splitscreen cap lane adds `splitscreen_cap_results.csv` with requested-player clamp assertions and over-cap leakage checks.
- `pids.txt`: launched process metadata
- `stdout/`: captured process stdout
- `instances/home-*/.barony/log.txt`: engine logs used for assertions
- Per-instance `models.cache` files are removed by the smoke runner during cleanup to avoid runaway disk usage.
- If `--outdir` is reused, runners clear volatile per-run state (`instances/`, `stdout/`, `summary.env`, pid/csv files) before launch so stale logs cannot satisfy assertions.

Mapgen sweeps additionally emit:

- `mapgen_results.csv`
- `mapgen_heatmap.html`
- `smoke_aggregate_report.html`

Soak/adversarial/churn runs emit similar CSV + `smoke_aggregate_report.html`.

## Smoke Env Vars Used by Runtime Hooks

These are read by `MainMenu.cpp` / `net.cpp` when set:

- `BARONY_SMOKE_AUTOPILOT=1`
- `BARONY_SMOKE_ROLE=host|client|local`
- `BARONY_SMOKE_CONNECT_ADDRESS=127.0.0.1:57165`
- `BARONY_SMOKE_CONNECT_DELAY_SECS=<int>`
- `BARONY_SMOKE_RETRY_DELAY_SECS=<int>`
- `BARONY_SMOKE_EXPECTED_PLAYERS=<1..15>`
- `BARONY_SMOKE_AUTO_START=0|1`
- `BARONY_SMOKE_AUTO_START_DELAY_SECS=<int>`
- `BARONY_SMOKE_AUTO_ENTER_DUNGEON=0|1` (host-only, smoke-only)
- `BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS=<int>`
- `BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS=<int>` (host-only, smoke-only)
- `BARONY_SMOKE_AUTO_KICK_TARGET_SLOT=<1..14>` / `BARONY_SMOKE_AUTO_KICK_DELAY_SECS=<int>`
- `BARONY_SMOKE_AUTO_PLAYER_COUNT_TARGET=<2..15>` / `BARONY_SMOKE_AUTO_PLAYER_COUNT_DELAY_SECS=<int>`
- `BARONY_SMOKE_AUTO_LOBBY_PAGE_SWEEP=0|1` / `BARONY_SMOKE_AUTO_LOBBY_PAGE_DELAY_SECS=<int>`
- `BARONY_SMOKE_SEED=<seed string>`
- `BARONY_SMOKE_AUTO_READY=0|1`
- `BARONY_SMOKE_TRACE_READY_SYNC=0|1` (host-only, smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_ACCOUNT_LABELS=0|1` (host-only, smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_SLOT_LOCKS=0|1` (host-only, smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_PLAYER_COUNT_COPY=0|1` (host-only, smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_LOBBY_PAGE_STATE=0|1` (host-only, smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_REMOTE_COMBAT_SLOT_BOUNDS=0|1` (smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN=0|1` (smoke-only local splitscreen baseline logging)
- `BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN_CAP=0|1` (smoke-only local splitscreen cap logging)
- `BARONY_SMOKE_TRACE_JOIN_REJECTS=0|1` (host-only, smoke-only diagnostic logging)
- `BARONY_SMOKE_TRACE_STATUS_EFFECT_QUEUE=0|1` (smoke-only diagnostic logging)
- `BARONY_SMOKE_AUTO_PAUSE_PULSES=<int>` / `BARONY_SMOKE_AUTO_PAUSE_DELAY_SECS=<int>` / `BARONY_SMOKE_AUTO_PAUSE_HOLD_SECS=<int>` (host-only, smoke-only)
- `BARONY_SMOKE_AUTO_REMOTE_COMBAT_PULSES=<int>` / `BARONY_SMOKE_AUTO_REMOTE_COMBAT_DELAY_SECS=<int>` (host-only, smoke-only)
- `BARONY_SMOKE_LOCAL_PAUSE_PULSES=<int>` / `BARONY_SMOKE_LOCAL_PAUSE_DELAY_SECS=<int>` / `BARONY_SMOKE_LOCAL_PAUSE_HOLD_SECS=<int>` (local-role, smoke-only)
- `BARONY_SMOKE_AUTO_SPLITSCREEN_CAP_TARGET=<2..15>` / `BARONY_SMOKE_SPLITSCREEN_CAP_DELAY_SECS=<int>` / `BARONY_SMOKE_SPLITSCREEN_CAP_VERIFY_DELAY_SECS=<int>` (local-role, smoke-only)
- `BARONY_SMOKE_FORCE_HELO_CHUNK=0|1`
- `BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX=<64..900>`
- `BARONY_SMOKE_HELO_CHUNK_TX_MODE=normal|reverse|even-odd|duplicate-first|drop-last|duplicate-conflict-first` (host-only, smoke-only)
- `BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS=<1..15>` (smoke-only mapgen scaling override)
- `BARONY_SMOKE_START_FLOOR=<0..99>` (host-only, smoke-only start-floor override)
- `BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL=0|1` (host-only, smoke-only same-level reload sampling)
- `BARONY_SMOKE_MAPGEN_RELOAD_SEED_BASE=<int>` (host-only, smoke-only per-sample seed base for same-level reload mode)

These hooks are dormant unless `BARONY_SMOKE_AUTOPILOT` or explicit smoke role settings are enabled.
