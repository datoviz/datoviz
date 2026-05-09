# Scene Public API Surface

This document is the normative bridge between the semantic scene spec and the draft public C
headers.

It defines the first header-drafting target for interaction objects, selection/link/probe result
types, shared scales and colorbars, and retained text/annotation objects. Detailed behavior remains
in the specialized spec documents and active proposals.


## Normative Status

This document is normative for public API shape until `include/datoviz/scene.h`,
`include/datoviz/scene/*.h`, and `spec/scene/headers/scene_api.h` are updated.

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
4. `spec/scene/headers/scene_api.h` remains the implementation-facing draft spelling until the
   installed public headers catch up.


## Opaque Handles Versus Public Structs

Use opaque handles for retained scene-owned objects:

1. `DvzInteractionPolicy`
2. `DvzSelection`
3. `DvzLinkChannel`
4. `DvzPinnedReadout`
5. `DvzScale`
6. `DvzColormap`
7. `DvzColorbar`
8. `DvzFont`
9. `DvzText`
10. `DvzAnnotation`

Use public structs for value-like data:

1. creation descriptors,
2. formatting descriptors,
3. request descriptors,
4. pick, hover, selection, and probe result payloads,
5. color stops and categorical entries,
6. text style and placement descriptors.

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

Before implementation starts, keep tiny usage examples for:

1. [examples/API_MESH_SELECTION_LINK.md](../examples/API_MESH_SELECTION_LINK.md),
2. [examples/API_IMAGE_PROBE_PINNED_READOUT.md](../examples/API_IMAGE_PROBE_PINNED_READOUT.md),
3. [examples/API_SCALE_COLORBAR_ANNOTATION.md](../examples/API_SCALE_COLORBAR_ANNOTATION.md).

These examples are API pressure tests. They do not need to compile until the corresponding header
draft exists, but awkward examples should block implementation.
