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

The native 3D example is now represented by `examples/c/visuals/mesh.c`, and the manual scene
smoke matrix is recorded in `docs/architecture/manual_scene_smoke.md`. The `hello_*` C example
smoke set was reported successful on `2026-05-14`, including the paired offscreen capture examples
that cover the readback side of the mesh smoke path. The next implementation work should therefore
move to the targeted hygiene/static-analysis pass unless a specific manual smoke gap is promoted
first.

Recommended priority:

1. Continue the targeted hygiene/static-analysis pass over the hot scene/DRP2/app paths. The first
   `2026-05-14` slice hardened DRP2 texture-transfer layout overflow validation and image-probe
   plan byte-size checks; the second slice hardened borrowed frame-target depth attachment
   retirement.
2. Add live image-probe, multi-panel, or partial texture-update examples only if manual validation
   needs them before more API work.
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
