# Errors And Logging

Datoviz diagnostics should name the scene-visible object, phase, and recoverability before backend
details. Backend error codes may appear as context, but user-facing messages should explain the
affected scene, visual, frame plan, runtime service, or query.

## Error Surfaces

| Surface | Typical behavior |
| --- | --- |
| `DvzResult` | `DVZ_OK` (`0`) on success and `DVZ_ERROR` (`-1`) on failure. |
| Other C return values | Pointer, boolean, count, status, and third-party-style `int` functions have their own contracts. Check the generated C API instead of applying `DvzResult` rules to them. |
| Result/status enum | Query, capability, runtime, or validation statuses distinguish hit/miss/unsupported/failure cases. |
| Diagnostic report | Structured or textual report associated with validation, frame planning, emission, runtime submission, completion, or WebGPU/WASM execution. |
| Log output | Human-readable diagnostics for development and runtime troubleshooting. |

## Diagnostic Phases

| Phase | Examples |
| --- | --- |
| Validation | Missing visual attributes, wrong resource kind, invalid ranges, incompatible panel target. |
| Capability adaptation | Requested feature unavailable, explicit fallback selected, optional feature disabled. |
| Frame planning | Unresolved dependency, unsupported pass arrangement, invalid query/readback plan. |
| Runtime submission | DRP2 validation failure, missing runtime capability, rejected command stream. |
| Runtime completion | Readback failure, stale query result, swapchain/presentation failure. |

## Recoverability

| Severity | Meaning |
| --- | --- |
| Fatal | The current operation cannot proceed. |
| Recoverable | The operation failed or degraded, but the app can continue and may retry or choose a fallback. |
| Warning | Non-blocking issue that should be visible to users or developers. |
| Info | Diagnostic context or capability/status note. |

A query miss is not an error. Unsupported query capability, failed readback, invalid resource usage,
or failed runtime submission are errors or recoverable diagnostics depending on context.

## Logging And Tracing

Useful diagnostics while developing:

| Tool/env | Purpose |
| --- | --- |
| `DVZ_DRP2_TRACE=normal` or `full` | Print DRP2 trace output for runtime command inspection. |
| `DVZ_DRP2_TRACE_COLOR=0` | Disable colored DRP2 trace output. |
| `NO_COLOR=1` | Disable colorized terminal output where supported. |
| Browser console | Primary surface for WebGPU adapter, capability, shader, and live-route diagnostics. |
| `examples/webgpu/COMPAT.md` | Local WebGPU evidence and known browser/runtime diagnostics. |

## Common Failure Classes

| Symptom | First checks |
| --- | --- |
| Blank frame | Scene hierarchy, visual attachment, required attributes, item counts, panel domain/camera, depth/blend state. |
| Wrong colors | Color role, sampled field format, scale binding, sRGB/linear expectations, screenshot/export path. |
| Query returns unsupported | Visual family query support, query target kind, runtime readback capability, WebGPU subset status. |
| WebGPU route fails | Manifest status, standalone live route, browser adapter support, console diagnostics. |
| Native window fails | Platform support, Vulkan loader/ICD, GLFW/window-system availability, offscreen smoke result. |

## See Also

- [Debug rendering output](../how-to/debug-rendering.md)
- [Diagnose WebGPU support](../how-to/debug-webgpu.md)
- [Diagnose build and platform issues](../how-to/diagnose-platform.md)
- [DRP2 command streams](../advanced/drp2-command-streams.md)
