# Status

- Task: document and apply a cleanup/usage policy for `docs/tasks/`.
- Date: 2026-05-06
- State: completed

## Plan

1. Inspect the existing `docs/tasks/` layout and identify the smallest useful policy/index docs to add. ✅
2. Add repo-facing documentation for task-record lifecycle, statuses, and retention. ✅
3. Validate formatting/diff cleanliness and commit the documentation changes. ✅

## Work completed

- Added `docs/tasks/README.md`.
- Documented:
  - why `docs/tasks/` records are committed,
  - required task-directory structure,
  - roles of `STATUS.md` and `NEXT_STEPS.md`,
  - status conventions,
  - retention / superseded / abandoned guidance,
  - a lightweight current task index.

## Validation

- `git diff --check` ✅

## Current state

- `docs/tasks/` now has an explicit repository-local policy and index.
- Historical task records remain intact; the new README clarifies that this is expected.
