# Smoke Test Harness (macOS)

This folder contains blackbox-oriented smoke scripts for LAN lobby automation, HELO chunking checks, and map generation sweeps.

All main runners support `--app <path>` and optional `--datadir <path>` so you can run a locally built binary against a specific asset directory.

## Runner Architecture

- Entry point: `python3 tests/smoke/smoke_runner.py <lane> [options]`.
- `smoke_runner.py` composes top-level lane parser registration (`argparse`) from lane modules.
- Shared framework helpers live under `tests/smoke/smoke_framework/` (`common`, `process`, `summary`, `csvio`, `logscan`, `fs`, `local_lane`, `orchestration`, `tokens`, `statusfx`, `mapgen`, `mapgen_schema`, `mapgen_validation`, `mapgen_parser`, `mapgen_sweep_lane`, `mapgen_matrix_lane`, `mapgen_runtime`, `stats`, `helo_metrics`, `core_lane`, `core_parser`, `splitscreen_parser`, `splitscreen_runtime`, `splitscreen_baseline_lane`, `splitscreen_cap_lane`, `splitscreen_lanes`, `lan_helo_chunk_lane`, `lan_helo_chunk_parser`, `lan_helo_chunk_args`, `lan_helo_chunk_launch`, `lan_helo_chunk_runtime`, `lan_helo_chunk_post`, `lan_helo_chunk_summary`, `lane_helpers`, `lane_matrix`, `lane_status`, `churn_statusfx_lane`, `churn_join_leave`, `churn_statusfx_parser`, `lobby_remote_lane`, `lobby_remote_parser`, `self_check_lane`, `parser_common`, `reports`).
- `pyproject.toml` documents runtime expectations (`stdlib`-only dependencies) and an installable console script.
- `lan-helo-chunk` execution and parser registration are split across `smoke_framework/lan_helo_chunk_lane.py` and `smoke_framework/lan_helo_chunk_parser.py`.
- `lan-helo-chunk` argument normalization and launch/runtime plumbing are further split into `smoke_framework/lan_helo_chunk_args.py` and `smoke_framework/lan_helo_chunk_launch.py`.
- `lan-helo-chunk` handshake/wait-loop runtime state now lives in `smoke_framework/lan_helo_chunk_runtime.py`.
- `lan-helo-chunk` post-run metrics collection and result gating are split into `smoke_framework/lan_helo_chunk_post.py`.
- `lan-helo-chunk` summary payload assembly is centralized in `smoke_framework/lan_helo_chunk_summary.py`.
- `lan-helo-chunk` runtime polling uses cached log scanning to avoid repeated full-file rescans.
- `core` lane execution/parser ownership is split across `smoke_framework/core_lane.py` and `smoke_framework/core_parser.py`.
- `join-leave-churn` and `status-effect-queue-init` execution/parser ownership is split across `smoke_framework/churn_statusfx_lane.py` and `smoke_framework/churn_statusfx_parser.py`.
- `join-leave-churn` lifecycle/churn-loop/ready-sync/summary internals are split into `smoke_framework/churn_join_leave.py`.
- `lobby` remote lane execution/parser ownership is split across `smoke_framework/lobby_remote_lane.py` and `smoke_framework/lobby_remote_parser.py`.
- `splitscreen` parser/runtime/lane ownership is split across `smoke_framework/splitscreen_parser.py`, `smoke_framework/splitscreen_runtime.py`, `smoke_framework/splitscreen_baseline_lane.py`, and `smoke_framework/splitscreen_cap_lane.py` (`smoke_framework/splitscreen_lanes.py` remains a compatibility shim).
- Repeated parser args (`--app`, `--datadir`) are centralized via `smoke_framework/parser_common.py`.
- Mapgen lane orchestration is split across `smoke_framework/mapgen_sweep_lane.py` and `smoke_framework/mapgen_matrix_lane.py`.
- Shared mapgen metric schema and validation live in `smoke_framework/mapgen_schema.py` and `smoke_framework/mapgen_validation.py`.
- Shared numeric/statistical helpers for mapgen reports live in `smoke_framework/stats.py`.
- Mapgen parser registration lives in `smoke_framework/mapgen_parser.py`.
- Shared lane argument validation, nested child-lane invocation, and single-lane rollup helpers live in `smoke_framework/lane_helpers.py`.
- Aggregate report launching is shared via `smoke_framework/reports.py`.
- Shared lane pass/fail and count bookkeeping lives in `smoke_framework/lane_matrix.py`.
- Shared pass/fail status helpers live in `smoke_framework/lane_status.py`.
- Lightweight framework self-check lane lives in `smoke_framework/self_check_lane.py`.

## Lanes

- `lan-helo-chunk`
  - Base host/client orchestration lane.
  - Produces `summary.env`, stdout logs, per-instance homes/logs, and supports mapgen/reload and lobby instrumentation flags.
- `helo-soak`
  - Repeats LAN HELO smoke runs (default 10x).
  - Emits `soak_results.csv` and aggregate HTML.
- `helo-adversarial`
  - Runs adversarial tx-mode matrix (`reverse`, `even-odd`, `duplicate-first`, `drop-last`, `duplicate-conflict-first`).
  - Emits `adversarial_results.csv` and aggregate HTML.
- `lobby-kick-target`
  - Sweeps player counts and validates host auto-kick behavior at highest target slot.
  - Emits `kick_target_results.csv`.
- `save-reload-compat`
  - Runs owner-encoding save/reload compatibility sweep across 1..15p lanes.
  - Emits `save_reload_owner_encoding_results.csv`.
- `join-leave-churn`
  - Repeatedly kills/relaunches clients and verifies rejoin progress.
  - Optional ready-sync and join-reject trace assertions.
- `status-effect-queue-init`
  - Startup (1p/5p/15p) plus rejoin lanes with queue-owner safety assertions.
  - Emits `status_effect_queue_results.csv`.
- `lobby-slot-lock-kick-copy`
  - Validates default slot-lock snapshots and occupied-slot reduction prompt variants.
  - Emits `slot_lock_kick_copy_results.csv`.
- `lobby-page-navigation`
  - Full-lobby page sweep and page/focus/alignment assertions.
  - Emits `page_navigation_results.csv`.
- `remote-combat-slot-bounds`
  - Pause/unpause/combat pulse lane with remote-combat slot bounds and event assertions.
  - Emits `remote_combat_results.csv`.
- `splitscreen-baseline`
  - Local 4p baseline assertions for local lobby/camera/HUD/pause and transition flow.
  - Emits `splitscreen_results.csv`.
- `splitscreen-cap`
  - `/splitscreen` over-cap clamp assertion lane (legacy local cap remains 4).
  - Emits `splitscreen_cap_results.csv`.
- `mapgen-sweep`
  - Player-count sweep lane with mapgen telemetry CSV + heatmap + aggregate HTML.
  - Supports simulated mapgen players and in-process same-level sampling/sweeps.
- `mapgen-level-matrix`
  - Multi-floor mapgen matrix lane with per-floor and cross-floor aggregate outputs.
- `framework-self-check`
  - Lightweight parser/helper wiring checks for smoke framework internals.

## Files

- `smoke_runner.py`
  - Primary CLI + lane registration and top-level orchestration entrypoint.
- `smoke_framework/`
  - Shared runner framework helpers (filesystem, process lifecycle, summary parsing/writing, csv/log/token parsing, local lane prep, and nested lane orchestration).
- `.python-version`
  - Local `pyenv` interpreter pin for smoke tooling (`3.11`).
- `generate_mapgen_heatmap.py`
  - Converts mapgen CSV output into HTML heatmap.
- `generate_smoke_aggregate_report.py`
  - Produces aggregate HTML summaries from lane CSV outputs.

## Quick Start

Run HELO chunking smoke for 4 players:

```bash
python3 tests/smoke/smoke_runner.py lan-helo-chunk \
  --instances 4 \
  --force-chunk 1 \
  --chunk-payload-max 200
```

Run with local build binary + Steam asset directory (avoids replacing Steam app executable):

```bash
python3 tests/smoke/smoke_runner.py lan-helo-chunk \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  --instances 4 \
  --force-chunk 1 \
  --chunk-payload-max 200
```

Run HELO smoke with account-label coverage assertions:

```bash
python3 tests/smoke/smoke_runner.py lan-helo-chunk \
  --instances 8 \
  --force-chunk 1 \
  --chunk-payload-max 200 \
  --trace-account-labels 1 \
  --require-account-labels 1
```

Run mapgen sweep for players `1..15`, one run each:

```bash
python3 tests/smoke/smoke_runner.py mapgen-sweep \
  --min-players 1 \
  --max-players 15 \
  --runs-per-player 1 \
  --auto-enter-dungeon 1 \
  --force-chunk 1 \
  --chunk-payload-max 200

# Faster single-instance mapgen sweep (simulated mapgen players 1..15):
python3 tests/smoke/smoke_runner.py mapgen-sweep \
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

# Per-floor matrix sweep (default levels: 1,7,16,33):
python3 tests/smoke/smoke_runner.py mapgen-level-matrix \
  --runs-per-player 2 \
  --simulate-mapgen-players 1 \
  --inprocess-sim-batch 1 \
  --inprocess-player-sweep 1 \
  --mapgen-reload-same-level 1 \
  --levels 1,7,16,33
```

In `--simulate-mapgen-players 1` mode, `--inprocess-sim-batch 1` runs all samples for a given player count in one runtime by using repeated smoke-driven dungeon transitions.
With `--inprocess-player-sweep 1` and same-level reload enabled, the sweep can also keep one runtime alive while stepping mapgen player overrides across the full player-count range.
Choose procedural floors for mapgen balancing lanes; fixed/story floors will fail fast with `mapgen_wait_reason=reload-complete-no-mapgen-samples`.
The sweep now sets extra transition headroom automatically so sparse/no-generate floors do not stall sample collection.

Run a 10x HELO soak:

```bash
python3 tests/smoke/smoke_runner.py helo-soak \
  --runs 10 \
  --instances 8 \
  --force-chunk 1 \
  --chunk-payload-max 200
```

Run adversarial HELO matrix:

```bash
python3 tests/smoke/smoke_runner.py helo-adversarial \
  --instances 4 \
  --chunk-payload-max 200
```

Run lobby auto-kick target matrix:

```bash
python3 tests/smoke/smoke_runner.py lobby-kick-target \
  --min-players 2 \
  --max-players 15
```

Run save/reload owner-encoding compatibility sweep:

```bash
python3 tests/smoke/smoke_runner.py save-reload-compat
```

Run join/leave churn smoke:

```bash
python3 tests/smoke/smoke_runner.py join-leave-churn \
  --instances 8 \
  --churn-cycles 2 \
  --churn-count 2
```

Run churn with ready-state snapshot assertions:

```bash
python3 tests/smoke/smoke_runner.py join-leave-churn \
  --instances 8 \
  --churn-cycles 2 \
  --churn-count 2 \
  --auto-ready 1 \
  --trace-ready-sync 1 \
  --require-ready-sync 1
```

Run status-effect queue initialization + rejoin safety lane:

```bash
python3 tests/smoke/smoke_runner.py status-effect-queue-init \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run default slot-lock + kick-copy matrix lane:

```bash
python3 tests/smoke/smoke_runner.py lobby-slot-lock-kick-copy \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run lobby page navigation/alignment lane:

```bash
python3 tests/smoke/smoke_runner.py lobby-page-navigation \
  --app /Users/sayhiben/dev/Barony-8p/build-mac/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run remote-combat slot-bounds lane:

```bash
python3 tests/smoke/smoke_runner.py remote-combat-slot-bounds \
  --app /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run local 4-player splitscreen baseline lane:

```bash
python3 tests/smoke/smoke_runner.py splitscreen-baseline \
  --app /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run local splitscreen cap lane (`/splitscreen 15` should clamp to 4):

```bash
python3 tests/smoke/smoke_runner.py splitscreen-cap \
  --app /Users/sayhiben/dev/Barony-8p/build-mac-smoke/barony.app/Contents/MacOS/barony \
  --datadir "$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources"
```

Run lightweight framework parser/helper self-checks:

```bash
python3 tests/smoke/smoke_runner.py framework-self-check
```

Run lightweight helper unit tests:

```bash
python3 -m unittest discover -s tests/smoke/tests -p "test_*.py"
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
