# App Resource Injection Plan

> **Execution Status**
> - **Status:** `SOON`
> - **Updated on:** `2026-05-19`
> - **Purpose:** define the public app API needed to let callers provide GPU, runtime, and window
>   host resources instead of forcing `dvz_app()` to allocate everything.

This plan is about app ownership and public API shape. The test-runner scheduling and shared
fixture plan lives in [TEST_RUNNER_SCHEDULING.md](TEST_RUNNER_SCHEDULING.md) and should depend on
this API rather than owning its design.


## Motivation

`dvz_app(scene)` is convenient, but it currently creates a full stack every time:

1. `DvzWindowHost`,
2. `DvzGpuCtx`,
3. `DvzDrp2Runtime`,
4. per-window `DvzWindow` and `DvzCanvas` objects.

That is correct for simple applications, but too rigid for hosted integrations, notebooks,
benchmark harnesses, and test fixtures that already own a Vulkan context or want to amortize GPU
setup cost across multiple app instances.

The goal is to keep `dvz_app(scene)` as the high-level default while adding a public constructor
that can borrow caller-provided resources.


## Proposed Public API

Add a resource bundle to `include/datoviz/app.h`:

```c
typedef struct DvzAppResources
{
    DvzGpuCtx* gpu_ctx;
    DvzDrp2Runtime* runtime;
    DvzWindowHost* window_host;
} DvzAppResources;
```

Add a constructor:

```c
DVZ_EXPORT DvzApp* dvz_app_with_resources(
    DvzScene* scene, const DvzAppConfig* config, const DvzAppResources* resources);
```

Rules:

1. `resources == NULL` means the app creates and owns every top-level resource, matching
   `dvz_app_with_config(scene, config)`.
2. A `NULL` field means the app creates and owns that specific resource.
3. A non-`NULL` field is borrowed and must outlive the app.
4. The app destroys only resources it owns.
5. Borrowed resources are exclusive to the app for the duration of the app unless the specific
   resource contract says otherwise.

`dvz_app(scene)` should remain a thin default wrapper. `dvz_app_with_config()` can also become a
wrapper around `dvz_app_with_resources(scene, config, NULL)`.


## Ownership Changes

Add explicit ownership flags to `DvzApp`:

```c
bool owns_gpu_ctx;
bool owns_runtime;
bool owns_window_host;
```

`dvz_app_destroy()` must use these flags:

1. destroy app windows and canvases first,
2. destroy the runtime only when `owns_runtime` is true,
3. destroy the GPU context only when `owns_gpu_ctx` is true,
4. destroy the window host only when `owns_window_host` is true.

Borrowed resources should be nulled in the app during destroy after dependent objects have been
destroyed, but not destroyed by the app.


## Compatibility Checks

When callers provide resources, construction must validate compatibility before creating windows or
canvases:

1. If both `gpu_ctx` and `runtime` are provided, the runtime must be compatible with the borrowed
   GPU context. If there is no runtime/device accessor yet, add a narrow checkable path or document
   that the caller is responsible until such accessors exist.
2. If `runtime` is provided without `gpu_ctx`, app window creation still needs a device for
   canvases. Either require `gpu_ctx` whenever `runtime` is provided, or derive the required device
   and allocator from a runtime config only if that is reliable.
3. A provided `window_host` must already have required backends registered. The app must not assume
   ownership of backend process state.
4. `DvzAppConfig` instance-extension fields only affect app-created GPU contexts. They should be
   ignored, or explicitly rejected, when a borrowed `gpu_ctx` is provided.

The conservative first implementation should require `gpu_ctx != NULL` when `runtime != NULL`.


## Runtime Sharing Policy

The public API may allow a borrowed `DvzDrp2Runtime`, but the contract should be strict:

1. The runtime is mutable and not thread-safe.
2. It must not be used concurrently by multiple apps, tests, or caller code.
3. It must not be reset or destroyed while the app exists.
4. It must be compatible with the app's GPU context and canvas frame targets.

For test performance, prefer sharing `DvzGpuCtx` per worker while keeping `DvzDrp2Runtime` per app
until runtime reset and object-table reuse have explicit regression coverage.


## Test And Fixture Use

The intended test-runner use is:

```c
DvzAppResources resources = {
    .gpu_ctx = worker->gpu_ctx,
    .runtime = NULL,
    .window_host = worker->window_host,
};

DvzApp* app = dvz_app_with_resources(scene, NULL, &resources);
```

This amortizes Vulkan instance/device/allocator setup while preserving fresh per-case app runtime,
window, canvas, scene, visual, request, and capture state.

Do not share one `DvzApp` across unrelated tests. `DvzApp` binds to a borrowed scene, registers
scene callbacks, owns a growing fixed window array, and owns mutable runtime/canvas state.


## Implementation Steps

1. Add `DvzAppResources` and `dvz_app_with_resources()` to `include/datoviz/app.h`.
2. Add ownership flags to the internal `DvzApp` struct.
3. Refactor `dvz_app_with_config()` so resource creation and ownership assignment are explicit.
4. Add borrowed-resource construction paths and compatibility checks.
5. Update `dvz_app_destroy()` to respect ownership flags.
6. Add focused tests for all ownership combinations that are initially supported:
   - app owns all resources,
   - borrowed `DvzGpuCtx`,
   - borrowed `DvzGpuCtx` plus borrowed `DvzWindowHost`,
   - rejected incompatible or incomplete resource bundles.
7. Add app documentation comments with explicit lifetime rules.


## Guardrails

1. Do not make the app destroy borrowed resources.
2. Do not share a borrowed runtime concurrently.
3. Do not relax scene lifetime: the scene remains borrowed and must outlive the app.
4. Do not create a second app presentation path; this constructor must reuse existing
   scene -> DRP2 -> vklite/canvas execution.
5. Keep the default `dvz_app(scene)` path simple and unchanged for normal users.


## Validation

For the API implementation:

1. run `git diff --check`,
2. run `just build`,
3. run focused app tests,
4. run focused canvas/DRP2 smoke tests if ownership changes touch frame targets or runtime setup,
5. run one app-offscreen benchmark before and after test fixture migration to verify the API enables
   real setup-time reduction.
