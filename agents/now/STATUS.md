# Datoviz v0.4 Status

Status: active RC preparation. Updated: 2026-05-30.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

Next critical path: RC1 release proof.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| WebGPU/WASM experimental path | WebGPU fixture runner works; WASM scene export is still missing. | Define the portable scene/DRP2/WGSL subset, Emscripten profile, point-scene emit API, and runner smoke. |
| v0.3 visible parity audit | Missing. | Table each visible capability as fixed, deferred, or external/GSP. |
| Public API/status cleanup | Missing. | Mark public surfaces as supported, experimental, advanced/unstable, deferred, or external/GSP. |
| Release example proof | Partial. | Compact native + WebGPU proof set with validation notes and captured artifacts where needed. |

Closed first slices that should stay in validation: raw `ctypes`, retained textured mesh, text, 2D
axes/ticks, colorbars, labels/readouts, scale bars, app/offscreen rendering, broad item/sample query
paths, and scene visual-boundary checks.


## Active Lanes

1. **Release closure:** feature/status table, visible parity audit, API disposition, known gaps.
2. **Example proof:** C examples and fixture smokes for the declared release surface, especially
   retained textured mesh and composed annotation/layout examples.
3. **WebGPU/WASM:** supported subset, diagnostics, portable target, browser/runner smoke.
4. **Runtime hardening:** concrete scene -> DRP2 -> vklite/canvas/app lifetime, resize, descriptor,
   repeated-frame, or churn bugs.
5. **Docs inventory:** public header inventory, ownership notes, raw `ctypes` scope, WebGPU/WASM
   scope, known issues, and GSP/VisPy2 boundary.

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
