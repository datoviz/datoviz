# Scene Material Model Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define a shared material API for scene visuals with both a fast Phong model and a
>   higher-quality standard model.


## Decision

Keep ambient/diffuse/specular/shininess as an intentional user-facing material option, but do not
keep the current primitive-specific API shape.

Phong and standard materials are not mutually exclusive. They are two models inside one material
system:

1. `ambient/diffuse/specular/shininess` should remain available as a fast Phong-style model for
   large datasets and users who want predictable low-cost lighting;
2. the new material state should expose a more rational shared model based on base color, roughness,
   specular strength, opacity, alpha mode, depth cueing, and optional sci-viz clarity controls;
3. both models should be selected through the same `DvzMaterialDesc` value descriptor and
   `dvz_visual_set_material()` setter;
4. examples can expose a quality/speed choice by switching material model rather than switching
   visual family or shader API.

This gives users a deliberate speed/quality choice while moving the architecture away from
shader-family-specific parameter blocks.


## Current State

The scene material layer currently exists internally:

1. `DvzSceneMaterialState` stores alpha mode, opacity, light direction, ADS fields, depth cueing,
   scalar modulation placeholders, and a version;
2. `DvzSceneMaterialParams` is the GPU payload consumed by primitive/mesh/sphere material shaders;
3. `dvz_visual_set_primitive_shading()` currently updates ADS-style fields;
4. `dvz_visual_set_depth_cue()` shares depth cueing across point/pixel/primitive/mesh/sphere where
   supported;
5. sphere currently reuses the primitive shading descriptor for specular highlights.

This is already better than one-off uniforms, but the public naming and parameter model still imply
that only primitive shading exists.


## Target Material Concepts

Introduce material models with explicit semantics:

```c
typedef enum DvzMaterialModel
{
    DVZ_MATERIAL_MODEL_UNLIT = 0,
    DVZ_MATERIAL_MODEL_PHONG,
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

Phong fields:

1. `ambient`;
2. `diffuse`;
3. `specular`;
4. `shininess`.

Phong fields should live in the same material state, but be interpreted only when
`model == DVZ_MATERIAL_MODEL_PHONG`. This is not a legacy fallback; it is the fast material model.


## Public API Direction

The final public API should be one typed material setter:

```c
typedef struct DvzPhongMaterial
{
    float ambient;
    float diffuse;
    float specular;
    float shininess;
} DvzPhongMaterial;

typedef struct DvzStandardMaterial
{
    float roughness;
    float specular;
    float metallic;
    float emissive[3];
    float rim_strength;
} DvzStandardMaterial;

typedef struct DvzMaterialDesc
{
    DvzMaterialModel model;
    DvzAlphaMode alpha_mode;
    float opacity;
    float base_color_factor[4];
    float light_direction[3];
    DvzPhongMaterial phong;
    DvzStandardMaterial standard;
} DvzMaterialDesc;

DVZ_EXPORT DvzMaterialDesc dvz_material_desc(void);
DVZ_EXPORT int dvz_visual_set_material(DvzVisual* visual, const DvzMaterialDesc* desc);
```

`dvz_visual_set_primitive_shading()` should not remain the long-term public API. It can exist only
as temporary migration scaffolding while the scene material path is being refactored.

Do not expose a heap-allocated public `DvzMaterial` object yet. A value descriptor is enough for the
current retained scene model and keeps ownership simple.


## Model Mapping

When `dvz_visual_set_material()` is called with `DVZ_MATERIAL_MODEL_PHONG`:

1. set `material.model = DVZ_MATERIAL_MODEL_PHONG`;
2. copy ADS fields into Phong material fields;
3. update the shared GPU material payload;
4. leave depth cueing in the existing depth-cue setter unless material ownership is deliberately
   broadened later.

When `dvz_visual_set_material()` is called with `DVZ_MATERIAL_MODEL_STANDARD`:

1. set standard fields directly;
2. evaluate standard lighting in shared shader helpers;
3. mark the material uniform dirty;
4. do not require ADS fallback values.

This makes the models cooperative under one API, while keeping their semantics distinct.


## Shader Direction

Short term:

1. keep existing Phong lighting in primitive/mesh/sphere shaders;
2. rename helper comments and internal descriptor names so they are not primitive-only;
3. make the material uniform layout explicitly versioned or documented.

Medium term:

1. add a shared GLSL material helper that can evaluate `UNLIT`, `PHONG`, and `STANDARD`;
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

1. Rename internal comments/types where practical from primitive-only shading to material/Phong
   shading without changing behavior.
2. Add `DvzMaterialModel` and standard material fields to `DvzSceneMaterialState`.
3. Add an internal material descriptor conversion helper for Phong and standard fields.
4. Add `dvz_material_desc()` and `dvz_visual_set_material()` once the internal state and tests are
   stable.
5. Retire or de-emphasize `dvz_visual_set_primitive_shading()` after the unified material setter
   exists.
6. Port primitive/mesh/sphere shaders to a shared material evaluation helper.
7. Update sphere SSAO example to expose material model selection and the relevant model fields.


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
