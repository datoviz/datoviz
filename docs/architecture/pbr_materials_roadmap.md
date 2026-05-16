# PBR Materials Roadmap

Date: 2026-05-16

This note records a long-term direction for physically based rendering (PBR) in the v0.4 scene
stack, plus small near-term steps that make the path easier without committing Datoviz to a full
PBR renderer now.

The main architectural position is:

```text
PBR is a material model, not a visual family.
```

A visual family answers what geometry or data is drawn. A material model answers how a surface
interacts with light. PBR should therefore attach to geometry-like visuals through the same
retained material path that already serves unlit, Phong, depth cueing, WBOIT, depth peeling, and
G-buffer planning.


## Current Position

The active scene stack already has several useful foundations:

1. `DvzMaterialDesc` exists as the public material descriptor.
2. `DVZ_MATERIAL_MODEL_STANDARD` exists as the first standard material intent.
3. `DvzStandardMaterial` already carries roughness, specular, metallic, emissive, and rim fields.
4. `src/scene/glsl/scene_material.glsl` centralizes material evaluation for lit visual shaders.
5. Mesh, primitive, and sphere visuals can use the shared material path.
6. Sphere impostors have analytic normals, making them a good uniform-material PBR target.
7. Mesh and primitive visuals can participate in normal/depth G-buffer and SSAO paths when normals
   are available.
8. Visual pass capability resolution is centralized in `src/scene/visual_pipeline.c`.

The current `STANDARD` shader branch should be treated as an approximate standard material, not as
complete PBR. That is acceptable. The important part is preserving a stable user intent so the
implementation can become physically based later without changing common user code.


## Long-Term Goal

The long-term goal is a standard metallic-roughness material model that can support scientific
surface rendering, molecular surfaces, meshes imported from external assets, sphere impostors, and
future tube/path geometry.

The target material feature set is:

1. uniform material factors:
   - base color factor;
   - opacity;
   - metallic;
   - roughness;
   - emissive factor;
   - optional occlusion strength;
2. optional material texture maps:
   - base color;
   - metallic-roughness;
   - normal;
   - emissive;
   - occlusion;
3. correct linear-space lighting and sRGB handling;
4. direct-light physically based BRDF, likely Cook-Torrance with GGX distribution;
5. optional image-based lighting later:
   - irradiance map;
   - prefiltered environment map;
   - BRDF lookup texture;
6. compatibility with existing scene effects:
   - depth cueing as a post-material modifier;
   - SSAO through G-buffer normals/depth;
   - WBOIT and depth peeling for transparent materials;
   - object-id and outline passes later.


## Visual Families

PBR should first support visuals whose fragments represent surfaces with meaningful normals.

### Mesh

Mesh should be the primary PBR target. It naturally supports indexed surfaces, vertex normals,
UVs, tangents, material texture maps, and imported asset workflows.

The long-term mesh path should support:

1. positions;
2. indices;
3. normals;
4. UV coordinates;
5. optional tangents for normal mapping;
6. optional vertex colors;
7. material factors and material texture maps.

### Sphere

Sphere impostors are a strong second target. They have analytic normals and are useful for atoms,
particles, markers, and other dense scientific scenes.

Initial sphere PBR should stay factor-only. Texture maps should not be designed around sphere
impostors, because mesh is the more representative material-texture case.

### Primitive

Primitive triangle visuals can use the same material path when normals are available. This is useful
for low-level tests and simple user geometry, but primitive should not become a parallel mesh
renderer. If a surface needs indices, UVs, tangents, and material maps, mesh should be the canonical
family.

### Future Tube Or Surface-Like Families

A future tube visual could benefit from PBR-style lighting for streamlines, bonds, neurites, paths,
and trajectory geometry. Plain line and path visuals should remain non-PBR unless they are rendered
as real tube or ribbon surfaces with meaningful normals.

### Non-PBR Families

Image, point, pixel, and volume visuals should not expose PBR in the usual surface-material sense.
They may use lighting-like effects, depth cueing, transfer functions, EDL, SSAO, or gradient
shading, but those are separate scientific visualization techniques rather than surface PBR.


## Public API Direction

Keep the public API declarative and material-centered:

```c
DvzMaterialDesc mat = dvz_material_desc();
mat.model = DVZ_MATERIAL_MODEL_STANDARD;
mat.base_color_factor[0] = 0.8f;
mat.base_color_factor[1] = 0.7f;
mat.base_color_factor[2] = 0.6f;
mat.base_color_factor[3] = 1.0f;
mat.standard.metallic = 0.0f;
mat.standard.roughness = 0.45f;
dvz_visual_set_material(mesh, &mat);
```

Do not add a separate `DvzPbrMaterial` or `dvz_visual_set_pbr()` path unless the existing material
descriptor proves insufficient. The existing `DVZ_MATERIAL_MODEL_STANDARD` enum should carry the
long-term standard metallic-roughness intent.

If material texture maps become public, prefer semantic slots over shader-specific names:

```c
dvz_visual_set_texture(mesh, "base_color", field_or_texture);
dvz_visual_set_texture(mesh, "metallic_roughness", field_or_texture);
dvz_visual_set_texture(mesh, "normal", field_or_texture);
dvz_visual_set_texture(mesh, "emissive", field_or_texture);
dvz_visual_set_texture(mesh, "occlusion", field_or_texture);
```

The exact texture object type can remain undecided. The important early decision is the slot
vocabulary and the separation between material factors and optional material resources.


## New Visual Family Question

Do not add a PBR visual family. PBR should be a material model that applies to eligible visual
families.

A future `surface` visual family may be worth considering only if it represents a genuinely higher
level retained surface object distinct from mesh. For now, mesh should remain the canonical indexed
surface family. Convenience builders can lower into mesh:

```text
shape -> mesh
heightfield -> mesh
gltf primitive -> mesh
molecular surface -> mesh
```

Reserve new visual families for different rendering representations, such as sphere impostors,
tubes, volumes, text, glyphs, or other families with distinct geometry generation and pass
capabilities.


## Near-Term Future-Proofing

These steps are useful now because they establish stable attachment points while avoiding a large
renderer commitment.

### Clarify `DVZ_MATERIAL_MODEL_STANDARD`

Document `DVZ_MATERIAL_MODEL_STANDARD` as the standard metallic-roughness material intent, even if
the current shader remains approximate.

This gives users and internals a stable target:

```text
UNLIT      color/emissive only
PHONG      compatibility/simple lighting
STANDARD   future physically based metallic-roughness path
```

The implementation can then evolve from approximate lighting to real GGX/direct-light PBR without
renaming the public API.

### Add Mesh UV Attribute Support

Add `"texcoords"` support to mesh visuals before material texture sampling is implemented. This is
the highest-leverage small data-model step for future PBR.

The first form can reuse the generic dense data API:

```c
dvz_visual_set_data(mesh, "texcoords", uv, vertex_count);
```

A typed convenience wrapper can come later:

```c
dvz_mesh_texcoords(mesh, first, count, uv);
```

Even if no PBR shader samples material maps yet, UV support should be validated, serialized,
tracked in visual resources, and visible to pipeline capability logic.

### Reserve Tangent Attribute Semantics

Normal mapping requires tangent-space data. Reserve this mesh attribute convention:

```text
"tangent" vec4f
```

The `xyz` components hold the tangent direction. The `w` component holds handedness. This matches
common asset pipelines and glTF practice.

Datoviz does not need to generate tangents immediately. It only needs to avoid choosing a different
shape later.

### Reserve Material Texture Slot Names

Reserve the semantic slot vocabulary now:

```text
base_color
metallic_roughness
normal
emissive
occlusion
```

This can start as internal validation and documentation before becoming a public binding API.
Stable names will make future shader, serialization, and import work less disruptive.

### Split Material Factors From Material Resources Internally

The current retained material state is mostly uniform-factor state. Future PBR needs two related
but distinct concepts:

```text
material factors
  scalar/vector values uploaded through a small uniform buffer

material resources
  optional sampled textures or fields bound by semantic material slot
```

Internally, this suggests keeping something like:

```c
visual->material        /* model, alpha, factors, depth cue, versions */
visual->material_maps   /* optional resources for base_color, normal, etc. */
```

This keeps `dvz_visual_set_material()` simple while leaving room for texture maps and importers.

### Make Pipeline Capabilities Material-Aware

Visual pass capability resolution should gradually include material facts, not just visual-family
facts. Useful questions include:

1. does the visual have normals?
2. does the material need lighting?
3. does the material need UVs?
4. does the material need tangents?
5. does the material use sampled maps?
6. can this visual emit normal/depth G-buffer data?
7. can this visual participate in SSAO, WBOIT, depth peeling, object-id, or outline passes?

The long-term selection rule should be:

```text
visual family + material model + pass kind + available attributes -> shader/pipeline/bind layout
```

This keeps PBR out of `scene_emit.c` special cases.

### Keep A Material Shader Library Boundary

Continue treating `scene_material.glsl` and the matching WGSL material include as the owner of
material evaluation.

The material shader boundary should eventually expose:

```text
evaluate_unlit()
evaluate_phong()
evaluate_standard()
apply_depth_cue()
```

The standard evaluator can first remain approximate, then move to a real BRDF once the data model
and pipeline selection are ready.


## Suggested Implementation Slices

An incremental path that keeps risk low:

1. Documentation and naming:
   - clarify `DVZ_MATERIAL_MODEL_STANDARD`;
   - document reserved material texture slots;
   - document mesh UV and tangent conventions.
2. Mesh attribute groundwork:
   - accept mesh `"texcoords"` dense data;
   - reserve mesh `"tangent"` validation, or explicitly reject it with a future-facing diagnostic;
   - include UV presence in visual descriptors.
3. Internal resource model:
   - add a small internal material-map slot table;
   - do not sample maps yet;
   - make serialization/debug output preserve material intent.
4. Direct-light standard material:
   - replace the approximate standard shader branch with GGX direct-light evaluation;
   - support mesh, primitive-with-normals, and sphere;
   - keep texture maps disabled.
5. Texture-map support:
   - add base-color and metallic-roughness maps first;
   - add normal maps only after tangent support is validated;
   - add emissive and occlusion maps after the binding path is stable.
6. Environment lighting:
   - add irradiance/prefilter/BRDF-LUT resources;
   - keep this optional and panel- or scene-scoped.
7. Import and asset workflows:
   - lower external assets into existing mesh/material/texture slots;
   - avoid making importers define renderer architecture.


## Things To Avoid For Now

1. Do not create a `pbr` visual family.
2. Do not introduce a second mesh or surface renderer path.
3. Do not expose a public `DvzMaterial*` object until shared material ownership is needed.
4. Do not design material textures around sphere impostors.
5. Do not add image-based lighting before direct-light standard material evaluation is clean.
6. Do not let PBR-specific conditionals accumulate in scene emission; route them through material
   state, pass capabilities, shader descriptors, and technique planning.

