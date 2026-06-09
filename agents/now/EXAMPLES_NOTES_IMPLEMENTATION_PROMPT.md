# Examples Notes Implementation Prompt

Status: active handoff. Created: 2026-06-09. Refreshed: 2026-06-09 after
the runtime/readability checkpoint.

Use this prompt for the next agent tasked with closing every suggestion in
`EXAMPLES_NOTES.md`.


## Prompt

You are working in `/home/cyrille/GIT/Viz/datoviz` on `v0.4-dev`. Read
`AGENTS.md`, `agents/now/START.md`, `agents/now/STATUS.md`, and
`agents/now/RELEASE.md` before changing files. Then read `EXAMPLES_NOTES.md`.

Goal: implement, verify, and document closure for the unresolved or partial
ledger entries in `EXAMPLES_NOTES.md`. Most original suggestions were closed by
2026-06-09 commits; do not re-open resolved items unless new evidence shows the
closure is wrong. Do not remove a note or call an item resolved unless there is
concrete evidence in source, tests, generated gallery metadata, or validation
output.

Constraints:

1. Keep the v0.4 architecture. Do not preserve v0.3 compatibility at the
   expense of v0.4 correctness.
2. Do not stage, commit, or push `data` submodule changes or generated/runtime
   binary payloads unless explicitly approved in the current turn.
3. Prefer the scenario runner for examples unless the point of the example is
   the app/window/canvas path that the scenario runner wraps.
4. Keep examples modular and gallery-proof. If a visual complaint exposes an
   underlying runtime bug, fix the underlying implementation and add a focused
   regression where practical.
5. Keep the active runtime path unified:
   scene frame plans -> DRP2 command streams -> vklite runtime ->
   canvas/stream frame execution -> optional app presentation.
6. Validate narrowly while iterating, then run `git diff --check` before
   finalizing. Before any commit, run `git status --short` and
   `git diff --cached --stat`.


## Required Workflow

1. Build an explicit checklist from `EXAMPLES_NOTES.md`, preserving every
   suggestion.
2. For each item, classify it as `done`, `partial`, `needs-code`,
   `needs-design-decision`, or `deferred-with-reason`.
3. Implement all `needs-code` and unblocked `partial` items.
4. For broad or risky items, make logical checkpoint commits after relevant
   checks pass. Do not commit unrelated user changes.
5. Update `EXAMPLES_NOTES.md` as a closure ledger:
   - mark resolved items with the source file and validation used;
   - keep unresolved items visible with the exact blocker or next action;
   - do not silently delete suggestions.
6. Update manifest/docs generated from example metadata when examples are
   renamed, moved, added, or have changed validation requirements.
7. Run the narrowest relevant validations. At minimum run `git diff --check`.
   For example/runtime code, prefer `just build`, focused `just test ...`, and
   targeted example smokes where the environment supports Vulkan.


## Current Unresolved Queue

Treat the following ledger entries as active. Runtime/readability fixes and
example-behavior decisions are now resolved unless later validation proves the
closure wrong.

Resolved in checkpoint `showcases/surface_grid` / `features/bounds_overlay`:

- `showcases/surface_grid`: ported as `examples/c/showcases/surface_grid.c`
  with `dvz_geom_surface_grid()` and a derived wireframe overlay.
- `features/bounds_overlay.c`: ported as diagnostic
  `examples/c/features/bounds_overlay.c` with 2D and 3D retained visual bounds.

Resolved in checkpoint `runtime/readability`:

- `features/controller_fly`: `src/controller/fly.c` now uses the configured
  world-up basis for yaw/pitch, with a z-up drag-sign regression in controller
  tests.
- `features/technique_depth_test`: GLSL point fragment shaders use
  derivative-based disc edge coverage without MSAA.
- `features/gui_viewport`: `dvz_view_resize_scaled()` and GUI viewport runtime
  sizing fixes address Retina framebuffer mismatch, stale initial frames, and
  resize wobble.
- `showcases/choropleth`: Retina title/subtitle readability and fixed-aspect
  right-drag zoom are fixed.
- `showcases/scientific_plotting`: fixed pixel reserves prevent x-axis clipping.
- `showcases/textured_planet`: lighting and orbit-camera zoom limits/speed are
  adjusted.
- `features/gui_cimgui` and `features/gui_controls`: kept as explicit
  app/window GUI-host exceptions.
- `features/input_events`: defaults to a live event-printing GLFW window, with
  `--synthetic` preserved for smoke validation.
- `features/offscreen_capture`: keeps exact output-pixel semantics and asserts
  requested framebuffer/PNG size.

General `EXAMPLES_NOTES.md` comments still to close:

- Improve top-of-file comments/descriptions so they can be reused on example
  webpages.
- Ensure all examples use the scenario helper, with explicit exceptions for
  examples whose purpose is lower-level app/window/canvas/host integration.
- Complete the builtin-shapes parity audit against v0.3: list what v0.3 had,
  what v0.4 now has, what is missing, and whether missing items are added or
  deferred.

Further examples already resolved or audited:

- `showcases/surface_grid`, legacy-like, with wireframes.
- `features/instancing`: current public example is
  `features/selection_mesh_instances`; a plain non-selection instancing example
  remains optional.
- `features/isolines`: current implementation exists in
  `examples/c/features/isolines.c`.
- `raw_triangle_vklite` and `raw_triangle_drp2`: current examples live under
  `examples/c/advanced/`.
- `bounds_overlay.c` legacy port into one feature example.
- `arcball_gizmo.c`: superseded by synchronized inset
  `examples/c/features/orientation_gizmo.c`.


## Closure Criteria

The task is complete only when:

1. Every `EXAMPLES_NOTES.md` suggestion is marked resolved with evidence,
   implemented, or left as an explicit deferred item with rationale and owner.
2. Source, CMake, manifest, docs, capture tooling, and generated example pages
   agree on names and categories.
3. Any runtime bug discovered while improving examples has a focused test or a
   documented manual validation path.
4. `git diff --check` passes.
5. The final report lists:
   - commits made;
   - validations run and their results;
   - examples not visually smoke-tested because of environment constraints;
   - any remaining explicit deferrals.
