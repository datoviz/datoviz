# GSP Texture2D Field-Sampling Integration Plan

Status: proposal for near-term v0.4-dev work. Updated: 2026-07-05.

Purpose: record the Datoviz-side changes needed for GSP to advertise strict unlit RGBA8
Texture2D mesh rendering, and use that pressure to promote a general field-slot sampling API before
v0.4 RC.


## Boundary

GSP owns the high-level protocol and VisPy2/Matplotlib-facing API. Datoviz owns the retained scene
surface that a GSP adapter can call:

```text
GSP Texture2D resource + MeshVisual UVs
  -> Datoviz mesh visual
  -> "texcoords" dense attribute
  -> "texture" sampled-field slot + slot sampling state
  -> material + sampler state
  -> scene -> DRP2 -> native/WebGPU runtime
```

Do not add GSP-specific object types, backend-native texture handles, shader names, or old
`dvz_visual_set_texture_*()` public wrappers. The public texture binding path stays
`DvzSampledField` plus `dvz_visual_set_field(visual, slot_name, field)`.


## Current Datoviz Surface

| Need | Existing public surface | Status for GSP |
| --- | --- | --- |
| Mesh visual | `dvz_mesh(scene, flags)` | usable |
| Vertex positions/colors/UVs/normals | `dvz_visual_set_data()` or `dvz_mesh_set_geometry()` | usable |
| Texture payload | `dvz_sampled_field()` + `dvz_sampled_field_set_data()` | usable |
| Mesh texture binding | `dvz_visual_set_field(mesh, "texture", field)` | usable |
| Unlit material | `DvzMaterialDesc.model = DVZ_MATERIAL_MODEL_UNLIT`; `dvz_visual_set_material()` | usable |
| Color/data role | `DvzSampledFieldDesc.color_role` | usable with `DVZ_COLOR_ROLE_LINEAR_COLOR` |
| Image sampler state | `dvz_image_set_sampling(image, DvzImageSampling)` | too image-specific for the long-term API |
| Mesh texture sampler state | internal `image_nearest_sampler` plumbing exists, but no public field-slot setter and retained textured mesh emission currently always creates a linear sampler | blocker |


## Required GSP Semantics

The first GSP Texture2D mesh slice needs this exact behavior:

1. RGBA8 2D texture, row 0 treated as the top image row by the adapter contract.
2. Per-vertex UVs; `u` left to right and `v` bottom to top.
3. Fixed nearest minification and magnification filtering.
4. Clamp-to-edge wrapping and no mipmap sampling.
5. No sRGB decode or color management for this GSP resource; bytes are normalized as `byte / 255`.
6. Fragment output is `vertex_rgba * texture_rgba`, clamped to `[0, 1]`.
7. No lighting, camera, normal, depth-cue, or backend material side effect may modify the result.

Datoviz does not need to expose those GSP terms in its public API. It needs enough generic scene API
surface for an adapter to request them explicitly and prove them with fixtures.


## Public API Direction

Add a general field-slot sampling descriptor and setter:

```c
typedef enum
{
    DVZ_FIELD_FILTER_LINEAR = 0,
    DVZ_FIELD_FILTER_NEAREST,
} DvzFieldFilter;

typedef enum
{
    DVZ_FIELD_ADDRESS_CLAMP_TO_EDGE = 0,
    DVZ_FIELD_ADDRESS_REPEAT,
    DVZ_FIELD_ADDRESS_MIRROR_REPEAT,
} DvzFieldAddressMode;

typedef enum
{
    DVZ_FIELD_MIPMAP_NONE = 0,
} DvzFieldMipmapMode;

typedef struct DvzFieldSamplingDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzFieldFilter min_filter;
    DvzFieldFilter mag_filter;
    DvzFieldAddressMode address_u;
    DvzFieldAddressMode address_v;
    DvzFieldAddressMode address_w;
    DvzFieldMipmapMode mipmap_mode;
} DvzFieldSamplingDesc;

DvzFieldSamplingDesc dvz_field_sampling_desc(void);

DvzResult dvz_visual_set_field_sampling(
    DvzVisual* visual, const char* slot_name, const DvzFieldSamplingDesc* desc);
```

Rules:

1. Sampling is retained state on a visual slot, not on `DvzSampledField`.
2. The same sampled field may be bound to multiple visuals or slots with different sampling.
3. `desc == NULL` restores that visual slot's family default.
4. The setter may be called before or after `dvz_visual_set_field()`; validation of the pair happens
   when the slot is resolved for frame-plan emission.
5. Unknown slots, unsupported family/slot combinations, invalid enum values, and illegal
   format/semantic/sampling combinations return `DVZ_ERROR` with deterministic diagnostics.
6. The first implementation may support only clamp-to-edge and no mipmaps in native/WebGPU runtime
   emission, but the public descriptor should make those defaults explicit and mechanically
   inspectable.
7. `dvz_image_set_sampling()` should be removed before RC or reimplemented as a private migration
   helper. Public examples should use `dvz_visual_set_field_sampling(image, "field", &desc)`.

This shape matches v0.4 public API conventions: a flat public `Desc`, a by-value descriptor
initializer, an object-level retained-state mutator, `DvzResult` return, and no backend handles in
user code. It is an intentional pre-RC break that avoids a family-specific setter pileup.


## Why Slot-Level, Not Field-Level

`DvzSampledField` is the data resource: format, semantic, color role, dimensions, and bytes.
Sampling is how a particular draw path uses that data.

A field-level sampler would make this valid reuse impossible:

```text
same RGBA8 field
  -> image "field" slot, linear preview
  -> mesh "texture" slot, nearest GSP conformance fixture
  -> query/sample path, exact texel lookup
```

The correct ownership point is therefore:

```text
(DvzVisual*, slot_name) owns sampling state
DvzSampledField owns data state
```

That also matches the existing named-slot model already used by `dvz_visual_set_field()`,
`dvz_visual_set_scale()`, and future material-map slots.


## Family/Slot Defaults

| Visual slot | Default | Allowed first-slice overrides | Notes |
| --- | --- | --- | --- |
| `image:"field"` | linear, clamp, no mipmaps | nearest or linear | Replaces `dvz_image_set_sampling()`. |
| `mesh:"texture"` | linear, clamp, no mipmaps | nearest or linear | GSP uses nearest. Existing visual examples keep linear unless changed. |
| `labels:"field"` | nearest, clamp, no mipmaps | none or nearest only | Integer labels must not linearly filter. |
| `glyph:"field"` | linear or current atlas default, clamp, no mipmaps | family-owned | Text/glyph quality should not be changed by GSP mesh needs. |
| `volume:"field"` | profile-owned | format/semantic dependent | Scalar volumes may allow linear; labels/data profiles may require nearest. |


## Internal Implementation Plan

1. Add `DvzFieldFilter`, `DvzFieldAddressMode`, `DvzFieldMipmapMode`, and
   `DvzFieldSamplingDesc` in the scene public type/enums headers.
2. Add `dvz_field_sampling_desc()` and `dvz_visual_set_field_sampling()` to the public scene
   surface, with `dvz_ffi_field_sampling_desc()` only if `datoviz.raw` needs an out-pointer helper.
3. Replace the image-only retained sampler field with a per-visual slot-sampling map keyed by
   stable field slot names. Keep the first storage simple because current visuals have few slots.
4. Move validation into the visual registry or field-binding layer so each family/slot declares
   default sampling and allowed overrides.
5. Lower resolved slot sampling into frame-plan metadata. Replace `image_nearest_sampler` with a
   small resolved sampling record rather than a boolean.
6. In `src/scene/runtime/render_emit_prepare.c`, create or reuse sampler objects from the resolved
   sampling descriptor for both image and textured mesh branches.
7. Preserve DRP2 clamp-to-edge/no-mipmap defaults where runtime support is currently narrow, but
   do not hide those choices from scene validation.
8. Keep shader slots and field slot vocabulary unchanged: mesh texture remains `"texture"`.
9. Keep material evaluation unchanged; GSP can select `DVZ_MATERIAL_MODEL_UNLIT` through the
   existing material API.
10. Update public examples and generated docs away from `dvz_image_set_sampling()`.


## Adapter Guidance For GSP

Once slot-level sampling exists, the GSP Datoviz adapter should lower a strict Texture2D mesh as
follows:

1. Upload `Texture2D.image` as `DVZ_FIELD_FORMAT_RGBA8_UNORM`,
   `DVZ_FIELD_SEMANTIC_COLOR`, and `DVZ_COLOR_ROLE_LINEAR_COLOR`.
2. Bind it with `dvz_visual_set_field(mesh, "texture", field)`.
3. Upload `texcoords` after flipping the GSP `v` coordinate if the adapter contract treats
   `image[0]` as the top row.
4. Upload `normal` even for unlit texture meshes if the current Datoviz descriptor requires it;
   constant `(0, 0, 1)` is acceptable for planar unlit GSP fixtures.
5. Set a material descriptor with `model = DVZ_MATERIAL_MODEL_UNLIT`, base color factor
   `(1, 1, 1, 1)`, and opacity `1`.
6. Initialize `DvzFieldSamplingDesc` with nearest minification, nearest magnification,
   clamp-to-edge address modes, and no mipmaps.
7. Call `dvz_visual_set_field_sampling(mesh, "texture", &sampling)`.

`DVZ_COLOR_ROLE_DATA` is not the right role for a direct RGBA color sampled field because Datoviz
color fields require `srgb_color` or `linear_color`. `linear_color` is the generic Datoviz role
that lets the GSP adapter avoid sRGB decode for unmanaged RGBA8 bytes.


## Validation

Add focused tests before advertising this path to GSP:

1. API state tests:
   - `dvz_visual_set_field_sampling()` accepts `image:"field"` and `mesh:"texture"`;
   - rejects unknown slots and unsupported family/slot pairs;
   - rejects invalid enum values;
   - rejects linear filtering for integer label fields;
   - `NULL` restores the family/slot default;
   - bumps retained state so a later frame plan observes the change.
2. DRP2/scene emission tests:
   - image with default sampling emits the same sampler behavior as before the API break;
   - image with nearest slot sampling emits a nearest sampler;
   - textured mesh with default sampling emits a linear sampler;
   - textured mesh with nearest slot sampling emits a nearest sampler;
   - labels stay nearest.
3. Runtime visual fixture:
   - 2x2 RGBA8 texture with four distinct corners;
   - UV samples near texel centers and clamp edges;
   - white and non-white vertex colors to prove multiplication;
   - unlit material;
   - no depth cue;
   - readback or screenshot assertions with the existing color-management tolerance policy.
4. WebGPU parity fixture if the browser textured-mesh route remains public for RC.

The GSP adapter should not advertise strict Texture2D mesh capability until those tests prove
nearest filtering, clamp behavior, orientation, linear color role, and unlit multiplication.


## Non-Goals

1. No standalone public sampler object with independent lifetime for v0.4 RC.
2. No texture object public API separate from sampled fields.
3. No LOD bias, anisotropy, comparison sampler, or border-color API in this slice.
4. No PBR material texture map expansion.
5. No RGB, float, compressed, external-file, or atlas resource contract for GSP.
6. No public shader-slot, bind-group, DRP2 id, Vulkan, or WebGPU handle exposure.
7. No source compatibility alias for removed v0.3 texture helpers.


## Open Risks

| Risk | Plan |
| --- | --- |
| Exact screenshot bytes may still differ because final display/capture is sRGB RGBA8 | Use the color-management conformance tolerance policy; keep GSP's unmanaged contract at the adapter/resource level. |
| Mesh textured descriptor currently requires normals | Keep the requirement for now; GSP can synthesize normals for unlit meshes. Revisit only if this becomes an ergonomic blocker for ordinary Datoviz users. |
| This breaks `dvz_image_set_sampling()` public examples and bindings | Accept the pre-RC break; migrate examples/docs/bindings in the same implementation branch. |
| A descriptor with address and mipmap fields is broader than current runtime emission | Keep validation honest: unsupported descriptor values fail until DRP2/native/WebGPU support exists. |
| Existing textured examples may expect smooth sampling | Preserve linear as the default; GSP must opt into nearest. |


## Files Inspected

| File | Relevant evidence |
| --- | --- |
| `AGENTS.md` and `agents/now/START.md` | scene/app path, public API guardrails, no legacy texture shortcuts |
| `agents/now/STATUS.md` | GSP backend lane and texture-backed visual guardrails |
| `spec/api/PUBLIC_API_CONVENTIONS.md` | public mutator naming and `DvzResult` policy |
| `spec/scene/api/API_SURFACE.md` | leaf visual and retained-state API boundary |
| `spec/scene/visuals/MESH.md` | retained textured mesh status and texture/color role rules |
| `spec/scene/visuals/GSP_MAPPING.md` | GSP adapter mapping through public Datoviz scene APIs |
| `spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md` | sampled-field role and semantic model |
| `spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md` | color-role and screenshot/readback caution |
| `include/datoviz/scene.h` | mesh, material, image sampling, and sampled-field public surface |
| `include/datoviz/scene/field.h` | `DvzSampledFieldDesc`, formats, semantics, and color roles |
| `src/scene/visuals/image/api.c` | current retained image sampler setter pattern |
| `src/scene/visuals/mesh/lowering.c` | existing mesh lowering of `image_nearest_sampler` |
| `src/scene/runtime/render_emit_prepare.c` | current textured-mesh hardcoded linear sampler |
| `src/scene/shaders/wgsl/mesh_textured.frag.wgsl` | texture * vertex color * material evaluation path |
