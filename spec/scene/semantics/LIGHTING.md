# Scene Lighting Model

This document defines the scene-level lighting model for Datoviz v0.4.


## Purpose

The lighting model provides scene-owned light sources that 3D visual families can consume
to produce shaded rendering.

It is intentionally simple for v0.4 — covering the most common scientific visualization needs
— while being designed to extend cleanly to physically-based rendering (PBR) and hardware
ray tracing in future versions.


## Position

Light sources sit:

1. inside the scene layer, above DRP2,
2. alongside visuals, resources, and controllers as scene-owned objects,
3. consumed by visual families that declare lighting support (`mesh`, `sphere`, `volume`).

Families that do not declare lighting support (`point`, `path`, `glyph`, `image`, etc.) ignore
scene lights entirely.


## Core Rule

Light sources are first-class scene-owned handles.
They are created, configured, and destroyed through the scene layer.
Visual families consume them through the scene's light state, not through per-visual bindings.


## Light Source Types

Three types cover v0.4 scientific visualization needs.


### Ambient Light

Global fill light with no direction.

```text
light = dvz_light_ambient(scene, color, intensity)
```

| Parameter | Type | Description |
|---|---|---|
| `color` | `vec3` (RGB) | light color, typically white `{1, 1, 1}` |
| `intensity` | `float` | contribution scale, `[0, 1]` typical range |

One ambient light is created by default at `intensity = 0.15`.


### Directional Light

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


### Point Light

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


## Default Lighting

A newly created scene has one ambient and one directional light at neutral settings.
3D visuals are usable without any lighting configuration.

The default lights may be retrieved and modified:

```text
dvz_scene_default_ambient(scene)      // returns the default ambient handle
dvz_scene_default_directional(scene)  // returns the default directional handle
```


## Light Count Limit

The scene supports up to a fixed maximum number of simultaneous light sources.
The minimum guaranteed limit is 8.
This bound exists to keep GPU uniform buffers simple and predictable.
Exceeding the limit produces a diagnostic and the excess lights are ignored.


## Lifecycle

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
| `mesh` | yes — Phong/Blinn-Phong in v0.4, PBR-ready material fields reserved |
| `sphere` | yes — impostor shading uses scene lights |
| `volume` | optional — shading mode uses scene lights when enabled |
| `point`, `marker`, `path`, `segment`, `glyph`, `image`, `pixel`, `primitive` | no |

Visual families declare their lighting participation in their family contracts.
A family that does not declare lighting support receives no light contribution regardless of
what lights exist in the scene.


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
