# Scene Implementation Slices

This directory turns mature scene semantics into implementation-ready work packets.

The rest of `spec/scene` defines the model. These files define narrow slices that should be
possible to implement, test, and review without reopening the broad design.


## Authority

Slice documents are implementation plans, not new semantic sources of truth.

Use this authority order:

1. specialized semantic, visual, interaction, pipeline, and validation specs define behavior,
2. installed public headers define names and signatures that already exist,
3. these slice documents define the next concrete implementation boundary,
4. proposals remain rationale and backlog material.

If a slice conflicts with a normative spec, update the normative spec first and then adjust the
slice.


## Files

1. [TEXT_RENDERING_SLICE.md](TEXT_RENDERING_SLICE.md): first retained text/glyph rendering path.
2. [ANNOTATION_LABEL_SLICE.md](ANNOTATION_LABEL_SLICE.md): first rendered label annotation path.
3. [COLORBAR_RENDERING_SLICE.md](COLORBAR_RENDERING_SLICE.md): first rendered continuous colorbar.
4. [LEGEND_SLICE.md](LEGEND_SLICE.md): deferred categorical/discrete legend boundary.


## Feature Readiness Matrix

| Feature | Specified | API declared | Retained state | Rendered | Pickable | Exported | Focused tests | Next slice |
|---|---|---|---|---|---|---|---|---|
| Text object | yes | transitional `DvzVisual* dvz_text()`; semantic `DvzText` target | visual-backed text state | first rendered slice active | no | raster capture only | text realization, atlas growth, runtime readback, app/offscreen smoke | integration/API cleanup |
| Glyph visual contract | yes | yes, `dvz_glyph()` | active as text lowering target | atlas-backed glyph path active | no | raster capture only | covered through text/glyph tests | API boundary cleanup |
| Label annotation | yes | yes, `DvzAnnotation` / `dvz_annotation_label()` | yes | simple glyph lowering active; richer readouts pending | no | raster capture only | annotation bookkeeping and realization tests | [ANNOTATION_LABEL_SLICE.md](ANNOTATION_LABEL_SLICE.md) |
| Generic annotation kinds | broad semantics | partial, `DvzAnnotationKind` | partial | no | no | no | no rendered path | after label slice |
| Continuous scale | yes | yes, `DvzScale` | yes | active for image/volume colormap binding | no | capture only | scale/field tests | colorbar slice |
| Colormap | yes | yes, `DvzColormap` | yes | active for image/volume colormap binding | no | capture only | scale/field tests | colorbar slice |
| Continuous colorbar | yes | yes, `DvzColorbar` | yes | no ticks/ramp labels | no | no | bookkeeping only | [COLORBAR_RENDERING_SLICE.md](COLORBAR_RENDERING_SLICE.md) |
| Discrete legend | yes, broad | no active public handle | no | no | no | no | none | [LEGEND_SLICE.md](LEGEND_SLICE.md) |
| Scale bar measurement | proposal only | no dedicated public handle | no | no | no | no | none | after label and text slices |
| Dimension measurement | proposal only | no dedicated public handle | no | no | no | no | none | after label and text slices |


## Slice Template

Each implementation slice should answer these questions before code starts:

1. Which installed or proposed public API does the slice use?
2. Which retained structs and dirty bits are required?
3. Which validation failures must be caught before planning?
4. What exact `FramePlan` contribution is produced?
5. What DRP2 resources, shaders, buffers, textures, and passes are emitted?
6. Which capability checks produce diagnostics instead of silent fallback?
7. Which focused tests fail before the implementation and pass after it?
8. Which example or smoke target proves the slice outside isolated tests?


## Current Implementation Order

Use this order unless a concrete user task changes priority:

1. Use the landed `dvz_text()`/glyph path for axes, colorbars, labels, annotations, and readouts.
2. Reconcile the transitional visual-backed text API with the semantic `DvzText` target API.
3. Finish data/world text placement and depth policy.
4. Render a continuous `DvzColorbar` ramp plus title/tick labels.
5. Add a dedicated scale-bar measurement annotation.
6. Define and implement `DvzLegend` only after categorical scale labels and ordering are concrete.
