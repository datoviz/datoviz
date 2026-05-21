# Scene Public API Surface

This document is the normative bridge between the semantic scene spec and the draft public C
headers.

It defines the first header-drafting target for interaction objects, selection/link/probe result
types, shared scales and colorbars, and retained text/annotation objects. Detailed behavior remains
in the specialized spec documents and active proposals.


## Normative Status

This document is normative for public API shape policy. Installed headers under
`include/datoviz/scene*.h` are now the source of truth for names and signatures that already exist.

If this document conflicts with a specialized semantic spec, the specialized spec owns behavior and
this document should be corrected to match it.


## Header Ownership

The public scene API should use this split:

1. [include/datoviz/scene.h](../../../include/datoviz/scene.h) remains the
   umbrella header.
2. Small, focused public subheaders under `include/datoviz/scene/` are preferred once a topic would
   make `scene.h` hard to scan.
3. Shared public descriptors, enums, and result structs belong in `include/datoviz/scene/types.h`
   or focused sibling subheaders.
4. Historical sketches under `spec/scene/headers/` are informative only; do not use them as public
   API source material without first promoting the relevant idea into this document or a specialized
   spec.


## Current Header And Implementation Boundary

The first public header split has landed. Treat these groups as implemented APIs with source tests:

1. scene / figure / panel lifecycle,
2. frame plans and DRP2 emitters,
3. point, primitive, mesh, path-as-line/strip, and image visuals,
4. retained visual attributes and scene buffers,
5. sampled fields and image field binding,
6. scale/colormap core and image colormap binding,
7. panzoom and arcball controllers,
8. interaction policy, selection, link-channel, pick/probe queue, hover-state, and pinned-readout
   bookkeeping,
9. first point-pick and image-probe execution through the DRP2 runtime request processor,
10. font/text and annotation retained-object bookkeeping,
11. first rendered text/glyph output through atlas-backed scene resources.

Treat these installed declarations as draft contracts until implemented in `src/scene`:

1. semantic `DvzText*` public surface migration from the current visual-backed text entry point,
2. rendered non-label annotations and callouts,
3. rendered colorbar ticks/labels,
4. selection highlight rendering and broader link-driven state propagation,
5. mesh/object picking and richer probe payloads,
6. broad mapped attributes beyond the current image colormap path.

Colorbars, text, and annotations are retained semantic objects. Text/glyph rendering exists as a
first slice, but the visual-backed text API must be migrated so retained `DvzText` is the v0.4
source of truth.

Implementation-ready rendering work for those retained objects is tracked in:

1. [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md),
2. [../slices/ANNOTATION_LABEL_SLICE.md](../slices/ANNOTATION_LABEL_SLICE.md),
3. [../slices/COLORBAR_RENDERING_SLICE.md](../slices/COLORBAR_RENDERING_SLICE.md).


## Opaque Handles Versus Public Structs

Use opaque handles for retained scene-owned objects:

1. `DvzInteractionPolicy`
2. `DvzSelection`
3. `DvzLinkChannel`
4. `DvzPinnedReadout`
5. `DvzSampledField`
6. `DvzScale`
7. `DvzColormap`
8. `DvzColorbar`
9. `DvzFont`
10. `DvzText`
11. `DvzAnnotation`
12. `DvzOrientationGizmo`

Use public structs for value-like data:

1. creation descriptors,
2. formatting descriptors,
3. request descriptors,
4. pick, hover, selection, and probe result payloads,
5. color stops and categorical entries,
6. text style and placement descriptors,
7. sampled-field creation and update descriptors.
8. orientation-gizmo placement and source-binding descriptors.

Public structs must not expose backend handles, atlas pages, command buffers, runtime object ids, or
Vulkan/DRP2 execution details.


## Lifetime Rules

Scene-owned retained objects are destroyed with their owning scene unless they have an explicit
destroy API.

Panel-attached retained objects are still scene-owned. The panel attachment controls placement,
visibility, controller participation, and invalidation scope; it does not transfer ownership to the
panel.

Result structs are caller-owned values. Any pointer inside a result must either be valid only until
the next poll/read call and documented as borrowed, or replaced by fixed-size storage / ids.


## Orientation Gizmo Surface

The first v0.4 gizmo API should expose a passive orientation gizmo, not a generic `DvzGizmo`
handle. The object shows panel orientation through a small axis triad and does not edit scene data.

Recommended public naming:

```text
DvzOrientationGizmo
dvz_panel_orientation_gizmo(panel, &desc)
dvz_orientation_gizmo_set_visible(gizmo, visible)
dvz_orientation_gizmo_set_source(gizmo, controller_or_camera)
dvz_orientation_gizmo_set_layout(gizmo, &layout)
```

The descriptor should cover:

1. anchor corner or normalized panel-local rectangle;
2. pixel or panel-relative size;
3. source controller or camera binding;
4. visibility;
5. optional axis length and style knobs.

The implementation should lower the triad to ordinary generated mesh geometry. Geometry generation
belongs in `geom` through `dvz_geom_gizmo_axes()` when that module is active; scene/app owns inset
placement, synchronization, redraw requests, viewport/scissor handling, and depth policy.

Do not use the old v0.3 `panel.gizmo()` name as the C contract. Bindings may add a shorter alias
later, but the C API should keep `orientation` in the name because an interactive transform gizmo is
a separate future feature.

The transform gizmo should use a distinct future handle such as `DvzTransformGizmo`. It depends on
object transform ownership, selection state, and richer picking identities, and must not be coupled
to the passive orientation widget.


## SampledField Surface

The next public scene API should introduce `DvzSampledField` as the shared regular-grid data object.

The first surface should expose:

1. a dedicated public subheader, likely `include/datoviz/scene/field.h`,
2. one creation descriptor with dimension, format, resolution, and semantic hints,
3. one geometry descriptor with optional axis and physical metadata,
4. one scene-owned opaque handle,
5. full payload replace,
6. rectangular or box subregion update,
7. semantic visual binding through field slots such as `"field"`,
8. descriptor readback for tooling and probes.

`DvzSampledField` should cover both direct color textures and scalar/vector fields. It should not
be limited to one current image implementation detail such as `f32` CPU fallback uploads.

The public API should describe field semantics, not backend texture execution strategy. GPU-native
sampled-texture realization and CPU-side derived RGBA fallback should both remain internal
execution choices behind the same field object.


## Interaction Surface

The first public interaction surface should include:

1. an interaction policy object,
2. visual picking capability declarations,
3. explicit panel pick/probe requests,
4. scene polling for pick/probe results,
5. retained hover state,
6. retained selection objects,
7. scene-owned link channels,
8. optional pinned readout objects.

The API should keep policy, state, and result payloads separate. A callback-only API is not enough,
because external UI, tests, and synchronous inspection need retained queryable state.


## Pick Result Shape

`DvzPickResult` should be a fixed-layout public struct for the first slice.

It should carry:

1. status / hit flag,
2. panel identity,
3. visual identity,
4. raw hit kind and raw id,
5. resolved target kind and resolved id,
6. optional instance id,
7. logical pointer position in panel coordinates,
8. optional world or data coordinate when available.

Raw and resolved identities must both remain visible. Raw identity preserves backend precision;
resolved identity expresses the scene-level target used by selection and linking.


## Probe Result Shape

`DvzProbeResult` should be a fixed-layout public struct with an extensible payload area.

It should carry:

1. status / hit flag,
2. panel and visual identity,
3. sampled coordinate in data or world space when available,
4. sampled value kind,
5. numeric scalar/vector payload for common values,
6. optional label/category id,
7. scale or colormap reference when the value is mapped,
8. source pick result id when the probe was derived from picking.

Avoid a visual-specific probe struct family in the first API. Visual-specific interpretation can be
added through payload kind, flags, and documented optional fields.


## Selection And Link Keys

Selection contents should store resolved scene targets, not raw hit payloads.

Linking should be channel-based:

1. `DvzLinkChannel` is a scene-owned handle.
2. Channels may have a public stable string name.
3. Link keys should use a fixed public key type for the first slice, preferably a 64-bit integer key
   with optional string labels added later.
4. A local visual identity may map to zero or one key per channel.
5. Multiple local identities may share one key.

Changing an active link channel must not reinterpret existing selection contents retroactively.


## Scale, Colormap, And Colorbar Surface

Scales and colormaps are scene-owned semantic objects.

Colorbars are panel-attached explanatory objects bound to a scale; they do not own the scale or the
colormap.

The first public surface should expose:

1. continuous scale creation and domain/view-range setters,
2. unit and label metadata,
3. built-in colormap selection,
4. custom color-stop ramps,
5. diverging center support,
6. panel-attached colorbar creation,
7. colorbar orientation, anchor, title, and formatting overrides.

Interactive range editing should be represented as interaction policy on the scale/colorbar pair,
not as an external UI-only behavior.


## Text And Annotation Surface

Text should be a retained semantic object or retained annotation object with semantic content and
placement descriptors. The implementation may lower text objects to `glyph` visual contributions,
but glyph atlas details are not part of the public text API.

The first public surface should expose:

1. font resource handles,
2. retained text handles,
3. text style descriptors,
4. screen-space and world-space placement descriptors,
5. retained annotation handles,
6. label, callout, scale-bar, dimension, and pinned-readout annotation kinds,
7. shared formatting descriptors.

Text callers should bind content, style, font, placement, and color. They should not manage glyph
atlases, glyph UVs, or text render-pass resources directly.

For v0.4, `DvzText*` is the target public text handle. The existing `DvzVisual* dvz_text()` shape is
not a compatibility constraint; either reshape `dvz_text()` to return `DvzText*` or replace it with
a clearly named semantic constructor. `dvz_glyph()` may remain low-level, but axes, colorbars,
annotations, legends, and readouts should consume semantic text or annotation APIs.


## Formatting Descriptor Policy

Use one shared base formatting descriptor for numeric and textual formatting across:

1. scales,
2. axes,
3. colorbars,
4. measurements,
5. annotations,
6. probe/readout labels.

Small per-domain extensions are acceptable. Duplicating unrelated format structs for each domain is
not the preferred first API because it will make linked axes, colorbars, and measurement overlays
drift.


## Required API-Shape Examples

Keep tiny usage examples for:

1. [examples/api/API_MESH_SELECTION_LINK.md](../examples/api/API_MESH_SELECTION_LINK.md),
2. [examples/api/API_IMAGE_PROBE_PINNED_READOUT.md](../examples/api/API_IMAGE_PROBE_PINNED_READOUT.md),
3. [examples/api/API_SCALE_COLORBAR_ANNOTATION.md](../examples/api/API_SCALE_COLORBAR_ANNOTATION.md),
4. [examples/api/API_SAMPLED_FIELD.md](../examples/api/API_SAMPLED_FIELD.md).

These examples are API pressure tests. They may reference drafted APIs that are not fully implemented
yet, but awkward examples should still block broadening the public surface.
