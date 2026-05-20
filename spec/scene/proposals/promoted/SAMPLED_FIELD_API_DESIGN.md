> **Execution Status**
> - **Status:** `PARTIALLY PROMOTED`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve `SampledField` API spellings and unresolved choices after promotion of
>   resource, image, volume, validation, and public-API rules.

# SampledField API Design

This is a promoted proposal record, not the canonical `SampledField` specification.


## Decision Addressed

The original proposal chose one scene-owned `DvzSampledField` resource for regular `2D` and `3D`
sampled grids. Image, volume, probe/readout, and future field consumers borrow this resource and
layer interpretation on top of it.

The retained rationale is that sampled data should be authoritative scene state, while runtime
textures, CPU colorized caches, staging buffers, and derived slice resources remain execution
artifacts.


## Canonical Specs

Active rules moved to:

1. [`../../pipeline/RESOURCE_MODEL.md`](../../pipeline/RESOURCE_MODEL.md) for ownership, sharing,
   dirty tracking, data ownership, and the `SampledField` resource class.
2. [`../../pipeline/INVALIDATION_AND_CACHING.md`](../../pipeline/INVALIDATION_AND_CACHING.md) for
   update invalidation and upload planning.
3. [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md) and
   [`../../visuals/VOLUME.md`](../../visuals/VOLUME.md) for visual binding rules.
4. [`../../api/API_SURFACE.md`](../../api/API_SURFACE.md) for public surface requirements.
5. [`../../api/API_IMPLEMENTATION_READINESS.md`](../../api/API_IMPLEMENTATION_READINESS.md) and
   [`../../headers/scene_public_api_draft.h`](../../headers/scene_public_api_draft.h) for header
   readiness.


## Proposal-Owned API Sketch

Keep these names as implementation candidates until absorbed by the public API draft:

```c
typedef struct DvzSampledField DvzSampledField;

typedef enum { DVZ_FIELD_DIM_2D = 0, DVZ_FIELD_DIM_3D } DvzFieldDim;

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

typedef struct
{
    DvzFieldDim dim;
    DvzFieldFormat format;
    DvzFieldSemantic semantic;
    uint32_t width, height, depth;
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

typedef struct { uint32_t x, y, z, width, height, depth; } DvzFieldRegion;

typedef struct
{
    const void* data;
    uint64_t bytes_per_row;
    uint64_t rows_per_image;
} DvzFieldDataView;
```

Candidate calls:

```c
DvzSampledField* dvz_sampled_field(DvzScene* scene, const DvzSampledFieldDesc* desc);
bool dvz_sampled_field_destroy(DvzSampledField* field);
bool dvz_sampled_field_set_data(DvzSampledField* field, const DvzFieldDataView* view);
bool dvz_sampled_field_update_region(
    DvzSampledField* field, DvzFieldRegion region, const DvzFieldDataView* view);
bool dvz_sampled_field_set_geometry(DvzSampledField* field, const DvzFieldGeometry* geometry);
const DvzSampledFieldDesc* dvz_sampled_field_desc(const DvzSampledField* field);
bool dvz_visual_set_field(DvzVisual* visual, const char* slot, DvzSampledField* field);
```

Initial slot names remain `"field"` for `image` and `volume`.


## Format Sketch To Preserve

The first enum should stay explicit enough for validation and runtime capability matching. The
proposed baseline is:

1. scalar `R`: `R8_UNORM`, `R8_UINT`, `R8_SINT`, `R8_SNORM`, `R16_UNORM`, `R16_UINT`,
   `R16_SNORM`, `R16_SINT`, `R16_FLOAT`, `R32_UINT`, `R32_SINT`, `R32_FLOAT`;
2. two-channel `RG`: `RG8_UNORM`, `RG8_UINT`, `RG8_SINT`, `RG16_UNORM`, `RG16_UINT`,
   `RG16_SINT`, `RG16_FLOAT`, `RG32_UINT`, `RG32_SINT`, `RG32_FLOAT`;
3. four-channel `RGBA`: `RGBA8_UNORM`, `RGBA8_UINT`, `RGBA8_SINT`, `RGBA16_UNORM`,
   `RGBA16_UINT`, `RGBA16_SINT`, `RGBA16_FLOAT`, `RGBA32_UINT`, `RGBA32_SINT`,
   `RGBA32_FLOAT`.

The first supported runtime subset may be smaller, but unsupported formats should diagnose rather
than adapt silently. Do not add public `RGB*` formats in the first pass; callers should use `RGBA*`.


## Remaining Backlog

1. Decide whether `dvz_sampled_field_destroy()` is needed in the first slice or whether
   scene-destroy-only lifetime is enough.
2. Decide whether per-channel semantic labels belong in `DvzSampledFieldDesc`.
3. Decide whether zero `bytes_per_row` / `rows_per_image` means tight packing or whether the fields
   are always explicit.
4. Decide whether probe results expose direct field handles, stable field ids, or both.
5. Treat `dvz_visual_set_texture(...)` and `dvz_visual_set_texture_f32(...)` as transitional image
   convenience wrappers once the shared field path is implemented.
