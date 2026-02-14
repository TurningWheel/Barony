# [PR7] Smoke Runner Lanes (Non-Mapgen)

## Ticket Metadata
- Type: Engineering
- Priority: Medium
- Epic: Multiplayer Expansion 1-15
- Risk: Low-Medium
- Depends On: PR6
- Blocks: Reliable networking/combat/splitscreen regression evidence

## Background
Smoke hooks exist after PR6, but repeatable validation still requires runner scripts and docs. Non-mapgen lanes should land separately from mapgen integration/tuning to keep reviewer burden and risk contained.

## What and Why
Add reproducible non-mapgen smoke lanes (lobby/chunking/churn/combat/splitscreen/save-reload) to enforce regression checks as multiplayer support expands.

## Scope
### In Scope
- Non-mapgen smoke scripts and docs in `tests/smoke/`, including:
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
- Shared helper extraction where needed (recommended):
  - `tests/smoke/lib/common.sh` for `is_uint`, key-value parsing, env/bootstrap, cache cleanup utilities.

### Out of Scope
- Any gameplay/source code behavior changes
- Mapgen integration runners or mapgen balancing logic

## Implementation Instructions
1. Add/clean non-mapgen smoke scripts and README documentation.
2. Standardize exact `key=value` parsing for summary/env reads to avoid false negatives (`connected` vs `over_cap_connected` style collisions).
3. Consolidate duplicate shell helper logic into `tests/smoke/lib/common.sh` where possible.
4. Keep scripts parameterized and artifact-oriented (`--outdir`, deterministic summary outputs).
5. Keep mapgen-specific scripts and logic out of this PR (they belong in PR8).

## Suggested Commit Structure
1. Add common shell helpers and parser utilities.
2. Add/update non-mapgen runner scripts.
3. Add smoke README and usage examples.

## Validation Plan
- Script lint/parse check:
```bash
bash -n tests/smoke/*.sh
```
- Execute at least:
  - One short 4p lane.
  - One short 15p lane.
- Validate artifact output includes machine-readable summary files.

## Acceptance Criteria
- [ ] Non-mapgen smoke runner scripts are present and documented.
- [ ] Scripts pass `bash -n` and run at least one 4p and one 15p smoke lane.
- [ ] Key-value parsing is exact and robust.
- [ ] No C++ gameplay/network source files are modified.
- [ ] No mapgen integration/balance logic is included.

## Review Focus
- Script correctness and maintainability.
- Parser robustness and artifact consistency.
- Clear separation from mapgen concerns.

## Rollback Strategy
Revert scripts/docs only; no runtime source behavior dependencies.

## Extraction Plan (from PR #940 / 8p-mod)
Use script-only slices from `fc6c6ced`, `4f24a78a`, `0f87d77f`, `14ae1081`, `9ed76e40`, `396263f1`, `b018bfeb`, excluding mapgen runners and source-code changes.
