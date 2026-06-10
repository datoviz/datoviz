# EXAMPLES_NOTES 2026-06-10 Implementation Plan

Status: ready for implementation. Created: 2026-06-10.

This handoff covers the current working-tree `EXAMPLES_NOTES.md` review list. It is separate from
`EXAMPLES_NOTES_IMPLEMENTATION_PROMPT.md`, which closed the older 2026-06-09 ledger.


## Starting Context

Before implementing, read:

1. `AGENTS.md`;
2. `agents/now/START.md`;
3. `agents/now/STATUS.md`;
4. `agents/now/RELEASE.md`;
5. `spec/scene/README.md`;
6. `spec/scene/examples/V04_INTERACTION_AND_SHOWCASE_PLAN.md`;
7. `spec/scene/api/REFERENCE_GRID.md`;
8. current `EXAMPLES_NOTES.md`.

Current `EXAMPLES_NOTES.md` has 32 top-level bullets. Treat `features/technique_edl`, nested under
`features/technique_depth_cue`, as its own work item. Some bullets contain multiple atomic fixes;
split them while tracking implementation progress.


## Completed Or Excluded From This Batch

Recent commits already cover these active-plan items. Do not reopen them unless validation reveals a
regression:

1. `549950678 Clamp small retained app layouts`: small-window logical layout clamp and tests.
2. `3e7c46b45 Synchronize GUI viewport resize display`: GUI viewport resize display policy.
3. `0b61550a0 Polish bounds and geometry examples`: bounds overlay padding, `panel_domain_fit`, and
   `triangulation_polygon`.
4. `41955ff3c Document reference grid API direction`: durable reference-grid API direction.

The remaining `features/reference_grid` example work in this batch is only to put the cube on the
plane. Do not implement infinite/fog/reference-grid API behavior until that API is deliberately
scheduled from `spec/scene/api/REFERENCE_GRID.md`.


## Required Commit Sequence

Use one logical checkpoint commit per section below. Do not stage unrelated dirty files. Always run
`git diff --check` before each commit and final handoff. For code batches, run the narrowest
relevant tests first, then broader checks as needed.


### 1. Tracking Checklist

Create a local tracking checklist from the current `EXAMPLES_NOTES.md`.

Required content:

1. preserve every current top-level bullet;
2. split multi-issue bullets into atomic items;
3. mark recent-commit exclusions from this handoff;
4. mark `features/technique_edl` as a separate item even though it is nested in the notes;
5. record validation expectations per group.

Suggested commit:

```sh
git commit -m "Track June examples cleanup batch"
```


### 2. Shared Interaction Fixes

Implement these before visual polish because multiple examples depend on them.

Panzoom:

1. `showcases/choropleth`: aspect-fixed right-drag zoom currently treats the in/out boundary like
   `x = 0` by deriving vertical shift from horizontal shift only.
2. Change aspect-locked right-drag behavior so the natural boundary is `y = -x`.
3. Zoom in when dragging right or up.
4. Add focused `panzoom_arcball` coverage.

Guide labels:

1. `features/guide_lines` and `features/guide_spans` labels are currently data-space labels.
2. Replace that with screen-space or panel-space label placement so labels keep constant size and
   sensible offsets during zoom.
3. Keep guide geometry in data space.
4. Add focused interaction/text placement tests.

Click selection:

1. `features/picking`, `features/selection_pixel`, and `features/selection_mesh_instances`
   currently apply the last hover query on click.
2. Click should query/apply the clicked position, not stale hover state.
3. Background click should clear selection.
4. Respect asynchronous query semantics from the portable scenario runner.
5. Add tests proving stale hover cannot select the wrong item.

Suggested commit:

```sh
git commit -m "Fix shared example interaction behavior"
```


### 3. 3D Context Polish

Use the existing finite `DvzReferenceGrid` API.

Items:

1. `features/animation_tracks`: add XZ reference grid.
2. `features/controller_arcball`: add XZ reference grid.
3. `features/controller_orbit_camera`: add XZ reference grid.
4. `features/controller_turntable`: add XZ reference grid.
5. `features/controller_fly`: reduce left-drag look sensitivity using `DvzFlyDesc.look_speed`.
6. `features/reference_grid`: place the cube directly on the grid plane.

Suggested commit:

```sh
git commit -m "Polish 3D controller example context"
```


### 4. Linked Panel And Technique Examples

Use existing scene controller links where possible. `dvz_controller_link()` currently supports
panzoom and arcball links. It does not support orbit-camera links, so prefer linked arcballs for
examples that use panel camera plus arcball transforms.

Items:

1. `features/lighting`: add panel legends, widen sphere spacing, make the initial camera much less
   zoomed in, and link panel arcballs.
2. `features/material_mesh`: use different panels with the same mesh and different materials, linked
   arcballs, and panel legends.
3. `features/technique_depth_cue`: add linked arcball comparison and panel labels.
4. `features/technique_edl`: add linked arcball comparison and panel labels.
5. `features/technique_msaa`: replace separate arcballs with linked panels.
6. `features/technique_ssao`: tune SSAO parameters, increase blur as needed, and add arcball.
7. `features/technique_transparency`: investigate why the first two panels are empty. Treat this as
   a possible technique/render-path bug before reducing it to example polish.

Suggested commit:

```sh
git commit -m "Improve linked technique examples"
```


### 5. Example Visual Polish

Mostly independent visual/readability fixes.

Items:

1. `showcases/brain_volume`: change default view to the other side.
2. `showcases/linked_probe_colorbar`: improve top-left text contrast, padding, and clipping
   distance.
3. `showcases/surface_grid`: inspect and tune bottom-grid lighting/material.
4. `showcases/textured_planet`: automatic camera motion should resume or continue after mouse
   interaction instead of stopping permanently.
5. `composites/polygon`: use fixed/equal aspect and ensure all polygons are initially visible.
6. `features/annotation_readout`: increase label/readout offset so the first character is visible
   and there is no point overlap.
7. `features/axis_labels`: remove or make intentional the small bordered-panel appearance.
8. `features/builtin_shapes_2d`: enforce fixed/equal aspect.
9. `features/builtin_shapes_3d`: rebalance object layout for symmetry.
10. `features/gui_controls`: show more Datoviz GUI wrapper capabilities. Prefer existing wrappers in
    `examples/c/example_gui_controls.c`.
11. `features/overlay_card`: replace the odd visual with a simple curve or similarly clear signal.
12. `features/visibility`: toggle visibility on and off at 2 Hz.
13. `features/video_export`: plain run should record a short video and stop automatically. Prefer
    an example-specific runner/default override rather than changing all scenario defaults.

Suggested commit:

```sh
git commit -m "Polish reviewed public examples"
```


### 6. Graph Content Decision And Implementation

Do not treat `composites/graph` as ordinary polish.

Direction:

1. Use a deterministic scientific graph rather than a generic social graph.
2. Preferred examples: small brain connectivity graph or protein interaction network.
3. Keep data and helper structs example-local for now, per
   `spec/scene/proposals/future/GRAPH_NETWORK_DESIGN.md`.
4. Do not promote new public `DvzGraph` API as part of this batch.
5. Include meaningful community labels and stable visual styling.

Suggested commits:

```sh
git commit -m "Design graph example replacement"
git commit -m "Replace graph example content"
```


## Validation

Minimum per documentation-only commit:

```sh
git diff --check
git status --short
```

Minimum per code commit:

```sh
git diff --check
just build
just test <narrow-filter>
```

Recommended focused filters:

1. panzoom: `just test panzoom`
2. guide/text placement: `just test interaction`
3. query and click selection: `just test query`, plus relevant scenario/app tests
4. controller links: `just test panzoom_arcball`
5. scene rendering or technique changes: `just test scene`, or narrower technique/app filters when
   available

For changed public examples:

1. build the example;
2. run the scenario or native runner path where available;
3. capture PNG/smoke frame where environment supports it;
4. update manifest/catalog metadata only when status or behavior changes;
5. keep WebGPU/browser status honest.


## Extremely Short Future-Agent Prompt

Read `AGENTS.md`, `agents/now/START.md`, `STATUS.md`, `RELEASE.md`, `spec/scene/README.md`,
`spec/scene/examples/V04_INTERACTION_AND_SHOWCASE_PLAN.md`, this file, and current
`EXAMPLES_NOTES.md`. Implement the June 10 examples cleanup plan end to end with one logical commit
per section: tracking checklist, shared interaction fixes/tests, 3D context polish, linked
technique examples, visual polish, graph redesign. Do not touch recent completed work unless it
regresses; do not implement reference-grid infinite/fog semantics yet; run narrow tests plus
`git diff --check` before each commit.
