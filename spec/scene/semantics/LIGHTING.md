# Scene Lighting Model

This document defines the scene-level lighting model for Datoviz v0.4.


## Purpose

The current active lighting slice provides per-visual material, Phong and standard surface
shading, view-dependent limb shading, and depth-cue controls for 3D-capable visual families.

It is intentionally simple for v0.4 — covering the most common scientific visualization needs
— while being designed to extend cleanly to physically-based rendering (PBR) and hardware
ray tracing in future versions.


## Current Active Model

The active implementation does not expose scene-owned light handles yet. Shading state is carried by
the visual/material block and emitted with the visual pipeline.

Current supported concepts:

1. per-visual material state,
2. Phong/Blinn-Phong and standard coefficients for lit primitive/mesh/sphere-style shader paths,
3. a view- and light-dependent limb material for thin translucent shells,
4. depth cueing for point, pixel, primitive, mesh, and sphere visuals,
5. shader selection based on the visual family, material state, and available attributes.

The limb model is a lightweight surface approximation, not volumetric atmospheric scattering. It
uses mesh normals, the camera direction, and the material light direction to concentrate alpha near
silhouettes, taper it smoothly at the geometry boundary, and fade it across a configurable
day/night terminator. The geometry remains an ordinary primitive or mesh; no atmosphere-specific
visual family exists.


## Core Rule

Lighting is currently visual-owned, not scene-owned. Scene-owned ambient, directional, and point
lights are future API work and must not be described as required for the active v0.4 slice.

## Linear-Light Arithmetic

Lighting calculations are performed in linear RGB. Material colors, light colors, ambient/diffuse/
specular terms, and texture colors must be linearized before lighting if they originate from
sRGB-authored scene values or `srgb_color` textures.

The output of lighting remains linear until the final display or standard image-export encode. PBR
or future physically based shading paths must also assume linear-light arithmetic.

See [COLOR_MANAGEMENT.md](COLOR_MANAGEMENT.md).


## Future Scene-Owned Lights

The following model remains the planned direction once the scene grows first-class light handles.


### Light Source Types

Three types cover v0.4 scientific visualization needs.


#### Ambient Light

Global fill light with no direction.

```text
light = dvz_light_ambient(scene, color, intensity)
```

| Parameter | Type | Description |
|---|---|---|
| `color` | `vec3` (RGB) | light color, typically white `{1, 1, 1}` |
| `intensity` | `float` | contribution scale, `[0, 1]` typical range |

One ambient light is created by default at `intensity = 0.15`.


#### Directional Light

Parallel rays from a fixed direction, as if from a distant source.

```text
light = dvz_light_directional(scene, direction, color, intensity)
```

| Parameter | Type | Description |
|---|---|---|
| `direction` | `vec3` | unit vector pointing **toward** the light source |
| `color` | `vec3` (RGB) | light color |
| `intensity` | `float` | contribution scale |

One directional light is created by default at direction `{1, 1, 1}` (normalized), white,
`intensity = 1.0`.
This gives a reasonable default appearance for 3D visuals without any user configuration.


#### Point Light

Radiates equally in all directions from a position in scene space.

```text
light = dvz_light_point(scene, position, color, intensity, attenuation)
```

| Parameter | Type | Description |
|---|---|---|
| `position` | `vec3` | position in `VisualSpace` |
| `color` | `vec3` (RGB) | light color |
| `intensity` | `float` | contribution scale |
| `attenuation` | `float` | distance falloff coefficient |


### Default Lighting

A newly created scene has one ambient and one directional light at neutral settings.
3D visuals are usable without any lighting configuration.

The default lights may be retrieved and modified:

```text
dvz_scene_default_ambient(scene)      // returns the default ambient handle
dvz_scene_default_directional(scene)  // returns the default directional handle
```


### Light Count Limit

The scene supports up to a fixed maximum number of simultaneous light sources.
The minimum guaranteed limit is 8.
This bound exists to keep GPU uniform buffers simple and predictable.
Exceeding the limit produces a diagnostic and the excess lights are ignored.


### Lifecycle

```text
dvz_light_set_intensity(light, intensity)   // update intensity at any time
dvz_light_set_color(light, color)
dvz_light_set_direction(light, direction)   // directional only
dvz_light_set_position(light, position)     // point only
dvz_light_destroy(light)                    // explicit release
```

All lights are destroyed when the scene is destroyed.
Changing any light property marks the scene dirty and triggers a redraw for affected panels.


## Which Families Consume Lighting

| Family | Lighting support |
|---|---|
| `primitive` | active when normals/material shader path are present |
| `mesh` | active — per-visual Phong/Blinn-Phong material state |
| `sphere` | planned/partial — impostor shading uses the same material direction |
| `volume` | active first slice for volume state; gradient/transfer lighting remains spec work |
| `point`, `pixel` | active depth cueing |
| `marker`, `path`, `segment`, `glyph`, `image` | no current light contribution |

Visual families declare their lighting participation in their family contracts.
A family that does not declare lighting support receives no light contribution. Depth cueing is a
separate material effect and may apply to families that do not consume Phong lighting.


## Future Development: Physically-Based Rendering (PBR)

The v0.4 lighting model uses a Blinn-Phong shading model (ambient + diffuse + specular).

The design is intentionally forward-compatible with PBR:

1. **Light source types are valid in PBR** — ambient, directional, and point lights exist in
   energy-conserving PBR models. Adding PBR does not require changing the light source API.
   Only the light contribution computation in shaders changes.

2. **Material fields are reserved for PBR** — the `mesh` and `sphere` visual parameter blocks carry
   reserved `metallic` and `roughness` fields, zero-initialized and unused in v0.4.
   Activating PBR rendering in a future version uses these fields without changing the public
   API surface.

3. **Image-based lighting (IBL)** — PBR typically uses an HDR environment map as a complex
   ambient source. A future scene-level resource `dvz_scene_set_env_map(scene, hdr_texture)`
   would complement the existing light handles without replacing them.

4. **Upgrade path** — switching a scene from Phong to PBR should require only a capability
   flag change, not a scene rebuild. The scene API and material fields are the same; the
   shader selection is a capability adaptation decision.

See `visuals/MESH.md` and `visuals/SPHERE.md` for the reserved PBR material fields.


## Future Development: Hardware Ray Tracing

Hardware ray tracing (Vulkan Ray Tracing extension) is a rendering paradigm change, not a
lighting model change.

The scene layer is already forward-compatible because:

1. **The scene API does not change** — the scene layer emits a `FramePlan`; whether a node in
   that plan executes as rasterization or ray tracing is a runtime/DRP2 decision.

2. **`RayTraceNode` as a future `FramePlan` node type** — a future `RayTraceNode` would
   replace `RenderNode` for ray-traced visuals. The scene emits it when ray tracing is
   requested and available. See `pipeline/FRAME_PLAN.md`.

3. **Capability adaptation governs the switch** — ray tracing capability is detected at
   runtime. `validation/ADAPTATION.md` governs whether a visual falls back to rasterization
   or requires ray tracing. The user policy is declarative; the scene does not hard-code
   rendering paths.

4. **Acceleration structures (BVH)** — construction and management of BVH acceleration
   structures is a DRP2-side concern. The scene layer does not need to know about it.

5. **Lighting with ray tracing** — ray-traced lighting (shadows, global illumination,
   reflections) uses the same scene-owned light sources. No API change is needed at the
   scene level.
