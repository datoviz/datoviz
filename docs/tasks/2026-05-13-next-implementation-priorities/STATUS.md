# Status

- Task: record concrete implementation briefs for the next six v0.4 priority lanes.
- Started: 2026-05-13
- State: completed planning record

## Scope

This task records implementation-level next steps for:

1. native 3D/manual smoke slice,
2. narrow WebGPU/DRP2 feasibility spike,
3. targeted hygiene/static-analysis pass,
4. manual interactive smoke coverage,
5. rendered text/colorbar/annotation realization,
6. selective visual-family expansion.

## Current Decision

The next branch work should not treat these as one large mixed refactor. They should be separate
lanes with separate validation gates.

Recommended priority:

1. Native 3D/manual smoke first.
2. Manual interactive smoke and targeted hygiene close behind.
3. WebGPU feasibility early, while the implemented visual set is still small.
4. Rendered text/colorbars/annotations after the runtime path has been pressure-tested.
5. Additional visuals only after native 3D and WebGPU constraints have exposed contract gaps.

## Validation

Planning-only task. Validation for this record:

```bash
git diff --check
```

## Source Of Truth

Detailed lane checklists live in:

```text
docs/tasks/2026-05-13-next-implementation-priorities/NEXT_STEPS.md
```
