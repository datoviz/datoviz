# Examples Notes Implementation Prompt

Status: active handoff. Created: 2026-06-09. Refreshed: 2026-06-09 after
today's example-polish commits.

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

Treat the following ledger entries as active. Everything else in
`EXAMPLES_NOTES.md` is already resolved or audited unless later validation
proves otherwise.

Runtime or visual fixes:

- `features/controller_fly`: investigate left-right drag in `src/controller/fly.c`;
  fix if broken and add a focused sign regression or manual validation note.
- `features/technique_depth_test`: diagnose shader edge antialiasing without
  relying on MSAA; fix or record the shader limitation precisely.
- `features/gui_viewport`: fix Retina logical/framebuffer mismatch, initial
  zoomed viewport frames, and resize lag/wobble in the GUI viewport runtime.
- `showcases/choropleth`: improve Retina title/subtitle readability and make
  right-drag zoom preserve fixed aspect.
- `showcases/scientific_plotting`: fix x-axis clipping at the panel bottom.
- `showcases/textured_planet`: lighten dark-side lighting enough to inspect and
  add sensible zoom limits or less sensitive/log-like zoom behavior.

Example behavior and policy:

- `features/gui_cimgui`: keep the app/window path only if it remains a justified
  GUI-host exception; otherwise move to the scenario path.
- `features/gui_controls`: decide whether builtin controls are still a public
  v0.4 feature and keep or retire the example accordingly; app/window path is
  justified only for GUI host integration.
- `features/input_events`: prefer a live window that prints keyboard and mouse
  events in real time; keep synthetic emitted-event coverage as test/smoke
  behavior if useful.
- `features/offscreen_capture`: keep exact-output-pixel semantics unless an
  explicit render-scale/supersample option is added; record the decision in the
  example/docs.

Unblocked example ports:

- `showcases/surface_grid`: port a v0.4 showcase from the legacy surface-grid
  examples, using `dvz_geom_surface_grid()` and a visible wireframe overlay.
- `features/bounds_overlay.c`: port the legacy diagnostic bounds overlay into
  one focused v0.4 feature example, or split only if the legacy behavior is
  genuinely two separate concepts.

General `EXAMPLES_NOTES.md` comments to close:

- Improve top-of-file comments/descriptions so they can be reused on example
  webpages.
- Ensure all examples use the scenario helper, with explicit exceptions for
  examples whose purpose is lower-level app/window/canvas/host integration.
- Complete the builtin-shapes parity audit against v0.3: list what v0.3 had,
  what v0.4 now has, what is missing, and whether missing items are added or
  deferred.

Further examples to add or explicitly defer:

- `showcases/surface_grid`, legacy-like, with wireframes.
- `features/instancing`, single mesh instanced multiple times with different
  transforms.
- `features/isolines`, if the current implementation is insufficient after
  audit.
- `raw_triangle_vklite` and `raw_triangle_drp2`, if the current advanced
  examples are insufficient after audit.
- `bounds_overlay.c` legacy port into one or two feature examples.
- `arcball_gizmo.c` legacy port as a feature example with a small bottom-right
  inset synchronized with a main-panel mesh.


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
