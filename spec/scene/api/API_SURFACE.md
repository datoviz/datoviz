# Scene Public API Surface

This document is the normative bridge between the semantic scene spec and the draft public C
headers.

It defines the first header-drafting target for interaction objects, selection/link/probe result
types, shared scales, colorbars, legends, and retained text/annotation objects. Detailed behavior
remains in the specialized spec documents and active proposals.


## Normative Status

This document is normative for public API shape policy. Installed headers under
`include/datoviz/scene*.h` are now the source of truth for names and signatures that already exist.

If this document conflicts with a specialized semantic spec, the specialized spec owns behavior and
this document should be corrected to match it.

Scene public APIs also follow the cross-module conventions in
[`../../api/PUBLIC_API_CONVENTIONS.md`](../../api/PUBLIC_API_CONVENTIONS.md). This document may add
scene-specific rules, but it should not redefine general C API, binding, or struct-versus-setter
policy.


## Agent-Default Scene Path

For generated user code and ordinary documentation examples, the preferred scene path is:

```text
figure -> panel -> visual or retained object -> data/resources -> render/show/capture -> pick/probe
```

This path is the public surface that coding agents should infer for common requests such as
"render a point cloud", "show an image with a colorbar", "make an offscreen capture", or "probe an
image pixel".

The API should therefore keep these concepts easy to identify:

1. scene, figure, and panel lifecycle;
2. typed visual or retained-object constructors;
3. explicit data/resource binding;
4. controller and interaction attachment;
5. render, show, capture, and readback entry points;
6. validation and diagnostics before backend execution.

DRP2, vklite, Vulkan, and backend-specific resource paths remain documented for advanced work, but
they should not be required to answer normal scene/app usage. Public examples should only route to
those layers when the example explicitly targets protocol, backend, or runtime internals.


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
3. point, pixel, marker, primitive, segment/path, image, labels, mesh, sphere, volume, and glyph
   visuals,
4. retained visual attributes and scene buffers,
5. sampled fields and image field binding,
6. scale/colormap core and image colormap binding,
7. panzoom, object/model arcball, fly, and turntable controllers,
8. interaction policy, selection, link-channel, pick/probe queue, hover-state, and pinned-readout
   bookkeeping,
9. first request execution slices through the DRP2 runtime request processor, including item
   picking for point-like, stroke, primitive, image, mesh, sphere, and volume proxy targets plus
   image pixel and labels segment probe payloads,
10. font/text and annotation retained-object bookkeeping,
11. semantic `DvzText*` public surface and first rendered text/glyph output through atlas-backed
    scene resources,
12. rendered label annotations through the text/glyph path,
13. rendered continuous colorbar ramp, ticks, title, and labels,
14. rendered 2D/3D scale bars through `dvz_scale_bar()`,
15. retained categorical scale entries and rendered categorical legends,
16. semantic polygon and polygon-set composites lowered to fill/stroke visuals,
17. retained per-visual local transforms with copy-out inspection helpers.

Treat these installed declarations as draft contracts until implemented in `src/scene`:

1. rendered non-label annotations, rich readouts, and callouts,
2. broader link-driven state propagation,
3. mesh face/region picking, exact path/marker semantics, label GPU probing, and richer probe
   payloads,
4. broad mapped attributes beyond the current image/volume colormap paths,
5. shared legend/colorbar layout and richer legend composition beyond the first categorical legend
   slice.

The built-in marker-symbol parity declarations are installed: `DvzSymbolSet`, `DvzSymbolId`,
built-in symbol creation, marker symbol-set binding, and the per-item `symbol` attribute. Bitmap,
SDF, and MSDF source APIs copy payloads into scene-owned symbol records; homogeneous texture-backed
marker symbols render through scene-owned atlas textures. SVG path import is installed as
`dvz_symbol_svg_path()` when msdfgen SVG support is available. Mixed-encoding fallback policy
remains active v0.4 parity work. Marker `shape` remains the compatibility path for built-in code-SDF
symbols.

Colorbars, legends, text, labels, and scale bars are retained semantic objects. Text/glyph
rendering, label annotation rendering, scale-bar rendering, continuous colorbar rendering,
categorical legend rendering, and integer label rendering exist as first slices; remaining work
should use those semantic objects rather than visual-private or backend-shaped state.

Implementation-ready rendering work for those retained objects is tracked in:

1. [../slices/TEXT_RENDERING_SLICE.md](../slices/TEXT_RENDERING_SLICE.md),
2. [../slices/ANNOTATION_LABEL_SLICE.md](../slices/ANNOTATION_LABEL_SLICE.md),
3. [../slices/COLORBAR_RENDERING_SLICE.md](../slices/COLORBAR_RENDERING_SLICE.md).


## Leaf Visuals And Semantic Composites

The scene API should distinguish low-level render leaves from semantic objects that may compose
several leaves.

`DvzVisual` is the public handle for leaf render families such as point, marker, segment, path,
mesh, image, sphere, and volume. These families map closely to one visual data contract and one
rendering path.

Semantic scene objects may own, derive, or coordinate one or more leaf visuals internally. Examples
include axes, colorbars, annotations, orientation gizmos, and future polygon or graph objects. These
objects should expose typed APIs for normal use rather than requiring users to manage their internal
visual composition.

Composite object constructors should follow the existing scene style:

```text
dvz_<object>(scene, flags)
```

Composite role/property setters should follow the cross-module role/property rule:

```text
dvz_<object>_<role>_<property>()
```

Examples:

```text
dvz_polygon_set_fill_color()
dvz_polygon_set_stroke_width_px()
dvz_graph_set_node_sizes()
dvz_graph_set_edge_colors()
```

Advanced APIs may expose generated visuals by stable role names, such as `"fill"`, `"stroke"`,
`"nodes"`, or `"edges"`, but this should be an escape hatch for integration and tests. It should not
replace typed object APIs for common user workflows.

Vector APIs should follow the same boundary. The public vector visual exposes one semantic mutation
surface for straight and curved vectors while lowering internally to generated leaf roles such as
`"shaft"`, `"head_start"`, and `"head_end"`. Arrowhead behavior should be shared across straight and
curved variants through one head-style semantic contract rather than per-leaf ad hoc setters.

Scientific plotting APIs should follow the same leaf-versus-semantic split. Datoviz may expose
retained scene building blocks for guide lines/spans, bars or interval series, bands/ribbons, and
trace collections when those objects need panel-domain attachment, retained rendering, annotation
integration, or coordinated visual roles. GSP/VisPy2 owns high-level plotting functions, statistical
transforms, Python data adaptation, and domain-specific recipes. The design boundary is recorded in
[`../composites/SCIENTIFIC_PLOTTING_BOUNDARY.md`](../composites/SCIENTIFIC_PLOTTING_BOUNDARY.md).


## Retained Visual Item Range API

A narrow pre-RC public visual-range surface is allowed because it is generic, FFI-friendly, and
lowers to existing DRP2 draw offset/count fields. It must not introduce domain-specific temporal,
event, particle, or track semantics.

Proposed public shape:

```c
typedef struct DvzItemRange
{
    uint32_t first_item;
    uint32_t item_count;
} DvzItemRange;

int dvz_visual_set_item_range(DvzVisual* visual, uint32_t first_item, uint32_t item_count);
DvzResult dvz_visual_clear_item_range(DvzVisual* visual);
bool dvz_visual_get_item_range(const DvzVisual* visual, DvzItemRange* out);
```

API rules:

1. `first_item` and `item_count` use logical item units;
2. `item_count == 0` renders nothing;
3. clearing the range restores the full visual;
4. invalid ranges fail validation;
5. range changes do not upload data;
6. pick/query results use global item identity;
7. family specs own support status and lowering details.

This API is not a substitute for future scalar mappings, broader attribute views, visual modifiers,
custom visuals, GPU compaction, or indirect draw.


## Transform And Controller Semantics

The public API distinguishes three transform owners:

1. **visual-local transform**: retained on a `DvzVisual` with
   `dvz_visual_set_transform()`, `dvz_visual_clear_transform()`,
   `dvz_visual_has_transform()`, and `dvz_visual_get_transform()`;
2. **object/model arcball**: `DvzArcball` is an object/model controller and does not move the panel
   camera;
3. **turntable camera orbit**: `DvzTurntable` moves the panel camera around a stable-up pivot and
   does not mutate visual-local model matrices.

The effective world transform for controller-attached visuals is:

```text
clip = projection * view * controller_model * visual_local_model * position
```

For turntable and fly camera motion, `controller_model` remains identity and the camera effect is in
`view`. For object/model arcball, the camera view remains stable and the arcball contribution is
model-space. Fixed overlays skip panel/controller model participation as before, but still apply
their retained visual-local transform.

Examples should use these concepts intentionally:

1. use visual-local transforms for object spin, object placement, and per-visual presentation;
2. use object/model arcball when user input should manipulate model state under a fixed camera;
3. use turntable or fly pivot gestures when user input should inspect a stable scene from different
   viewpoints;
4. avoid baking controller motion into vertex attributes or field payloads.

Inspection APIs should copy state out, never expose mutable internal pointers. This keeps bindings
and UI inspectors stable while allowing renderer-side transform storage to change.

Nonlinear coordinate transforms are deferred. Do not add public v0.4 setters or descriptor fields
that accept projection parameters without executing them. Future transform fields should either be
appended to `DvzVisualAttachDesc`, which already has the public struct ABI prologue, or introduced
through a new growable descriptor with the same `struct_size`/`flags` convention. Current v0.4
examples should use CPU-side pre-projection before upload for polar/geographic coordinates.

Custom visual/render shaders are also deferred. `DvzSceneComputeDesc` remains the narrow
experimental compute-to-render interop path and must not be documented as built-in visual shader
replacement. Built-in visual shader identity may be recorded in DRP2 metadata for tooling and
replay, but the shader ABI remains scene-internal unless a future custom visual family API
explicitly exports it.


## Panel View And Coordinate Spaces

The public v0.4 coordinate-space surface is:

1. `DVZ_VISUAL_COORD_VIEW`: metric panel view coordinates, affected by panel view/framing policy;
2. `DVZ_VISUAL_COORD_DATA`: source data/domain coordinates mapped through the panel DATA-to-VIEW model;
3. `DVZ_VISUAL_COORD_PANEL`: normalized panel coordinates over the full panel rectangle, intentionally
   viewport-shaped.

`DVZ_COORD_VISUAL` is not part of the v0.4 release surface. Callers must choose the intended
coordinate space explicitly instead of relying on the v0.4-dev compatibility alias.

Panel 2D aspect policy belongs to the panel view/framing API. Domain-fit compatibility aliases are
not part of the release surface:

1. `DvzPanelDomainFit`;
2. `DvzPanelDomainFitMode`;
3. `DVZ_PANEL_DOMAIN_FIT_*`;
4. `DVZ_PANEL_DOMAIN_ASPECT_*`;
5. `dvz_panel_domain_fit()`;
6. `dvz_panel_set_domain_fit()`;
7. `dvz_panel_clear_domain_fit()`.

The release-candidate names for the surviving view/framing surface are
`DvzPanelView2DDesc`, `dvz_panel_view2d_desc()`, `dvz_panel_set_view2d()`,
`dvz_panel_clear_view2d()`, and `dvz_panel_view2d_extent()`. The ownership split is fixed: source
panel domains are not mutated to apply equal-aspect framing.


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
9. `DvzLegend`
10. `DvzFont`
11. `DvzText`
12. `DvzAnnotation`
13. `DvzOrientationGizmo`

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
dvz_orientation_gizmo_set_layout(gizmo, &layout)
```

The descriptor should cover:

1. anchor corner or normalized panel-local rectangle;
2. pixel or panel-relative size;
3. visibility;

The source is the panel passed at creation time. The gizmo is passive: during frame preparation it
observes that panel's effective rendered orientation, including camera/view context and any bound
controller model transform.

4. optional axis length and style knobs.

The implementation should lower the triad to ordinary generated mesh geometry. Geometry generation
belongs in `geom` through `dvz_geometry_gizmo_axes()` once that generator lands; scene/app owns inset
placement, synchronization, redraw requests, viewport/scissor handling, and depth policy.

Do not use the old v0.3 `panel.gizmo()` name as the C contract. Bindings may add a shorter alias
later, but the C API should keep `orientation` in the name because an interactive transform gizmo is
a separate future feature.

The transform gizmo should use a distinct future handle such as `DvzTransformGizmo`. It depends on
object transform ownership, selection state, and richer picking identities, and must not be coupled
to the passive orientation widget.


## SampledField Surface

The installed public scene API includes `DvzSampledField` as the shared regular-grid data object.

The current surface exposes:

1. the dedicated public subheader `include/datoviz/scene/field.h`,
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

The installed public interaction surface includes:

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


## Scale, Colormap, Colorbar, And Legend Surface

Scales and colormaps are scene-owned semantic objects.

Colorbars are explanatory objects bound to a scale; they do not own the scale or the colormap.
They may be attached to a panel edge with a fixed pixel reserve or detached with explicit anchored
panel/figure pixel placement.

Legends are distinct explanatory objects bound to categorical scales. They should use a dedicated
`DvzLegend` handle rather than hiding categorical behavior behind `DvzColorbar`, because legends and
colorbars have different content models, validation rules, and future interaction policies.

The first public surface should expose:

1. continuous scale creation and domain/view-range setters,
2. unit and label metadata,
3. built-in colormap selection,
4. custom color-stop ramps,
5. retained categorical scale entries with stable ids, order, labels, and sample colors,
6. diverging center support,
7. panel-attached colorbar creation,
8. panel pixel reserve accessors and plot-rectangle queries,
9. colorbar orientation, anchor, placement, title, geometry, and formatting overrides,
10. panel-attached legend creation for categorical scales,
11. legend title, anchor, placement, geometry, and visibility overrides.

Installed colorbar layout mutation is represented by `dvz_colorbar_set_layout()`, which accepts the
same `DvzColorbarDesc` layout fields used at creation time. This avoids destroy/recreate churn in
GUI and application code.

Axis layout uses the same resolved panel reserve model. `DvzAxisStyle` carries fixed-pixel reserve,
tick-gap, and label-gap fields; attached X/Y axes contribute bottom/left reserve bands when an
explicit positive axis reserve is configured, while colorbars contribute the edge selected by their
anchor. The resolved plot rectangle combines these contributions with the explicit user/base panel
reserve.

Interactive range editing should be represented as interaction policy on the scale/colorbar pair,
not as an external UI-only behavior.

The first legend surface should be deliberately narrow:

```text
DvzLegend
DvzLegendDesc
dvz_legend(panel, scale, desc)
dvz_legend_destroy(legend)
dvz_legend_set_layout(legend, desc)
dvz_legend_set_title(legend, title)
```

`DvzLegendDesc` should mirror the implemented colorbar placement vocabulary where it applies:
attached panel-edge placement contributes fixed logical-pixel reserve to the selected edge, while
detached placement uses `DvzPlacement` in panel or figure pixel space. The first implementation may
support only vertical stacked entries, but the descriptor should leave room for a future horizontal
or wrapped layout without changing the constructor signature.


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

For v0.4, `DvzText*` is the public text handle. `dvz_glyph()` may remain low-level, but axes,
colorbars, annotations, legends, and readouts should consume semantic text or annotation APIs.


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

Keep tiny usage examples for mesh selection links, image probe readouts, scales/colorbars/
annotations, and sampled fields in
[examples/scenarios/api_sketches/API_PRESSURE_SKETCHES.md](../examples/scenarios/api_sketches/API_PRESSURE_SKETCHES.md).

These examples are API pressure tests. They may reference drafted APIs that are not fully implemented
yet, but awkward examples should still block broadening the public surface.
