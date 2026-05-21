# Scene visual data view API plan

> **Execution Status**
> - **Status:** `READY FOR IMPLEMENTATION`
> - **Updated on:** `2026-05-21`
> - **Purpose:** remove the need for examples and app-level tests to inspect internal
>   `DvzVisual` attribute slots.

This plan records a small public scene API cleanup. The current retained visual API is mostly
write-only: callers can attach dense attribute data with `dvz_visual_set_data()`,
`dvz_visual_set_data_many()`, and `dvz_visual_set_data_range()`, but there is no public way to
inspect the retained dense data that a visual currently owns.

That gap encourages code outside scene internals to include `_scene.h`, walk `visual->attrs[]`, or
duplicate helpers like `_app_visual_attr_index()`. That is fragile because the real internal lookup
goes through `_attr_storage_name()`, so aliases such as `"diameter" -> "size"`,
`"radius" -> "size"`, and `"stroke_width" -> "line_width"` can drift from copied lookup code.

This is separate from [../../later/SCENE_SHARED_VISUAL_DATA_API.md](../../later/SCENE_SHARED_VISUAL_DATA_API.md).
That later note is about reusable attribute stores shared by several visuals. This plan is only a
read-only view over one visual's currently retained dense data.


## Goals

1. Provide a narrow public read API for retained visual attribute data.
2. Keep attribute indexes, `DvzVisualAttr`, `attr_count`, and `attrs[]` private.
3. Preserve existing setter APIs and visual storage internals.
4. Make alias handling match the internal scene implementation.
5. Remove duplicated attr-index helpers from app/example-style code.


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

Do not expose visual attribute indexes as public API. Indexes are an implementation detail and will
become more fragile as attributes move between dense CPU data, scene buffers, fields, and generated
GPU-side data.


## Validation

For the first implementation patch:

1. `git diff --check`
2. `just build`
3. `just test scene` or the narrowest scene filter covering visual attributes and app glyph bounds
