# Example: Linked Panels With Shared Probe And Consolidated Colorbar

This example instantiates a multi-panel scene with shared data, linked interaction, transient
annotations, and one consolidated explanatory object.


## Scene Setup

1. one scene,
2. two 2D panels showing the same scalar field with different view states,
3. one shared `image` visual backed by one scalar `SampledField`,
4. one panel-local `marker` overlay visual for highlighted points of interest,
5. one `LinkedPanelsController` coordinating crosshair and probe behavior,
6. one shared probe annotation state mirrored across both panels,
7. one consolidated scene-shared colorbar attached to the overall two-panel layout,
8. picking and readback enabled for hover probe updates,
9. latest-request-wins hover behavior per source panel.


## Family And Variant

Primary visual families:

1. `image`
2. `marker`

Variant axes:

1. `image` uses scalar-field plus colormap mode,
2. `image` is pick-aware for probe routing,
3. `marker` remains optional and highlight-oriented rather than the primary data carrier,
4. annotations include crosshair guides, probe labels, and one shared colorbar.


## Resource Schema Instance

Scene-facing resources:

1. one source scalar `SampledField` in `DataSpace`,
2. one derived `SampledField` or equivalent image-ready resource in `VisualSpace`,
3. one `StyleBlock` defining the active colormap and domain policy,
4. one source `ItemTable` for optional highlighted marker positions,
5. one derived normalized `ItemTable` for the marker overlay,
6. panel-local picking `DerivedField` resources for each panel,
7. one probe `ReadbackTarget` per panel,
8. panel-local derived annotation resources for crosshair guides and probe labels,
9. one shared derived colorbar resource set for ramp, ticks, and labels.

Logical shared-state requirements:

1. one semantic scalar mapping identity shared by both panels and by the consolidated colorbar,
2. one current hover `request_id` per panel,
3. scene or panel generation data used to reject stale probe results,
4. scene-level probe state that is updated only from current accepted pick results.


## Transform Pipeline

For the shared scalar field:

1. source scalar samples live in `DataSpace`,
2. scene normalization or placement derives an image-ready visual representation in `VisualSpace`,
3. both panels consume the same normalized field resource,
4. each panel applies its own panzoom afterward.

For the marker overlay:

1. marker anchors originate in `DataSpace`,
2. normalization maps them into the same `VisualSpace` as the image,
3. both panels may view the same overlay data through different panel-local transforms.

For linked probe annotations:

1. pointer interaction begins in one panel,
2. picking identifies the relevant scene position or item identity,
3. the request is tagged with panel identity plus current request and generation state,
4. probe state is stored at scene level only after a current result is accepted,
5. crosshair guides and probe labels are derived separately for each panel,
6. the shared colorbar remains viewport-relative and does not follow panzoom.

The important split is:

1. shared data normalization remains scene-level,
2. panel navigation remains panel-local,
3. probe annotation placement is panel-local,
4. colorbar explanation is scene-shared but layout-aware.

The important freshness and aggregation rules are:

1. hover probe results from one panel must not overwrite newer probe state from that same panel,
2. linked updates in the other panel must derive from the accepted current scene-level probe state,
3. the consolidated colorbar is valid only because both panels share the same semantic scalar mapping
   identity.


## FramePlan Shape

Typical steady frame with no hover change:

1. no data upload for the image field,
2. no data upload for markers unless highlight state changed,
3. two `RenderNode` instances for the visible color passes, one per panel,
4. one annotation contribution set for panel A,
5. one annotation contribution set for panel B,
6. one shared annotation contribution set for the consolidated colorbar.

Typical frame during hover or crosshair motion:

1. one picking `RenderNode` for the active source panel when a fresh probe sample is needed,
2. one `ReadbackNode` for the hover result,
3. panel-local annotation updates for both panels,
4. no rebuild of the shared image resource,
5. no colorbar rebuild unless the scalar mapping changed.

Acceptance rule for the hover result:

1. apply it only if the result matches the current `request_id` for the source panel,
2. discard it if the source panel has already issued a newer hover request,
3. discard it if the relevant scene or panel generation changed enough to make the result stale.

Typical frame after colormap-domain change:

1. optional update of the image style or parameter resources,
2. colorbar tick and ramp regeneration,
3. possible redraw of both panels,
4. no mandatory change to the underlying scalar field resource.

The shared colorbar should remain aggregated only while the semantic scalar mapping identity remains
the same across the two panel views.


## DRP2 Categories Implied

1. resource writes for dirty style, marker, or annotation resources,
2. render-pass lifecycle for both visible panels,
3. render-pass lifecycle for panel-local picking when active,
4. draw commands for image, marker, and annotation contributions,
5. copy or readback service path for probe resolution,
6. queue submission.


## Pressure On The Spec

This example checks that:

1. one normalized scene resource can feed several panels without duplication,
2. linked-panel interaction can produce panel-local annotations from shared scene state,
3. picking and probe semantics survive a multi-panel routing path,
4. a consolidated colorbar is treated as an annotation-side semantic object rather than a visual,
5. panel-local transforms do not force regeneration of shared explanatory objects,
6. the scene can keep shared semantics and panel-local layout separate at the same time,
7. stale hover results are safely dropped,
8. shared colorbar aggregation depends on stable mapping identity rather than visual resemblance.
