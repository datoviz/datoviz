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
5. [RC3_LIGHTING_FOUNDATION_SLICE.md](RC3_LIGHTING_FOUNDATION_SLICE.md): required pre-RC3 scene-owned ambient/directional lighting, direct/indirect shader decomposition, and correct GTAO consumption.
6. [MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md](MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md): optional point-light, two-sided-lighting, and checkerboard Klein-bottle expansion after the required lighting foundation.


## Feature Readiness Matrix

| Feature | Specified | API declared | Retained state | Rendered | Pickable | Exported | Focused tests | Next slice |
|---|---|---|---|---|---|---|---|---|
| Text object | yes | yes, semantic `DvzText*` | retained semantic text state | first rendered slice active | no | raster capture only | text realization, atlas growth, runtime readback, app/offscreen smoke | data/world placement and diagnostics |
| Glyph visual contract | yes | yes, `dvz_glyph()` low-level | active as text lowering target | atlas-backed glyph path active | no | raster capture only | covered through text/glyph tests | keep low-level boundary |
| Label annotation | yes | yes, `DvzAnnotation` / `dvz_annotation_label()` | yes | simple glyph lowering active; richer readouts pending | no | raster capture only | annotation bookkeeping and realization tests | data-anchored/readout polish |
| Generic annotation kinds | broad semantics | partial, `DvzAnnotationKind` | partial | no | no | no | no rendered path | after label slice |
| Continuous scale | yes | yes, `DvzScale` | yes | active for image/volume colormap binding | no | capture only | scale/field tests | colorbar slice |
| Colormap | yes | yes, `DvzColormap` | yes | active for image/volume colormap binding | no | capture only | scale/field tests | colorbar slice |
| Continuous colorbar | yes | yes, `DvzColorbar` | yes | ramp, ticks, title, and labels active | no | raster capture only | colorbar realization and app/offscreen smoke | shared layout and categorical legend follow-up |
| Discrete legend | yes, broad | no active public handle | no | no | no | no | none | [LEGEND_SLICE.md](LEGEND_SLICE.md) |
| Scale bar measurement | yes | yes, `dvz_scale_bar()` | yes | 2D and 3D first slices active | no | raster capture only | formatting, realization, stream, and churn tests | release validation smoke |
| Dimension measurement | proposal only | no dedicated public handle | no | no | no | no | none | after label and text slices |
| Lighting foundation | approved required slice | descriptor names pending API review | material-owned compatibility state only | current visual-owned Phong/Standard path | n/a | raster capture only | current material/GTAO tests; semantic split pending | [RC3_LIGHTING_FOUNDATION_SLICE.md](RC3_LIGHTING_FOUNDATION_SLICE.md) before RC3 freeze |


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

1. Complete [RC3_LIGHTING_FOUNDATION_SLICE.md](RC3_LIGHTING_FOUNDATION_SLICE.md) before the next release-candidate freeze.
2. Prove text, axes, colorbars, label annotations, and scale bars in the release example/fixture set.
3. Finish data/world text placement and depth policy where release examples require it.
4. Harden shared panel-edge layout across axes, colorbars, legends, annotations, and readouts.
5. Decide whether rendered pinned readouts are required; otherwise defer richer readout UI.
6. Define and implement `DvzLegend` only after categorical scale labels and ordering are concrete.

Point-light evaluation, two-sided lighting, and the multi-light Klein-bottle showcase remain optional feature work and are not RC3 or RC4 release blockers.
