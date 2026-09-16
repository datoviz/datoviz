# Interactive Mesh Performance Handoff

Status: active non-blocking v0.4-final measurement work and preserved early-v0.5 implementation sequence. Updated: 2026-09-16.

This handoff records execution evidence and task order for large semantically partitioned meshes. Durable query behavior lives in [GPU_QUERY_SYSTEM.md](../../spec/scene/interaction/GPU_QUERY_SYSTEM.md), asynchronous completion in [ASYNC_QUERY_EXECUTION.md](../../spec/scene/proposals/future/ASYNC_QUERY_EXECUTION.md), mesh-part state in [MESH_PART_STATE.md](../../spec/scene/proposals/future/MESH_PART_STATE.md), and embedded presentation in [GUI_VIEWPORT_PRESENTATION.md](../../spec/architecture/GUI_VIEWPORT_PRESENTATION.md).

## Consumer Evidence

An IBL D070 atlas workload measured 486,674 vertices, 966,645 triangles, and 1,140 mesh components at 900 x 720 on an NVIDIA RTX 5090 using Datoviz `18b4840ff`. Five randomized fresh processes used 30 warm-up and 120 measured frames.

| Scenario | Median frame/run time | Isolated phase | Median RSS |
| --- | ---: | ---: | ---: |
| Direct populated surface | 4.14 ms | — | approximately 363 MiB |
| Complete GUI without pointer query | 7.34 ms | GUI viewport 5.12 ms | — |
| One pointer-driven FACE query per frame | 11.30 ms | Query 9.87 ms | approximately 535 MiB |
| Four pointer events per frame | 11.93 ms | Query 10.42 ms | — |

The burst workload emitted 480 pointer events but executed 120 queries, confirming latest-position coalescing within each frame. The remaining hover cost is one synchronous query per rendered pointer frame, not an event backlog. Full retained-tree construction costs less than one millisecond per frame, and a complete static 2D atlas view measures about 0.28 ms per frame; neither is a priority optimization target.

The GUI result was collected after `18b4840ff`, which prevents duplicate scheduler and GUI rendering of embedded sources. The remaining populated-viewport cost includes the source render and the device-wide wait in the strict resolution path.

## RC3 And v0.4-Final Work

These changes are useful but non-blocking and may land only as focused commits with passing validation:

1. Split query timing into build, FramePlan/DRP2 emission and validation, backend execution, download/wait, and decode/readout without adding synchronization for measurement.
2. Record first-query versus steady-state behavior, derived query vertices, upload bytes, retained resource bytes, submissions, completions, failures, superseded requests, and coalesced requests.
3. Retain deterministic synthetic large-mesh ITEM/FACE and GUI/tree/embedded-viewport benchmark workloads with machine-readable output.
4. Evaluate a mesh-only one-pixel physical query attachment while preserving logical source-pixel semantics and exact results.
5. Investigate indexed exact-face query geometry only after counters establish the current allocation and upload shape.

Ordinary CI asserts deterministic lifecycle, command, upload, allocation, freshness, and result-shape invariants rather than wall-clock thresholds. Before/after timing comparisons use fixed dimensions, warm-up, fresh processes, matched builds, matched backend/validation settings, randomized run order, and recorded hardware and commit identities.

## Stop And Retention Rules

- Do not retain the one-pixel target if edge, depth, transform, panel-offset, DPI, native, or WebGPU query semantics differ, or if representative end-to-end improvement is negligible.
- Do not retain indexed exact-face lowering if it exposes backend primitive identity, loses supported backend behavior, changes public results, or fails to reduce representative retained/uploaded bytes materially.
- Do not add an atlas-specific API, mesh-region alias, CPU visual-picking fallback, or second per-part state mechanism.
- Do not make asynchronous query execution, mesh parts, or GUI viewport pipelining an RC3 release gate.
- Keep pointer-rate policy, ontology lookup, and domain face-to-region mapping application-owned.

## Post-v0.4 Sequence

1. Implement bounded asynchronous query execution and later-frame completion while retaining the explicit synchronous helper.
2. Implement mesh parts in independently validated slices: normalized indexed subdraws, retained part layout/state, GPU state buffer, direct subdraw lowering, ITEM identity, batched Python facade, then native/WebGPU and transparency coverage.
3. Implement GUI viewport submitted/completed/displayed generations, bounded source-image buffering, scoped completion, fair multi-viewport scheduling, and deferred retirement before removing the steady-state device-wide wait.
4. Add combined part-plus-face identity only after ordinary part interaction no longer depends on expanded face query geometry.
5. Consider faster subdraw lowering, culling, or indirect commands only when direct multipart benchmark evidence crosses a recorded trigger.

## Validation Routes

Query code changes require focused scene query tests, `just build`, `just test scene`, `just spec-check`, native validation-layer coverage, relevant WebGPU fixture/browser checks, and `git diff --check`. Shader or shader-ABI changes additionally require `just shader-abi-check`. Public headers or bindings require `just ctypes` and `just ctypes-check` before Python validation.

GUI viewport synchronization changes require focused GUI viewport tests, idle and multi-viewport scheduling coverage, resize/hide/show/destruction smokes, native validation layers, bounded live runs, and the architecture contract's generation and lifetime cases.

## Release Records

Update `STATUS.md` only when retained work changes current candidate evidence or gates. Record exact candidate commit and artifact/platform validation in `RELEASE.md` only after the implementation is selected and validated. When this execution lane is complete, preserve durable behavior in `spec/` and remove this handoff; Git history remains the completed-work archive.
