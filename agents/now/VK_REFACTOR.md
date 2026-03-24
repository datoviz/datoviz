> **Implementation Status**
> - **Status:** `SUBSTANTIALLY COMPLETED`
> - **Verified on:** `2026-03-24`
> - **Codebase alignment:** Public GPU ownership/API cleanup is complete (`DvzGpu*` removed from
>   public headers, index/descriptor flow active). `vklite` no longer includes vk-private headers;
>   `proto` is removed from the active tree; `surface`/`swapchain` are accessor-driven; several
>   `vklite` wrappers are now opaque; the broad boundary reshape is effectively complete for the
>   active low-level surface.
> - **Current progress:** Steps 0-6 are effectively complete for the active `vklite` surface.
>   The remaining work is no longer another ownership-boundary migration; it is focused lifecycle
>   hardening, allocator-interop robustness, and keeping narrow validation loops current.
> - **Current closure note:** `DvzBarriers` is intentionally still public because it serves as a
>   mutable command-recording builder rather than an owned wrapper. The remaining clearly sensitive
>   public exposure is now mostly on the `vk` allocator side, especially the intentional advanced
>   interop surface in `vk/memory_interop.h`.
> - **Technical-debt note:** The most direct public VMA include leak has now been removed from
>   `vk/memory.h`: `DvzAllocationFlags` is Datoviz-owned and VMA stays internal to `src/vk`.
>   Remaining allocator debt is narrower: advanced allocator/interop semantics now live behind the
>   opt-in `vk/memory_interop.h` header, while the core allocation path stays in `vk/memory.h`.
>   `vklite` and canvas still intentionally expose allocator-oriented object references such as
>   `DvzVma*`. Import/export declarations remain intentionally public there even when some paths are
>   lightly used today, because later interop hardening should build on that public surface rather
>   than reintroducing private vk helpers.
> - **Remaining gap:** Non-GPU ownership boundaries between `vk` and `vklite` are now mostly closed
>   for the active `vklite` wrapper surface. The public headers now describe the intended wrapper
>   lifecycle more explicitly. The remaining work is mainly:
>   1. dedicated `vk/memory_interop.h` export/import coverage,
>   2. focused failure-path testing around submit/recreate/partial-init unwind,
>   3. keeping the focused runners and docs aligned with the active low-level contract.

## Immediate next steps (2026-03-24)

1. Treat the `vklite` wrapper-opacity and vk-private-header removal passes as complete for the
   active surface.
2. Keep `DvzBarriers` public and documented as an intentional builder/config type for command
   recording rather than trying to force opacity there.
3. Treat `vk/memory_interop.h` as the remaining intentional advanced escape hatch and harden it
   with dedicated export/import tests rather than more public-API reshaping.
4. Keep the queue model public as an intentional low-level planning API; do not reopen queue
   encapsulation unless a concrete correctness problem appears.
5. Keep focused lifecycle tests current around repeated submit, destroy-before-record, update/restart
   failure, recreate/reset semantics, and partial-init unwind.
6. Keep the focused runners (`dvztest_vk`, `dvztest_canvas`, unified `dvztest`) aligned with the
   active low-level suite so new lifecycle regressions are not accidentally omitted from the narrow
   validation loops.

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
4. Verify manual/create-wrapper flows and fixture-driven flows both follow the same ownership
   contract.

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
1. Update `agents/done/OWNERSHIP.md` with final boundary status for non-GPU slice.
2. Set this file status to `COMPLETED` only when Step 8 is fully green.
3. Record exact commands/tests run and date.

Exit criteria:
1. Docs and code agree on completed ownership state.


## Current findings (refreshed `2026-03-24`)

1. Boundary status:
   - `src/vklite/*.c` no longer depends on `src/vk/_*.h`.
   - `rg -n '#include "_.*\.h"' src/vklite` now shows only shared `src/common` or
     `src/vklite`-private helpers, not vk-private leakage.
   - `rg -n 'DvzGpu\*' include/datoviz` remains clean.
2. Public-API exposure still intentional on the active branch:
   - `include/datoviz/vklite/sync.h`: `DvzBarriers` remains public by design as a mutable
     barrier-builder/config type.
   - `include/datoviz/vk/queues.h`: `DvzQueueCaps`, `DvzQueue`, and `DvzQueues` remain public as
     an intentional low-level queue-planning/snapshot API.
   - `include/datoviz/vk/memory_interop.h`: remains the explicit advanced interop surface for
     external-memory import/export and raw memory-handle plumbing.
3. Runtime owner/lifecycle shape on the active surface:
   - `DvzInstance`: caller-owned via `dvz_instance_create()` / `dvz_instance_destroy()`.
   - `DvzDevice`: caller-owned via `dvz_device_create()` / `dvz_device_destroy()`.
   - `DvzVma`: caller/module-owned via `dvz_device_allocator()` / `dvz_allocator_destroy()`.
   - `DvzSurface`: cache-owning wrapper; destroys cached query state but does not own wrapped native
     `VkSurfaceKHR`.
   - `DvzSwapchain`: owns swapchain/image-view resources it creates; borrows `DvzSurface` and
     logical-device state.
   - Opaque `vklite` wrappers (`commands`, `sync` owners, `buffers`, `images`, `sampler`, `slots`,
     `graphics`, `compute`, `shader`, `rendering`) follow the active create/init/destroy/free
     pattern and borrow `DvzDevice` plus any allocator/parent wrapper they depend on.
4. Validation evidence currently on record:
   - `2026-03-23`: `just build` passed; `just test` passed (`146/146`); `just test vk`,
     `just test vklite`, and `just test canvas` all passed on that revision.
   - `2026-03-24`: after the latest lifecycle-hardening slice, `just build` passed, and focused
     regressions passed for:
     - `test_vklite_commands_destroy_without_recording`
     - `test_stream_update_restart_failure_stops_stream`
     - `test_stream_attach_sink_name_prefers_requested_then_auto`
   - `testing/components/dvztest_vk.c` was updated on `2026-03-24` so the focused vk runner now
     includes `test_vklite_commands_destroy_without_recording`; this runner had drifted slightly
     behind the unified `vklite` suite before that fix.
5. Remaining work before this document can reasonably be marked `COMPLETED`:
   - Add dedicated `vk/memory_interop.h` coverage for export/import and failure-path behavior.
   - Keep lifecycle/failure-path regressions expanding around submit failure, recreate/reset, and
     partial-init unwind; the recent `commands_destroy_without_recording` and
     `stream_update_restart_failure_stops_stream` tests are examples of the right kind of coverage.
   - Re-run the full validation gate (`just build`, `just test vk`, `just test vklite`,
     `just test canvas`, and ideally full `just test`) on the same post-hardening revision and
     record the exact outcomes here before declaring the plan complete.


## Definition of done

1. `vklite` has no dependency on vk private headers/types.
2. Public vk/vklite APIs encode ownership clearly and avoid internal pointer exposure.
3. Lifecycle behavior is deterministic across the active wrapper/create-wrapper and fixture-driven
   low-level flows.
4. Legacy ownership compatibility layers are removed.
5. Build and targeted test gates are fully green.
