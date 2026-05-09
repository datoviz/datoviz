> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended active v0.4 direction for shared colormap scales, colorbars,
>   and user-facing range control.

# Colorbar and Colormap Design

This note narrows the broader scales and legends/colorbars material into the active design choices
that matter now for scientific visualization workflows.


## Objective

Support colormaps and colorbars as first-class scientific semantics, including:

1. shared color scales,
2. panel-attached colorbars,
3. unit-aware labels and ticks,
4. interactive range selection,
5. reuse across image, volume, mesh-scalar, and future field-based visuals.


## Existing Grounding In The Repo

There is already substantial background context:

1. scale model:
   [spec/scene/semantics/SCALES.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/semantics/SCALES.md)
2. legend/colorbar semantics:
   [spec/scene/semantics/LEGENDS_AND_COLORBARS.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/semantics/LEGENDS_AND_COLORBARS.md)
3. annotation/measurement direction:
   [spec/scene/proposals/ANNOTATION_MEASUREMENT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/ANNOTATION_MEASUREMENT_DESIGN.md)
4. volume direction:
   [spec/scene/proposals/VOLUME_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/VOLUME_DESIGN.md)

This note makes the active decisions more explicit.


## Core Recommendation

Colorbars should attach to shared scale semantics, not to one visual-private shader state.

Recommended split:

1. `scale` is the semantic source of truth,
2. visuals reference scales,
3. colorbars explain scales,
4. UI edits such as visible range changes mutate the scale or a declared view over that scale.

This is the cleanest way to keep images, volumes, and scalar-mapped meshes coherent.


## Why This Matters

Without explicit scale ownership:

1. one colorbar cannot cleanly explain several visuals using the same semantic mapping,
2. range edits become ad hoc uniform mutations,
3. unit formatting gets duplicated,
4. volume, image, and mesh scalar overlays drift apart semantically.


## Scale Ownership

Recommended rule:

1. scales remain scene-owned objects,
2. visuals reference one or more scales by stable identity,
3. colorbars reference scales rather than visuals directly by default,
4. visual-local overrides are allowed only when declared explicitly.

This gives the right semantics for reuse and export.


## Colorbar Ownership

Colorbars should be panel-attached semantic objects by default.

Why:

1. panels may show different subsets or different views of the same data,
2. one scale may need different explanatory placement in different panels,
3. panel-local layout is the natural home for axis-adjacent explanatory objects.

Scene-shared colorbars can still exist later for export or consolidated layouts.


## Interactive Range Selection

Range control is important now and should be explicit.

Recommended model:

1. each color scale has a semantic domain,
2. a colorbar may show and manipulate a current visible range/view of that domain,
3. UI controls can mutate that visible range through the scale object,
4. dependent visuals and explanatory objects become dirty through shared scale invalidation.

This should work for:

1. image contrast/windowing,
2. volume slice intensity range,
3. scalar mesh overlays,
4. probe-driven scale inspection.


## Domain Versus View Range

Do not conflate the full semantic data domain with the currently emphasized visible range.

Recommended split:

1. full semantic domain
2. current display range or window

This is especially important for:

1. medical/brain imaging windowing,
2. clipped contrast ranges,
3. consistent exported annotations even when the data domain is wider than the current displayed
   range.


## Colormap Choices

Recommended baseline support:

1. named perceptual colormaps
2. custom color-stop ramps
3. diverging scales with explicit center
4. categorical palettes as a separate semantic case

Recommendation:

1. keep the existing scale vocabulary from `semantics/SCALES.md`,
2. do not collapse categorical legends and continuous colorbars into one ambiguous object,
3. allow colorbars to reflect linear, log, and similar scale semantics through shared scale logic.


## Unit And Tick Formatting

Colorbars should share formatting machinery with axes and measurement overlays where possible.

Recommended behavior:

1. scale units appear on colorbar labels,
2. ticks derive from the scale’s semantic domain and formatting policy,
3. range/view changes update tick generation and label formatting consistently,
4. local formatting overrides may exist, but they should layer on top of the shared scale/domain
   formatting machinery rather than replacing it wholesale.

This is one reason the axes/domain and measurement notes matter here.


## Relationship To Volume

Volume visuals are a major consumer of this design.

Recommended direction:

1. slice and DVR color mapping should be able to reference shared color scales where appropriate,
2. opacity transfer remains volume-specific and is not forced into the same object as the color scale,
3. volume-specific colorbar variants may later explain both color and transfer behavior, but the
   core colorbar/scale model should stay shared.


## Relationship To Image

Image visuals should be straightforward consumers of the same color-scale model.

Recommended behavior:

1. image pixel values map through a referenced scale,
2. image pixel picking can report both sampled value and scale-aware interpretation later,
3. colorbars explain that scale directly.


## Relationship To Mesh

Scalar-colored meshes should also be able to use the same scale system.

Recommended behavior:

1. per-vertex or per-face scalar mappings reference a scene scale,
2. a colorbar can explain that scale without caring whether the consumer is image, mesh, or volume
   slice,
3. lighting and material remain separate from the scalar-color semantic mapping.


## Annotation And Layout Implications

Colorbars are annotation-side objects and should behave like them.

Recommended capabilities:

1. panel-edge anchoring,
2. horizontal or vertical orientation,
3. title/label support,
4. tick generation and optional threshold markers,
5. pickability at the object level later if needed.


## External UI Implications

UI sliders and controls should mutate scene-owned scales, not bypass them.

Recommended rule:

1. an external UI widget adjusts scale range, palette, or center,
2. the scale object becomes dirty,
3. visuals and colorbars referencing that scale update coherently.

This is the right retained model for dockable tools or property panels.


## Initial Public API Direction

The exact names can still move, but the conceptual API should support:

1. create/update scene-owned scale,
2. bind scale to visuals,
3. create panel-attached colorbar referencing that scale,
4. mutate scale range/view from external UI or application code.

Likely conceptual calls:

1. `dvz_scale_*`
2. `dvz_visual_set_scale(...)`
3. `dvz_colorbar(panel, scale, flags)`
4. `dvz_scale_set_domain(...)`
5. `dvz_scale_set_view_range(...)`


## Immediate Scope Recommendation

The narrowest useful active implementation target is:

1. scene-owned continuous color scale,
2. panel-attached colorbar bound to that scale,
3. interactive visible-range update path,
4. shared use by image and volume slice first,
5. unit-aware labels and ticks.


## Explicit Non-Goals For The First Slice

1. every possible legend subtype,
2. a full theming/styling language,
3. one monolithic object that merges color mapping, opacity transfer, and every explanatory widget,
4. backend-shaped palette/texture APIs in the public scene layer.
