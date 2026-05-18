> **Execution Status**
> - **Status:** `INFORMATIVE CONSOLIDATION CHECKPOINT`
> - **Updated on:** `2026-05-09`
> - **Purpose:** record the current high-priority v0.4 technical decisions around scene resources,
>   controllers, picking, transparency, text, and measurement annotations before implementation
>   hardens.

# High-Priority Spec Decisions

This note captures the current decisions and recommendations discussed after the `mesh` and `geom`
design passes.

Most topics in this checkpoint now have focused normative specs or active proposals. Treat this
file as a historical consolidation aid; when it conflicts with a specialized spec, update and follow
the specialized spec.


## Scope

This note focuses on:

1. scene resource ownership and update model,
2. controller and transform behavior,
3. picking requirements,
4. transparency / WBOIT direction,
5. text architecture,
6. measurement and annotation requirements,
7. near-term spec priorities.


## Scene Resource Ownership

Recommended ownership split:

1. CPU geometry lives in `DvzGeometry`,
2. uploaded scene mesh resources are distinct scene-owned assets,
3. visuals borrow references to reusable geometry/texture/sampler resources.

Why:

1. this supports resource sharing across visuals and panels,
2. it keeps CPU geometry distinct from uploaded runtime assets,
3. it leaves room for efficient partial updates and browser portability.

Recommended resource families:

1. geometry resources,
2. texture resources,
3. sampler resources,
4. later material resources if a real need appears.

Visuals should not own resource lifetime by default. Shared resource lifetime should remain with
the scene or resource registry.


## Resource Update Model

This needs to be first-pass design, not later cleanup.

Phase-1 requirements:

1. full replacement updates,
2. vertex buffer subrange updates,
3. index buffer subrange updates,
4. texture region updates,
5. dirty-range tracking at the scene resource layer.

Why:

1. dynamic scientific visuals need partial updates now,
2. text atlas growth and refresh needs region uploads,
3. image and mesh workflows should not require whole-resource replacement for small changes.

Recommended contract:

Geometry resources:

1. full replace,
2. vertex subrange upload,
3. index subrange upload.

Texture resources:

1. full replace,
2. 2D region upload `(x, y, width, height)`,
3. later layer/mip-specific updates if needed.

This should be expressed as a scene/resource API, not in Vulkan-native terms.


## Transform and Controller Model

Current recommended model:

1. geometry stores local/object-space vertices,
2. visuals own object/model transforms,
3. panels own fixed camera/view/projection state,
4. controllers intentionally mutate either model transforms or camera state depending on controller
   type.

For the current v0.4 path, arcball should mutate the model transform while the camera remains
fixed.

Why:

1. this matches the intended scientific-inspection use case,
2. it keeps camera/world conventions stable,
3. it gives a direct “rotate the object in place” behavior.

Recommended public controller split:

1. model arcball,
2. later camera orbit controller as a distinct mode.

Do not merge those into one ambiguous controller contract.


## Picking

Picking needs to be designed now at the visual-family level.

Recommended picking scope:

1. object-level picking for all pickable visuals,
2. mesh face-level picking for mesh visuals,
3. vertex-level picking for visuals such as point, scatter, path, and similar data-driven
   primitives.

Deferred:

1. mesh vertex-level picking unless a concrete need appears,
2. more exotic primitive-resolution modes not needed by active visuals.

Why:

1. precise part-of-visual picking is critical for scientific visualization,
2. point/scatter/path-like visuals need vertex-level precision now,
3. mesh face picking is the right first high-resolution mesh mode.

Recommended pick result shape:

1. logical visual/object id,
2. pick kind,
3. optional mesh face id,
4. optional vertex/element id for point/scatter/path families,
5. optional hit position later.

Recommended architecture:

1. scene owns logical id resolution tables,
2. picking is request/readback based,
3. backends do not expose raw implementation ids to the public API.


## Transparency and WBOIT

Transparency is not just a material scalar. It changes pass structure and must be designed now.

Recommended direction:

1. keep opacity/material alpha as material data,
2. represent transparency mode as a visual/render mode,
3. design the scene -> frame-plan -> DRP2 -> runtime path with WBOIT in mind now.

Immediate transparency modes to reserve:

1. opaque,
2. transparent_wboit.

What should be specified now:

1. multi-pass sequencing in frame plans,
2. extra render targets needed by WBOIT,
3. resolve/composite pass structure,
4. fallback behavior when transparency mode is unavailable,
5. interaction with picking and readback.

WBOIT is high-priority and should be treated as an early architectural target, not only as a later
feature wish.


## Text

Text is an active design topic now. It must support both annotation text and world-space 3D text.

Required text scope:

1. screen-space annotation text,
2. world-space 3D text,
3. simple equation/math rendering path,
4. shared font/atlas/resource ownership,
5. backend-agnostic rendering architecture.

Recommended architecture split:

1. shaping/layout,
2. font loading/metrics,
3. atlas or glyph resource generation,
4. text visual rendering,
5. optional equation backend emitting glyph/rule/rect composition.

Recommended practical baseline:

1. HarfBuzz for shaping,
2. FreeType for font loading and glyph metrics/outlines,
3. atlas-backed text rendering as the first runtime path.

Why:

1. it is practical now,
2. it supports both 2D and 3D placement,
3. it leaves room for later backends.

World-space text should be first-class, not deferred. Text visuals should support:

1. 3D anchor position,
2. optional orientation/billboard behavior,
3. scale in world or screen units,
4. explicit depth behavior choices later.


## Math / Equation Text

The text architecture should leave room for a simple equation backend.

Recommended direction:

1. do not implement full TeX layout internally,
2. let an equation backend emit a structured draw/list composition,
3. support glyph runs plus simple rules/rectangles/backgrounds in the text/annotation layer.

This would fit a MicroTeX-style frontend if it can emit:

1. glyph ids,
2. positions/transforms,
3. rules/lines,
4. rectangles/boxes.

The long-term text system should not depend on image-visual hacks such as rounded-corner images to
represent equation backgrounds or rules.


## Alternative / Future Text Backends

There is value in keeping the text rendering API backend-agnostic.

One plausible future or optional backend is the recently opened Slug algorithm path for direct GPU
font rendering from outlines.

Useful references:

1. [Slug Library](https://sluglibrary.com/)
2. [Hackaday, March 20, 2026: Slug Algorithm Now In Public Domain](https://hackaday.com/2026/03/20/slug-algorithm-for-on-gpu-rendering-of-fonts-with-bezier-curves-now-in-public-domain/)

Recommendation:

1. do not require a Slug-like backend for the first text implementation,
2. keep the API open enough that an atlas-backed path and a direct-GPU-outline path can both fit
   later.


## Measurement and Annotation

Axes alone are not enough. The annotation system should explicitly support measurement-oriented
visual aids.

Immediate useful requirements:

1. adaptive scale bars,
2. 2D dimensions with units,
3. 3D bounding box overlays,
4. 3D dimension annotations with units.


## Adaptive Scale Bar

This should be a first-class annotation primitive.

Behavior:

1. horizontal or otherwise oriented line,
2. label chosen from “nice” values,
3. automatically adapts to pan/zoom and 3D camera/object scale context,
4. unit-aware formatting such as `2 cm`, `5 mm`, `1 mm`.

This is valuable in both 2D and 3D scientific visualization and should be included in the spec
early.


## 3D Bounding Box and Dimensions

This should also be treated as a real annotation/measurement feature.

Recommended support:

1. antialiased 3D bounding box overlay around a selected/current object,
2. optional emphasized edges/corners,
3. optional dimension labels with units,
4. compatibility with fixed-camera, model-arcball interaction.


## Recommended Spec Priority Order

This sequence is historical. Current priority should come from
[`../../README.md`](../../README.md), [`../README.md`](../README.md), and the active/future proposal
indexes, not from this checkpoint.

Given the current active needs, the recommended next spec sequence is:

1. mesh scene API and resource ownership,
2. text system architecture,
3. transparency / WBOIT contract,
4. picking contract,
5. transform/controller model with model-space arcball,
6. resource partial-update contract,
7. measurement/annotation primitives,
8. axes/domain mapping.


## Explicit Decisions Captured Here

The main decisions recorded in this note are:

1. subrange/region updates are required now, not later,
2. arcball should mutate model transforms while camera stays fixed,
3. picking must support mesh face picking and vertex-level picking for point/scatter/path-like
   visuals,
4. WBOIT should be designed now as a high-priority transparency mode,
5. world-space 3D text is required now,
6. the text architecture should remain open to both atlas-based and future direct-GPU-outline
   backends,
7. adaptive scale bars and 3D dimension/bounding-box annotations are part of the active
   annotation-design problem, not decoration.
