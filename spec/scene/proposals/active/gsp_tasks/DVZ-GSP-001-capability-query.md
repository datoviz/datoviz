# DVZ-GSP-001: Public Runtime Capability Query

## Goal

Expose a public C API that fills `DvzCapabilitySnapshot` from a live Datoviz runtime/view/app,
preferably from `DvzView` because the active runtime reuse and render target are view-owned.

## Files To Inspect/Change

| File | Reason |
|---|---|
| `include/datoviz/app.h` | likely home for `dvz_view_capabilities()` |
| `include/datoviz/scene/types.h` | `DvzCapabilitySnapshot` fields |
| `include/datoviz/scene/frame_plan.h` | default snapshot helpers |
| `src/app/app.c` | `_app_apply_runtime_caps()` already computes runtime-backed fields |
| `src/scene/frame_plan/capabilities.c` | default snapshot and ABI validation |
| `src/scene/query/policy.c` | query profile consumption |
| `src/app/tests/test_app.c` or `src/scene/tests/app.c` | native/offscreen validation |
| `testing/test_ctypes_raw_smoke.py` and `tools/bindings/ctypes_render_smoke.py` | Python binding smoke coverage |

## Non-Goals

1. Do not expose Vulkan handles or backend structs.
2. Do not add GSP-specific capability fields.
3. Do not duplicate the whole DRP2 fixture capability schema in public scene API.
4. Do not change query profile semantics except to report actual support.

## Implementation Notes

Preferred API shape:

```c
DVZ_EXPORT bool dvz_view_capabilities(const DvzView* view, DvzCapabilitySnapshot* out);
```

Implementation should start with `dvz_capability_snapshot()`, apply the same runtime-backed facts
currently applied by `_app_apply_runtime_caps()`, then apply view/target facts where needed. The
function should return `false` for invalid pointers or unavailable runtime and leave `out` either
zeroed or documented as unchanged.

If maintainers prefer app ownership, add only one public function first and document why `DvzApp` or
`DvzView` owns the query.

## Tests/Validation

1. C test creates scene/app/offscreen view, calls the capability query, and verifies a valid ABI
   prologue plus sane nonzero limits.
2. Test verifies readback/query profile fields are coherent: profile support implies readback and
   required render-target format support.
3. Raw ctypes smoke loads the function and calls it when a runtime is available.
4. Run narrow app/scene test plus `git diff --check`.

## Stop Conditions

1. Querying capabilities would require exposing Vulkan/device/runtime handles.
2. Runtime-backed fields disagree with the app draw path in a way that changes rendered behavior.
3. The preferred owner (`DvzView` vs `DvzApp`) becomes a high-level API decision; pause and consult
   maintainers before implementation.
