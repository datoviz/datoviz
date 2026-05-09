# Scene Text

This document defines text content, placement, font/atlas resources, DPI behavior, and interaction
semantics for the scene layer.

Text is a scene-side semantic feature. It may lower to the `glyph` visual family, annotation
objects, axis labels, legends, colorbars, probes, or equation/layout display lists. Those rendering
paths must resolve back to semantic scene text objects.


## Normative Status

This document is normative for the current v0.4 text model.

[../proposals/TEXT_DESIGN.md](../proposals/TEXT_DESIGN.md) remains the rationale and staging note
for future implementation details such as equation backends, color-font handling, and direct GPU
outline rendering.


## Semantic Purpose

Text exists to support:

1. screen-space labels, annotations, probe readouts, scale bars, legends, and colorbars,
2. world-space labels and measurement annotations,
3. axis and tick labels derived from data-space values,
4. simple equation or math display through a structured composition backend,
5. DPI-aware scientific output that remains legible across displays and exports.

Text is not a backend overlay. It participates in scene ownership, validation, invalidation,
resource planning, export, and picking according to this contract.


## Core Rule

Text content, style, placement, and semantic identity are scene concepts.

Glyph atlas pages, glyph packing, UV coordinates, and rasterization details are runtime-resource
details. Public scene APIs and readback/export paths must not expose atlas internals as the only
meaning of text.


## Architecture Split

The text pipeline is split into these stages:

1. content specification,
2. shaping and layout,
3. font loading and metrics,
4. glyph resource generation,
5. scene resource updates,
6. renderable contribution emission,
7. optional export/readback representation.

These stages may be implemented incrementally, but the API and resource model should not collapse
them into one per-glyph upload API.


## Resource Model

Text uses explicit scene-owned resources.

Required resource roles:

1. font resource: source font bytes/path, metrics identity, and shaping-facing identity,
2. glyph atlas resource: atlas image pages plus glyph metadata and UV mapping,
3. text object or glyph visual: content, placement, style, transform, and semantic identity.

Text objects, glyph visuals, and annotation objects should borrow shared font and atlas resources.
They should not own private atlas lifetime by default.


## Content Model

Scene text content may enter at three levels:

1. plain text string,
2. shaped glyph run,
3. structured display list from an equation or layout backend.

Plain strings are the common public path. Shaped runs and display lists exist so advanced callers
or language bindings can bypass internal shaping without exposing atlas packing as public state.


## Shaping and Layout

Text shaping is a dedicated semantic step.

Rules:

1. public text APIs must not assume ASCII-only text,
2. single-line scientific labels must remain easy to express,
3. style runs may influence shaping when required by the font or shaping backend,
4. paragraph layout is not a phase-1 requirement,
5. full TeX layout is not implemented inside Datoviz.

Equation support should be modeled as an external or frontend backend that emits a structured
composition of glyph runs, rules, boxes, and transforms.


## Placement Modes

Text supports two primary placement modes.

Screen-space text:

1. uses figure, panel, or viewport-local logical coordinates,
2. uses logical-pixel or screen-relative size,
3. is unaffected by model-space arcball unless explicitly attached to scene geometry,
4. is the preferred path for axes, labels, legends, colorbars, HUD overlays, and probe readouts.

World-space text:

1. anchors to a 2D or 3D data/object/world position,
2. may use billboard or world-oriented placement,
3. may use world-unit or screen-size scaling,
4. participates in panel transforms and depth policy according to its placement descriptor.

World-space text must not be implemented as an undocumented screen-space hack.


## Scale and Orientation

Text scale mode is explicit:

1. screen-size text keeps approximately constant on-screen size,
2. world-size text scales with scene/world units.

World-space orientation is explicit:

1. screen-aligned billboard,
2. world-oriented text plane,
3. future custom-basis or axis-aligned modes when needed.

Implementations may initially support a subset, but unsupported modes must validate or adapt
explicitly rather than silently changing semantics.


## Style and Color

The baseline style model includes:

1. regular,
2. bold,
3. italic,
4. underline.

Rules:

1. color is run-level by default,
2. per-glyph color override is allowed for richer content paths,
3. underline is a derived decoration primitive, not a required font face,
4. alpha follows the normal scene transparency and render-mode rules,
5. rich text editing and CSS-like styling are out of scope for the first text slice.


## DPI Behavior

Scene-facing text sizes are expressed in logical units:

1. logical pixels or screen-relative units for screen-size text,
2. world units for world-size text.

Runtime realization uses physical resolution derived from the active DPI scale.

DPI changes must mark dependent text resources dirty. The implementation may patch or rebuild atlas
resources, but the semantic text size and placement must not change merely because the display DPI
changed.


## Update and Invalidation

Text depends on partial resource updates.

Required update behavior:

1. changed strings dirty the affected text object and glyph-run geometry,
2. newly needed glyphs dirty atlas regions,
3. DPI changes dirty dependent atlas resources and screen-space text geometry,
4. style or placement changes dirty only the minimal affected scope when practical,
5. text updates appear in the frame plan through normal resource/upload nodes.


## Picking and Selection

The first text picking contract is object-level.

Rules:

1. picking resolves to semantic text object identity,
2. glyph-level and substring-level picking are deferred,
3. text selection/highlight integrates with the shared scene interaction model,
4. hit testing may be approximate until a concrete workflow needs sub-run precision.


## Export and Readback

Export paths should preserve semantic text when possible.

Rules:

1. vector export should prefer text or structured glyph/run output over raster-only atlas quads
   when the target format supports it,
2. image export may use the runtime rasterization path,
3. readback and diagnostics should report semantic text object identity, not glyph atlas internals.


## Capability and Fallback

Text capability adaptation must be explicit.

Examples:

1. missing shaping backend may restrict supported scripts,
2. missing color-glyph support may fallback to monochrome glyph rendering,
3. unsupported orientation or depth modes may fail validation or adapt with diagnostics,
4. atlas size limits may trigger atlas paging, simplification, or validation failure.

Fallbacks must emit diagnostics when they change visible semantics.


## Non-Goals

The first text slice does not include:

1. full paragraph layout,
2. full TeX implementation inside Datoviz,
3. glyph-level public interaction semantics,
4. CSS-like rich text styling,
5. public exposure of atlas UVs or glyph packing as the primary API.


## Related Specs

1. [ANNOTATIONS.md](ANNOTATIONS.md)
2. [AXES.md](AXES.md)
3. [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md)
4. [SCALES.md](SCALES.md)
5. [../pipeline/RESOURCE_MODEL.md](../pipeline/RESOURCE_MODEL.md)
6. [../pipeline/INVALIDATION_AND_CACHING.md](../pipeline/INVALIDATION_AND_CACHING.md)
7. [../integration/HIGH_DPI.md](../integration/HIGH_DPI.md)
8. [../interaction/PICKING.md](../interaction/PICKING.md)
