# Color Management Implementation Plan

Status: ready for near-term v0.4 implementation.

This is the pickup plan for implementing the scene color-management contract in
[`../semantics/COLOR_MANAGEMENT.md`](../semantics/COLOR_MANAGEMENT.md). Datoviz v0.4 may break
v0.3 compatibility where needed to make color/data intent explicit and mechanically hard to misuse.


## Pickup For Future Agents

Start here when implementing color management:

1. read [`../semantics/COLOR_MANAGEMENT.md`](../semantics/COLOR_MANAGEMENT.md);
2. read [`../pipeline/RESOURCE_MODEL.md`](../pipeline/RESOURCE_MODEL.md) for `SampledField`
   resource-role requirements;
3. read [`../validation/RENDER_CONFORMANCE.md`](../validation/RENDER_CONFORMANCE.md) for required
   color-management fixtures;
4. implement the steps below in order, with one checkpoint commit after each testable slice.

Do not preserve ambiguous v0.3 texture/color behavior if it conflicts with explicit v0.4 color
roles, linear rendering, or one final sRGB encode.


## Implementation Steps

1. Add a breaking scene-facing color-role enum.

   Required values:

   ```c
   DVZ_COLOR_ROLE_SRGB_COLOR
   DVZ_COLOR_ROLE_LINEAR_COLOR
   DVZ_COLOR_ROLE_DATA
   ```

   Put the role on `SampledField` / texture resource descriptors and scene resource metadata. Do
   not hide it only in backend texture creation.

2. Require or infer texture roles at API boundaries.

   New v0.4 APIs should require an explicit role. Transitional helpers such as `dvz_texture_2d`,
   `dvz_texture_3d`, or old image/mesh/sphere/volume binding shortcuts may infer the documented
   default only during migration and should emit a diagnostic.

3. Thread roles through scene planning and DRP2 emission.

   Carry the role through resource metadata, dirty/revision state, `FramePlan` resource records,
   DRP2 texture/create/upload commands, DRP2 recording/replay metadata, and WebGPU fixture
   serialization.

4. Add shared sRGB conversion helpers and tests.

   Implement focused helpers for sRGB `u8` -> linear `f32`, linear `f32` -> sRGB `u8`, and alpha
   pass-through. Do not introduce a generic display `gamma` API.

5. Update shader contracts.

   Linearize `rgba_u8` vertex/uniform colors before lighting, blending, transparency, volume
   compositing, or post-processing arithmetic. Decode `srgb_color` texture samples through hardware
   sRGB sampling where available or explicit shader conversion. Never decode `data` textures.

6. Audit render target and resolve paths.

   Intermediate color targets, WBOIT accumulation, post-processing targets, MSAA resolves, and
   generated render inputs must store linear color. Final display and standard screenshot output
   must encode to sRGB exactly once.

7. Update screenshot and classify scientific readback paths.

   PNG screenshots are sRGB `u8` RGBA. Explicit linear `f16` or `f32` scientific/export image
   readback is deferred beyond RC1. Keep that future API in app/canvas/runtime code, not scene
   graph objects.

8. Add diagnostics.

   Missing roles should be validation errors for new APIs. Warn on suspicious combinations such as
   `srgb_color` for normal, depth, picking, id, label, or mask fields, and `data` used as direct
   display color without a colormap or explicit color-volume declaration.

9. Add focused tests before broad refactors.

   Minimum tests:

   - sRGB helper unit tests;
   - scene resource tests proving sampled fields retain color roles;
   - scene -> DRP2 emission tests proving roles survive lowering;
   - image, mesh, sphere, and volume texture-role tests;
   - mid-gray screenshot or fixture that catches missing or double final sRGB encoding;
   - alpha/WBOIT fixture over a non-black background;
   - colormap fixture proving sRGB-authored tables are linearized before compositing.

10. Rename data-domain transfer controls away from display-gamma terminology.

    Replace v0.4-facing data-normalization fields named `gamma` with `power_exponent`,
    `scale_power`, or equivalent names. Reserve `sRGB encode/decode` for color-management behavior.


## Suggested Checkpoint Commits

1. resource enum and metadata plumbing;
2. API-boundary role defaults/diagnostics;
3. DRP2 and WebGPU serialization role propagation;
4. shader/runtime linearization and render-target audit;
5. screenshot/readback encoding behavior;
6. conformance fixtures and cleanup of `gamma` terminology.


## Validation Floor

For each checkpoint:

```sh
git diff --check
just build
just test scene
```

For shader/runtime/output changes, add the narrowest relevant native or WebGPU smoke available in
the current environment. If Vulkan or browser execution is unavailable, record that explicitly in
the commit or handoff.
