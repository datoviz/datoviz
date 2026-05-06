# Status

- Task: audit documentation and agent-facing files against the current code state after the scene panel/runtime fixes.
- Date: 2026-05-06
- State: completed

## Plan

1. Inspect the main repo-facing docs and agent files for claims about current scene/DRP2/runtime behavior. ✅
2. Compare those claims with the implemented multi-panel runtime behavior and current validation state. ✅
3. Summarize recommended doc updates, including whether README / AGENTS / task records should change. ✅
4. Apply the minimal high-value doc sync. ✅

## Findings summary

### Clear high-priority updates

- `AGENTS.md` was stale on current validation counts (`just test scene` still said `34/34`; current state is `52/52`).
- `agents/now/V0_4_NEXT_STEPS.md` had the same stale scene-suite count and was missing the panel-region/runtime rendering update.
- `ARCHITECTURE.md` was directionally correct, but undersold the implemented point-path surface area (multi-panel rendering, retained render behavior, panel-region handling).

### README status

- `README.md` was mostly fine as a public/release-facing overview.
- A light update to the v0.4 status/roadmap wording improved alignment without turning it into an internal architecture document.

### Larger branch-vs-release docs mismatch

- `docs/reference/api_c.md` appears substantially v0.3-oriented and does not match the active v0.4 scene API shape.
- `docs/guide/common.md` and related guide pages also look release/public-API oriented rather than current v0.4-dev source-of-truth docs.
- If `docs/` is meant to describe the current branch, it needs a broader refresh; if it is meant to stay release-oriented, it should likely carry a clearer note about that.

## Work completed

Updated the minimal sync set:

1. `AGENTS.md`
2. `agents/now/V0_4_NEXT_STEPS.md`
3. `ARCHITECTURE.md`
4. `README.md` (light-touch status/roadmap refresh)

The updates:

- refreshed `just test scene` from `34/34` to `52/52`,
- documented the active multi-panel/runtime panel-region behavior,
- clarified that the current active slice is point-based but no longer just a trivial single-panel path.

## Validation

- `git diff --check` ✅

## Current state

- Main agent-facing branch docs now match the current validated scene/DRP2/runtime state much better.
- The broader `docs/guide/*` and `docs/reference/*` trees still look release-oriented and remain a separate follow-up decision.
