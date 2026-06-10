# EXAMPLES_NOTES 2026-06-10 Checklist

Status: active implementation checklist.

Source: root `EXAMPLES_NOTES.md` review list from 2026-06-10. Keep this file as local execution
state for the June examples cleanup batch; durable API decisions belong in `spec/`.


## Excluded Recent Work

Do not reopen these unless validation reveals a regression:

1. `549950678 Clamp small retained app layouts`.
2. `3e7c46b45 Synchronize GUI viewport resize display`.
3. `0b61550a0 Polish bounds and geometry examples`.
4. `41955ff3c Document reference grid API direction`.

Reference-grid infinite/fog semantics are deferred. This batch may only use the existing finite
reference-grid API and place the cube on the current plane.


## Shared Interaction Fixes

Validation: `git diff --check`, `just build`, focused `just test panzoom`, `just test interaction`,
`just test query` or narrower scenario/query tests when available.

- [x] `showcases/choropleth`: aspect-fixed right-drag zoom boundary should be `y = -x`, not `x = 0`.
- [x] `showcases/choropleth`: aspect-fixed right-drag zoom should zoom in when dragging right or up.
- [x] `features/guide_lines`: keep guide geometry in data space but place labels in screen/panel
  space with constant readable size and stable offsets during zoom.
- [x] `features/guide_spans`: keep guide geometry in data space but place labels in screen/panel
  space with constant readable size and stable offsets during zoom.
- [x] `features/picking`: click selection must query/apply the clicked position instead of stale
  hover state.
- [x] `features/picking`: background click should clear selection.
- [x] `features/selection_pixel`: fix click selection and Y-flip/readback behavior without using
  stale hover state.
- [x] `features/selection_pixel`: background click should clear selection.
- [x] `features/selection_mesh_instances`: fix instance hover/query identity and click selection
  without using stale hover state.
- [x] `features/selection_mesh_instances`: background click should clear selection.


## 3D Context Polish

Validation: `git diff --check`, `just build`, `just test panzoom_arcball` or narrower controller
tests when available, plus changed-example build/smoke where supported.

- [x] `features/animation_tracks`: add an XZ reference grid.
- [x] `features/controller_arcball`: add an XZ reference grid.
- [x] `features/controller_orbit_camera`: add an XZ reference grid.
- [x] `features/controller_turntable`: add an XZ reference grid.
- [x] `features/controller_fly`: reduce left-drag look sensitivity with `DvzFlyDesc.look_speed`.
- [x] `features/reference_grid`: place the cube directly on the existing finite grid plane.


## Linked Panel And Technique Examples

Validation: `git diff --check`, `just build`, focused scene/technique tests where available, plus
changed-example build/smoke where supported.

- [x] `features/lighting`: add panel legends.
- [x] `features/lighting`: widen sphere spacing.
- [x] `features/lighting`: make the initial camera much less zoomed in.
- [x] `features/lighting`: link panel arcballs.
- [x] `features/material_mesh`: show the same mesh in different panels with different materials.
- [x] `features/material_mesh`: link panel arcballs.
- [x] `features/material_mesh`: add panel legends.
- [x] `features/technique_depth_cue`: add linked arcball comparison and panel labels.
- [x] `features/technique_edl`: treat as its own item; add linked arcball comparison and panel
  labels.
- [x] `features/technique_msaa`: replace separate arcballs with linked panels.
- [x] `features/technique_ssao`: tune SSAO parameters, increase blur as needed, and add arcball.
- [x] `features/technique_transparency`: investigate empty first two panels as a possible
  technique/render-path bug before reducing it to example polish.


## Example Visual Polish

Validation: `git diff --check`, `just build`, focused scene/example tests where available, plus
changed-example build/smoke where supported.

- [x] `showcases/brain_volume`: change the default view to the other side.
- [x] `showcases/linked_probe_colorbar`: improve top-left text contrast.
- [x] `showcases/linked_probe_colorbar`: add more top padding and avoid clipping at the top border.
- [x] `showcases/surface_grid`: inspect and tune bottom-grid lighting/material.
- [x] `showcases/textured_planet`: automatic camera motion should resume or continue after mouse
  interaction.
- [x] `composites/polygon`: use fixed/equal aspect.
- [x] `composites/polygon`: ensure all polygons are initially visible.
- [x] `features/annotation_readout`: increase label/readout offset so the first character is visible
  and there is no point overlap.
- [x] `features/axis_labels`: remove the odd small bordered-panel appearance or make it intentional.
- [x] `features/builtin_shapes_2d`: enforce fixed/equal aspect.
- [x] `features/builtin_shapes_3d`: rebalance object layout for symmetry.
- [x] `features/gui_controls`: show more Datoviz GUI wrapper capabilities, preferably reusing
  wrappers from `examples/c/example_gui_controls.c`.
- [x] `features/overlay_card`: replace the odd visual with a simple curve or similarly clear
  signal.
- [x] `features/visibility`: toggle visibility on and off at 2 Hz.
- [x] `features/video_export`: plain run should record a short video and stop automatically, using
  an example-specific default instead of changing all scenario defaults.


## Graph Content Decision And Implementation

Validation: `git diff --check`, `just build`, graph/scene tests where available, plus changed-example
build/smoke where supported.

- [x] `composites/graph`: choose a deterministic scientific graph, preferably brain connectivity or
  protein interaction.
- [x] `composites/graph`: keep helper structs and data example-local.
- [x] `composites/graph`: do not promote a new public `DvzGraph` API in this batch.
- [x] `composites/graph`: include meaningful community labels and stable visual styling.
