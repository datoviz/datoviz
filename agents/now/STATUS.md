# Datoviz v0.4 Status

Status: active RC preparation. Updated: 2026-06-04.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

Next critical path: RC1 release proof.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| WebGPU/WASM experimental path | WebGPU fixture runner works; generic WASM scene ABI emits split DRP2 packets for point, pixel, basic marker, primitive, RGBA8 image, basic mesh, panzoom, and a 3D mesh/arcball proof. Fresh 2026-06-05 browserless and headless-browser proof is recorded in `examples/webgpu/COMPAT.md` and `docs/reference/webgpu-subset.md`. | Harden diagnostic ABI behavior and remove remaining demo shortcuts without expanding the RC subset. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04; native GPU execution skipped in this shell because Vulkan instance creation failed. | Record native Vulkan execution evidence in a Vulkan-capable environment and capture a release artifact from `examples/c/showcases/gpu_particle_smoke.c`. |
| Qt/PyQt hosted path | Native Qt hosting has an optional example path; PyQt needs a native Qt bridge because current PyQt6 wheels do not expose `QVulkanInstance::setVkInstance()` or `vkInstance()`. | Implement the optional `datoviz_qtbridge` provider from `spec/scene/integration/QT_HOST_BRIDGE.md` and prove the PyQt hosted example. |
| v0.3 visible parity audit | Missing. | Table each visible capability as fixed, deferred, or external/GSP. |
| Public API/status cleanup | Missing. | Mark public surfaces as supported, experimental, advanced/unstable, deferred, or external/GSP. |
| Release example proof | Partial. | Compact native + WebGPU proof set, plus one short public `examples/c/features/` example per v0.4 feature. |

Closed first slices that should stay in validation: raw `ctypes`, retained textured mesh, text, 2D
axes/ticks, colorbars, labels/readouts, scale bars, app/offscreen rendering, broad item/sample query
paths, scene visual-boundary checks, WebGPU fixture runner, and WASM point/panzoom scene smoke.

New Python binding direction: keep `datoviz.raw` as the exact generated `ctypes` layer, and make
top-level `import datoviz as dvz` the planned array-aware facade that preserves `dvz_*` names while
accepting NumPy arrays for policy-declared data arguments. Source of truth:
[../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md).


## Active Lanes

1. **Release closure:** feature/status table, visible parity audit, API disposition, known gaps.
2. **Example proof:** C examples and fixture smokes for the declared release surface, especially
   one short feature example per public v0.4 feature, retained textured mesh, and composed
   annotation/layout examples.
3. **WebGPU/WASM:** supported subset docs, diagnostics, portable target hardening,
   browser/runner smoke, and next visual-family expansion.
4. **Compute+graphics:** minimal DRP2 sync objects/barriers, native compute-to-render proof,
   WebGPU parity diagnostics, and a C-first particle-advection gallery target.
5. **Runtime hardening:** concrete scene -> DRP2 -> vklite/canvas/app lifetime, resize, descriptor,
   repeated-frame, or churn bugs.
6. **Qt/PyQt provider:** optional Qt bridge shared library, dynamic Python loader, binding
   diagnostics, and hosted PyQt smoke proof.
7. **Docs inventory:** public header inventory, ownership notes, raw `ctypes` scope, array-aware
   Python facade scope, WebGPU/WASM scope, known issues, and GSP/VisPy2 boundary.

When adding visible capability work, prefer gallery-proof improvements first, then vector visual
polish, label query hardening, explanatory layout proof, and optional experimental splats only if
release-proof lanes remain on track.


## References

1. [RELEASE.md](RELEASE.md)
2. [DOCUMENTATION.md](DOCUMENTATION.md)
3. [../../spec/scene/examples/PLANNING.md](../../spec/scene/examples/PLANNING.md)
4. [../../spec/scene/api/API_SURFACE.md](../../spec/scene/api/API_SURFACE.md)
5. [../../spec/scene/validation/DEFERRED_TRACKER.md](../../spec/scene/validation/DEFERRED_TRACKER.md)
6. [../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md](../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md)
7. [SHADER_TRANSFORM_FUTURE_COMPAT.md](SHADER_TRANSFORM_FUTURE_COMPAT.md)
8. [../../spec/scene/integration/OPTIONAL_PROVIDERS.md](../../spec/scene/integration/OPTIONAL_PROVIDERS.md)
9. [../../spec/scene/integration/QT_HOST_BRIDGE.md](../../spec/scene/integration/QT_HOST_BRIDGE.md)
10. [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md)


## Validation Defaults

Documentation-only:

```sh
git diff --check
git status --short
```

Scene/DRP2/runtime code:

```sh
just build
just test <filter>
just spec-check
```

Add Vulkan validation or bounded GLFW/offscreen smoke for graphics lifetimes, command buffers,
render targets, swapchains, or synchronization.
