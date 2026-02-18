> **Implementation Status**
> - **Status:** `PARTIALLY COMPLETED`
> - **Verified on:** `2026-02-18`
> - **Codebase alignment:** Public GPU ownership/API cleanup is complete (`DvzGpu*` removed from public headers, index/descriptor flow active).
> - **Current progress:** Step 0 completed, Step 1 contract defined, Step 2 completed for bootstrap/proto flows, Step 4 completed for `DvzSurface`/`DvzSwapchain` (opaque public handles + accessor migration).
> - **Remaining gap:** Non-GPU ownership boundaries between `vk` and `vklite` still need tightening across the rest of vklite public structs and lifecycle matrices.

# Datoviz v0.4-dev VK/VKLite Ownership Boundary Refactor Plan

This plan defines the next ownership slice after GPU API cleanup: strict public/internal separation and
single-owner lifecycle contracts across `vk` and `vklite`.

Primary decisions:
1. `vklite` must consume `vk` only through public headers under `include/datoviz/vk/`.
2. Private headers (`src/vk/_*.h`) are internal to `src/vk` and vk-focused tests only.
3. Runtime objects have one clear owner and one destroy path.
4. No legacy compatibility shims are required for removed public APIs.


## Scope

In scope:
1. `vk` <-> `vklite` API boundary hardening.
2. Removal of `vklite` dependencies on vk private headers/types.
3. Public API additions where needed to replace private access.
4. Lifecycle/ownership cleanup for instance/device/surface/swapchain/present paths.
5. Regression tests and migration of active call sites.

Out of scope:
1. Activation of scaffold modules (`color`, `wasm`, renderer/scene/client).
2. Backend feature expansion unrelated to ownership boundaries.


## Target invariants (must hold)

1. No `src/vklite/*.c` file includes any `src/vk/_*.h` header.
2. No public header exposes private vk struct layout or mutable internal pointers.
3. `vklite` can initialize and run via public vk API only.
4. Destroying owner objects tears down dependents deterministically without double-free/borrow ambiguity.
5. Tests cover invalid index/handle paths and lifecycle edge cases.


## Step-by-step execution plan

## Step 0 - Baseline inventory and freeze

Goal:
1. Produce exact list of current leaks and ownership ambiguities before edits.

Actions:
1. Inventory all includes of `"_*.h"` from `src/vklite/`.
2. Inventory public structs in `include/datoviz/vk/` and `include/datoviz/vklite/` that expose mutable pointers or internal state.
3. Inventory create/destroy APIs and map owner object for each runtime type.
4. Record the inventory in this document under a short "Current findings" block.

Exit criteria:
1. Leak list and ownership map are explicit and actionable.


## Step 1 - Define public boundary contract

Goal:
1. Lock what `vklite` is allowed to read from `vk`.

Actions:
1. Define minimal public accessors needed by `vklite` (examples: instance handle, physical-device handle, queue family index/caps, swapchain support summary).
2. Ensure each accessor returns value snapshots or opaque handles, not private structs.
3. Document contract in header comments where ambiguity exists.

Exit criteria:
1. A complete accessor list exists before implementation.


## Step 2 - Add missing public vk accessors

Goal:
1. Expose required data through stable public APIs.

Actions:
1. Add APIs in `include/datoviz/vk/*.h` for any data still fetched via private structs.
2. Implement in `src/vk/*.c` using private internals.
3. Keep APIs narrow: return immutable info or handles only.
4. Add argument validation and consistent error logging.

Exit criteria:
1. Every `vklite` need has a public vk accessor.


## Step 3 - Remove vk private-header usage from vklite

Goal:
1. `vklite` compiles without importing vk internals.

Actions:
1. Replace `#include "_*.h"` in `src/vklite/` with `#include "datoviz/vk/*.h"` equivalents.
2. Refactor call sites to use new public accessors.
3. Delete any vklite-side logic that assumes private vk struct layout.

Exit criteria:
1. `rg -n '#include "_.*\.h"' src/vklite` returns no vk private-header dependencies.


## Step 4 - Harden public structs and configs

Goal:
1. Remove ownership-ambiguous fields from public structs.

Actions:
1. Audit public structs for direct internal pointers and mutable state that should be private.
2. Replace risky fields with handles/indexes/config values.
3. Keep struct updates minimal and explicit (no silent behavior changes).
4. Update constructors/init functions to derive internals from stable inputs.

Exit criteria:
1. Public structs no longer encode private ownership details.


## Step 5 - Unify lifecycle semantics

Goal:
1. Ensure one owner and one destroy path per runtime object.

Actions:
1. For each object type, define owner and allowed borrowed references.
2. Remove "sometimes-owned" behavior branches.
3. Make destroy functions idempotent where appropriate and safe for partial init failure unwind.
4. Verify bootstrap/manual flows both follow the same ownership contract.

Exit criteria:
1. Ownership matrix is explicit and respected by code.


## Step 6 - Aggressive legacy removal

Goal:
1. Eliminate dead transitional surfaces since backward compatibility is not required.

Actions:
1. Remove deprecated wrappers and compatibility entry points replaced by new APIs.
2. Delete unused symbols and stale comments that imply old ownership behavior.
3. Update tests and tools to use only current API surface.

Exit criteria:
1. No legacy boundary shims remain for vk/vklite ownership.


## Step 7 - Test expansion for boundary safety

Goal:
1. Lock behavior with focused regression coverage.

Actions:
1. Add tests for invalid handles/indexes in new accessors.
2. Add lifecycle tests for create failure unwind and double-destroy safety.
3. Add tests proving vklite paths work with public vk APIs only.
4. Keep `vk`, `vklite`, and `canvas` suites green.

Exit criteria:
1. New edge cases are covered and reproducible.


## Step 8 - Validation gate

Goal:
1. Enforce boundary and ownership invariants before completion.

Checks:
1. `just build` succeeds.
2. `just test vk` succeeds.
3. `just test vklite` succeeds.
4. `just test canvas` succeeds.
5. `rg -n '#include "_.*\.h"' src/vklite` shows no vk-private leakage.
6. `rg -n 'DvzGpu\*' include/datoviz` remains clean (except intentional internal-only mentions if any).

Exit criteria:
1. All checks pass on the same revision.


## Step 9 - Completion update

Goal:
1. Mark closure with explicit evidence.

Actions:
1. Update `agents/OWNERSHIP.md` with final boundary status for non-GPU slice.
2. Set this file status to `COMPLETED` only when Step 8 is fully green.
3. Record exact commands/tests run and date.

Exit criteria:
1. Docs and code agree on completed ownership state.


## Current findings (to be filled during Step 0)

1. Step 0 snapshot date: `2026-02-18`.
2. Include leak inventory (`src/vklite/*.c`):
   - `rg -n '#include "_.*\.h"' src/vklite` shows only shared `src/common` internals
     (`_alloc.h`, `_assertions.h`, `_compat.h`, `_log.h`).
   - No `src/vk/_*.h` include is present in `src/vklite/*.c`; invariant #1 is currently satisfied.
3. Public struct exposure inventory (ownership-sensitive):
   - `include/datoviz/vk/bootstrap.h`: `DvzBootstrap` publicly exposes mutable ownership bits
     (`owns_instance`, `owns_device`) and owned/borrowed resource fields (`DvzInstance*`, `DvzDevice*`,
     inline `DvzVma allocator`), creating policy ambiguity in public ABI.
   - `include/datoviz/vk/memory.h`: `DvzVma` and `DvzAllocation` expose allocator internals and mutable
     mapping state (`VmaAllocator`, `VmaAllocation`, `VmaAllocationInfo`, `void* mmap`).
   - `include/datoviz/vklite/surface.h`: `DvzSurface` exposes mutable cache pointers
     (`VkSurfaceFormatKHR* formats`, `VkPresentModeKHR* present_modes`) and raw capability state.
   - `include/datoviz/vklite/swapchain.h`: `DvzSwapchain` exposes mutable ownership-adjacent internals
     (`DvzSurface* surface`, `VkDevice device`, `VkSwapchainKHR handle`, `VkImage* images`,
     `VkImageView* image_views`).
   - Multiple vklite resource structs publicly expose borrowed owner pointers (`DvzDevice*`,
     `DvzVma*`, `DvzImages*`, `DvzBuffer*`) and raw Vulkan handles, notably in
     `buffers.h`, `images.h`, `commands.h`, `slots.h`, `descriptors.h`, `graphics.h`,
     `compute.h`, `sampler.h`, `sync.h`, and `shader.h`.
4. Create/destroy + owner map (public API view):
   - `DvzInstance`: created by `dvz_instance_create()`, destroyed by `dvz_instance_destroy()`;
     owner is caller or `DvzBootstrap` when bootstrap owns instance.
   - `DvzDevice`: created by `dvz_device_create()`, destroyed by `dvz_device_destroy()`;
     owner is caller or `DvzBootstrap` when bootstrap owns device.
   - `DvzVma`: initialized by `dvz_device_allocator()`, destroyed by `dvz_allocator_destroy()`;
     owner is caller container (`DvzBootstrap` or module object).
   - `DvzSurface`: initialized via `dvz_surface_init_*()` and wrapped by `dvz_surface_wrap_native()`,
     destroyed by `dvz_surface_destroy()` (cache teardown only; wrapped native `VkSurfaceKHR` is external).
   - `DvzSwapchain`: initialized/configured by `dvz_swapchain_init_from_device()` + `dvz_swapchain_device()`,
     lifecycle via `dvz_swapchain_recreate()/acquire()/present()`, destroyed by `dvz_swapchain_destroy()`;
     borrows `DvzSurface`, owns swapchain/image-view allocations it creates.
   - `DvzBuffer` / `DvzImages` / `DvzImageViews` / `DvzSampler` / `DvzSlots` / `DvzGraphics` /
     `DvzCompute` / `DvzCommands` / `DvzFence` / `DvzSemaphore`:
     init + `*_create()` (where applicable) + `*_destroy()` pattern; each borrows `DvzDevice`
     (and allocator/parent object where relevant) and owns its Vulkan object handles.
5. Immediate Step 1 inputs (accessor contract focus):
   - Boundary leakage is currently more about public mutable struct layout than private-header includes.
   - Priority hardening targets are `DvzBootstrap`, `DvzSurface`, `DvzSwapchain`, and memory structs
     (`DvzVma`, `DvzAllocation`) before broader vklite struct privatization.
6. Step 1 boundary contract (implemented subset):
   - `vklite` runtime code must treat `DvzBootstrap` as API-managed state, using bootstrap accessors instead
     of direct field mutation for instance/device/allocator ownership transitions.
   - Queue-family extraction in cross-module configuration flows should prefer queue accessors
     (`dvz_queue_family()`) over direct struct-field reads.
7. Step 2 accessor additions completed in this slice:
   - Added public APIs in `include/datoviz/vk/bootstrap.h` and implementations in `src/vk/bootstrap.c`:
     - `dvz_bootstrap_set_instance(...)`
     - `dvz_bootstrap_set_device(...)`
     - `dvz_bootstrap_create_allocator(...)`
   - Migrated `src/vklite/proto.c` to consume only bootstrap accessors for instance/device/allocator paths.
   - Migrated `src/vklite/tests/*.c` bootstrap call sites away from direct `bootstrap.{instance,device,
     allocator,owns_*}` field access; tests now use bootstrap accessors and setter APIs.
   - Demoted `proto` from installed public header to internal helper:
     moved header to `src/vklite/_proto.h`, switched internal/test includes, and removed
     `include/datoviz/vklite/proto.h`.
8. Validation run on `2026-02-18` after this slice:
   - `just build` (pass)
   - `just test vk` (pass, `48/48`)
   - `just test vklite` (pass, `25/25`)
   - `just test canvas` (pass, `26/26`)
   - `rg -n '#include "_.*\.h"' src/vklite` reviewed: shared `src/common` internals only, no vk-private
     include leakage observed.
   - `rg -n 'DvzGpu\*' include/datoviz` clean.
9. Step 4 hardening slice on `2026-02-18`:
   - Added `DvzSurface` accessors in `include/datoviz/vklite/surface.h` and `src/vklite/surface.c` for
     readiness/handle/capabilities, format/present-mode counts and indexed fetch, and preferred defaults.
   - Added `DvzSwapchain` state/config getters in `include/datoviz/vklite/swapchain.h` and
     `src/vklite/swapchain.c` for readiness, handle, image count, resolved format/space/mode, and config.
   - Migrated `src/vklite/swapchain.c` internal resolution logic from direct `surface->...` field access to
     `dvz_surface_*` accessors.
   - Migrated `src/vklite/tests/test_present.c` assertions/config reads away from direct mutable
     `surface`/`swapchain` field reads where public getters are now available.
10. Step 4 opaque-handle follow-up on `2026-02-18`:
   - Made `DvzSurface` and `DvzSwapchain` opaque in public headers
     (`include/datoviz/vklite/surface.h`, `include/datoviz/vklite/swapchain.h`):
     removed public struct layout exposure and added explicit `create/free` lifecycle entry points.
   - Moved concrete layouts to internal headers `src/vklite/_surface.h` and `src/vklite/_swapchain.h`.
   - Added missing accessors needed by downstream modules:
     - `dvz_surface_extent()`
     - `dvz_swapchain_extent()`
     - `dvz_swapchain_image()`
     - `dvz_swapchain_image_view()`
   - Migrated canvas present backend (`src/canvas/swapchain_sink.c`) and vklite present tests
     (`src/vklite/tests/test_present.c`) to pointer-owned wrappers + accessor-only data reads.
   - Kept `dvz_surface_destroy()` / `dvz_swapchain_destroy()` as reusable teardown operations and introduced
     `dvz_surface_free()` / `dvz_swapchain_free()` for allocation ownership release.
11. Validation run on `2026-02-18` after Step 4 slice:
   - `just build` (pass)
   - `just test vklite` (pass, `25/25`)
   - `just test vk` (pass, `48/48`)
   - `just test canvas` (pass, `26/26`)


## Definition of done

1. `vklite` has no dependency on vk private headers/types.
2. Public vk/vklite APIs encode ownership clearly and avoid internal pointer exposure.
3. Lifecycle behavior is deterministic across bootstrap/manual paths.
4. Legacy ownership compatibility layers are removed.
5. Build and targeted test gates are fully green.
