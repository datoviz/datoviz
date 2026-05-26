# Scene Napari Image And Labels Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining 2D image/label work needed for napari-class layer rendering.


## Current State

Durable behavior and pressure-test material now lives in specialized spec files:

1. [`../../../spec/scene/visuals/IMAGE.md`](../../../spec/scene/visuals/IMAGE.md) owns the `image`
   visual contract, current implementation status, scalar/heatmap direction, contours, and image
   probing caveats.
2. [`../../../spec/scene/integration/napari/NAPARI.md`](../../../spec/scene/integration/napari/NAPARI.md)
   owns the future napari-backend integration boundary.
3. [`../../../spec/scene/examples/napari/LARGE_LABELS_SEGMENTATION.md`](../../../spec/scene/examples/napari/LARGE_LABELS_SEGMENTATION.md)
   owns the labels-demo dataset, preprocessing, shader intent, and staged acceptance criteria.
4. [`../../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md`](../../../spec/scene/validation/IMAGE_PICKING_RECOVERY.md)
   owns the image probe and segmentation-hover recovery guardrails.

The active v0.4 code already has retained `SampledField` resources, 2D image visuals backed by
sampled fields, image dirty-region uploads, z-layered panel attachment, scale/colormap bookkeeping,
categorical scale/legend bookkeeping and rendering, first-class `dvz_labels()` integer sampled-field
rendering, label GLSL/WGSL shader variants, selected-label/boundary/hidden-label styling, and image
probe/readback plumbing. Scalar fields are still commonly colorized through a staged RGBA path, and
raw label-ID GPU probing remains a follow-up.

Use this file only for execution sequencing. Do not duplicate napari or image visual semantics here.


## Remaining Image And Labels Work

Recommended follow-up commits:

1. Add native shader-side scalar image lookup: raw scalar texture, contrast/gamma params, colormap
   palette texture, opacity, and raw-value probe semantics.
2. Add raw label-ID GPU probing so hover/click readout does not rely on CPU coordinate lookup.
3. Preserve label IDs as integer sampled-field data; labels must use nearest sampling or texel
   fetch, with label `0` transparent by default.
4. Continue broadening label palette/hash color, selected-label-only, contour, opacity, hidden-ID,
   and background-id params as small parameter or palette updates rather than full texture rewrites.
5. Keep napari-style blend modes separate from alpha modes: source-over, additive, minimum, opaque,
   and no-depth translucent behavior should be explicit 2D layer-compositing policy.
6. Harden image and label probes so readback returns semantic raw values, data coordinates, visual
   identity, and latest-request-wins hover behavior.
7. Treat N-D slicing and thick-slice projection as adapter-owned for the first napari path; Datoviz
   should receive display-ready 2D fields and apply validated full or region updates.
8. Keep 3D volume and 3D labels work in
   [`SCENE_VOLUME_RENDERING_FOLLOWUP.md`](SCENE_VOLUME_RENDERING_FOLLOWUP.md).


## Example Pressure Tests

Recommended example order:

1. scalar image colormap with contrast/gamma/opacity controls;
2. integer labels overlay with categorical legend, shader-side selection/boundary styling, and
   temporary CPU hover/click lookup, matching `examples/c/showcase/labels.c`;
3. label-id GPU probe/readback;
4. multi-layer image stack with napari-style blend-mode controls;
5. dirty-tile or multiscale level-switching smoke after sampled-field region updates are stable.

The large-label segmentation pressure test should continue to use the dataset and validation policy
from `spec/scene/examples/napari/LARGE_LABELS_SEGMENTATION.md`.


## Validation

For docs-only edits:

```text
git diff --check
```

For image/label implementation work:

```text
just build
just test scene
just test drp2
git diff --check
```

For shader/runtime label work, also run a bounded offscreen or GLFW smoke and, when WGSL is touched,
the WebGPU fixture/preflight path.
