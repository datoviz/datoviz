# Scene Material Model Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define a better shared material model for scene visuals while preserving the
>   current ambient/diffuse/specular/shininess controls as a compatibility and simple-shading path.


## Decision

Keep the existing ambient/diffuse/specular/shininess controls, but do not make them the long-term
center of the material architecture.

They are not mutually exclusive with a better material model:

1. `ambient/diffuse/specular/shininess` should remain available as a classic Blinn-Phong style
   shading descriptor;
2. the new material state should expose a more rational shared model based on base color, roughness,
   specular strength, opacity, alpha mode, depth cueing, and optional sci-viz clarity controls;
3. classic ADS parameters can map into the shared material uniform payload for existing examples and
   tests;
4. newer visuals and examples should prefer the shared material descriptor once it exists.

This lets current primitive/mesh/sphere code keep working while the architecture moves away from
shader-family-specific parameter blocks.


## Current State

The scene material layer currently exists internally:

1. `DvzSceneMaterialState` stores alpha mode, opacity, light direction, ADS fields, depth cueing,
   scalar modulation placeholders, and a version;
2. `DvzSceneMaterialParams` is the GPU payload consumed by primitive/mesh/sphere material shaders;
3. `dvz_visual_set_primitive_shading()` updates ADS-style fields;
4. `dvz_visual_set_depth_cue()` shares depth cueing across point/pixel/primitive/mesh/sphere where
   supported;
5. sphere currently reuses the primitive shading descriptor for specular highlights.

This is already better than one-off uniforms, but the public naming and parameter model still imply
that only primitive shading exists.


## Target Material Concepts

Introduce an internal material model with explicit shading families:

```c
typedef enum DvzMaterialModel
{
    DVZ_MATERIAL_MODEL_UNLIT = 0,
    DVZ_MATERIAL_MODEL_CLASSIC,
    DVZ_MATERIAL_MODEL_STANDARD,
} DvzMaterialModel;
```

Suggested standard/shared fields:

1. `base_color_factor`: RGBA multiplier applied after per-vertex/per-item color;
2. `opacity`;
3. `alpha_mode`;
4. `roughness`;
5. `specular`;
6. `metallic`, default `0`;
7. `emissive`;
8. `fresnel_strength` or `rim_strength` for optional sci-viz shape readability;
9. `light_direction` for simple direct lighting until image-based/environment lighting exists;
10. depth cueing fields already present in `DvzSceneMaterialState`.

Keep classic fields:

1. `ambient`;
2. `diffuse`;
3. `specular`;
4. `shininess`.

Classic fields should live in the same material state, but be interpreted only when
`model == DVZ_MATERIAL_MODEL_CLASSIC` or when a compatibility setter updates the state.


## Public API Direction

Keep the existing API for now:

```c
int dvz_visual_set_primitive_shading(
    DvzVisual* visual, const DvzPrimitiveShadingDesc* desc);
```

But treat it as a classic-shading helper. It should be valid for primitive, mesh, and sphere until a
better public name is introduced.

Add a new typed API later:

```c
typedef struct DvzMaterialDesc
{
    DvzMaterialModel model;
    DvzAlphaMode alpha_mode;
    float opacity;
    float base_color_factor[4];
    float roughness;
    float specular;
    float metallic;
    float emissive[3];
    float rim_strength;
    float light_direction[3];
} DvzMaterialDesc;

DVZ_EXPORT DvzMaterialDesc dvz_material_desc(void);
DVZ_EXPORT int dvz_visual_set_material(DvzVisual* visual, const DvzMaterialDesc* desc);
```

Do not expose a heap-allocated public `DvzMaterial` object yet. A value descriptor is enough for the
current retained scene model and keeps ownership simple.


## Compatibility Mapping

When `dvz_visual_set_primitive_shading()` is called:

1. set `material.model = DVZ_MATERIAL_MODEL_CLASSIC`;
2. copy ADS fields into classic material fields;
3. update the shared GPU material payload;
4. leave depth cueing and alpha mode untouched unless the setter explicitly controls them.

When `dvz_visual_set_material()` is called with `DVZ_MATERIAL_MODEL_STANDARD`:

1. set standard fields directly;
2. derive fallback ADS values for older shaders only if the shader has not yet been ported;
3. mark the material uniform dirty;
4. do not destroy classic values, so switching back to classic can preserve user choices if useful.

This makes the models cooperative rather than mutually exclusive.


## Shader Direction

Short term:

1. keep existing classic lighting in primitive/mesh/sphere shaders;
2. rename helper comments and internal descriptor names so they are not primitive-only;
3. make the material uniform layout explicitly versioned or documented.

Medium term:

1. add a shared GLSL material helper that can evaluate `UNLIT`, `CLASSIC`, and `STANDARD`;
2. convert primitive, mesh, and sphere color shaders to call the same helper;
3. ensure G-buffer shaders write enough data for SSAO and future material-aware postprocesses;
4. keep point/pixel/material modulation support simple unless lighting is explicitly requested.

Standard material lighting should start as a pragmatic microfacet-inspired model, not a full PBR
renderer:

1. Lambert diffuse;
2. roughness-controlled Blinn/GGX-like specular lobe;
3. Schlick Fresnel approximation if it is visually useful;
4. optional rim/fresnel clarity term for scientific shape perception.


## Visual Coverage

Initial supported visuals:

1. primitive;
2. mesh;
3. sphere.

Later supported visuals:

1. path, for line material and coverage controls;
2. marker, if marker visual is activated in v0.4;
3. volume, using a volume-specific material model rather than forcing surface fields;
4. image, mostly for alpha/opacity/colormap modulation rather than lighting.


## Implementation Order

Recommended commits:

1. Rename internal comments/types where practical from primitive-only shading to material/classic
   shading without changing behavior.
2. Add `DvzMaterialModel` and standard material fields to `DvzSceneMaterialState`.
3. Add an internal material descriptor conversion helper for classic and standard fields.
4. Keep `dvz_visual_set_primitive_shading()` as the classic compatibility setter.
5. Add `dvz_material_desc()` and `dvz_visual_set_material()` only after the internal state and tests
   are stable.
6. Port primitive/mesh/sphere shaders to a shared material evaluation helper.
7. Update sphere SSAO example to expose roughness/specular once the standard model is active.


## Validation

Focused validation:

```text
cmake --build build --target dvztest_scene hello_sphere_ssao_glfw -j 8
./build/testing/dvztest_scene test_scene_indexed_primitive_shading_updates_runtime
./build/testing/dvztest_scene test_scene_sphere_emit_glsl_executes
./build/testing/dvztest_scene test_scene_visual_alpha_mode
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

Before public API changes:

```text
just test scene
just spec-check
```

