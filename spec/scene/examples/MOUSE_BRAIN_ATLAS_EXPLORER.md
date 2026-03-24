# Example: Mouse Brain Atlas Explorer With Transparent Regions And Slice Probe

This example pressure-tests a realistic neuroscience scene with nested anatomical regions, a
transparent enclosing surface, interior region meshes, one slice-like image backed by volumetric
sampling, picking-driven selection, and one linked 2D subplot.


## Scene Setup

1. one scene,
2. one primary 3D panel showing a mouse brain atlas volume and anatomical surfaces,
3. one linked 2D panel showing derived plots that update when the slice position changes,
4. one outer `mesh` visual for the whole-brain surface,
5. one interior `mesh` visual or grouped `mesh` representation for atlas regions,
6. one `image` visual in slice-like mode backed by a volumetric `SampledField`,
7. one 3D camera controller for orbit, pan, and wheel zoom,
8. one selection controller for click-to-select region picking,
9. one hover or probe controller for slice inspection,
10. panel-local annotations for probe readout and optional slice guide plane,
11. scene-owned selection and visibility state for atlas regions,
12. scene-external UI controls such as a tree view, visibility toggles, opacity sliders, and filter
    selectors that mutate scene state but are not themselves scene visuals.


## Family And Variant

Primary visual families:

1. `mesh`
2. `image`

Likely variant axes:

1. outer brain `mesh` uses transparent surface styling,
2. region `mesh` uses per-region color and opacity plus selection-dependent emphasis,
3. `mesh` picking is enabled and should preserve at least region-level identity,
4. `image` uses slice-like mode backed by volumetric sampling,
5. `image` exposes filter-dependent sampled values and coordinate readout,
6. the linked 2D panel may use `path`, `point`, or `image` depending on the derived subplot.


## Resource Schema Instance

Scene-facing resources:

1. one `IndexedGeometry` for the outer brain surface,
2. one `IndexedGeometry` or one grouped mesh-oriented resource set for atlas regions,
3. one volumetric `SampledField` for the source scalar or multi-channel volume,
4. one `StyleBlock` for outer surface appearance,
5. one `StyleBlock` or equivalent parameter resource for per-region visibility, opacity, and
   selection emphasis,
6. one `StyleBlock` for slice placement, filter choice, and color interpretation,
7. one or more panel-local picking `DerivedField` resources,
8. one or more `ReadbackTarget` objects for click selection and probe sampling,
9. one panel-local annotation resource set for probe labels, crosshair, or slice-plane guides,
10. one resource set for the linked 2D subplot driven by the current slice state.

Logical scene state:

1. one stable atlas-region identity space independent of draw batching,
2. one region hierarchy model used by the external tree view,
3. one scene-owned visibility state per region or per hierarchy node,
4. one scene-owned opacity override per region or per hierarchy node,
5. one scene-owned selected-region identity,
6. one current slice-placement state,
7. one current volume-filter selection,
8. one current probe state containing panel id, world or atlas coordinates, and sampled value,
9. one request or generation tracking state for stale-result rejection.


## Transform Pipeline

For atlas meshes:

1. mesh geometry originates in atlas or world-aligned `DataSpace`,
2. scene normalization places outer surface and region meshes into one shared 3D `VisualSpace`,
3. panel-local camera transforms apply afterward.

For the slice image:

1. the volume lives in volumetric `DataSpace`,
2. the selected slice plane is scene-owned semantic state,
3. slice sampling remains an `image`-family mode over volumetric sampling,
4. the placed slice image exists in the same 3D `VisualSpace` as the meshes,
5. panel-local camera transforms apply afterward.

For the linked subplot:

1. the slice state or probe state drives a derived scene resource for the secondary panel,
2. the linked 2D panel consumes that derived resource through its own panel-local transform,
3. changing the 3D camera does not require rebuilding the linked 2D subplot resource.

The important splits are:

1. region hierarchy, selection, visibility, opacity, slice placement, and filter choice are
   scene-owned semantic state,
2. the tree view and sliders are external UI that mutate scene state rather than scene primitives,
3. camera navigation is panel-local controller state,
4. slice sampling semantics remain distinct from panel viewing transforms,
5. linked-panel updates derive from accepted scene state rather than directly from backend events.


## FramePlan Shape

Typical steady frame:

1. no geometry upload if the atlas meshes are unchanged,
2. optional style or parameter upload when one region visibility, opacity, selection state, slice
   placement, or filter changes,
3. one `RenderNode` for the main 3D panel color pass,
4. optional additional render or composition nodes if the chosen transparency path requires them,
5. one `RenderNode` for the linked 2D subplot panel,
6. optional annotation contributions for the probe label and slice guide plane.

Typical frame during click selection:

1. one picking `RenderNode` for the 3D panel when fresh selection resolution is needed,
2. one `ReadbackNode` for the clicked pixel,
3. scene-level interpretation back to stable region identity,
4. style or parameter updates to emphasize the selected region and de-emphasize the others,
5. redraw of the affected panel or panels.

Typical frame during slice probing:

1. one picking or probe-oriented `RenderNode` and `ReadbackNode` when the clicked or hovered slice
   location needs coordinate and sampled-value resolution,
2. the result is applied only if request and generation data still match current scene state,
3. probe annotations update in the 3D panel,
4. linked 2D subplot resources may update if the interaction model requires it,
5. camera-only motion does not force mesh or volume resource rebuild.


## DRP2 Categories Implied

1. resource writes for mesh styles, region visibility state, slice parameters, filter parameters,
   annotations, and linked subplot resources,
2. render-pass lifecycle for the main 3D panel,
3. render-pass lifecycle for the linked 2D panel,
4. optional extra render or composition stages for transparency,
5. render-pass lifecycle for picking or probe passes when active,
6. draw commands for mesh, image, annotation, and subplot contributions,
7. copy or readback service path for selection and probe results,
8. queue submission.


## Pressure On The Spec

This example checks that:

1. a realistic scientific scene can combine `mesh` and slice-like `image` semantics in one 3D panel,
2. stable region identity survives batching and round-trips through picking,
3. scene-owned selection state can drive per-region opacity and highlight policy cleanly,
4. external UI widgets such as tree views and sliders can remain outside scene while still mutating
   scene-owned semantic state,
5. linked secondary panels can update from slice state without coupling their resources to 3D camera
   motion,
6. slice interaction can return world or atlas coordinates plus sampled value as scene-level probe
   semantics rather than backend payload leakage,
7. transparency requirements are visible as capability-sensitive planning pressure without forcing
   the scene spec to adopt backend-shaped OIT vocabulary too early,
8. the current `mesh` family likely needs a clearer grouped-identity or region-identity story when
   one visual represents many selectable anatomical regions,
9. the current `image` picking contract likely needs an explicit coordinate-readout expectation for
   slice-like modes,
10. the scene spec should eventually state more clearly how filter selection affects derived-field
    caching and invalidation for volumetric sampling workflows.
