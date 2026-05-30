# Example Overhaul Execution

> **Status:** active execution guide
> **Scope:** final v0.4 C example migration, old-example handling, and per-example workflow

Use this file for the migration loop only. Canonical example staging lives in
[PLANNING.md](PLANNING.md), source layout and metadata live in [ORGANIZATION.md](ORGANIZATION.md),
shared data/cache rules live in [POLICIES.md](POLICIES.md), and visual capture defaults live in
[STYLE.md](STYLE.md).


## Decisions

1. Work one example at a time, in the pickup order from [PLANNING.md](PLANNING.md).
2. Do a small structure and metadata pass before broad rewrites, but do not polish many examples in
   parallel.
3. Follow the current C tree reset from [ORGANIZATION.md](ORGANIZATION.md): public `features`,
   `visuals`, and `showcases`, plus flat non-public `lab` and temporary `legacy`.
4. Treat old v0.4-dev examples as source material, not the final catalog.
5. Promote only examples that map to a scenario ID, visual-family requirement, feature requirement,
   or gallery/showcase target. Keep diagnostics, stress, and low-level work in `lab` unless promoted.
6. Do not delete temporary examples during the first migration pass unless they are duplicated by a
   validated replacement. Prefer quarantining or de-indexing first.


## Old Example Handling

Classify each existing file before moving or rewriting it.

| Class | Action |
| --- | --- |
| Promoted release proof | Move or rewrite into the final lane and attach the canonical scenario ID. |
| Visual-family baseline | Keep under `examples/c/visuals/` only if it demonstrates one active visual family with minimal unrelated features. |
| Feature proof | Move to `examples/c/features/` when the example is about axes, colorbars, scale bars, annotations, overlays, picking, probing, selection, panels, updates, or controllers. |
| Rendering technique | Move to `examples/c/lab/` or `examples/c/legacy/` unless it is rewritten as a polished public teaching example. |
| Showcase | Keep under `examples/c/showcases/` only when it is composed, polished, and gallery-facing. |
| Runtime or low-level tool | Move to flat `examples/c/lab/` when still useful, otherwise to `examples/c/legacy/`. |
| Temporary lab, stress, or diagnostic | Keep buildable in flat `examples/c/lab/` when useful; otherwise archive under `examples/c/legacy/`. |
| Superseded duplicate | Remove only after the replacement builds, runs, and has equivalent or better validation. |

Useful historical promotion candidates include `protein.c`, `lidar.c`, `brain.c`, `ibl_brain.c`,
`labels.c`, `scatter_axes.c`, `image_probe.c`, `linked_panels.c`, scale-bar examples,
visual-family baselines, and WebGPU fixtures. Treat names as historical clues; scenario ID and
release role decide final location.


## Per-Example Loop

1. Choose the scenario or lane target from [PLANNING.md](PLANNING.md) and
   [EXAMPLE_COVERAGE.md](../../docs/EXAMPLE_COVERAGE.md).
2. Classify the old example as promote, rewrite, quarantine, keep as a tool, or remove after
   replacement.
3. Define the smallest useful scene, data shape, interaction state, and validation result before
   editing code.
4. Apply the relevant [STYLE.md](STYLE.md) preset and metadata from
   [ORGANIZATION.md](ORGANIZATION.md).
5. Implement or migrate one public idea per file unless it is explicitly a showcase.
6. Make it smoke-testable with bounded frame counts, offscreen/capture paths, deterministic data,
   and optional-backend gates.
7. Capture or validate. For gallery examples, verify a nonblank capture. For interaction examples,
   include a bounded manual or automated check.
8. Update metadata and indexes.
9. Run `git diff --check` and the narrowest relevant example/build/test path.
10. Record follow-ups explicitly instead of hiding them in code comments or leaving the example
    half-promoted.


## Stop Conditions

Stop and update the plan before continuing if:

1. a required example exposes a missing public API or runtime feature;
2. the old example cannot be preserved without changing its behavior;
3. the example needs an asset without recorded source, license, size budget, and cache path;
4. graphics-backend validation cannot be reproduced locally;
5. migration would delete useful diagnostic coverage before a replacement exists.
