> **Execution Status**
> - **Status:** `MATERIAL API AND RC3 LIGHTING FOUNDATION IMPLEMENTED; FULL PBR DEFERRED`
> - **Updated on:** `2026-08-15`
> - **Purpose:** preserve the implemented ownership decisions and define the additive boundary for future material and lighting work.

# Material And Lighting API

## Current Contract

Datoviz separates three kinds of state:

1. lights are explicit scene-owned reusable objects;
2. panels select bounded ordered sets of those lights;
3. visuals own material response and shading intent.

This is implemented for ambient and directional lights, panel defaults and overrides, primitive/mesh/sphere materials, shared native/WGSL evaluation, and material-aware GTAO. The exact release-gate implementation and evidence are recorded in [`../../slices/RC3_LIGHTING_FOUNDATION_SLICE.md`](../../slices/RC3_LIGHTING_FOUNDATION_SLICE.md); normative behavior lives in [`../../semantics/LIGHTING.md`](../../semantics/LIGHTING.md).

## Implemented Public Shape

Lighting uses one descriptor-oriented lifecycle:

```c
DvzLightDesc desc = dvz_light_desc(DVZ_LIGHT_DIRECTIONAL);
desc.direction[0] = -0.45f;
desc.direction[1] = +0.35f;
desc.direction[2] = +0.82f;
DvzLight* light = dvz_light(scene, &desc);

DvzLight* lights[] = {light};
dvz_panel_set_lights(panel, lights, 1);
```

The public API provides `dvz_light_set_desc()`, focused color/intensity/direction setters, explicit destruction, `dvz_panel_set_lights()`, `dvz_panel_reset_lights()`, and access to the scene default ambient and directional handles. Descriptors, duplicate handles, capacity, cross-scene references, mutation, destruction, and repeated emission are validated.

Materials use value descriptors rather than scene-owned material handles:

```c
DvzMaterialDesc material = dvz_standard_material_desc();
material.standard.roughness = 0.45f;
material.standard.specular = 0.50f;
material.standard.metallic = 0.0f;
dvz_visual_set_material(mesh, &material);
```

`DvzMaterialDesc` contains model, alpha, opacity, base color, and model-specific response. It contains no light placement, direction, color, or energy.

## Ownership And Invalidation

Lights belong to one scene. Panels hold ordered references and inherit a useful scene default set until explicitly overridden. Cross-scene and duplicate references are rejected. Light mutation or destruction invalidates only affected panels through reverse references; no dangling handle survives into a frame plan.

Materials remain visual-owned because one geometry resource may be rendered with different response in different visuals and panels. Transient selection/highlight state remains separate from the base material.

## Active Light Types

Ambient lights contribute constant indirect irradiance. Directional lights contribute world-space parallel direct illumination. The panel payload reserves position and attenuation lanes, but point and spot evaluation is not active.

Point lights, attenuation, two-sided lighting, and the RGB Klein-bottle showcase are the optional [`../../slices/MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md`](../../slices/MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md). They extend the implemented ownership and payload rather than reopening it.

## Material Models

The active models are unlit, Phong, Standard, and limb. Depth cueing remains a separate typed effect that composes with material output.

Phong retains classic ambient, diffuse, specular, and shininess response. Its ambient field is a material response multiplier; panel ambient lights own indirect radiance.

Standard is the long-term metallic-roughness intent but remains a compact non-energy-conserving v0.4 approximation. Roughness maps to a Blinn-Phong-style exponent, specular is a white artistic multiplier, metallic suppresses diffuse, emissive is independent, and rim strength is artistic. Standard never reads Phong fields.

Limb is a thin-shell presentation model using surface normal, camera direction, and the first directional light. It is not atmospheric scattering and bypasses ambient visibility.

## Direct, Indirect, And GTAO Boundary

Shared GLSL and WGSL evaluation follows:

```text
direct = evaluate_direct_lighting(material, panel_lights, surface)
indirect_diffuse = evaluate_indirect_diffuse(material, panel_lights, surface)
linear_radiance = emissive + direct + ambient_visibility * indirect_diffuse
```

GTAO affects only indirect diffuse. Direct diffuse, direct specular, emissive, unlit, limb, transparent, volume, and overlay contributions remain independent unless a later semantic contract explicitly opts in.

## GPU And Runtime Boundary

Each panel lowers its active lights into one normalized fixed-capacity uniform payload with an explicit active count. Compatible material-facing visuals share the panel buffer through set 1 binding 4; material uploads do not contain duplicate light state. Bind-group cache identity includes panel-light resource identity.

The payload and shader contract are shared by native GLSL and WebGPU WGSL. Scene occlusion and GTAO/depth-peel bindings keep their existing set 2 and set 3 roles. No lighting feature creates another scene, renderer, presentation, or backend-specific ownership path.

## Transparency And Render Products

Material owns opacity and alpha mode; graph and visual state own opaque, WBOIT, and depth-peeling pass structure. Surface normal/depth products and ambient visibility are graph resources, not hidden material state. Selection and highlighting may alter presentation without mutating the retained base material.

## Future PBR Growth

The implemented Standard descriptor and lighting ownership are designed to support a later physically based evaluator without a new `DvzPbrMaterial` or `pbr` visual family. The authoritative ordered roadmap is [`../../../../docs/architecture/pbr_materials_roadmap.md`](../../../../docs/architecture/pbr_materials_roadmap.md).

Before replacing the Standard evaluator, specify dielectric F0 or index of refraction, metallic tint, roughness mapping, energy conservation, rim compatibility, HDR output, and GLSL/WGSL conformance. Material textures then require a resource layer separate from factors, followed by tangent validation for normal maps. Environment lighting remains panel- or scene-scoped and feeds indirect illumination; it does not belong in `DvzMaterialDesc`.

## Guardrails

1. Do not restore material-owned light direction or keep dual lighting authority.
2. Do not add a second material setter, PBR visual family, mesh renderer, or backend-specific scene-light model.
3. Do not silently reinterpret Standard as physically based.
4. Do not conflate opacity with pass selection, authored occlusion maps with GTAO, or environment lighting with material factors.
5. Do not add point lights, material maps, HDR/tonemapping, or image-based lighting to a release gate without explicit promotion.
6. Preserve one scene-to-FramePlan-to-DRP2-to-vklite runtime path.
