# Preferred Scene API Profile

This document selects the current preferred scene-facing API defaults for Datoviz v0.4.

`spec/scene/headers/scene_api.h` is the authoritative draft C spelling. This document remains
normative for the design rationale and the Python binding architecture.


## Position

This document sits:

1. above `OBJECT_MODEL.md`,
2. above `VISUAL_CONTRACT.md` and `pipeline/RESOURCE_MODEL.md`,
3. below `headers/scene_api.h`, which is the authoritative C spelling.


## Normative Status

This document is normative for the current preferred API direction.

It should be read as:

1. recording the design rationale behind `headers/scene_api.h`,
2. constraining implementation-oriented exploration to one coherent profile,
3. leaving only a small set of explicitly deferred questions open.

Rationale paragraphs and examples in this document are informative.


## Language Target

The C scene API is the primary and canonical public surface.

It must be designed as an FFI target:

1. opaque handles only in public headers,
2. descriptor structs for construction,
3. explicit lifecycle with paired create/destroy,
4. no raw function pointers in public structs.

Python binds to the C API through an auto-generated ctypes binding layer (`datoviz/_ctypes.py`),
produced by `tools/parse_headers.py` and `tools/build_ctypes.py` — the same pipeline used in
v0.3, updated for v0.4 headers.
A thin Python sugar layer sits above the generated binding and adds ergonomics (NumPy arrays,
keyword arguments, context managers, inline scale shortcuts).
All scene logic lives in C; the Python layers add no logic of their own.

See `IMPLEMENTATION_NOTES.md` for the full three-tier binding architecture and v0.3 pipeline
details.


## Core Rule

The preferred scene API profile should expose semantic scene objects and semantic resource roles,
while keeping planning, validation, adaptation, and runtime submission explicit in the model.


## Preferred Construction Model

The preferred construction model is:

1. family-specific constructors for visual creation — each visual type has its own creation
   function because resource schemas and initialization requirements differ,
2. generic attribute write functions for data upload — family-specific data preparation is an
   implementation concern, not a scene API concern; the write surface is uniform,
3. allocation separated from creation — a visual is created without a committed item count;
   data is written when available and size is established on first write,
4. optional explicit pre-allocation hint for performance when count is known upfront.

Conceptually:

```c
// Family-specific creation (variant axes encoded as flags)
DvzVisual* point = dvz_point(scene, 0);
DvzVisual* path  = dvz_path(scene, 0);
DvzVisual* image = dvz_image(scene, DVZ_IMAGE_RGBA);

// Optional pre-allocation hint
dvz_visual_alloc(point, n);

// Generic attribute write — uniform across all visual types.
// Attribute name is a string matching the spec name ("position", "color", …).
// n = 1 → CONSTANT source; n = item_count → PER_ITEM; n = group_count → PER_GROUP.
dvz_visual_set_data(point, "position", xyz,  n);
dvz_visual_set_data(point, "color",    rgba, n);

// For span-structured visuals (path, glyph), span sizes declare topology.
// Semantic groups use a separate "group_id" attribute.
dvz_visual_set_data(path, "span_sizes", path_sizes, n_paths);
```

Attribute names are plain strings matching the per-family spec names. There is no
`DVZ_ATTR_*` enum. Visual handles are opaque `DvzVisual*`; there is no `DvzVisualType`
enum in the public API — family identity is fixed at construction and not queried.


## Preferred Ownership Model

The preferred ownership model is:

1. `Scene` owns shared resources (scales, fonts, textures), global invalidation state, and the
   collection of figures,
2. `Figure` owns layout (panels, margins, render-target binding) — one figure per output window
   or offscreen target; one scene may have multiple figures (e.g. two windows sharing a GPU
   context),
3. `Panel` owns panel-local view state, controllers, axes, and panel-local attachments,
4. one `FramePlan` per figure is built each frame,
5. the runtime remains below the scene semantic layer.


## Preferred Visual And Resource Binding Model

The preferred binding model is:

1. visuals bind resources by semantic role,
2. resource roles come from scene contract vocabulary, not slot numbers,
3. grouped resources remain explicit scene concepts.

Conceptually:

```c
dvz_visual_set_texture(visual, "texture", tex);   // bind a DvzTexture* to a named slot
dvz_visual_set_scale(visual,   "colormap", scale); // bind a DvzScale* to a named slot
```

Resource roles are identified by the slot name string from the per-family spec (e.g. `"texture"`,
`"colormap"`). There is no numeric role enum or public parameter-block struct.
Item and indexed-geometry resources are managed internally through `dvz_visual_set_data`.


## Preferred Parameter Model

The preferred parameter model is:

1. named string parameter setters as the primary primitive,
2. optional reusable `DvzStyle` objects for groups of visual defaults,
3. variant selection passed as flags at construction time,
4. Python sugar layer adds keyword-argument convenience above the C setters.

Conceptually:

```c
// Visual-wide parameters set by name (value is a typed pointer)
dvz_visual_set_param(visual, "linewidth", &lw);
dvz_visual_set_param(visual, "size_space", &space);

// Optional reusable defaults.
DvzStyle* style = dvz_style(scene);
dvz_style_set_param(style, "linewidth", &lw);
dvz_visual_set_style(visual, style);

// Mutability hint (optional, default is DYNAMIC)
dvz_visual_set_mutability(visual, "position", DVZ_MUTABILITY_STATIC);
```


## Preferred Mapping And Explanation Model

The preferred explanation model is:

1. explicit mapping identity by default,
2. legends and colorbars attach to that mapping identity,
3. implicit aggregation is allowed only when the mapping identity is semantically identical.

Conceptually:

```c
// Typed constructors are preferred
DvzScale* scale = dvz_scale_color(scene, "viridis", 0.0, 1.0);
dvz_visual_set_scale(visual, "colormap", scale);

// Updates without re-uploading data
dvz_scale_set_domain(scale, new_min, new_max);
dvz_scale_set_colormap(scale, "plasma");

// Colorbar attaches to the same scale identity via the legend/colorbar API
```

The explicit scale handle preserves sharing identity — multiple visuals or a colorbar can
reference the same `DvzScale*`. An inline anonymous shortcut is available at the Python sugar
layer for single-visual cases. See `SCALES.md` for the full model.


## Preferred Validation And Adaptation Surface

The preferred validation and adaptation surface is:

1. explicit validation entry points remain representable,
2. explicit capability snapshot and policy entry points remain representable,
3. frame build may fuse them operationally, but not semantically.

Conceptually:

```c
dvz_scene_validate(scene, &report);
dvz_scene_set_capabilities(scene, &caps);  // caps queried from dvz_runtime_get_capabilities()
dvz_scene_adapt(scene, &report);
```

The required ordering remains:

1. invalidation resolution,
2. validation,
3. capability adaptation,
4. scene-level `FramePlan` construction.


## Preferred Picking Model

The preferred picking model distinguishes two kinds:

1. **Click and query picking** — synchronous, blocking. `dvz_panel_pick` submits a request and
   blocks until the result arrives via the runtime's synchronous completion helper. This is the
   primary API for interactive click selection and tool-driven inspection.
2. **Hover picking** — asynchronous callback. The scene delivers results via a callback registered
   with `dvz_panel_on_hover`. Latest-request-wins semantics: stale hover results are discarded.

In both cases, results carry scene-visible identity (panel, visual, item, group) rather than
backend identity. See `interaction/PICKING.md` for the complete model.


## Preferred Build And Submission Surface

The preferred build and submission model is:

1. scene mutation is separate from redraw requests,
2. redraw requests are separate from frame build,
3. frame build is separate from runtime submission,
4. runtime submission stays below the scene API boundary.

Conceptually:

```c
// DvzFigure is the per-window layout object; redraw and frame build are per-figure.
dvz_figure_request_redraw(fig, DVZ_REDRAW_SCENE, NULL);
DvzFramePlan* fp = dvz_figure_build_frame(fig, DVZ_BUILD_FLAGS_NONE, &report);
DvzDrp2CommandStream* commands = dvz_frame_plan_emit_drp2(fp, &report);
dvz_runtime_submit_commands(rt, commands);
dvz_frame_plan_destroy(fp);
```

The public API may wrap conversion and submission in `dvz_runtime_submit_frame_plan()`, but the
primary runtime boundary is the DRP2 command stream.

**Terminology note**: `canvas` (from `RUNTIME_BOUNDARY.md`) is the application-owned window +
swapchain object. The scene never holds a canvas reference. A `DvzRenderTarget` is the
scene-facing logical output handle resolved by the runtime — not a canvas.


## Preferred Error And Diagnostics Surface

The preferred diagnostics model is:

1. scene validation and adaptation produce scene-visible diagnostics,
2. runtime failures are mapped back to scene-visible plan, target, or resource identity,
3. backend handles never become required to interpret failures.


## Resolved C API Decisions

**Naming convention** — `dvz_` prefix throughout. Opaque handles (`DvzScene*`, `DvzFigure*`,
`DvzPanel*`, `DvzVisual*`, etc.). Descriptor structs named `DvzXxxDesc`. There is no
`DvzVisualType` enum in the public API — family identity is fixed at construction by the typed
constructor and is not queryable. The internal spec term "family" is not exposed to users.

**Constructors** — family-specific constructors as the primary public surface (`dvz_point()`,
`dvz_path()`, `dvz_image()`, etc.) because resource schemas and initialization requirements
differ per type. A generic `dvz_visual_create()` may exist internally but is not the user-facing
default.

**Data upload** — `dvz_visual_set_data(visual, attr_name, data, n)` uniform across all visual
types. `attr_name` is the string from the per-family spec ("position", "color", …). `n`
determines the source: `1` → CONSTANT, `item_count` → PER_ITEM, `span_count` → PER_SPAN,
`group_count` → PER_GROUP.
Span sizes for span-structured visuals (path, glyph) are written via the `"span_sizes"` attribute.
Semantic groups are written through `"group_id"`: per item for flat visuals, per span for
span-structured visuals.
Partial updates via `dvz_visual_set_data_range`. Mutability hints via `dvz_visual_set_mutability`.

**Allocation** — separated from creation. Visuals are created without a committed item count.
Size is established on first write. An optional `dvz_visual_alloc(visual, n)` hint is available
for pre-allocation.

**Parameters** — `dvz_visual_set_param(visual, name, value)` as the primary surface. No
per-family typed setter functions and no public parameter-block struct. `DvzStyle` is an optional
reusable defaults object layered over the same named parameters. The Python sugar layer adds
keyword-argument convenience above the named-param setter.

**Picking** — click and query picking are synchronous and blocking:
`dvz_panel_pick(panel, x, y, DVZ_PICK_CLICK, &result)` returns a filled `DvzPickResult`.
Hover picking is asynchronous via a callback:
`dvz_panel_on_hover(panel, my_hover_callback, user_data)`.
See `interaction/PICKING.md` for the full model including latest-request-wins hover semantics.

**`FramePlan` inspection** — readable and serializable through a diagnostics or test interface
only. Not a first-class public user-facing API.

**Inline vs explicit mapping construction** — both are supported. Explicit handles are the
  preferred default and support sharing across visuals and attachment to colorbars. An inline
  shortcut (anonymous mapping, no shareable identity) is available at the Python sugar layer for
  single-visual cases. See `SCALES.md` for the full model.


## Relationship To Other Scene Docs

This document should be read together with:

1. `headers/scene_api.h` for the authoritative C API spelling,
2. `VISUAL_CONTRACT.md` and `VISUAL_FAMILY_RULES.md` for family contract details,
3. `pipeline/RESOURCE_MODEL.md` for logical resource classes,
4. `VALIDATION.md` and `ADAPTATION.md` for stage ordering and failure semantics,
5. `RUNTIME_BOUNDARY.md` for the lower execution boundary and service model,
6. `IMPLEMENTATION_NOTES.md` for one tentative implementation-facing translation of this profile.
