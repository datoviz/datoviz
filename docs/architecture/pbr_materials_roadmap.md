# PBR Materials Roadmap

Status: lighting foundation implemented; full PBR deferred. Updated: 2026-08-15.

Datoviz treats physically based rendering as a material model applied to eligible surface visuals, not as a separate visual family or renderer. The active runtime remains `scene frame plans -> DRP2 command streams -> vklite runtime -> canvas/stream execution`.

## Current Position

The low-regret foundation required before a physically based BRDF is implemented:

1. `DvzMaterialDesc` and `dvz_visual_set_material()` provide one retained material path for primitive, mesh, and sphere visuals.
2. `DVZ_MATERIAL_MODEL_UNLIT`, `DVZ_MATERIAL_MODEL_PHONG`, `DVZ_MATERIAL_MODEL_STANDARD`, and the limb model lower through shared GLSL and WGSL helpers in `src/scene/shaders/{glsl,wgsl}/scene_material.*`.
3. Scene-owned ambient and directional lights, panel-local ordered light sets, useful scene defaults, reverse-reference invalidation, and one shared per-panel GPU payload are implemented.
4. Materials contain surface response only; light direction, color, placement, and energy are panel-light state.
5. Shared material evaluation separates direct illumination, indirect diffuse illumination, emissive output, and ambient visibility. GTAO modulates only indirect diffuse.
6. Standard material factors have explicit v0.4 semantics and focused offscreen trends for roughness, metallic, and emissive response.
7. Mesh UV attributes and a textured mesh path exist. Material texture sets, tangents, and normal mapping do not.
8. Material-aware pass capability, surface normal/depth products, GTAO, WBOIT, and depth peeling already use the unified scene/FramePlan/DRP2 architecture.

The completed release-gate boundary and its validation evidence are recorded in [`../../spec/scene/slices/RC3_LIGHTING_FOUNDATION_SLICE.md`](../../spec/scene/slices/RC3_LIGHTING_FOUNDATION_SLICE.md). The normative active semantics are [`../../spec/scene/semantics/LIGHTING.md`](../../spec/scene/semantics/LIGHTING.md).

## What `STANDARD` Means In v0.4

`DVZ_MATERIAL_MODEL_STANDARD` is the stable standard metallic-roughness material intent, but its v0.4 shader is deliberately not an energy-conserving PBR BRDF.

Current behavior is:

1. roughness maps inversely to a Blinn-Phong-style highlight exponent;
2. specular is a nonnegative white artistic strength, not dielectric F0;
3. metallic suppresses diffuse response but does not tint specular response;
4. emissive is independent of scene lighting and ambient visibility;
5. rim strength is an additive artistic view-dependent term;
6. material evaluation is linear-light, while the current normalized scene-color attachments may clamp radiance on storage.

A true GGX implementation must land as an explicit semantic change with native GLSL/WGSL trend tests. Documentation must not call the v0.4 approximation physically based.

## Release Boundary

The lighting foundation is complete and required for RC3. Full PBR, point lights, the RGB Klein-bottle showcase, material texture maps, HDR scene color, tone mapping, and image-based lighting are optional and must not delay RC3 or RC4.

Until the exact v0.4 candidate is secure, the rendering lane is validation and preservation work:

1. keep the panel-light and material ownership split intact;
2. retain direct/indirect and GTAO-isolation regressions;
3. keep the Standard approximation documented and tested consistently;
4. rerun native and browser shader checks on exact candidates;
5. fix measured regressions without expanding the material feature set.

## Long-Term Target

The target is a metallic-roughness material model for scientific surfaces, molecular meshes, imported assets, sphere impostors, and future tube or ribbon surfaces with meaningful normals.

The target feature set is:

1. uniform base color, opacity, metallic, roughness, emissive, and optional occlusion-strength factors;
2. direct-light Cook-Torrance evaluation with GGX distribution, a documented dielectric F0 or index-of-refraction convention, energy conservation, metallic specular tint, and explicit roughness remapping;
3. optional semantic texture maps for base color, metallic-roughness, normal, emissive, and occlusion;
4. tangent-space normal mapping using a validated `vec4` tangent convention with handedness;
5. HDR linear scene color followed by explicit exposure and tone mapping;
6. optional image-based lighting with environment radiance, diffuse irradiance, prefiltered specular radiance, and a BRDF lookup resource;
7. compatibility with GTAO, depth cueing, WBOIT, depth peeling, picking, query, and future outline passes through existing graph products rather than material-specific renderer forks.

## Visual-Family Scope

Mesh is the primary PBR target because it supports indexed geometry, normals, UVs, tangents, vertex colors, and imported asset workflows. Primitive may use the same factor-only path when normals are present. Sphere impostors are a strong factor-only target because they provide analytic normals; texture-map architecture must not be designed around spheres.

Image, point, pixel, text, and volume visuals do not become PBR surfaces. They may use depth cues, transfer functions, EDL, AO, gradient shading, or other scientific techniques under their own contracts. Paths should use surface lighting only when represented as future tube or ribbon geometry with meaningful normals.

## Ordered Implementation Lanes

These lanes are additive and should be promoted independently only after the preceding contract is stable.

### 1. HDR Scene Color And Presentation

Define a floating-point scene-color product, exposure, tone mapping, output encoding, capture semantics, and adaptation for backends that cannot honor the preferred format. Material evaluation must continue returning linear radiance and must not own display transforms.

### 2. Factor-Only Direct GGX

Specify the direct-light equations early, but land and tune the production evaluator only after the HDR output contract is available. Define dielectric F0, metallic tint, roughness floor/remapping, energy conservation, rim behavior, multiple-direct-light accumulation, color ranges, and compatibility expectations for existing Standard users. Implement the same equations in GLSL and WGSL for mesh, normal-bearing primitive, and sphere. Keep texture maps and image-based lighting disabled.

### 3. Material Resources

Add a material-resource layer distinct from uniform material factors. Start with semantic `base_color` and `metallic_roughness` sampled-field slots. Resource identity, bind-layout selection, cache invalidation, serialization, and missing-map fallback must be explicit and backend-neutral.

### 4. Tangents And Normal Mapping

Add validated per-vertex `vec4` tangents only when the normal-map path is implemented end to end. Specify tangent generation, handedness, mirrored UVs, absent tangents, nonuniform transforms, and native/WGSL parity before enabling the semantic `normal` map slot.

### 5. Remaining Maps

Add emissive and occlusion maps after the binding architecture is stable. Texture occlusion is material-local authored data and remains distinct from panel-local screen-space ambient visibility; their combination must be specified rather than inferred.

### 6. Environment Lighting

Add panel- or scene-scoped HDR environment resources, diffuse irradiance, prefiltered specular radiance, and a BRDF lookup table. These resources feed the existing indirect term and do not belong in `DvzMaterialDesc`.

### 7. Import And Asset Workflows

Lower glTF or other external assets into the existing mesh, material-factor, material-resource, sampler, and texture contracts. Importers consume renderer architecture; they do not define a parallel material system.

## Independent Optional Lighting Lane

Point-light evaluation, attenuation, two-sided lighting, and the RGB Klein-bottle showcase remain the separate [`../../spec/scene/slices/MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md`](../../spec/scene/slices/MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md). They reuse the implemented scene-owned light and panel-payload architecture but are not prerequisites for a factor-only directional GGX evaluator.

## Architectural Guardrails

1. Do not add a `pbr` visual family, second mesh renderer, or backend-specific scene-light model.
2. Do not move light placement, direction, color, environment resources, exposure, or tone mapping into `DvzMaterialDesc`.
3. Do not reinterpret Standard fields silently or claim physical correctness without explicit equations and conformance tests.
4. Do not add image-based lighting before direct-light and HDR output semantics are defined.
5. Do not accumulate PBR switches in scene emission; lower material, attribute, resource, pass, and capability facts through the existing visual pipeline.
6. Do not conflate authored occlusion maps, GTAO ambient visibility, shadows, or transparency.
7. Do not make PBR expansion a v0.4 release blocker without an explicit maintainer scope decision.

## Readiness Checklist For A Promoted PBR Slice

A future implementation slice is ready only when it specifies:

1. exact public semantics and compatibility treatment;
2. GLSL/WGSL equations and toleranced conformance trends;
3. required attributes, sampled resources, bind layouts, and cache identity;
4. linear-light, HDR storage, exposure, tone-mapping, and capture boundaries;
5. interaction with GTAO, transparency, depth cueing, and render products;
6. capability adaptation and diagnostics;
7. representative mesh, sphere, metallic, dielectric, roughness, emissive, and mixed-light fixtures;
8. release scope and proof that it does not create another runtime path.
