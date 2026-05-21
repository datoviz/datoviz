# Scene Visual Data View API

> **Execution Status**
> - **Status:** `COMPLETED`
> - **Updated on:** `2026-05-21`
> - **Purpose:** remove the need for examples and app-level tests to inspect internal
>   `DvzVisual` attribute slots.
> - **Implemented in:** `e8d5bed1` and `cc6bad7c`.


## Completion Record

The public read-only visual data view API is now implemented:

1. `DvzVisualDataView` lives in `include/datoviz/scene/types.h`.
2. `dvz_visual_data()` lives in `include/datoviz/scene.h` and `src/scene/visual_attrs.c`.
3. The query path uses `_attr_index()`, so aliases still resolve through `_attr_storage_name()`.
4. The API exposes retained dense per-item CPU payloads only; buffer-backed, metadata-only, and
   missing attributes return `-1`.
5. `src/scene/tests/app.c` no longer duplicates visual attribute index lookup for glyph pixel bounds.
6. Focused coverage is registered as `scene/scene-graph/data_view`.

Validation recorded on 2026-05-21:

1. `git diff --check`
2. `just build`
3. `just test scene` with `369/369` tests passing.


## Original Plan

This plan records a small public scene API cleanup. The current retained visual API is mostly
write-only: callers can attach dense attribute data with `dvz_visual_set_data()`,
`dvz_visual_set_data_many()`, and `dvz_visual_set_data_range()`, but there is no public way to
inspect the retained dense data that a visual currently owns.

That gap encourages code outside scene internals to include `_scene.h`, walk `visual->attrs[]`, or
duplicate helpers like `_app_visual_attr_index()`. That is fragile because the real internal lookup
goes through `_attr_storage_name()`, so aliases such as `"diameter" -> "size"`,
`"radius" -> "size"`, and `"stroke_width" -> "line_width"` can drift from copied lookup code.

This is separate from [../later/SCENE_SHARED_VISUAL_DATA_API.md](../later/SCENE_SHARED_VISUAL_DATA_API.md).
That later note is about reusable attribute stores shared by several visuals. This plan is only a
read-only view over one visual's currently retained dense data.


## Related Internal Boundary Finding

The 2026-05-21 path-native stroke work exposed a broader internal design smell: several
`visual_*` files are named and used as generic scene visual infrastructure, but they still contain
family-specific knowledge for path, segment, image, volume, glyph, and other visuals. Examples
include derived upload cache construction, metadata field selection, descriptor-kind resolution,
shader identity, vertex-buffer layout, and pass capability decisions.

This plan should not try to solve that whole internal architecture issue. Its public data-view API
is still useful because it removes one reason for app/example code to inspect `DvzVisual` internals.
However, while implementing this plan, avoid adding any new visual-family branching to generic code
unless it already exists on the setter path being mirrored. The intended direction is that generic
visual code should dispatch to family-local behavior, not accumulate more path/segment/image-specific
cases.

A follow-up internal refactor should introduce a small visual-family operations boundary, with
family-owned hooks for:

1. retained state initialization and destruction;
2. derived upload emission;
3. metadata/resource declaration;
4. descriptor resolution;
5. shader identity;
6. vertex layout and pipeline state;
7. pass-capability mapping.

That boundary would keep path-specific code in the path family implementation, segment-specific code
in the segment family implementation, and leave generic frame-plan/runtime code as dispatch and
orchestration only.


## Goals

1. Provide a narrow public read API for retained visual attribute data.
2. Keep attribute indexes, `DvzVisualAttr`, `attr_count`, and `attrs[]` private.
3. Preserve existing setter APIs and visual storage internals.
4. Make alias handling match the internal scene implementation.
5. Remove duplicated attr-index helpers from app/example-style code.
6. Avoid increasing visual-family-specific logic in generic visual files.


## Non-Goals

1. Do not expose generated GPU resources such as path adjacency buffers, derived image quads, or
   segment stroke caches through `dvz_visual_data()`.
2. Do not expose `DvzVisualAttr` or visual attribute indexes.
3. Do not introduce a new generic switchboard for visual families as part of this public API patch.
4. Do not solve the visual-family operations refactor in this plan; record that as a separate
   internal architecture task.


## Proposed API

Add a lightweight public view type in `include/datoviz/scene/types.h`:

```c
struct DvzVisualDataView
{
    const void* data;
    uint64_t item_count;
    uint32_t item_size;
    DvzVisualAttrSource source;
    DvzVisualAttrMutability mutability;
    uint64_t version;
};
typedef struct DvzVisualDataView DvzVisualDataView;
```

Add the public query function in `include/datoviz/scene.h` near the visual data setters:

```c
DVZ_EXPORT int dvz_visual_data(
    const DvzVisual* visual, const char* attr_name, DvzVisualDataView* out);
```

Expected behavior:

1. return `0` and fill `out` when the named attribute has retained dense CPU data;
2. return `-1` when the visual, name, output pointer, attribute, or dense data payload is missing;
3. resolve attribute-name aliases with the same `_attr_storage_name()` path used by setters;
4. expose read-only data and metadata only, not ownership or mutable internals;
5. leave buffer-backed, field-backed, and future non-dense attributes as explicit non-matches for
   this first slice.


## Implementation Steps

1. Add `DvzVisualDataView` to `include/datoviz/scene/types.h`.
2. Document `dvz_visual_data()` in `include/datoviz/scene.h`.
3. Implement `dvz_visual_data()` in `src/scene/visual_attrs.c` using `_attr_index()`.
4. Add focused scene tests that cover:
   - normal lookup after `dvz_visual_set_data()`;
   - alias lookup, for example point `"diameter"` resolving to stored `"size"`;
   - missing or metadata-only attributes returning `-1`.
5. Remove `_app_visual_attr_index()` from `src/scene/tests/app.c`.
6. Rewrite `_app_glyph_pixel_bounds()` to use `dvz_visual_data()` views for `"position"`,
   `"bounds"`, and `"angle"`.
7. Avoid broad churn in internal tests that intentionally inspect scene internals; convert only
   duplicated app/example-style helpers in the first patch.


## Follow-Ups

After the read-only view lands, add semantic helpers only when a real workflow needs them:

1. `dvz_visual_has_data()` or `dvz_visual_data_info()` for lightweight metadata queries;
2. `dvz_visual_item_count()` if item count becomes a common public need;
3. text or bounds-specific APIs, such as text pixel bounds, when callers need layout information
   rather than raw glyph attributes.
4. an internal visual-family operations table so family-specific upload, metadata, descriptor,
   shader, pipeline, and pass-capability code can move out of generic `visual_*` switchboards.

Do not expose visual attribute indexes as public API. Indexes are an implementation detail and will
become more fragile as attributes move between dense CPU data, scene buffers, fields, and generated
GPU-side data.


## Validation

For the first implementation patch:

1. `git diff --check`
2. `just build`
3. `just test scene` or the narrowest scene filter covering visual attributes and app glyph bounds
