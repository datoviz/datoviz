# GSP Visual Family Mapping

Status: informative v0.4-dev adapter mapping. `spec/scene/` remains the authority for scene
semantics; public headers remain the authority for callable C APIs.

This file records the Datoviz-owned visual-family mapping a GSP adapter can use without reading
private implementation files. It deliberately uses Datoviz scene identities and public C handles,
not GSP protocol ids.


## Adapter Rules

1. Create visuals through the public constructors listed below.
2. Upload dense item data through `dvz_visual_set_data()`, `dvz_visual_set_data_range()`, or
   `dvz_visual_set_data_many()`.
3. Bind retained fields, scales, textures, and buffers through public resource APIs.
4. Enable queries explicitly with `dvz_visual_set_query_capabilities()`.
5. Treat unsupported targets as explicit `DvzQueryStatus` outcomes, not misses.
6. Use `dvz_visual_id()` and query-result Datoviz ids for adapter reverse maps.


## First GSP Slice

| Family | Constructor | Required data/resources | Optional data/resources | Query support | GSP status |
|---|---|---|---|---|---|
| `point` | `dvz_point(scene, flags)` | `position` vec3, `color` RGBA8, `diameter_px` float | `item_state`, link keys, `dvz_point_set_style()` | `DVZ_QUERY_CAPABILITY_ITEM`; returns visual id, family, item id, link key when present | ready |
| `image` | `dvz_image(scene, flags)` | either legacy quad `position` + `texcoords`, or item `position` + `extent`; 2D `DvzSampledField` on `"field"` or texture wrapper | `anchor`, `tex_rect`, `dvz_visual_set_scale(image, "color", scale)`, colormap/scale resources | item, pixel, and sample paths are implemented; pixel/sample payloads expose panel/display identity and decoded sample metadata where supported | ready with payload limits |
| `primitive` | `dvz_primitive(scene, topology, flags)` | `position` vec3, `color` RGBA8 | `normal`, `"index"` buffer | item-level primitive identity | usable, low-level only |
| `mesh` | `dvz_mesh(scene, flags)` | `position` vec3 | `color`, `normal`, `texcoords`, `instance_transform`, `"index"` buffer, `"texture"` sampled field, `dvz_mesh_set_geometry()` | item-level mesh identity; face/region payloads are not first-slice ready | usable with limits |
| `volume` | `dvz_volume(scene, flags)` | 3D `DvzSampledField` on `"field"` | opacity, sampling, render mode, slice axis/position, step count, bounds, axis mapping, value range, alpha stops, clipping, scale | item/object proxy picking and slice/sample query paths; DVR/MIP ray-hit semantics are deferred | limited |
| `text` / `glyph` | `dvz_text(panel, flags)` for semantic text, `dvz_glyph(scene, flags)` for low-level atlas quads | text string/style/placement for `DvzText`; glyph `position`, `bounds`, `texcoords`, `color`, `angle`, atlas field for `DvzGlyph` | text atlas renderer, glyph atlas, placement/style updates | no GSP-ready text/glyph query payload in the first slice | render-only first |


## Other Active Families

These families are active in v0.4-dev and may be useful to GSP later, but they are not required for
the first compatibility slice.

| Family | Constructor | Required data/resources | Query support | Adapter note |
|---|---|---|---|---|
| `pixel` | `dvz_pixel(scene, flags)` | `position`, `color`, `pixel_size_px` | item-level square picking | useful fallback for exact square markers |
| `marker` | `dvz_marker(scene, flags)` | `position`, `color`, `diameter_px`, `shape`; optional `angle` | item-level bounding/SDF marker picking | use when GSP needs named marker shapes |
| `sphere` | `dvz_sphere(scene, flags)` | `position`, `color`, `radius` | item-level impostor sphere picking | 3D scatter candidate when radius is in data/world units |
| `segment` | `dvz_segment(scene, flags)` | `position_start`, `position_end`, `color`, `stroke_width_px` | item-level stroke picking | independent line segments |
| `path` | `dvz_path(scene, flags)` | `position`, `color`, `stroke_width_px` | item-level lowered stroke picking | grouped/subpath polyline path |
| `vector` | `dvz_vector(scene, flags)` | straight mode: `position`, `vector`, `color`, `stroke_width_px`; curved mode: path-like data | item-level delegated stroke picking | quiver-style vectors; lowerings preserve source item identity |
| `labels` | `dvz_labels(scene, flags)` | integer sampled field on `"field"`, categorical scale on `"labels"` | label/segment probe paths | better than `image` for categorical label textures |
| `splat` | `dvz_splat(scene, flags)` | `position`, `color`, `sigma`, `angle` | none installed | render-only; do not expose as queryable |


## Resource Mapping

| Need | Datoviz API | GSP guidance |
|---|---|---|
| dense per-item attributes | `dvz_visual_set_data()`, `dvz_visual_set_data_range()`, `dvz_visual_set_data_many()` | use canonical public names from `README.md`; data is copied before return |
| shared/index buffers | `dvz_scene_buffer()`, `dvz_scene_buffer_set_data()`, `dvz_visual_set_buffer()` | use for mesh/primitive index buffers and shared retained payloads |
| 2D scalar/color fields | `dvz_sampled_field()`, `dvz_sampled_field_set_data()`, `dvz_sampled_field_update_region()`, `dvz_visual_set_field()` | preferred image/labels path; texture wrappers are transitional convenience APIs |
| 3D fields | same sampled-field APIs with 3D descriptors | preferred volume path |
| scalar/color/categorical mapping | `dvz_scale()`, `dvz_colormap()`, `dvz_visual_set_scale()` | adapter owns GSP scale ids; Datoviz exposes scene-local ids for reverse maps |
| text | `dvz_text()`, `dvz_text_set_string()`, `dvz_text_set_style()`, `dvz_text_set_placement()` | semantic text is retained scene state; low-level glyph visuals are implementation-facing unless GSP explicitly needs atlas quads |


## Query Contract Notes

The adapter should only advertise query targets that the Datoviz visual has enabled through
`dvz_visual_set_query_capabilities()`.

| Target | Current families | Result fields to rely on |
|---|---|---|
| `DVZ_SCENE_TARGET_ITEM` | point, pixel, marker, sphere, segment, path, vector, primitive, mesh, image, volume, labels | `scene_id`, `figure_id`, `panel_id`, `visual_id`, `visual_family`, `item_id`, `link_key` when present |
| `DVZ_SCENE_TARGET_PIXEL` | image | ids above plus pixel/sample fields documented by image query tests |
| `DVZ_SCENE_TARGET_SAMPLE` | image, volume | ids above plus sample/scalar/vector/category fields when the family decoder supports the field format |
| `DVZ_SCENE_TARGET_SEGMENT` | labels | label segment/category identity |

Do not infer support for face, vertex, glyph, text, DVR ray-hit, or MIP ray-hit payloads from
`DvzQueryResult` fields alone. Those targets need explicit implementation and tests before GSP
advertises them.


## WebGPU And WASM Notes

1. Prefer public POD descriptors, fixed-width ids, copied data uploads, and scene-owned resources.
2. Avoid native/backend handles in adapter-visible state.
3. Treat GLSL/Vulkan-only visual features as optional until the corresponding WGSL/WebGPU path is
   documented in the family status/spec.
4. For large fields, use region updates instead of full reuploads when possible.


## Files Inspected

| File | Evidence used |
|---|---|
| `include/datoviz/scene.h` | public constructors, visual data APIs, texture wrappers, volume and style setters |
| `include/datoviz/scene/field.h` | sampled-field binding path |
| `include/datoviz/scene/scale.h` | scale binding path |
| `include/datoviz/scene/text.h` | retained semantic text API |
| `include/datoviz/scene/types.h` | `DvzQueryResult` identity and payload fields |
| `src/scene/visuals/*/query.c` | installed per-family query ops and target coverage |
| `src/scene/tests/query.c` | active query coverage by family and target |
| `spec/scene/visuals/STATUS.md` | current implementation status matrix |
| `spec/scene/visuals/README.md` | installed public attribute matrix |
