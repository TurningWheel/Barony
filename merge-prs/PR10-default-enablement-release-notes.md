# [PR10] Default Enablement Flip and Release Notes

## Ticket Metadata
- Type: Engineering
- Priority: High
- Epic: Multiplayer Expansion 1-15
- Risk: Medium
- Depends On: PR1-PR9 complete and gated
- Blocks: Release readiness

## Background
Through PR9, 1-15 support is intentionally scaffolded behind defaults to reduce blast radius. The final step is enabling the new path by default only after all stability and balance gates are satisfied.

## What and Why
Flip `BARONY_SUPER_MULTIPLAYER` default ON and publish concise operator/reviewer release notes once all technical gates are green.

## Scope
### In Scope
- One-line default flip in `CMakeLists.txt` (`BARONY_SUPER_MULTIPLAYER`)
- Concise release-facing notes in `README.md` and/or `INSTALL.md` (if needed)

### Out of Scope
- Any gameplay/network/mapgen logic changes
- Smoke framework/script changes
- Additional refactors

## Implementation Instructions
1. Confirm all prior PR gates are complete and documented.
2. Change only the default value of `BARONY_SUPER_MULTIPLAYER` in `CMakeLists.txt`.
3. Update docs with short release notes:
   - 1-15 support now default-enabled.
   - Splitscreen remains capped at 4.
   - Smoke flags remain optional and compile-gated.
4. Keep this PR intentionally tiny and reviewable.

## Suggested Commit Structure
1. Default flip commit (`CMakeLists.txt` only).
2. Optional short release note commit (`README.md`/`INSTALL.md`).

## Validation Plan
- Full CI pass.
- Smoke sanity:
  - Baseline 4p lane.
  - 15p lane.
- Manual startup sanity with default settings.

## Acceptance Criteria
- [ ] `BARONY_SUPER_MULTIPLAYER` default is ON.
- [ ] No logic changes outside default/config/docs are present.
- [ ] CI is fully green.
- [ ] 4p and 15p smoke sanity lanes pass.
- [ ] Splitscreen cap remains 4.

## Review Focus
- Tiny scoped diff.
- Correct default and clear docs.
- No accidental extra behavior changes.

## Rollback Strategy
Flip the default back OFF with a single revert commit if post-merge telemetry/regression indicates risk.

## Extraction Plan (from PR #940 / 8p-mod)
Do not cherry-pick mixed historical commits; create a fresh commit on top of the merged stack.
