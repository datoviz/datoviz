# Task Records (`docs/tasks/`)

This directory stores durable repository task records.

These files are **intended to be committed** for substantive work on this branch. They are not
scratch notes; they are the primary handoff and execution record for future agents and maintainers.


## Purpose

Each task directory captures:

- what the task was,
- the short execution plan,
- validation performed,
- final current state,
- remaining next steps and risks,
- and any commit references worth following.

This keeps execution history close to the codebase instead of relying on ephemeral chat context.


## Directory format

Create one directory per task:

- `docs/tasks/YYYY-MM-DD-task-slug/`

Each task directory should normally contain:

- `STATUS.md`
- `NEXT_STEPS.md`


## File roles

### `STATUS.md`

Use `STATUS.md` for durable status and summary information:

- task summary,
- plan,
- work completed,
- validation,
- current state,
- commits made when relevant.

### `NEXT_STEPS.md`

Use `NEXT_STEPS.md` for resumability:

- remaining work,
- risks,
- recommended follow-up actions,
- useful resume commands,
- notes for the next agent.


## Status conventions

Prefer one of these task states in `STATUS.md`:

- `completed`
- `in progress`
- `blocked`
- `superseded`
- `abandoned`


## Retention policy

- **Commit** task records for meaningful repository work.
- **Do not delete** historical task directories by default.
- If a later task replaces an earlier one, mark the older task as `superseded` and point to the newer task.
- If a task was started accidentally or contains no meaningful work, `abandoned` is preferred over silent deletion.

Historical task records may describe intermediate states that were later fixed. That is normal; they are
execution history, not a continuously rewritten changelog.


## Recommended maintenance flow

When a substantive task starts:

1. create `docs/tasks/YYYY-MM-DD-task-slug/`,
2. add `STATUS.md` and `NEXT_STEPS.md`,
3. record the initial plan,
4. update the files after meaningful milestones,
5. finalize validation and current state before closing the task.

When a task finishes:

1. make sure `STATUS.md` reflects the final state,
2. make sure `NEXT_STEPS.md` is useful for future follow-up,
3. commit the task record with the corresponding work.


## Current task index

Current tracked task records in this directory:

- `2026-05-06-doc-state-audit` — completed
- `2026-05-06-fix-scene-offscreen-two-panel-failure` — completed
- `2026-05-06-scene-point-and-runtime-hardening` — completed
- `2026-05-06-task-docs-policy` — completed
- `2026-05-06-visual-destroy-live-stream-regression` — completed
- `2026-05-07-fix-scene-background-color-macos-build` — completed
- `2026-05-07-investigate-panzoom-zoom-wheel` — completed
- `2026-05-07-normalize-wheel-sign-backend` — completed
- `2026-05-08-fix-scene-converter-spirv-alignment-warning` — completed
- `2026-05-10-fix-scene-json-buffer-binding-metadata` — completed
- `2026-05-10-fix-scene-mesh-indexed-default-color-emits-draw-indexed` — completed
- `2026-05-13-drp2-trace-logging` — completed implementation, follow-up validation still useful
- `2026-05-13-doc-state-refresh` — completed

Update this index opportunistically when adding or superseding tasks; it does not need to be perfectly exhaustive
for older history, but it should stay accurate for current and recent work.
