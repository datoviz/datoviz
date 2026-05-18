> **Execution Status**
> - **Status:** `PARTIALLY PROMOTED`
> - **Updated on:** `2026-05-16`
> - **Purpose:** preserve `SampledField` API rationale and unresolved choices after promotion of
>   the active resource/visual rules.

# SampledField API Design

This promoted note preserves `SampledField` rationale, API sketches, and unresolved choices from
the earlier sampled-field design pass. Current resource, image, volume, validation, and public API
rules now live in specialized specs.

The original design goal was to give implementation work one explicit contract for:

1. regular 2D and 3D sampled data,
2. scalar, vector, and multi-channel color-like payloads,
3. shared data ownership across image, volume, probe, and future field consumers,
4. CPU fallback and GPU-native execution without changing scene semantics.


## Authority Note

The active `SampledField` resource contract is now owned by
[`../../pipeline/RESOURCE_MODEL.md`](../../pipeline/RESOURCE_MODEL.md). Image and volume binding rules
belong in [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md) and
[`../../visuals/VOLUME.md`](../../visuals/VOLUME.md). Public API shape belongs in
[`../../api/API_SURFACE.md`](../../api/API_SURFACE.md) and implementation-readiness tracking belongs in
[`../../api/API_IMPLEMENTATION_READINESS.md`](../../api/API_IMPLEMENTATION_READINESS.md).

This proposal remains useful for detailed enum/API sketches, rationale, and open choices. Do not
copy its long format lists into other specs unless those specs need the normative detail.
The detailed sections below are historical design material unless the owning specialized specs
explicitly cite or absorb them.


## Historical Objective

`SampledField` should become the scene-owned data object for regularly sampled raster or voxel data.

It should replace the current drift toward visual-private texture payloads such as:

1. image-owned RGBA texture bytes,
2. image-owned scalar F32 fallback staging,
3. future volume-private voxel upload paths.

The resource should be authoritative scene state. Any runtime-ready textures, staging buffers, or
derived colorized RGBA caches are execution artifacts, not the semantic source of truth.


## Core Recommendation

Use one opaque scene-owned `DvzSampledField` handle for regular sampled-grid data.

Recommended split:

1. `SampledField` owns sampled data layout, dimensions, format, and optional physical metadata,
2. visuals bind one or more sampled fields and decide how to interpret them,
3. scales, colormaps, transfer functions, and annotations remain separate retained semantics,
4. CPU and GPU execution modes materialize derived resources from the same field object.

This makes image, volume, probe, and future vector-field behavior coherent without forcing the
public API to expose backend texture details.


## Why This Matters

Without a shared field object:

1. images and volumes drift into parallel upload APIs,
2. probe/readout code has no shared semantic source for sampling metadata,
3. field data reuse across several visuals becomes ad hoc,
4. CPU fallback code starts to masquerade as the data model,
5. a later GPU-side scalar-image path risks API churn instead of a backend upgrade.


## Scope

The first `SampledField` design should cover:

1. regular 2D fields,
2. regular 3D fields,
3. `1..4` channels per sample,
4. unsigned integer and floating-point component formats,
5. scene-owned full replacement and subregion updates,
6. optional axis order and flip metadata,
7. optional origin, spacing, and unit metadata,
8. image and volume visual binding,
9. probe/readout metadata access.

It should be designed so that these future consumers fit naturally:

1. vector-field slice viewers,
2. label or mask overlays,
3. atlas-like glyph or sprite resources,
4. offscreen render outputs promoted into scene-owned resources.


## Non-Goals For The First Slice

The first `SampledField` design should not require:

1. irregular meshes or sparse fields,
2. compressed texture formats,
3. mipmap policy in the public scene API,
4. automatic file I/O loaders,
5. shared ownership across scenes,
6. full tensor semantics beyond component-count metadata,
7. public exposure of backend texture handles or sampler objects.


## Semantic Boundary

`SampledField` means:

1. one regular sampled grid,
2. one component layout,
3. one component scalar type,
4. one authoritative data payload,
5. optional metadata about physical arrangement and sample meaning.

It does **not** mean:

1. one specific shader interpretation,
2. one specific visual family,
3. one guaranteed GPU texture object,
4. one colormap,
5. one transfer function,
6. one screen-space representation.


## Capability Model

The resource should describe **storage** and **grid geometry**, while **interpretation** remains a
consumer concern.

That means one `SampledField` may represent:

1. a 2D scalar image,
2. a 3D scalar volume,
3. a 2D RGBA image,
4. a 3D RGB or RGBA voxel field,
5. a 2D vector field with two or three components,
6. a 3D vector field with three components,
7. a two-channel field later interpreted as magnitude/phase or complex-like data.

The resource itself should not hard-code which of these is the only valid meaning.


## Grid Model

The first contract assumes a **regular orthogonal grid**.

Required grid properties:

1. dimensionality: `2D` or `3D`,
2. resolution: `width`, `height`, optional `depth`,
3. axis order metadata,
4. axis flip metadata.

Optional physical metadata:

1. origin,
2. spacing,
3. unit strings.

`SampledField` should not require physical metadata for every use case, but when present it should
be stable and readable by probe/readout, annotation, and volume-slice code.


## Format Model

The resource should carry a public sample format enum.

The format must encode at least:

1. channel count,
2. scalar component type,
3. storage width.

The first enum should be explicit rather than split into separate `channel_count` and
`component_type` fields in the public API, because validation and runtime capability matching will
need concrete format cases anyway.

Recommended baseline formats:

1. `R8_UNORM`
2. `R8_UINT`
3. `R8_SINT`
4. `R8_SNORM`
5. `R16_UNORM`
6. `R16_UINT`
7. `R16_SNORM`
8. `R16_SINT`
9. `R16_FLOAT`
10. `R32_UINT`
11. `R32_SINT`
12. `R32_FLOAT`
13. `RG8_UNORM`
14. `RG8_UINT`
15. `RG8_SINT`
16. `RG16_UNORM`
17. `RG16_UINT`
18. `RG16_SINT`
19. `RG16_FLOAT`
20. `RG32_UINT`
21. `RG32_SINT`
22. `RG32_FLOAT`
23. `RGBA8_UNORM`
24. `RGBA8_UINT`
25. `RGBA8_SINT`
26. `RGBA16_UNORM`
27. `RGBA16_UINT`
28. `RGBA16_SINT`
29. `RGBA16_FLOAT`
30. `RGBA32_UINT`
31. `RGBA32_SINT`
32. `RGBA32_FLOAT`

The implementation may support a strict subset initially. Unsupported formats should fail
validation explicitly rather than silently adapting to an unrelated format.

The first runtime-ready subset should prioritize:

1. scalar formats that can be sampled and mapped to float for colormap-driven rendering,
2. integer scalar formats suitable for masks and labels,
3. `RGBA*` formats for direct color payloads,
4. no `RGB*` formats in the first public API; callers should use `RGBA*` instead.


## Sample Meaning Metadata

Storage format alone is not enough for higher-level features.

The resource should expose optional semantic hints such as:

1. `SCALAR`
2. `VECTOR_2`
3. `VECTOR_3`
4. `COLOR`
5. `LABEL`
6. `NORMAL`
7. `GENERIC`

These hints are not authoritative rendering modes. They exist so that:

1. validation can catch obviously invalid consumer bindings,
2. probes and UI can present better defaults,
3. volume and image visuals can detect common intended usage.

Interpretation is still finalized by the consumer visual or tool.


## Ownership And Lifetime

Recommended rule:

1. `DvzSampledField` is scene-owned,
2. panels never own a field,
3. visuals borrow field references,
4. a field may be referenced by many visuals and probes,
5. destroying the scene destroys all fields,
6. explicit field destroy API is optional in the first pass but acceptable.

The field should own its CPU-side authoritative payload unless the API very explicitly documents a
borrowed immutable upload mode. The first implementation should prefer copied scene-owned storage so
that retained updates, probing, and CPU fallback behavior are deterministic.


## Mutation Model

The first public API should support:

1. full payload replace,
2. rectangular or box subregion updates,
3. geometry metadata updates that do not replace the payload.

Subregion updates should be specified in sample coordinates, not byte offsets.

Recommended rule:

1. validation checks update bounds against field resolution,
2. field format never changes after creation,
3. dimensionality never changes after creation,
4. subregion updates preserve the existing field descriptor,
5. geometry metadata updates may be allowed independently.


## CPU Versus GPU Execution

The field object is semantic state. Runtime realization is separate.

Execution should be allowed to choose between:

1. direct GPU sampled-texture realization,
2. CPU-derived RGBA fallback cache,
3. derived slice textures for volume viewers,
4. readback-accessible staging when required by tools or tests.

None of those execution strategies should alter the scene API or the authoritative field contents.


## Invalidation Rules

`SampledField` updates should dirty dependents at the semantic level.

At minimum:

1. replacing field payload dirties every bound visual that consumes the field,
2. subregion updates dirty the same dependents, with region-granular planning allowed later,
3. changing origin/spacing or axis metadata dirties probes, annotations, and any visual relying on those
   coordinates,
4. changing only scale or colormap does **not** dirty the field itself.

This split is important because color semantics and field storage should remain decoupled.


## Validation Rules

Validation should reject or diagnose:

1. binding a `3D` field to a visual that requires `2D` without an explicit slice/view adapter,
2. binding a `2D` field to `volume`,
3. scalar-colormap image mode with a multi-channel non-scalar field unless the visual explicitly
   declares how to interpret it,
4. direct RGBA image mode with a scalar field,
5. probe requests outside field bounds when the consumer requested strict in-bounds semantics,
6. subregion updates whose box exceeds the field dimensions,
7. unsupported field format for the active runtime capability profile.


## Relationship To Existing Scene Objects

Recommended direction:

1. `image` visual should bind a `SampledField` instead of owning raw texture bytes directly,
2. `volume` visual should bind a `SampledField` instead of a volume-private texture API,
3. `Scale` remains the semantic mapping for scalar-to-color interpretation,
4. `Colorbar` explains a scale, not the field directly,
5. `ProbeResult` may report the field id and sample coordinate,
6. future derived fields remain a separate planning/runtime class.


## Public API Direction

The first public API should use:

1. one opaque handle: `DvzSampledField`,
2. one creation descriptor: `DvzSampledFieldDesc`,
3. one geometry descriptor: `DvzFieldGeometry`,
4. one update descriptor for rectangular/box writes: `DvzFieldRegion`,
5. one payload-view descriptor for data upload: `DvzFieldDataView`,
6. one visual binding entry point that binds a field semantically, not a backend texture.

The public header split should likely become:

1. `include/datoviz/scene/field.h`
2. `include/datoviz/scene.h` including `scene/field.h`


## Public Types

Recommended opaque handle:

```c
typedef struct DvzSampledField DvzSampledField;
```

Recommended enums:

```c
typedef enum
{
    DVZ_FIELD_DIM_2D = 0,
    DVZ_FIELD_DIM_3D,
} DvzFieldDim;

typedef enum
{
    DVZ_FIELD_FORMAT_R8_UNORM = 0,
    DVZ_FIELD_FORMAT_R8_UINT,
    DVZ_FIELD_FORMAT_R8_SINT,
    DVZ_FIELD_FORMAT_R8_SNORM,
    DVZ_FIELD_FORMAT_R16_UNORM,
    DVZ_FIELD_FORMAT_R16_UINT,
    DVZ_FIELD_FORMAT_R16_SNORM,
    DVZ_FIELD_FORMAT_R16_SINT,
    DVZ_FIELD_FORMAT_R16_FLOAT,
    DVZ_FIELD_FORMAT_R32_UINT,
    DVZ_FIELD_FORMAT_R32_SINT,
    DVZ_FIELD_FORMAT_R32_FLOAT,
    DVZ_FIELD_FORMAT_RG8_UNORM,
    DVZ_FIELD_FORMAT_RG8_UINT,
    DVZ_FIELD_FORMAT_RG8_SINT,
    DVZ_FIELD_FORMAT_RG16_UNORM,
    DVZ_FIELD_FORMAT_RG16_UINT,
    DVZ_FIELD_FORMAT_RG16_SINT,
    DVZ_FIELD_FORMAT_RG16_FLOAT,
    DVZ_FIELD_FORMAT_RG32_UINT,
    DVZ_FIELD_FORMAT_RG32_SINT,
    DVZ_FIELD_FORMAT_RG32_FLOAT,
    DVZ_FIELD_FORMAT_RGBA8_UNORM,
    DVZ_FIELD_FORMAT_RGBA8_UINT,
    DVZ_FIELD_FORMAT_RGBA8_SINT,
    DVZ_FIELD_FORMAT_RGBA16_UNORM,
    DVZ_FIELD_FORMAT_RGBA16_UINT,
    DVZ_FIELD_FORMAT_RGBA16_SINT,
    DVZ_FIELD_FORMAT_RGBA16_FLOAT,
    DVZ_FIELD_FORMAT_RGBA32_UINT,
    DVZ_FIELD_FORMAT_RGBA32_SINT,
    DVZ_FIELD_FORMAT_RGBA32_FLOAT,
} DvzFieldFormat;

typedef enum
{
    DVZ_FIELD_SEMANTIC_GENERIC = 0,
    DVZ_FIELD_SEMANTIC_SCALAR,
    DVZ_FIELD_SEMANTIC_VECTOR_2,
    DVZ_FIELD_SEMANTIC_VECTOR_3,
    DVZ_FIELD_SEMANTIC_COLOR,
    DVZ_FIELD_SEMANTIC_LABEL,
    DVZ_FIELD_SEMANTIC_NORMAL,
} DvzFieldSemantic;
```

Recommended descriptors:

```c
typedef struct
{
    DvzFieldDim dim;
    DvzFieldFormat format;
    DvzFieldSemantic semantic;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t flags;
} DvzSampledFieldDesc;

typedef struct
{
    uint32_t axis_order[3];
    bool axis_flip[3];
    double origin[3];
    double spacing[3];
    char unit[32];
} DvzFieldGeometry;

typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} DvzFieldRegion;

typedef struct
{
    const void* data;
    uint64_t bytes_per_row;
    uint64_t rows_per_image;
} DvzFieldDataView;
```

Notes:

1. `DvzSampledFieldDesc` carries immutable storage properties; `DvzFieldGeometry` carries mutable
   axis and physical metadata.
2. `origin` gives the physical position of sample `(0, 0, 0)`.
3. `spacing` gives the physical distance between neighboring samples along each logical axis.
4. `axis_order` maps stored array dimensions to logical field axes so callers do not have to
   reorder upstream data just to visualize it.
5. `axis_flip` indicates whether a logical axis runs in the opposite direction from the stored
   index progression.
6. `unit` names the physical unit used by `origin` and `spacing`, for example `"mm"` or `"a.u."`.
7. String fields use fixed storage here to keep the first public value types simple.


## Public Functions

Recommended first public API shape:

```c
DvzSampledField* dvz_sampled_field(DvzScene* scene, const DvzSampledFieldDesc* desc);
bool dvz_sampled_field_destroy(DvzSampledField* field);

bool dvz_sampled_field_set_data(
    DvzSampledField* field, const DvzFieldDataView* view);

bool dvz_sampled_field_update_region(
    DvzSampledField* field, DvzFieldRegion region, const DvzFieldDataView* view);

bool dvz_sampled_field_set_geometry(
    DvzSampledField* field, const DvzFieldGeometry* geometry);

const DvzSampledFieldDesc* dvz_sampled_field_desc(const DvzSampledField* field);
```

Recommended visual binding surface:

```c
bool dvz_visual_set_field(DvzVisual* visual, const char* slot, DvzSampledField* field);
```

Recommended immediate consumer slot names:

1. `image`: `"field"`
2. `volume`: `"field"`
3. future probe-only helpers may reference the field directly rather than via a visual slot


## Relationship To Existing Texture APIs

The current image-only texture calls should be treated as transitional:

1. `dvz_visual_set_texture(...)`
2. `dvz_visual_set_texture_f32(...)`

Recommended migration direction:

1. keep them temporarily as convenience wrappers,
2. internally lower them to `SampledField` creation or update,
3. prefer `dvz_sampled_field(...)` plus `dvz_visual_set_field(...)` in new examples and future API
   drafts,
4. only remove the convenience calls after the shared field path is stable and covered by tests.


## Probe And Readout Integration

`SampledField` should be the authoritative source for sample-space metadata used by probes.

Probe/readout consumers may need:

1. field id,
2. integer sample coordinate,
3. normalized coordinate,
4. physical/data coordinate derived from origin, spacing, axis order, and axis direction,
5. raw sampled value,
6. scale-mapped interpretation when relevant.

This means a future `DvzProbeResult` should be able to reference a `DvzSampledField*` or a stable
field id without depending on image-private or volume-private texture state.


## Examples

Expected usage shape for a scalar image:

```c
DvzSampledFieldDesc field_desc = {
    .dim = DVZ_FIELD_DIM_2D,
    .format = DVZ_FIELD_FORMAT_R32_FLOAT,
    .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
    .width = width,
    .height = height,
    .depth = 1,
};

DvzSampledField* field = dvz_sampled_field(scene, &field_desc);
dvz_sampled_field_set_geometry(field, &(DvzFieldGeometry){
    .axis_order = {0, 1, 2},
    .axis_flip = {false, false, false},
    .origin = {0.0, 0.0, 0.0},
    .spacing = {1.0, 1.0, 1.0},
    .unit = "px",
});
dvz_sampled_field_set_data(field, &(DvzFieldDataView){
    .data = values,
    .bytes_per_row = width * sizeof(float),
});

DvzVisual* image = dvz_image(scene, 0);
dvz_visual_set_field(image, "field", field);
dvz_visual_set_scale(image, "colormap", scale);
```

Expected usage shape for a direct RGBA image:

```c
DvzSampledFieldDesc rgba_desc = {
    .dim = DVZ_FIELD_DIM_2D,
    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
    .width = width,
    .height = height,
    .depth = 1,
};
```

Expected usage shape for a 3D vector field:

```c
DvzSampledFieldDesc vec_desc = {
    .dim = DVZ_FIELD_DIM_3D,
    .format = DVZ_FIELD_FORMAT_RGBA32_FLOAT,
    .semantic = DVZ_FIELD_SEMANTIC_VECTOR_3,
    .width = nx,
    .height = ny,
    .depth = nz,
};
```


## Implementation Guidance For A Future Agent

The first implementation should proceed in this order:

1. add public draft header spelling for `DvzSampledField`,
2. add scene-owned internal registry/storage for sampled fields,
3. migrate image visual texture state to bind a field handle,
4. preserve the current CPU scalar-to-RGBA fallback as one execution strategy,
5. update scene JSON/debug output to surface field bindings,
6. add tests for:
   - full replace,
   - subregion update,
   - shared field used by two visuals,
   - scalar image + scale path,
   - scalar signed/unsigned integer image + colormap path,
   - label or mask field path,
   - direct RGBA image path,
   - validation failure on incompatible field dimension.

Avoid starting with:

1. cross-scene sharing,
2. compressed formats,
3. GPU-only storage with no CPU authoritative copy,
4. volume-specific special cases baked into the base field object.


## Promotion Targets

This proposal has mostly promoted into:

1. `../../pipeline/RESOURCE_MODEL.md`
2. `../../pipeline/INVALIDATION_AND_CACHING.md`
3. `../../validation/VALIDATION.md`
4. `../../visuals/IMAGE.md`
5. `../../visuals/VOLUME.md`
6. `../../api/API_SURFACE.md`
7. `../../headers/scene_public_api_draft.h`

Remaining proposal-owned material:

1. detailed public enum sketches,
2. convenience function naming options,
3. unresolved lifetime and channel-metadata choices.


## Open Choices

These choices should be resolved during implementation, but they do not block the current spec:

1. whether `dvz_sampled_field_destroy()` is needed immediately or whether scene-destroy-only
   lifetime is enough for the first pass,
2. whether per-channel semantic labels belong in the first public descriptor,
3. whether `bytes_per_row` / `rows_per_image` should be zero-meaning-tight-pack or always explicit,
4. whether probe results should expose a direct field handle or only stable ids.
