# Scene Lighting Model

This document defines the scene-level lighting model for Datoviz v0.4.

## Purpose

The active lighting foundation separates reusable scene-owned light sources from visual-owned material response. It supports panel-local ambient and directional lighting, Phong and standard surface shading, view-dependent limb shading, depth cue controls, and panel-local GTAO ambient visibility.

The model is intentionally compact for v0.4 while preserving additive extension points for point lights, physically based BRDFs, environment lighting, shadows, and ray tracing. The release-gate scope is specified in [RC3_LIGHTING_FOUNDATION_SLICE.md](../slices/RC3_LIGHTING_FOUNDATION_SLICE.md).

## Core Ownership Rule

Lights are scene-owned reusable objects. Panels select bounded ordered light sets. Materials are visual-owned and contain only surface-response parameters; they do not own light direction, placement, color, or energy.

A newly created panel inherits the scene's default ambient and directional lights. An explicit panel set overrides that inheritance until `dvz_panel_reset_lights()` restores the defaults.

## Active Light Types

### Ambient Light

An ambient light contributes constant indirect irradiance with no direction. Its RGB color and nonnegative intensity are accumulated with other active ambient lights.

### Directional Light

A directional light contributes parallel direct illumination. Its direction is a finite nonzero vector pointing toward the light source and is normalized by the API. Its RGB color and nonnegative intensity scale direct diffuse and specular response.

### Deferred Point And Spot Lights

The normalized GPU payload reserves position and attenuation lanes, but point and spot evaluation are not active in this release gate. Adding them must extend the scene-light model without moving light authority back into materials.

## Public Lifecycle

Create ambient and directional lights through one descriptor path:

```c
DvzLightDesc desc = dvz_light_desc(DVZ_LIGHT_DIRECTIONAL);
desc.direction[0] = -0.45f;
desc.direction[1] = +0.35f;
desc.direction[2] = +0.82f;
DvzLight* light = dvz_light(scene, &desc);
```

Update lights with `dvz_light_set_color()`, `dvz_light_set_intensity()`, and `dvz_light_set_direction()`. Destroy them explicitly with `dvz_light_destroy()` or implicitly with the owning scene.

Assign an ordered panel set with `dvz_panel_set_lights(panel, lights, count)`. A panel accepts at most `DVZ_SCENE_MAX_PANEL_LIGHTS` active handles, currently 8. Duplicate handles and cross-scene handles are rejected. Light mutation and destruction invalidate only panels that reference the light through maintained reverse references.

The default handles are available through `dvz_scene_default_ambient()` and `dvz_scene_default_directional()`.

## Material Boundary

`DvzMaterialDesc` contains model, alpha, opacity, base color, and model-specific response controls. It has no light direction.

Phong `ambient` remains a classic-material multiplier for indirect diffuse response. Ambient radiance itself comes from the panel's ambient lights. Standard indirect diffuse derives from base color and metallic response and never reads Phong fields. Emissive response remains independent of light sources and ambient visibility.

`DVZ_MATERIAL_MODEL_STANDARD` is a compact v0.4 approximation, not an energy-conserving PBR BRDF. Roughness maps inversely to a Blinn-Phong highlight exponent, specular is a nonnegative white highlight multiplier rather than a dielectric F0, metallic suppresses diffuse without tinting specular, emissive is an independent semantic-sRGB contribution converted to linear light, and rim strength is an artistic additive edge term. A future GGX-based model may reuse the ownership and direct/indirect composition boundaries, but it must not silently claim that these approximation semantics were already physically based.

The limb model is a lightweight surface approximation for thin translucent shells, not volumetric atmospheric scattering. It uses mesh normals, camera direction, and the first active directional light to shape silhouette alpha and the day/night terminator. Limb evaluation bypasses ambient visibility.

## Direct And Indirect Composition

Shared GLSL and WGSL material evaluation follows this contract:

```text
direct = evaluate_direct_lighting(material, panel_lights, surface)
indirect_diffuse = evaluate_indirect_diffuse(material, panel_lights, surface)
linear_radiance = emissive + direct + ambient_visibility * indirect_diffuse
```

GTAO affects only indirect diffuse illumination. It does not darken direct diffuse, direct specular, emissive, unlit, limb, transparent, volume, or overlay contributions.

## GPU And Runtime Contract

Each panel lowers its active set into one fixed-capacity uniform payload containing an explicit active count and normalized light records. Compatible material-facing visuals share that buffer through set 1 binding 4; light data is not copied into per-visual material uploads.

The runtime keeps scene occlusion and GTAO/depth-peel bindings in their existing set 2 and set 3 lanes. Bind-group cache identity includes the panel-light buffer identity so persistent groups cannot retain another panel's lighting.

## Linear-Light Arithmetic

Lighting calculations are performed in linear RGB. Scene-authored sRGB colors and `srgb_color` textures are linearized before lighting. Material evaluation returns linear radiance and does not own display encoding or tone mapping. Existing normalized attachment formats may clamp on storage until an HDR scene-color product is introduced.

See [COLOR_MANAGEMENT.md](COLOR_MANAGEMENT.md).

## Which Families Consume Lighting

| Family | Lighting support |
|---|---|
| `primitive` | Active when normals and the lit material shader path are present |
| `mesh` | Active for normal-bearing material and textured-material paths |
| `sphere` | Active through the shared material evaluator |
| `point`, `pixel`, `marker` | Material/depth-cue paths may bind the panel payload; unlit output receives no light contribution |
| `path`, `segment`, `glyph`, `image` | No active light contribution |
| `volume` | Gradient and transfer-function lighting remain separate future work |

Visual families declare lighting participation through generic lowering facts. Depth cueing is a separate material effect and may apply to families that do not consume light contributions.

## PBR Upgrade Path

Scene-owned ambient and directional lights remain valid inputs for a future energy-conserving BRDF. Replacing the current standard approximation with metallic/roughness PBR changes material evaluation, not light ownership or panel selection.

Before that replacement, the PBR contract must define dielectric F0 or index-of-refraction semantics, metallic specular tint, roughness remapping and floor behavior, energy conservation, the status of the artistic rim term, and native GLSL/WGSL conformance trends. The replacement should land as an explicitly reviewed semantic change rather than an undocumented reinterpretation of the v0.4 Standard approximation.

Future image-based lighting may add an HDR environment resource, irradiance convolution, prefiltered specular data, and a BRDF lookup table. Those resources augment panel lighting and feed the existing indirect term. They do not belong in `DvzMaterialDesc`.

## Hardware Ray Tracing Upgrade Path

Ray tracing changes runtime execution rather than scene-light ownership. A future ray-trace FramePlan node, acceleration structures, and capability adaptation may consume the same scene-owned lights for direct visibility, shadows, reflections, and global illumination without changing the public light lifecycle.
