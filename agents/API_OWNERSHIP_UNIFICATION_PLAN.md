# Datoviz v0.4-dev API Ownership Unification Plan

This plan unifies Datoviz runtime ownership semantics while the API is still private and breaking
changes are acceptable.

Primary decision:
1. Use one runtime object model everywhere: opaque heap-owned handles with `create/destroy`.
2. Remove mixed caller-owned struct lifecycles from the public API for core runtime objects.


## Scope

In scope:
1. `instance`, `gpu` access surface, `device` public API redesign.
2. Call-site migration in active modules and tests.
3. Removal of legacy init/mutation patterns once migration lands.

Out of scope:
1. Renderer/scene/client scaffolding modules not currently active.
2. Non-runtime POD config/value types that are safe to keep by-value.


## Current problem statement

Today the API mixes two ownership styles:
1. Caller-owned structs (stack/caller memory): `DvzInstance`, `DvzDevice`.
2. Library-owned opaque pointers: `DvzWindow*`, `DvzCanvas*`, `DvzStream*`.

This creates avoidable ambiguity:
1. Different lifecycle patterns per object family.
2. Hidden rules about mutability/copyability of public structs.
3. Harder internal evolution because public structs expose internal state layout.


## Target model

For all runtime objects:
1. Public headers expose only forward declarations.
2. Construction uses `DvzX* dvz_x_create(const DvzXConfig* cfg)`.
3. Destruction uses `void dvz_x_destroy(DvzX* x)`.
4. Runtime mutation is either:
   1. explicit setter APIs with narrow contracts, or
   2. immutable config consumed at create time.
5. No public writable struct fields for runtime internals.


## API design rules

1. `*_create()` returns `NULL` on failure and never partially-initialized objects.
2. `*_destroy(NULL)` is always safe.
3. Config is POD, by-value or const pointer input, with default helper:
   1. `DvzXConfig dvz_x_default_config(void)`.
4. "Request feature/extension" builder calls become config fields, not post-hoc mutable internals.
5. Query APIs return immutable snapshots or typed handles, never internal raw struct pointers.


## Symbol migration map (old -> new)

## Instance

1. `void dvz_instance(DvzInstance* instance, int flags)`
   -> `DvzInstance* dvz_instance_create(const DvzInstanceConfig* cfg)`
2. `int dvz_instance_create(DvzInstance* instance, uint32_t vk_version)`
   -> folded into `dvz_instance_create(...)` (single-step create)
3. `void dvz_instance_destroy(DvzInstance* instance)`
   -> unchanged name, now frees heap object
4. `void dvz_instance_request_layer(...)`
   -> config field: `DvzInstanceConfig.layers[]` / count
5. `void dvz_instance_request_extension(...)`
   -> config field: `DvzInstanceConfig.extensions[]` / count
6. `void dvz_instance_validation_pre/post(...)`
   -> internalized (no public builder-stage API)
7. `DvzGpu* dvz_instance_gpus(DvzInstance* instance, uint32_t* count)`
   -> `const DvzGpuInfo* dvz_instance_gpus(const DvzInstance* instance, uint32_t* count)`


## Device

1. `void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device)`
   -> removed from public API
2. `void dvz_device_request_queues(...)`
   -> config field: `DvzDeviceConfig.queue_requests[]`
3. `bool dvz_device_request_extension(...)`
   -> config field: `DvzDeviceConfig.extensions[]`
4. `VkPhysicalDeviceVulkan1xFeatures* dvz_device_request_featuresXX(...)`
   -> config field group in `DvzDeviceConfig.features`
5. `int dvz_device_create(DvzDevice* device)`
   -> `DvzDevice* dvz_device_create(const DvzDeviceConfig* cfg)`
6. `void dvz_device_destroy(DvzDevice* device)`
   -> unchanged name, now frees heap object
7. `void dvz_device_request_canvas_extensions(DvzDevice* device)`
   -> helper on config: `dvz_device_config_enable_canvas_extensions(&cfg, true)`


## GPU selection surface

1. Public `DvzGpu` mutable/internal details
   -> replace with immutable `DvzGpuInfo` descriptor (name, queue caps summary, features summary)
2. `DvzGpu*` handoff through public API
   -> replace with stable index or opaque GPU handle token in config:
   1. `DvzDeviceConfig.gpu_index`


## Windows/canvas/stream

No ownership model change required (already create/destroy pointer-based).
Required adjustments:
1. Update all call sites to consume `DvzInstance*` and `DvzDevice*`.
2. Remove any assumptions of embedded device/instance structs.


## New config types (proposed)

1. `DvzInstanceConfig`
   1. app name/version
   2. validation flags
   3. portability flag
   4. requested layers/extensions arrays
   5. Vulkan API version
2. `DvzDeviceConfig`
   1. `const DvzInstance* instance`
   2. `uint32_t gpu_index`
   3. queue requests
   4. extension list
   5. feature bundle (`features10/11/12/13`)
   6. convenience booleans (`enable_canvas_extensions`, etc.)


## Public header refactor checklist

1. `include/datoviz/vk/instance.h`
   1. remove `struct DvzInstance` definition from public header
   2. add `DvzInstanceConfig` and create/destroy/query API
2. `include/datoviz/vk/device.h`
   1. remove `struct DvzDevice` definition from public header
   2. add `DvzDeviceConfig` and create/destroy/query API
3. keep internals in private headers under `src/vk/` and `src/common/` as needed
4. update umbrella exports in `include/datoviz/*.h`


## Internal refactor checklist

1. move struct definitions into private internal headers:
   1. `src/vk/instance_internal.h`
   2. `src/vk/device_internal.h`
2. keep allocator, object lifecycle, and feature wiring semantics unchanged internally first.
3. add constructor helpers to build runtime objects from configs.
4. remove dead builder paths after call-site migration.


## Migration phases

## Phase U0 - Freeze spec

Deliverables:
1. finalize this plan and symbol map.
2. define non-goals and deletion policy (no compatibility shim unless strictly required for migration).

Exit criteria:
1. no unresolved API naming or ownership ambiguities.


## Phase U1 - Introduce new APIs in parallel

Deliverables:
1. add new config structs and `create/destroy` constructors for instance/device.
2. keep old APIs temporarily for migration-only period.

Exit criteria:
1. both API families compile; new path can create instance+device.


## Phase U2 - Migrate all call sites

Targets:
1. `src/canvas/tests/test_canvas.c`
2. `testing/dvz_live_canvas.c`
3. `testing/dvztest.c` and vk/vklite/canvas tests
4. window/vk/vklite helpers and any active module callers

Exit criteria:
1. tree compiles with only new APIs used by active modules/tests.


## Phase U3 - Remove legacy APIs

Deliverables:
1. delete old stack-style init/request/create signatures.
2. remove transitional wrappers and stale docs/comments.

Exit criteria:
1. no references to removed symbols in `include/`, `src/`, `testing/`.


## Phase U4 - Validation and cleanup

Deliverables:
1. run build and target test suites.
2. verify docs/examples only show new ownership model.
3. ensure no public struct layout leaks remain.

Exit criteria:
1. green build/tests on supported runtime environments.
2. ownership model is consistent across all active runtime objects.


## Execution order (recommended)

1. Define `DvzInstanceConfig` and `dvz_instance_create/destroy`.
2. Add immutable GPU query API (`DvzGpuInfo`, index-based selection).
3. Define `DvzDeviceConfig` and `dvz_device_create/destroy`.
4. Migrate canvas/window/vklite test fixtures and smoke apps.
5. Remove old APIs and public struct definitions.
6. Final pass on docs/comments and tests.


## Risk register

1. Risk: Large call-site churn in tests.
   1. Mitigation: migrate fixtures first; they cover most call paths.
2. Risk: Feature/extension request regressions.
   1. Mitigation: explicit config defaults + parity tests against old behavior before deletion.
3. Risk: Hidden reliance on public struct fields.
   1. Mitigation: compile-break approach is desired; fix each caller intentionally.


## Verification matrix

Build:
1. `just build`

Focused tests:
1. `direnv exec . just test vk`
2. `direnv exec . just test vklite`
3. `direnv exec . just test canvas`
4. `direnv exec . just test window`

Smoke app:
1. `./build/testing/dvz_live_canvas --backend offscreen --duration 2`
2. `./build/testing/dvz_live_canvas --backend glfw --duration 2` (where supported)


## Done definition

1. Runtime API ownership is uniform: create/destroy opaque handles only.
2. No public runtime struct layouts for instance/device.
3. No active code path depends on legacy stack-style lifecycle APIs.
4. Tests and smoke apps run with the new model.
