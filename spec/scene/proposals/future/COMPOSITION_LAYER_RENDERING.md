# Composition-Layer Rendering Plan

Status: deferred until after v0.4.0.

This note records the architectural fix for visual layering across opaque and transparent render
passes. It is motivated by the v0.4 `features/user_scale` example, where an alpha-blended axis grid
is intended to sit behind data (`z_layer = -10`) but renders after an opaque path because blended
draws are emitted after opaque draws.


## Problem

Scene attachment order currently uses `z_layer` and insertion order inside the panel visual list.
Render planning then classifies visuals by alpha mode into opaque, blended, WBOIT, or depth-peel
passes. This makes alpha mode stronger than layer intent.

The failure mode is:

1. Axis grids are generated visuals with underlay intent and `z_layer = -10`.
2. Graphite-cyan grid style uses alpha below 255.
3. Axis preparation changes the grid visual to `DVZ_ALPHA_BLENDED`.
4. Blended passes execute after the opaque data pass.
5. The grid overlays opaque data even though its generated-visual policy says it is behind data.

This is not a user-scale-specific problem. Any translucent generated underlay, guide fill, reference
decoration, or future panel chrome can hit the same pass-order contradiction.


## Goal

Make high-level composition intent explicit and preserve it across pass planning.

Required behavior:

1. Underlays remain behind ordinary data even when they are alpha-blended.
2. Ordinary data keeps current opaque/transparent behavior unless explicitly attached elsewhere.
3. Overlays remain above ordinary data.
4. `z_layer` remains meaningful within a composition band.
5. The design remains backend-neutral and lowers through the existing scene -> FramePlan -> DRP2
   path.

Non-goals for the first slice:

1. General order-independent transparency for every possible interleaving of opaque and blended
   geometry.
2. Public API exposure unless a concrete need appears.
3. Family-specific render shortcuts.


## Proposed Model

Add an internal composition layer concept:

```c
typedef enum DvzSceneCompositionLayer
{
    DVZ_SCENE_COMPOSITION_UNDERLAY = 0,
    DVZ_SCENE_COMPOSITION_DATA = 1,
    DVZ_SCENE_COMPOSITION_OVERLAY = 2,
} DvzSceneCompositionLayer;
```

Generated visual policies should declare their intended composition layer. Suggested defaults:

| Role | Layer |
| --- | --- |
| Panel background | Underlay |
| Guide fill | Underlay |
| Axis grid | Underlay |
| Data default | Data |
| Guide line/outline | Overlay or Data, depending on final guide semantics |
| Axis marks/text | Overlay |
| Colorbar/legend/scalebar/overlay-card roles | Overlay |
| Bounds overlay | Overlay |

Ordinary `dvz_panel_add_visual()` attachments default to `DATA`.

The effective draw ordering key becomes:

```text
composition_layer, z_layer, insertion_index
```

Render pass classification still uses alpha mode, but only after composition layer has placed a
visual into the underlay, data, or overlay band.


## Emission Strategy

Panel emission should emit composition bands in this order:

1. Underlay opaque and underlay blended.
2. Data opaque and data transparent/WBOIT/depth-peel.
3. Overlay opaque and overlay blended.

The first implementation should support underlay and overlay source-over blended passes. WBOIT and
depth-peeling can remain data-layer techniques until there is a concrete underlay/overlay use case.

For alpha-blended underlays, emit a pre-data source-over pass that writes into the panel color
target before the data opaque pass. This preserves translucent grid/fill appearance without forcing
style code to preblend colors against a known background.


## Implementation Steps

1. Add the internal enum and a field to generated visual policy.
2. Add composition layer to `DvzPanelAttach` or a parallel internal attachment-resolved record.
   Prefer keeping the public `DvzVisualAttachDesc` unchanged for the first slice.
3. Resolve generated visual roles to composition layers in `generated_visual_policy.h`.
4. Extend `DvzPanelRenderVisualPlan` with composition layer.
5. Bucket panel render planning by composition layer before alpha-mode classification.
6. Emit underlay blended passes before data opaque passes.
7. Emit overlay blended passes after data passes.
8. Keep existing WBOIT/depth-peel paths data-layer-only unless tests force broader support.
9. Preserve the current `z_layer` and insertion-order behavior within each composition layer.


## Validation

Add focused frame-plan tests:

1. A translucent axis grid is planned before an opaque path.
2. An ordinary blended marker is planned after the grid and after the path.
3. Axis marks and text remain overlay visuals.
4. Existing generated visual z-layer policies still order correctly within their layer.
5. Toggling grid alpha between 255 and below 255 does not move the grid above data.

Add focused runtime/example validation:

```sh
just test axis
just test scene_techniques
just example-c features/user_scale
./build/examples/c/features/user_scale --png --user-scale 1.4
git diff --check
```

When the environment supports live Vulkan validation, also run the live `features/user_scale` quit
smoke and confirm no validation-layer teardown regressions.


## Interim Workaround

Do not use preblended colors as the architectural fix. Preblending a translucent grid against a
known panel background can improve one example, but it encodes render-order policy in style code and
breaks for other backgrounds, panels, and future composition targets.

