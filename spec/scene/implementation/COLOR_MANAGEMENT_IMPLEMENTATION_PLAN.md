# Color Management Implementation Record

Status: required v0.4 scene color-role implementation landed; explicit linear scientific export/readback remains deferred. Updated: 2026-08-01.

Use [../semantics/COLOR_MANAGEMENT.md](../semantics/COLOR_MANAGEMENT.md) for normative behavior. This record identifies the implemented boundary that future changes must preserve.

## Implemented Outcome

- `DvzColorRole` distinguishes sRGB-authored color, linear color, and non-color data.
- Sampled-field descriptors resolve or validate roles at the public scene boundary.
- Color roles propagate through retained field state, upload metadata, FramePlan, DRP2 texture creation and serialization, recording/replay, runtime resources, shader bindings, and WASM/WebGPU paths.
- Image, mesh, volume, marker-atlas, colormap, query, and generated texture paths apply explicit role policy.
- Shader/runtime paths linearize sRGB-authored inputs before lighting or compositing and leave data textures undecoded.
- Standard screenshot/export behavior remains sRGB `u8` RGBA with alpha treated linearly.
- Focused scene, DRP2, JSON, recording, query, texture-binding, shader, and conformance tests cover defaults, invalid semantic-role combinations, propagation, and output behavior.

## Remaining Boundary

Explicit linear `f16`/`f32` scientific image export/readback is deferred beyond v0.4. It belongs in app, Canvas, or runtime output APIs rather than the scene graph. HDR display management, ICC/OCIO pipelines, and general display-gamma controls remain out of scope.

## Change Checklist

Before changing sampled-field roles, texture uploads, shader color arithmetic, intermediate/final target formats, or screenshot/readback encoding:

1. preserve explicit role propagation through scene, FramePlan, DRP2, runtime, recording, and WebGPU boundaries;
2. keep lighting, blending, transparency, MSAA, postprocessing, and compositing in linear RGB;
3. keep alpha linear and final sRGB encoding exactly once;
4. add focused role and conformance tests;
5. run `just build`, relevant scene/DRP2/native/WebGPU tests, `just spec-check`, and `git diff --check`.
