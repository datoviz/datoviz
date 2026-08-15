# RC3 Lighting Foundation Slice

Status: complete at validated implementation head `8fd98715e`. Updated: 2026-08-15.

This slice makes the smallest pre-final lighting correction that prevents a later PBR rearchitecture. It separates panel lighting from visual materials and separates direct from indirect illumination in the shared shader contract. It does not implement full PBR, image-based lighting, or the optional multi-light showcase.

The requirements and sequence below are retained as completion evidence. They describe the implemented contract, not pending RC3 work.

The semantic sources of truth remain [LIGHTING.md](../semantics/LIGHTING.md), [MATERIAL_LIGHTING_API.md](../proposals/active/MATERIAL_LIGHTING_API.md), and [OCCLUSION_EFFECTS.md](../implementation/OCCLUSION_EFFECTS.md).

## Required Decisions

1. Lights are scene-owned reusable objects.
2. Panels select a panel-local ordered light set.
3. Materials remain visual-owned and contain surface response, not light placement or direction.
4. Ambient visibility affects only indirect diffuse illumination.
5. Direct lighting, emissive output, unlit output, transparency techniques, and volume semantics remain independent of ambient visibility unless their contracts explicitly opt in later.
6. Lighting arithmetic and material output remain linear; material evaluation must not own display encoding or tone mapping.
7. Native GLSL and WebGPU WGSL share the same lighting semantics and normalized payload.

## Required RC3 Scope

### Scene And Panel Ownership

Add the first descriptor-oriented scene-light API with scene-owned lifetime, validated updates, deterministic destruction, and cross-scene rejection. The initial active types are ambient and directional lights. Point lights remain an additive follow-up because the fixed-capacity representation reserves the type-specific position and attenuation shape without requiring point-light behavior in this release gate.

Panels select a bounded ordered set of scene-owned lights and inherit a useful neutral default set when none is assigned explicitly. Use a fixed capacity and explicit active count. Changing or destroying a light invalidates every affected panel through reverse references; it must not dirty every visual or leave dangling handles.

Finalize exact public names in an API review before implementation. Prefer one semantic `DvzLightDesc` creation path plus focused setters and panel light-set operations over constructor proliferation.

### Material Boundary

Remove `light_direction` from `DvzMaterialDesc` once panel lighting becomes authoritative and migrate all examples, tests, bindings, and documentation in the same implementation sequence. Do not retain dual material-owned and panel-owned authority after the migration.

Phong material coefficients remain model-specific surface-response controls. Its current `ambient` field may remain as a classic-material indirect-response multiplier, but ambient light color and energy come from panel lighting. Standard material evaluation must not read Phong fields implicitly; its indirect diffuse response derives from its own base color and metallic semantics.

### GPU And Runtime Boundary

Lower the active panel light set into one normalized, fixed-capacity GPU payload with an explicit active count. Keep that payload separate from per-visual material uploads and share it across compatible lit visuals in the panel. Bind it through generic lit-visual lowering facts rather than concrete visual-family checks.

The payload must leave additive room for point-light position and attenuation, additional direct lights, environment lighting inputs, and future shadow visibility without changing material ownership or the scene-to-DRP2 runtime path. Do not create a second renderer, presentation path, or backend-specific scene model.

### Shader Contract

Refactor the shared material evaluator around this semantic decomposition:

```text
direct = evaluate_direct_lighting(material, panel_lights, surface)
indirect_diffuse = evaluate_indirect_diffuse(material, panel_lighting, surface)
linear_radiance = emissive + direct + ambient_visibility * indirect_diffuse
```

For this slice, indirect illumination is constant ambient irradiance accumulated from the panel's ambient lights. A future environment map may replace or augment that irradiance and may use a bent normal without changing the composition contract.

Material evaluation returns linear radiance and must not perform display encoding or own tone mapping. Existing attachment formats may clamp on storage until an HDR scene-color product lands, but that limitation must not be embedded in the material API or lighting decomposition.

### GTAO Integration

Keep the existing panel-local `ambient_visibility` product and its surface-analysis placement. GTAO multiplies only `indirect_diffuse`; it does not darken direct diffuse, direct specular, emissive, unlit, transparent, volume, or overlay contributions.

After the ownership and shader contracts are correct, tune the default ambient irradiance and GTAO response so the default effect is clearly visible while remaining restrained. The public strength range may permit deliberately aggressive visibility shaping, but stronger settings must still operate only on indirect illumination.

The current local experiment that makes Standard shading read the shared material ambient slot is not the final architecture. Supersede it with explicit panel ambient irradiance and model-owned material response rather than preserving an implicit Phong-to-Standard field dependency.

## Required Tests

1. An ambient-only scene changes visibly when GTAO is enabled.
2. A directional-only scene is unchanged when GTAO is toggled.
3. A mixed scene changes only in its indirect contribution.
4. Emissive and unlit output are unchanged by GTAO.
5. Phong and Standard materials obey the same direct-versus-indirect contract without reading each other's model-specific fields.
6. Two panels may use distinct light sets without cross-panel state leakage.
7. Light update, destruction, capacity, cross-scene rejection, and repeated frame emission are deterministic.
8. One panel-light payload is shared across compatible lit visuals when practical and does not cause per-visual light uploads.
9. GLSL and WGSL implement equivalent lighting and GTAO behavior.
10. Disabling GTAO removes the ambient-visibility dependency without changing the panel-light contract.

## Explicitly Deferred

The following work remains additive after this slice and is not required before the next release candidate:

1. point and spot light evaluation;
2. the RGB multi-light Klein-bottle showcase and two-sided-lighting expansion;
3. metallic/roughness BRDF replacement;
4. HDR environment maps, irradiance convolution, prefiltered specular IBL, and BRDF lookup tables;
5. bent normals, temporal GTAO, and screen-space global illumination;
6. shadow maps, ray-traced direct visibility, clustered lighting, and ray-traced global illumination;
7. HDR scene-color attachments, exposure, and tone mapping unless another release gate independently requires them.

## Implementation Sequence

1. Finalize the public descriptor and panel light-set API, retained ownership, reverse references, default light set, and invalidation behavior.
2. Add the shared panel-light GPU payload and generic runtime binding while preserving the existing scene frame-plan to DRP2 to vklite path.
3. Remove material-owned light direction, migrate callers and generated bindings, and refactor the GLSL/WGSL material evaluators into direct and indirect contributions.
4. Connect `ambient_visibility` only to indirect diffuse, add semantic regressions, then tune the default ambient energy and useful GTAO strength range.
5. Validate native and WebGPU paths, examples, bindings, specifications, and repository hygiene before freezing the next release candidate.

Required validation after implementation:

```sh
just build
just test scene
just shader-abi-check
just ctypes
just ctypes-check
just spec-check
just wasm-scene-smoke
git diff --check
```

Run the relevant native offscreen GTAO examples and available WebGPU/browser smoke in addition to this minimum. Record unavailable environment-dependent validation honestly.

Validation at `8fd98715e` passed the full scene suite, native ambient-only/directional-only GTAO semantics, shader ABI checks, generated ctypes and ABI checks, specification checks, WebGPU fixture and WASM stream smoke, and visual inspection of the native lighting gallery capture. Browser smoke compiled the WGSL lighting path after the reserved payload-field regression was fixed; actual headless WebGPU submission remained skipped because the local browser instance was lost before queue submission.

## Acceptance

This slice is complete only when panel lighting is the sole authority for light direction and ambient energy, materials contain only model-specific surface response, the shared shader explicitly separates direct and indirect illumination, GTAO affects only indirect diffuse, the semantic tests pass in both shader languages, and the next release-candidate checklist records the validated implementation head.
