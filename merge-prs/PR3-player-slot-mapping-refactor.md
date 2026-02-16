# [PR3] Player-Slot Mapping Refactor (No Behavior Change)

## Ticket Metadata
- Type: Engineering
- Priority: Medium
- Epic: Multiplayer Expansion 1-15
- Risk: Low
- Status (Updated 2026-02-14): Planned, not yet isolated for review
- Depends On: PR2
- Blocks: PR4 reviewability

## Background
Slot-to-theme/color logic is duplicated across UI/gameplay files, which makes high-player-count work noisy and error-prone. A dedicated mapping layer reduces duplication before additional lobby/protocol complexity lands.

## What and Why
Consolidate player-slot visual mapping into one reusable module so future slot expansions are maintainable and deterministic, while preserving current visual behavior.

## Scope
### In Scope
- `src/player_slot_map.hpp`
- `src/actplayer.cpp`
- `src/interface/interface.cpp`
- `src/interface/playerinventory.cpp`
- `src/items.cpp`
- `src/items.hpp`
- `src/actitem.cpp`
- `src/interface/drawminimap.cpp`
- `src/ui/GameUI.cpp`
- Optional tiny text-only fix in `src/ui/MainMenu.cpp`

### Out of Scope
- Networking/protocol changes
- Smoke framework or scripts
- Mapgen/balance changes
- Build system changes

## Implementation Instructions
1. Introduce `src/player_slot_map.hpp` with shared slot mapping helpers.
2. Replace repeated slot/color/theme logic in listed files with helper calls.
3. Preserve legacy visual mapping for first slots and deterministic overflow cycling for higher slots.
4. Keep any optional `MainMenu` edits strictly text-level and non-behavioral.
5. Ensure no packet/gameplay/network semantics are changed.

## Suggested Commit Structure
1. Add shared slot-map header and helper definitions.
2. Migrate call sites in UI/gameplay files.
3. Optional tiny cleanup/text fix commit.

## Validation Plan
- Build sanity:
```bash
cmake -S . -B build-pr3 -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SUPER_MULTIPLAYER=ON
cmake --build build-pr3 -j8 --target barony
```
- Manual visual checks:
  - Normal mode player icon/theme mapping.
  - Colorblind mode mapping.
  - Overflow slot mapping remains deterministic.

## Acceptance Criteria
- [ ] All duplicated mapping logic is centralized through `player_slot_map.hpp` helpers.
- [ ] First-slot legacy visual outputs are unchanged.
- [ ] Overflow mapping is deterministic and repeatable.
- [ ] No networking/smoke/mapgen/build files are changed.
- [ ] Manual UI sanity confirms no visual regressions.

## Review Focus
- Behavior-preserving refactor quality.
- Mapping determinism and readability.
- No hidden side effects in unrelated gameplay code.

## Rollback Strategy
Revert this PR independently; no protocol/data compatibility impact.

## Extraction Plan (from PR #940 / 8p-mod)
File-scoped cherry-pick from `69013b2f`, `780c0287`, `bce9dd61`, `ae8b98dd` (items-only hunks), `0e95632b`; exclude unrelated hunks.
