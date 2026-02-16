# [PR5] Status-Effect Owner Encoding Hardening (Cap-Safe to 15)

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Medium-High
- Status (Updated 2026-02-14): Planned, not yet isolated for review
- Depends On: PR2
- Blocks: Correctness confidence for high-slot combat

## Background
Status-effect owner encoding assumptions were built around lower player caps. Extending to 15 players requires explicit encoding/decoding hardening to avoid misattribution and preserve save/combat compatibility.

## What and Why
Introduce cap-safe owner encoding helpers and migrate magic/effect call sites to prevent owner corruption at higher player indices.

## Scope
### In Scope
- `src/status_effect_owner_encoding.hpp`
- `src/magic/actmagic.cpp`
- `src/magic/castSpell.cpp`
- `src/entity.cpp`
- `src/actplayer.cpp`

### Out of Scope
- Smoke scripts/docs
- Build system changes
- Mapgen changes
- Lobby/protocol flow changes

## Implementation Instructions
1. Add `src/status_effect_owner_encoding.hpp` with explicit encode/decode helpers for player-owner values up to 15.
2. Migrate affected magic/entity call sites to use helper APIs instead of ad-hoc bit assumptions.
3. Preserve legacy compatibility semantics:
   - Existing `EFF_FAST` expectations remain valid.
   - Non-player/sentinel ownership remains safe and explicit.
4. Add local assertions/logging where useful to catch invalid owner values in debug paths.
5. Keep the PR limited to owner encoding correctness only.

## Suggested Commit Structure
1. Introduce owner-encoding helper header/API.
2. Apply helper migration to magic/entity/player call sites.
3. Small compatibility cleanup for legacy/sentinel handling.

## Validation Plan
- Build:
```bash
cmake -S . -B build-pr5 -G Ninja -DFMOD_ENABLED=OFF -DBARONY_SUPER_MULTIPLAYER=ON
cmake --build build-pr5 -j8 --target barony
```
- Targeted functional checks:
  - Spell/effect ownership at player slots 1, 8, and 15.
  - Non-player owner path remains valid.
- After smoke lanes exist (PR7+): run save/reload compatibility lane and attach artifact.

## Acceptance Criteria
- [ ] Owner encoding/decoding is centralized in `status_effect_owner_encoding.hpp`.
- [ ] No owner misattribution is observed at 1/8/15 slot tests.
- [ ] Legacy compatibility path (`EFF_FAST` and sentinel values) remains intact.
- [ ] No build/smoke/mapgen/protocol scope creep.

## Review Focus
- Bit-level compatibility and boundary handling.
- Correct migration coverage across all affected call sites.
- Clear non-player sentinel semantics.

## Rollback Strategy
Revert PR5 independently if regressions appear; no protocol schema migration is introduced.

## Extraction Plan (from PR #940 / 8p-mod)
Prefer clean cherry-pick of `b975122b` and restage only scoped files; exclude long-form docs churn.
