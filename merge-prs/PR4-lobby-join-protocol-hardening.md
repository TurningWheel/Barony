# [PR4] Lobby/Join Protocol Hardening for High Player Counts

## Ticket Metadata
- Type: Engineering
- Priority: Critical
- Epic: Multiplayer Expansion 1-15
- Risk: High
- Status (Updated 2026-02-14): Planned, not yet isolated for review
- Depends On: PR2, PR3
- Blocks: PR6-PR10 confidence

## Background
Large-lobby join/start reliability is a core blocker for 1-15 support. Existing join paths are fragile under high slot counts and payload pressure, especially around HELO capability signaling and chunk handling.

## What and Why
Harden join protocol and lobby flow so 4p and 15p sessions remain stable, while preserving compatibility/fallback behavior.

## Scope
### In Scope
- `src/net.cpp`
- `src/net.hpp`
- `src/ui/MainMenu.cpp`
- Minimal supporting changes only if required:
  - `src/game.cpp`
  - `src/interface/drawstatus.cpp`
  - `src/player.cpp`
  - `src/entity.cpp`

### Out of Scope
- Smoke framework architecture
- `tests/smoke/*` script additions
- Mapgen formulas or balance work

## Implementation Instructions
1. Implement/cleanly isolate JOIN capability and HELO chunk handling updates (`JOIN` len/capability path, `HLCN` handling).
2. Update `lobbyPlayerJoinRequest` API/signature in `src/net.hpp` and all required call sites.
3. Ensure chunk reassembly and fallback path both work:
   - New-capability clients use chunk path.
   - Legacy/small payload fallback remains functional.
4. Keep lobby readiness/start flow deterministic under high slots; avoid UI/net drift.
5. Keep this PR free of smoke runner/script additions; only core runtime behavior.

## Suggested Commit Structure
1. Protocol structure and capability-path updates.
2. Lobby/request flow wiring and fallback logic.
3. Small supporting fixes in dependent files.

## Validation Plan
- Manual:
  - 4-player lobby join/start cycle.
  - 15-player lobby join/start cycle.
  - Legacy fallback path sanity.
- Smoke evidence (local only, scripts not part of this PR):
  - HELO chunk lane
  - Legacy fallback lane

## Acceptance Criteria
- [ ] 15-player join/start path succeeds reliably in local validation.
- [ ] 4-player baseline flow remains stable.
- [ ] Capability/chunk path and legacy fallback both function.
- [ ] `src/net.cpp` / `src/ui/MainMenu.cpp` changes are focused and reviewable.
- [ ] No smoke framework/scripts or mapgen/balance changes are included.

## Review Focus
- Packet compatibility and fallback behavior.
- Reassembly safety and edge-case handling.
- Lobby state-machine correctness under churn.

## Rollback Strategy
Revert this PR while keeping PR2/PR3 scaffolding; protocol changes are isolated here.

## Extraction Plan (from PR #940 / 8p-mod)
Selectively extract net/lobby-only hunks from `a46cd9a3`, `780c0287` (ready-sync pieces), `b36471d6`, `4288b719`; reject `tests/smoke/*` and non-networking hunks.
