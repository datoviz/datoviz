# Bootstrap to GPU Context Migration Plan

Status: `COMPLETED`

Owner slice: `vk` / `vklite` startup and test infrastructure cleanup

Date: `2026-03-23`

Verified on: `2026-03-23`

Validation on this revision:

1. `just build`
2. `direnv exec . just test vk`
3. `direnv exec . just test vklite`
4. `direnv exec . just test`

Completion snapshot:

1. `include/datoviz/vk/gpu_ctx.h` and `src/vk/gpu_ctx.c` are now the active bring-up surface
2. all direct `bootstrap` code/users were removed
3. `include/datoviz/vk/bootstrap.h` and `src/vk/bootstrap.c` were deleted
4. migrated callers required explicit feature selection, so the final config surface also includes
   `dvz_gpu_ctx_config_features10()` and `dvz_gpu_ctx_config_features13()`


## Goal

Replace the current `bootstrap` bring-up helper with a cleaner split between:

1. explicit GPU-context configuration
2. owned GPU runtime state

The replacement should preserve the useful parts of `bootstrap` while removing its main design
weaknesses:

1. public mutable struct layout
2. partially initialized lifecycle
3. mixed policy + state responsibilities
4. awkward flag-driven creation flow


## Why this change

Current `bootstrap` is useful, but architecturally only medium-quality.

It currently mixes:

1. instance creation policy
2. GPU selection policy
3. device creation policy
4. allocator creation policy
5. ownership bookkeeping for externally attached objects
6. runtime handle storage

That is acceptable as a convenience helper, but it creates several problems:

1. callers can depend on `struct DvzBootstrap` layout in
   the old `include/datoviz/vk/bootstrap.h` surface
2. the object can exist in several partially configured states
3. the `flags` model hides lifecycle decisions instead of making them explicit
4. the default GPU selection policy is implicit and hardcoded
5. the abstraction is not clean enough to treat as a long-term foundation


## Naming decision

Use `ctx`, not `context`, for brevity.

Target names:

1. `DvzGpuCtx`
2. `DvzGpuCtxConfig`

Target function family:

1. `dvz_gpu_ctx_config(...)`
2. `dvz_gpu_ctx_config_*()`
3. `dvz_gpu_ctx(...)`
4. `dvz_gpu_ctx_*()`

Reasoning:

1. `ctx` is short but still standard and understandable
2. the names stay materially shorter than `gpu_context`
3. the prefix remains explicit enough to avoid ambiguity


## Target ownership split

### `DvzGpuCtxConfig`

Responsibilities:

1. express instance validation policy
2. express GPU selection policy
3. express allocator creation/export policy

Non-responsibilities:

1. no Vulkan handle ownership
2. no runtime state
3. no external attach bookkeeping


### `DvzGpuCtx`

Responsibilities:

1. own the created instance
2. own the selected GPU index / GPU descriptor access
3. own the created device
4. own the created allocator
5. expose narrow read-only accessors for instance/device/allocator/queue/error count

Non-responsibilities:

1. no broad public struct layout
2. no general-purpose mutable bring-up script object


## File layout

Planned files:

1. `include/datoviz/vk/gpu_ctx.h`
2. `src/vk/gpu_ctx.c`

`bootstrap.{h,c}` should remain temporarily during migration, then be deleted entirely once all
consumers have moved.


## API direction

Keep the first `gpu ctx` surface deliberately small.

Expected first public surface:

1. config constructor:
   - `dvz_gpu_ctx_config()`
2. config setters:
   - `dvz_gpu_ctx_config_validation()`
   - `dvz_gpu_ctx_config_gpu()`
   - `dvz_gpu_ctx_config_alloc()`
3. context constructor / destroy:
   - `dvz_gpu_ctx()`
   - `dvz_gpu_ctx_destroy()`
4. narrow getters:
   - `dvz_gpu_ctx_instance()`
   - `dvz_gpu_ctx_gpu_index()`
   - `dvz_gpu_ctx_gpu_info()`
   - `dvz_gpu_ctx_device()`
   - `dvz_gpu_ctx_alloc()`
   - `dvz_gpu_ctx_queue()`
   - `dvz_gpu_ctx_error_count()`

Avoid in the first pass:

1. public `struct DvzGpuCtx`
2. attach/wrap overloads in the same constructor
3. recreating `bootstrap` as another partially initialized mutable object
4. speculative support for every old edge case before a real caller needs it


## External wrapping policy

External attach/wrap cases should not distort the main owned-path API.

Recommended order:

1. implement the normal owned path first
2. migrate normal test/helper consumers first
3. add explicit wrap constructors only if required by real remaining callers

If wrapping is later needed, prefer explicit APIs such as:

1. `dvz_gpu_ctx_wrap_instance(...)`
2. `dvz_gpu_ctx_wrap_device(...)`
3. `dvz_gpu_ctx_create_device(...)`
4. `dvz_gpu_ctx_create_alloc(...)`

Do not overload `dvz_gpu_ctx(...)` into a catch-all lifecycle state machine.


## Migration order

1. Add `gpu_ctx.h`
2. Add `gpu_ctx.c`
3. Implement only the normal owned path
4. Migrate a small `vk` test first:
   [test_memory.c](/home/cyrille/GIT/Viz/datoviz/src/vk/tests/test_memory.c)
5. Migrate a small `vklite` test next:
   [test_buffers.c](/home/cyrille/GIT/Viz/datoviz/src/vklite/tests/test_buffers.c)
6. Migrate the new fixture layer:
   [fixture_gpu.c](/home/cyrille/GIT/Viz/datoviz/src/vklite/tests/fixture_gpu.c)
7. Migrate remaining direct `bootstrap` users
8. Add explicit wrap helpers only if needed for remaining callers
9. Remove all remaining `bootstrap` users
10. Delete `include/datoviz/vk/bootstrap.h`
11. Delete `src/vk/bootstrap.c`
12. Run full validation and update active refactor notes


## Completion criteria

This migration is complete only when all of the following hold on the same revision:

1. there is no `DvzBootstrap` symbol left in the source tree
2. there is no `dvz_bootstrap*` symbol left in the source tree
3. `include/datoviz/vk/bootstrap.h` is deleted
4. `src/vk/bootstrap.c` is deleted
5. `include/datoviz/vk/gpu_ctx.h` is the sole public bring-up API
6. `just build` passes
7. `just test vk` passes
8. `just test vklite` passes
9. `just test` passes


## Constraints

1. keep the first `gpu ctx` API minimal
2. do not preserve `bootstrap` quirks unless a real caller needs them
3. prefer explicit constructors over mutable attach-then-create flows
4. keep `DvzGpuCtx` opaque from the start
5. use standard short names: `ctx`, `alloc`, `queue`, `device`


## Notes for the first implementation pass

1. start with the normal owned path only
2. use real migrated call sites to decide which getters are actually necessary
3. move the fixture layer early because it will remove many downstream `bootstrap` dependencies
4. only design wrap/attach APIs after the owned path is proven and migrated
