> **Implementation Status**
> - **Status:** `COMPLETED` (for public GPU ownership/API boundary)
> - **Verified on:** `2026-02-18`
> - **Codebase alignment:** Public headers no longer expose `DvzGpu`, `DvzGpu*`, `dvz_instance_gpus()`, or `dvz_gpu_*` pointer APIs. Active call sites in `canvas` and vklite present/swapchain setup use device/index-driven APIs. Public GPU discovery/query now goes through `dvz_instance_gpu_count()`, `dvz_instance_gpu_info()`, `dvz_instance_gpu_queue_caps()`, and `dvz_instance_gpu_handle()`; bootstrap setup uses `dvz_bootstrap_gpu_index()`/`dvz_bootstrap_gpu_info()`.
> - **Boundary note:** Low-level `DvzGpu` pointer helpers are now internal-only (`src/vk/_gpu.h`) and remain used by vk internals and vk-focused tests.

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
4. "Request feature/extension" builder calls stay available as config helper APIs (and/or direct config
   fields), never as post-hoc runtime mutation on live objects.
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
   -> config helper: `dvz_instance_config_request_layer(&cfg, ...)`
5. `void dvz_instance_request_extension(...)`
   -> config helper: `dvz_instance_config_request_extension(&cfg, ...)`
6. `void dvz_instance_validation_pre/post(...)`
   -> internalized (no public builder-stage API)
7. `DvzGpu* dvz_instance_gpus(DvzInstance* instance, uint32_t* count)`
   -> `const DvzGpuInfo* dvz_instance_gpus(const DvzInstance* instance, uint32_t* count)`


## Device

1. `void dvz_gpu_device(DvzGpu* gpu, DvzDevice* device)`
   -> removed from public API
2. `void dvz_device_request_queues(...)`
   -> config helper: `dvz_device_config_request_queue(&cfg, ...)`
3. `bool dvz_device_request_extension(...)`
   -> config helper: `dvz_device_config_request_extension(&cfg, ...)`
4. `VkPhysicalDeviceVulkan1xFeatures* dvz_device_request_featuresXX(...)`
   -> config helper group and/or explicit fields in `DvzDeviceConfig.features`
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
   6. optional helper APIs to append requested layers/extensions safely
2. `DvzDeviceConfig`
   1. `const DvzInstance* instance`
   2. `uint32_t gpu_index`
   3. queue requests
   4. extension list
   5. feature bundle (`features10/11/12/13`)
   6. convenience booleans (`enable_canvas_extensions`, etc.)
   7. optional helper APIs for queue/extension/feature requests


## Config vs runtime split

1. Config-building phase:
   1. caller mutates `DvzInstanceConfig`/`DvzDeviceConfig` directly and/or through helper APIs.
   2. this is where request-layer/request-extension/request-feature operations live.
2. Runtime phase:
   1. `dvz_instance_create()` / `dvz_device_create()` consume finalized configs.
   2. no request/builder mutation APIs exist on live runtime objects.
   3. runtime objects are managed only through create/destroy and narrow runtime operations.


## Proposed concrete API surface

1. Instance config/builders:
   1. `DvzInstanceConfig dvz_instance_default_config(void)`
   2. `bool dvz_instance_config_request_layer(DvzInstanceConfig* cfg, const char* layer_name)`
   3. `bool dvz_instance_config_request_extension(DvzInstanceConfig* cfg, const char* extension_name)`
2. Instance runtime:
   1. `DvzInstance* dvz_instance_create(const DvzInstanceConfig* cfg)`
   2. `void dvz_instance_destroy(DvzInstance* instance)`
   3. `const DvzGpuInfo* dvz_instance_gpus(const DvzInstance* instance, uint32_t* gpu_count)`
3. Device config/builders:
   1. `DvzDeviceConfig dvz_device_default_config(const DvzInstance* instance)`
   2. `bool dvz_device_config_set_gpu_index(DvzDeviceConfig* cfg, uint32_t gpu_index)`
   3. `bool dvz_device_config_request_queue(DvzDeviceConfig* cfg, DvzQueueType type, uint32_t count)`
   4. `bool dvz_device_config_request_extension(DvzDeviceConfig* cfg, const char* extension_name)`
   5. `void dvz_device_config_enable_canvas_extensions(DvzDeviceConfig* cfg, bool enabled)`
   6. `void dvz_device_config_set_features10(DvzDeviceConfig* cfg, const VkPhysicalDeviceFeatures* f10)`
   7. `void dvz_device_config_set_features11(DvzDeviceConfig* cfg, const VkPhysicalDeviceVulkan11Features* f11)`
   8. `void dvz_device_config_set_features12(DvzDeviceConfig* cfg, const VkPhysicalDeviceVulkan12Features* f12)`
   9. `void dvz_device_config_set_features13(DvzDeviceConfig* cfg, const VkPhysicalDeviceVulkan13Features* f13)`
4. Device runtime:
   1. `DvzDevice* dvz_device_create(const DvzDeviceConfig* cfg)`
   2. `void dvz_device_destroy(DvzDevice* device)`
5. Rules:
   1. all `*_config_request_*` APIs are valid only before `*_create()`.
   2. runtime objects do not expose `request_*` APIs.
   3. helper functions and direct field assignment on config structs may coexist.


## Usage sketch

1. Instance:
   1. `DvzInstanceConfig icfg = dvz_instance_default_config();`
   2. `dvz_instance_config_request_layer(&icfg, "VK_LAYER_KHRONOS_validation");`
   3. `dvz_instance_config_request_extension(&icfg, "VK_EXT_debug_utils");`
   4. `DvzInstance* instance = dvz_instance_create(&icfg);`
2. Device:
   1. `DvzDeviceConfig dcfg = dvz_device_default_config(instance);`
   2. `dvz_device_config_set_gpu_index(&dcfg, 0);`
   3. `dvz_device_config_request_queue(&dcfg, DVZ_QUEUE_GRAPHICS, 1);`
   4. `dvz_device_config_enable_canvas_extensions(&dcfg, true);`
   5. `DvzDevice* device = dvz_device_create(&dcfg);`


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
4. remove dead runtime-mutation builder paths after call-site migration while retaining config helpers.


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
1. build passes and Vulkan-path suites pass in a runtime-capable environment.
2. public `instance/device` headers expose only opaque runtime handles and config/query APIs.
3. legacy runtime builder/mutation APIs are internal-only.


## Implementation status (current)

Completed:
1. Public `DvzInstance` and `DvzDevice` runtime struct layouts are removed from
   `include/datoviz/vk/instance.h` and `include/datoviz/vk/device.h`.
2. Public API constructors are config-first:
   1. `DvzInstance* dvz_instance_create(const DvzInstanceConfig* cfg)`
   2. `DvzDevice* dvz_device_create(const DvzDeviceConfig* cfg)`
3. Public config helper APIs for queue/extension/feature requests are retained.
4. Legacy runtime builder APIs (`dvz_instance_build`, `dvz_device_build`,
   `dvz_device_request_features*`, etc.) are now internal-only through private headers
   (`src/vk/_instance.h`, `src/vk/_device.h`).
5. `DvzBootstrap` now owns pointer-based runtime objects (`DvzInstance*`, `DvzDevice*`) rather
   than embedded public structs.
6. Teardown path is hardened (idempotent/null-safe destroy behavior for instance/device).
7. Active call sites/tests in vk/vklite/canvas/testing are migrated to the new public config
   create/destroy flow.

Notes:
1. Runtime-dependent test execution still depends on local Vulkan/Metal availability; in
   non-capable environments the runtime-unavailable paths are expected.
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
