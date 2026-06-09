# Datoviz v0.4 Status

Status: active RC preparation. Updated: 2026-06-09.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

Next critical path: RC1 release proof.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| WebGPU/WASM experimental path | WebGPU fixture runner works; generic WASM scene ABI emits split DRP2 packets for buffer-backed point/pixel positions, basic marker, segment/path with cap/join controls, primitive, RGBA8 image, basic retained 2D axes/ticks/grid labels, basic signed 2D labels, low-level atlas glyph, semantic bitmap text, basic/textured/material mesh, basic sphere, panzoom, a 3D sphere + textured mesh/arcball proof, and the first portable C scenario/frame-callback proof for `feature_timer_animation`. Native scenario runner now has requirements, portable event/post-frame hooks, and `feature_pick_point`/`feature_pick_marker`/`feature_pick_hover`/`feature_selection`/`image_probe` migrated off `native_view` as query/readback-shaped scenario proofs. WASM scenarios expose browser pointer/wheel event delivery, and point-query packet/readback plumbing exists for `feature_pick_point`. The frame artifact refactor is complete: scene emission returns artifact-owned stream snapshots, WASM/WebGPU consumes artifact packet spans, JSON is debug/fixture-only, and retained scene mutation no longer depends on raw emitted stream lifetime. | Extend narrow WASM/WebGPU request/query/readback from point query to marker, selection/hover, and one pixel probe; classify live WebGPU vs native-only examples from manifest metadata. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04; native GPU execution skipped in this shell because Vulkan instance creation failed. | Record native Vulkan execution evidence in a Vulkan-capable environment and capture a release artifact from `examples/c/showcases/gpu_particle_smoke.c`. |
| Qt/PyQt hosted path | Native Qt hosting has an optional example path; PyQt needs a native Qt bridge because current PyQt6 wheels do not expose `QVulkanInstance::setVkInstance()` or `vkInstance()`. | Implement the optional `datoviz_qtbridge` provider from `spec/scene/integration/QT_HOST_BRIDGE.md` and prove the PyQt hosted example. |
| v0.3 visible parity audit | Missing. | Table each visible capability as fixed, deferred, or external/GSP. |
| Public API/status cleanup | Missing. | Mark public surfaces as supported, experimental, advanced/unstable, deferred, or external/GSP. |
| Release example proof | Partial. The broad `EXAMPLES_NOTES.md` pass has closed most source/gallery polish items; `showcases/surface_grid`, `features/bounds_overlay`, and the runtime/readability batch are now resolved with native smoke evidence. The remaining queue is the broad comments/scenario-helper/builtin-shapes audit in `EXAMPLES_NOTES.md`. | Complete scenario-helper/comments/builtin-shapes audits and capture any additional focused native evidence where the environment supports Vulkan. |

Closed first slices that should stay in validation: frame artifact scene emission, raw `ctypes`, retained textured mesh, retained
DATA-coordinate visual attachments, text, 2D axes/ticks, colorbars, labels/readouts, scale bars,
app/offscreen rendering, broad item/sample query paths, scene visual-boundary checks, WebGPU fixture
runner, and WASM frame artifact packet scene smoke.

New Python binding direction: keep `datoviz.raw` as the exact generated `ctypes` layer, and make
top-level `import datoviz as dvz` the planned array-aware facade that preserves `dvz_*` names while
accepting NumPy arrays for policy-declared data arguments. Source of truth:
[../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md).


## Active Lanes

1. **Release closure:** feature/status table, visible parity audit, API disposition, known gaps.
2. **Example proof:** C examples and fixture smokes for the declared release surface, especially
   one short feature example per public v0.4 feature, retained textured mesh, and composed
   annotation/layout examples.
3. **WebGPU/WASM RC examples:** portable scenario host, native event/query scenario migrations,
   browser scenario event delivery, live website examples for most non-desktop scene scenarios,
   current subset diagnostics, compute particles, narrow request/query/readback,
   manifest-backed example classification, and browser/runner smoke evidence.
4. **Compute+graphics:** minimal DRP2 sync objects/barriers, native compute-to-render proof,
   WebGPU parity diagnostics, and a C-first particle-advection gallery target that becomes the
   browser compute proof once the WASM scenario host can drive it.
5. **Runtime hardening:** concrete scene -> DRP2 -> vklite/canvas/app lifetime, resize, descriptor,
   repeated-frame, or churn bugs.
6. **Qt/PyQt provider:** optional Qt bridge shared library, dynamic Python loader, binding
   diagnostics, and hosted PyQt smoke proof.
7. **Docs inventory:** public header inventory, ownership notes, raw `ctypes` scope, array-aware
   Python facade scope, WebGPU/WASM scope, known issues, and GSP/VisPy2 boundary.

Current runtime/WebGPU guardrail: keep texture-backed scene visuals on typed visual/draw-contract
DRP2 streams. Do not restore legacy texture-render shortcuts; WebGPU parity work should validate
capability diagnostics against the same streams used by native runtime execution.

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
11. [../../spec/scene/integration/WASM_DEBUGGABILITY_REFACTOR_PLAN.md](../../spec/scene/integration/WASM_DEBUGGABILITY_REFACTOR_PLAN.md)
    - includes the 2026-06 WASM stack-overflow failure mode and the required
      ASan/SAFE_HEAP/stack-usage workflow.


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
