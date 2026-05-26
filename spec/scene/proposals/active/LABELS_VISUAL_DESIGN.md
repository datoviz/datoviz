# Labels Visual Design

Status: active v0.4 design proposal with first implementation slices landed.

Current implementation status, as of 2026-05-26:

1. `DvzCategoryId` is a public signed 64-bit category identity used by categorical scales,
   retained scale category state, patch/remove APIs, probe results, and legend fallback labels.
2. The labels visual family exists, accepts 2D integer `DVZ_FIELD_SEMANTIC_LABEL` sampled fields,
   binds categorical scales through the `"labels"` slot, and renders signed/unsigned integer
   textures with exact texel fetch in GLSL and WGSL.
3. Labels presentation state setters exist for opacity, background ID, selected ID, hidden IDs,
   boundary state, fallback seed, and 3D slice placeholders. These are retained state today; shader
   uniforms for selected/hidden/boundary/fallback-seed effects remain follow-up work.
4. Categorical legends support signed and large positive IDs, while colorbars remain
   continuous-scale only. Labels examples should attach legends, not categorical colorbars.
5. `examples/c/showcase/labels.c` is the live labels example: signed integer label field,
   categorical scale, retained labels visual, attached legend, panzoom panel, GUI controls, and a
   working temporary selection highlight overlay until shader-side selected/hidden/boundary
   controls and raw label GPU probing land.


## Purpose

Datoviz should support napari-style label rendering as a first-class scene concept. A labels layer
is not a scalar image with a different colormap. It is an integer segmentation field with stable
category identities, background handling, categorical colors, selection/highlight state, boundaries,
painting updates, and GPU-side label-id probing.

The long-term architecture must keep raw label IDs on the GPU and avoid CPU materialization of RGBA
textures in the normal rendering path. CPU-colored RGBA overlays remain acceptable only as demos or
fallback prototypes and must not become the semantic contract.


## Core Direction

Implement a first-class `labels` visual family rather than only extending `image` with an implicit
label mode.

The labels visual may reuse image placement and generated-quad machinery internally, but it needs
separate validation, shader variants, resource binding, request handling, and public semantics.

Required invariants:

1. source data is an integer `DvzSampledField`;
2. source data is sampled with exact texel/voxel addressing, not linear interpolation;
3. background is ID-based, with `0` as the default transparent background;
4. colors are resolved on the GPU from label ID plus categorical style state;
5. unknown labels get deterministic fallback colors without requiring a dense palette;
6. picking/probing is GPU-side only and returns the raw integer label ID;
7. label color/name/order metadata lives in the categorical scale layer, not in the visual;
8. visual-local overrides are explicitly transient presentation state;
9. categorical label explanations use legends, not continuous colorbars.


## Public API Shape

The preferred public object is a normal retained visual:

```c
DvzVisual* dvz_labels(DvzScene* scene, uint32_t flags);
```

Field and scale binding should follow existing visual-resource conventions:

```c
dvz_visual_set_field(labels, "field", field);
dvz_visual_set_scale(labels, "labels", scale);
```

The bound field must have semantic `DVZ_FIELD_SEMANTIC_LABEL` or an explicitly accepted generic
integer semantic. Accepted first-slice formats are:

```text
R8_UINT, R16_UINT, R32_UINT
R8_SINT, R16_SINT, R32_SINT
```

Signed integer formats are required because real segmentation datasets may use negative IDs. The
default background ID remains `0`, but applications may set a negative background ID when their
data uses one.

Labels-specific setters should configure presentation behavior, not semantic label colors:

```c
int dvz_labels_set_opacity(DvzVisual* labels, float opacity);
int dvz_labels_set_background(DvzVisual* labels, DvzCategoryId label_id);
int dvz_labels_set_selected(DvzVisual* labels, DvzCategoryId label_id);
int dvz_labels_clear_selected(DvzVisual* labels);
int dvz_labels_set_hidden(DvzVisual* labels, const DvzCategoryId* ids, uint32_t count);
int dvz_labels_set_boundary(DvzVisual* labels, bool enabled, float width_px, DvzColor color);
int dvz_labels_set_fallback_seed(DvzVisual* labels, uint32_t seed);
```

For 3D label fields, support an axis-aligned slice path first:

```c
int dvz_labels_set_slice_axis(DvzVisual* labels, DvzVolumeAxis axis);
int dvz_labels_set_slice_position(DvzVisual* labels, double position);
```

Arbitrary MPR and categorical direct-volume rendering are follow-up work.


## Scale Ownership

The persistent `label_id -> rgba/name/order` relationship belongs to a categorical scale.

Use the existing scale direction conceptually:

```c
DvzScale* scale = dvz_scale(scene, &(DvzScaleDesc){
    .kind = DVZ_SCALE_CATEGORICAL,
    .label = "cell",
});

DvzScaleCategory categories[] = {
    {.category_id = 17, .order = 0, .label = "cell 17", .color = {255, 0, 0, 180}},
    {.category_id = 42, .order = 1, .label = "cell 42", .color = {0, 255, 0, 180}},
    {.category_id = -1, .order = 2, .label = "unassigned", .color = {128, 128, 128, 120}},
};

dvz_scale_set_categories(scale, categories, 3);
dvz_visual_set_scale(labels, "labels", scale);
```

`DvzScaleCategory.category_id` uses the shared signed semantic ID type so negative dataset IDs are
preserved without remapping, while large unsigned `R32_UINT` IDs still fit:

```c
typedef int64_t DvzCategoryId;
```

Use this type consistently in scale categories, label visual state, probe results, legends, and
serialization. The GPU shader path must validate that a category ID fits the bound texture format.
For example, `-1` is valid for `R8_SINT`, `R16_SINT`, and `R32_SINT`, but invalid for `R*_UINT`;
`4000000000` is valid for `R32_UINT`, but invalid for `R32_SINT`.

Scale mutation needs efficient batch and patch APIs. Prefer adding scale-level APIs rather than
visual color setters:

```c
bool dvz_scale_set_categories(DvzScale* scale, const DvzScaleCategory* categories, uint32_t count);
bool dvz_scale_update_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count);
bool dvz_scale_remove_categories(DvzScale* scale, const DvzCategoryId* ids, uint32_t count);
```

Every bound labels visual should observe scale dirtiness and upload the GPU style data derived from
the scale. Scale patch/remove APIs exist now; lowering scale entries into a sparse GPU style buffer
is still pending.


## Legends And Colorbars

Labels are categorical data, so their explanatory object is a `DvzLegend`, not a `DvzColorbar`.
The existing colorbar API should remain continuous-scale only and should continue to reject
categorical scales with a diagnostic that points users to legends. A categorical colorbar ramp would
be misleading for sparse, unordered, and possibly negative label IDs.

A labels visual should support the normal retained legend path:

```c
DvzVisual* labels = dvz_labels(scene, 0);
dvz_visual_set_field(labels, "field", field);
dvz_visual_set_scale(labels, "labels", scale);

DvzLegend* legend = dvz_legend(
    panel, scale, &(DvzLegendDesc){
                      .title = "Labels",
                      .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                  });
```

Legend entries are derived from retained categorical scale entries:

1. `DvzScaleCategory.category_id` is the stable semantic ID shown by the legend;
2. `DvzScaleCategory.order` controls deterministic display order, independent of numeric ID;
3. `DvzScaleCategory.label` is the preferred text;
4. missing labels fall back to a signed decimal rendering of `DvzCategoryId`;
5. `DvzScaleCategory.color` is the sample mark color and the explicit GPU style color.

Unknown hash-colored IDs do not appear in legends automatically because they are not retained
semantic categories. If an application wants unknown observed IDs in a legend, it should promote
them into the categorical scale through `dvz_scale_update_categories()`.

Compatibility work required by signed label IDs is now mostly landed:

1. `typedef int64_t DvzCategoryId` is public scene surface;
2. `DvzScaleCategory.category_id` and retained `DvzScaleCategoryState.category_id` use
   `DvzCategoryId`;
3. duplicate detection, category patch/remove APIs, probe category IDs, and legends use the shared
   type;
4. legend fallback labels use signed 64-bit formatting;
5. categorical colorbar rejection coverage remains in place, and signed-ID legend coverage includes
   negative and large positive IDs.


## Visual Overrides

Avoid APIs such as `dvz_labels_set_color()` for the canonical label color table. They blur the
boundary between semantic category state and visual-local presentation.

Visual-specific style overrides are still useful when they are explicit:

```c
int dvz_labels_set_style_overrides(
    DvzVisual* labels, const DvzLabelStyleOverride* overrides, uint32_t count);
int dvz_labels_clear_style_overrides(DvzVisual* labels);
```

These overrides are local presentation state. Practical use cases:

1. one panel shows all labels normally while another emphasizes a subset;
2. hover or selection highlights without mutating dataset category colors;
3. review workflows dim rejected labels or flash recently edited labels;
4. one overlay renders filled labels while another renders only boundaries;
5. multiple labels visuals share one categorical scale with different opacity or hidden-ID filters.


## Sparse GPU Style Lookup

Label IDs may be arbitrary and large because they come from the original integer data type. The
system should not allocate by maximum label ID.

Use a sparse-first GPU style model:

```text
raw integer label ID
    -> background/hidden/selected tests
    -> explicit sparse style lookup
    -> deterministic hash-color fallback
```

For the expected common case of hundreds to thousands of distinct labels, the default GPU structure
should be a sorted sparse style buffer:

```c
typedef struct DvzLabelGpuStyle
{
    uint32_t label_id_bits; /* raw integer payload in the bound texture format */
    uint32_t rgba;      /* packed RGBA8 */
    uint32_t flags;     /* hidden, selected, reserved */
    uint32_t reserved;
} DvzLabelGpuStyle;
```

The shader performs binary search over the sorted style table. This is compact, deterministic, and
simple enough for the first implementation: 1024 styles require about ten comparisons, and 4096
styles require about twelve. The scene lowers `DvzCategoryId` values to `label_id_bits` only after
validating them against the bound texture format. Sort order is format-specific: signed integer
textures sort by signed value, and unsigned integer textures sort by unsigned value.

Dense palettes should be an internal optimization only when the renderer proves the IDs are compact
enough. They are useful for `uint8`, `int8`, small 16-bit formats, or pre-compacted labels, but
they must not be the user-visible semantic model. A GPU hash table is a later optimization for very
large explicit style sets, not the first design.

Unknown IDs use deterministic hash coloring. Users should not have to provide every label color.
Hash-color inputs should include the label ID and a visual-local fallback seed so separate views can
choose coordinated or intentionally different default colors.


## Integer Missing Values

`NaN` and `Inf` handling is not relevant for integer label fields. Missing/background behavior is
ID-based:

```text
id == background_id        -> transparent
id in hidden set           -> transparent or dimmed according to visual policy
id missing from style table -> deterministic fallback color
```

The default background ID is `0`, matching common segmentation data and napari behavior.


## GPU-Only Fields

Labels must support fields whose data exists only on the GPU. This is required for segmentation
models, compute pipelines, CUDA or platform interop, and future napari adapters where the producer
already owns a GPU texture.

A CPU-owned label field has CPU data and dirty regions:

```text
DvzSampledField
  desc: R32_UINT, 2048x2048
  CPU data pointer: present
  dirty regions: upload to GPU
```

A GPU-only label field has a descriptor and runtime resource identity but no CPU payload:

```text
DvzSampledField
  desc: R32_UINT, 2048x2048
  CPU data pointer: NULL
  runtime texture/resource label: present
  ownership: borrowed or externally produced
```

The scene should be able to bind such a field to a labels visual and emit resource requirements
without uploading CPU bytes. Ownership must be explicit:

1. scene-owned CPU fields upload through frame-plan texture writes;
2. runtime-owned or external fields are borrowed by scene visuals;
3. borrowed textures are never destroyed or transitioned by the scene layer;
4. GPU probe requests read from the runtime texture, not from a CPU mirror.


## Rendering

Add separate shader variants instead of reusing scalar image colormap shaders:

1. `labels_2d` for 2D integer textures;
2. `labels_slice` for axis-aligned slices through 3D integer textures;
3. WGSL equivalents alongside GLSL from the start.

Use integer texture fetch:

```text
id = textureLoad(labels_tex, pixel_coord).r
```

or GLSL equivalent `texelFetch()` with `usampler2D` / `usampler3D` for unsigned formats and
`isampler2D` / `isampler3D` for signed formats.

Do not use normalized floating-point `sampler2D` texture filtering for label IDs. The shader should
derive integer texel coordinates from the interpolated UV and texture extent, then fetch exact IDs.

Fragment behavior:

```text
id = load_label(pixel_or_voxel_coord)
if id == background_id: discard or alpha = 0
style = lookup_sparse_style(id)
color = style.color if found else hash_color(id, fallback_seed)
apply hidden/selected/boundary policy
out = vec4(color.rgb, color.a * opacity)
```

Boundary mode compares neighboring IDs in texel space:

```text
id0 = label(x, y)
id1 = label(x + 1, y)
id2 = label(x, y + 1)
boundary = id0 != id1 || id0 != id2
```

The first 3D path should display one slice of a 3D integer field. Categorical DVR is deferred
because compositing category IDs along a ray is semantically ambiguous.


## GPU Probing

CPU-side label probing is not an acceptable implementation path.

The probe path must read the raw integer label ID from the GPU texture or an equivalent GPU request
target. The result then carries scene identity:

```text
value_kind  = LABEL
category_id = raw label ID
visual_id   = labels visual ID
coordinate  = data/world coordinate when available
uvw         = normalized texture coordinate
label       = scale label if available, otherwise "label <id>"
```

Resolving a human-readable string from the retained scale after GPU readback is allowed because the
identity came from the GPU. Returning labels by decoding rendered RGBA colors is not sufficient for
the final path.

Implementation plan:

1. Treat `DVZ_VISUAL_TYPE_LABELS` as probe-capable for `DVZ_SCENE_TARGET_SEGMENT` when the visual
   has a bound integer `DvzSampledField`.
2. Add a labels-specific probe dispatch in `src/scene/request_execute.c` instead of routing labels
   through the existing image RGBA probe decoder.
3. Reuse the labels/image quad geometry and texcoords to map the panel request coordinate to labels
   UV, then map UV to an exact integer texel coordinate. The mapping must match the labels fragment
   shader convention and must miss outside the labels quad.
4. Read one texel from the labels visual's integer GPU texture, preserving texture signedness and
   width:
   `R8_SINT`, `R16_SINT`, `R32_SINT`, `R8_UINT`, `R16_UINT`, and `R32_UINT`.
5. Decode the raw texel into `DvzCategoryId` without going through rendered colors. For example,
   `R32_SINT` `-7` returns `category_id = -7`, and `R32_UINT` `4000000000` returns
   `category_id = 4000000000`.
6. Apply background policy after readback. The default `background_id == 0` should return a miss
   unless a future probe flag explicitly requests background hits.
7. Resolve the optional display label from the bound categorical scale after GPU readback. Missing
   scale entries should fall back to signed decimal ID formatting.
8. Update `examples/c/showcase/labels.c` so hover and click both queue labels probes and consume
   `dvz_scene_poll_probe()` results. Remove the temporary CPU `_label_at_pointer()` path from the
   example once this lands.
9. Add focused coverage:
   2D signed labels probe with `-7`, 2D unsigned labels probe with `4000000000`, background miss,
   scale-label resolution, panzoom-transformed coordinate mapping, and a regression proving no
   hidden RGBA image visual is required.

The existing hidden RGBA image probe route may remain only as a compatibility path for image-based
segment masks. It is not the labels visual contract because it cannot preserve negative IDs, is
limited by the encoded payload width, and duplicates the labels texture.


## Live Example Target

The live labels example target is:

```text
examples/c/showcase/labels.c
```

Current concept: an interactive 2D segmentation board with a synthetic microscopy underlay and a
labels overlay. The label texture is a signed `R32_SINT` sampled field with `0` background,
negative IDs such as `-7` and `-100`, sparse positive IDs such as `17`, `42`, `89`, and `1009`,
and additional positive sparse IDs.

The first screen should be the working visualization, not a landing page:

1. central panzoom panel with the image and labels overlay;
2. right-side attached legend titled "Labels";
3. compact GUI panel for working visibility and selection controls;
4. hover readout showing raw ID through a temporary CPU coordinate lookup;
5. optional future status text for texture format and whether the field is CPU-owned or GPU-only.

Useful GUI controls:

1. opacity slider wired to `dvz_labels_set_opacity()`;
2. background ID selector/input, including `0` and `-1`;
3. selected-label dropdown populated from the categorical scale;
4. per-category visibility checkboxes that update hidden IDs;
5. boundary enable toggle, width slider, and color swatch;
6. fallback seed stepper to demonstrate deterministic unknown-label colors;
7. category table editor for color, label, and order changes;
8. buttons for "add unknown IDs to legend", "shuffle colors", and "reset scale";
9. texture format selector for signed and unsigned first-slice formats;
10. probe mode toggle that shows the GPU-returned raw ID without reading CPU data.

The current example intentionally exposes only controls that visibly work today: labels overlay
visibility, selected-label dropdown/click selection, selection-highlight visibility, outline width,
clear selection, reset view, and hover/selected readout. Selection feedback uses a transparent
highlight image overlay generated from the signed label field; the labels visual remains backed by
the raw integer texture. The example also exercises the runtime compatibility requirement that
integer labels textures use nearest samplers, including mipmap mode, not linear sampling.

Remaining example upgrades should become small incremental patches as the underlying labels
features land:

1. replace the temporary CPU hover/click lookup with raw labels GPU probing;
2. make opacity, selected, hidden, boundary, and fallback-seed controls affect the labels shader;
3. add scale editing controls once categorical scale updates lower into GPU style buffers;
4. add an unsigned `R32_UINT` mode showing very large IDs such as `4000000000`;
5. add an optional GPU-only field mode when borrowed texture binding exists.

The example should intentionally demonstrate the legend/colorbar boundary. It should create a
legend for the categorical labels scale and should not create a colorbar for labels. A nearby scalar
image underlay may have its own continuous colorbar only if that helps show the distinction.


## DRP2 And Runtime Requirements

The labels visual should harden these lower-layer capabilities:

1. typed integer 2D and 3D texture creation and upload for `R8_UINT`, `R16_UINT`, `R32_UINT`,
   `R8_SINT`, `R16_SINT`, and `R32_SINT`;
2. frame-plan upload metadata carrying non-RGBA texel sizes and texture formats;
3. runtime bind layouts for integer sampled textures plus uniform/storage style resources;
4. exact texel fetch shaders in GLSL and WGSL;
5. GPU readback path for a single integer label ID;
6. resource binding for GPU-only borrowed textures;
7. explicit validation when a backend cannot support the requested integer format or readback path.


## Implementation Phases

1. Done: add the public visual family enum/type, retained labels state, validation, and API.
2. Done: introduce signed `DvzCategoryId` and migrate categorical scale/probe/legend paths that
   need label IDs.
3. Done: harden categorical legends for signed IDs: retained category storage, duplicate detection,
   fallback formatting, scale dirtiness, and negative-ID tests.
4. Done: keep `DvzColorbar` continuous-only and retain categorical-scale rejection diagnostics.
5. Partly done: implement 2D label rendering with integer texture fetch, background transparency,
   and hash fallback colors. Shader-driven opacity is still pending.
6. Partly done: add scale patch/remove APIs. Lowering categorical scale entries to a sorted sparse
   GPU style buffer is pending.
7. Pending: add selected, hidden, fallback-seed, and boundary uniforms.
8. Pending: add GPU probe/readback returning raw label IDs.
9. Pending: add 3D axis-aligned label slice rendering.
10. Pending: add GPU-only field/resource binding for labels.
11. Done for first slice: add WGSL parity and fixture/preflight coverage for 2D labels.
12. Done for first slice: add a live labels example with a proper legend and GUI controls for the
    label settings.
13. Pending: add napari large-label pressure tests with arbitrary sparse IDs and thousands of
    categories.


## Non-Goals For The First Slice

1. CPU RGBA recoloring as the main implementation.
2. Dense palette as the public contract.
3. CPU-side picking/probing.
4. Arbitrary MPR.
5. Categorical DVR.
6. Painting/editing UI.
7. GPU hash-table lookup.
8. Out-of-core tiled labels.

These are valid future directions, but they should not delay the first correct architecture.
