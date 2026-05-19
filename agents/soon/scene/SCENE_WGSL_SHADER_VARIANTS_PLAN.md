# Scene WGSL Shader Variants Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining WGSL parity work after the initial retained-scene registry and
>   emission slices landed.


## Current State

The current WGSL parity queue lives in
[`../../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md`](../../../spec/scene/implementation/VISUAL_SHADER_REFACTOR.md).
That implementation note owns the active table for point/pixel, primitive/image, marker,
segment/path stroke, sphere, volume, and advanced-pass parity lanes.

The initial scene-owned WGSL path is in place for active point/pixel, primitive/lit primitive,
image, texture, fixture, fullscreen, material, and common-helper paths. Scene selects the requested
shader format during scene -> DRP2 emission, and the WebGPU runner should execute the stream it is
given rather than translating GLSL or substituting scene shaders in the browser.


## Remaining WGSL Work

Recommended follow-up commits:

1. Keep point/pixel and primitive/image parity intact whenever style, cue, picking, material,
   texture, or common-bind ABI changes.
2. Add marker WGSL only with committed source, instanced-quad lowering, registry coverage, DRP2
   emission tests, and a fixture/runtime smoke.
3. Add segment/path-stroke WGSL only after stroke vertex math, caps, joins, and style/material
   bindings are represented in the portable ABI.
4. Add sphere WGSL only after the WebGPU impostor lowering and depth behavior are explicit.
5. Add volume WGSL only after transfer-texture, clipping-plane, slice/MIP/composite, and sampler
   bindings are stable in the GLSL path.
6. Keep WBOIT, depth peel, SSAO, EDL, and G-buffer variants capability-gated until WebGPU support is
   intentionally activated for each pass.
7. Keep `tools/generate_missing_scene_wgsl.py` conservative: committed WGSL files remain the source
   reviewed by the registry, and the generator should not overwrite hand-edited variants by default.


## Ownership Rules

Use this boundary:

1. scene owns built-in shader semantics and source variants;
2. scene-to-DRP2 emission selects one backend-ready shader format for the target stream;
3. DRP2 transports selected shader modules;
4. the WebGPU backend accepts WGSL and rejects unsupported shader formats;
5. browser examples and dashboards stay backend harnesses, not scene shader translators.

Do not add browser-side GLSL-to-WGSL translation or scene shader replacement by hash.


## Validation

For WGSL source or registry changes:

```text
just build
just test scene
python3 tools/generate_missing_scene_wgsl.py --check
just webgpu-fixture-preflight
git diff --check
```

Run the browser fixture dashboard only when touching `examples/webgpu/` or a stream fixture consumed
by that dashboard.
