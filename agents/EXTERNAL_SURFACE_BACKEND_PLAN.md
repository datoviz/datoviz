# Phase 2 Plan: External-Surface Window Backend (Qt/PyQt First Consumer)

## Goal

Implement a backend-agnostic "external native surface" path so Datoviz can present through host-managed
windows/surfaces (PyQt/PySide, SDL, wx, native app shells) without adding Qt as a Datoviz build dependency.


## Scope

In scope:

1. `DVZ_BACKEND_WRAP` implementation in `src/window/`.
2. Public API to attach/update/detach externally-created Vulkan surfaces.
3. Backend-neutral Vulkan instance-extension query API.
4. Runtime and test wiring for external-surface present mode.
5. Python-facing compatibility requirements for ctypes users.

Out of scope:

1. Any direct Qt C++ integration or linking against Qt in Datoviz.
2. High-level Python convenience wrappers/UI widgets.
3. Wayland/X11/Win32-specific helpers in Datoviz core.


## Current Constraints (from codebase)

1. `DVZ_BACKEND_WRAP` exists but is unused (`include/datoviz/runner/enums.h`).
2. Window backend plugin model is ready (`include/datoviz/window/backend.h`, `src/window/window_host.c`).
3. Built-in backend registration currently includes only headless + GLFW (`src/window/window_host.c`).
4. Vulkan extension discovery is GLFW-centric in runtime/tests (`testing/dvz_live_canvas.c`,
   `src/vklite/tests/test_present.c`).
5. Present-mode canvas works against `DvzWindowSurface` + `VkSurfaceKHR`; no GLFW-only dependency in core
   swapchain/canvas path.


## Target Architecture

1. Host app (Python/Qt in Phase 2 consumer) owns:
   - Native window lifecycle.
   - Vulkan `VkSurfaceKHR` creation/destruction.
   - Required WSI instance extension list.
   - Event loop polling.
2. Datoviz owns:
   - Logical `DvzWindow`.
   - Input router and canvas/swapchain lifecycle.
   - Present rendering and stream/sink plumbing.
3. Contract boundary:
   - Host injects extension list before Vulkan instance creation.
   - Host injects/updates `VkSurfaceKHR` + size/scale on create/resize/recreate.
   - Datoviz never destroys externally-owned surfaces unless explicitly requested.


## Merge-Ready API Spec (Final)

This section is normative for implementation. Do not rename symbols or alter signatures.

### New Public Types

File: `include/datoviz/window/backend.h`

```c
typedef struct DvzWindowExternalSurfaceInfo DvzWindowExternalSurfaceInfo;

struct DvzWindowExternalSurfaceInfo
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    float scale_x;
    float scale_y;
    bool owned_by_datoviz;
};
```

### New Public Functions

File: `include/datoviz/window/backend.h`

```c
DVZ_EXPORT int dvz_window_wrap_attach_surface(
    DvzWindow* window, const DvzWindowExternalSurfaceInfo* info);

DVZ_EXPORT int dvz_window_wrap_update_surface(
    DvzWindow* window, const DvzWindowExternalSurfaceInfo* info);

DVZ_EXPORT void dvz_window_wrap_detach_surface(DvzWindow* window);

DVZ_EXPORT int dvz_window_wrap_set_required_extensions(
    DvzWindowHost* host, uint32_t count, const char* const* extensions);

DVZ_EXPORT uint32_t dvz_window_host_required_extension_count(
    DvzWindowHost* host, DvzBackend backend);

DVZ_EXPORT int dvz_window_host_required_extensions(
    DvzWindowHost* host, DvzBackend backend, uint32_t capacity, const char** out_extensions);

DVZ_EXPORT void dvz_window_register_wrap_backend(DvzWindowHost* host);
```

### New Backend Proc Hooks

File: `include/datoviz/window/backend.h`

```c
typedef uint32_t (*DvzWindowBackendRequiredExtensionCount)(
    DvzWindowBackend* backend, DvzWindowHost* host);

typedef const char* (*DvzWindowBackendRequiredExtensionAt)(
    DvzWindowBackend* backend, DvzWindowHost* host, uint32_t index);
```

`DvzWindowBackendProcs` must include:

```c
DvzWindowBackendRequiredExtensionCount required_extension_count;
DvzWindowBackendRequiredExtensionAt required_extension_at;
```

### Return-Code Contract (Locked)

1. `dvz_window_wrap_attach_surface(...)`:
   - `0` success
   - `-1` invalid args, wrong backend, or invalid handle tuple.
2. `dvz_window_wrap_update_surface(...)`:
   - `0` success
   - `-1` invalid args, wrong backend, or update rejected.
3. `dvz_window_wrap_set_required_extensions(...)`:
   - `0` success
   - `-1` invalid args or allocation failure.
4. `dvz_window_host_required_extensions(...)`:
   - non-negative count written on success (including `0`)
   - `-1` invalid args/backend unavailable.
5. `dvz_window_host_required_extension_count(...)`:
   - returns `0` on unavailable backend or no extensions.

### Ownership Contract (Locked)

1. Default for wrap path: host owns `VkSurfaceKHR` (`owned_by_datoviz = false`).
2. Datoviz destroys surface only when `owned_by_datoviz = true`.
3. `detach` clears `instance/surface` to null in `DvzWindowSurface` and does not destroy when host-owned.
4. `update` with null `surface` is valid and represents temporary surface loss.


## Public API Additions

## 1) External Surface Attachment API

File: `include/datoviz/window/backend.h`

Add:

```c
typedef struct DvzWindowExternalSurfaceInfo DvzWindowExternalSurfaceInfo;

struct DvzWindowExternalSurfaceInfo
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    float scale_x;
    float scale_y;
    bool owned_by_datoviz;
};

DVZ_EXPORT int dvz_window_wrap_attach_surface(
    DvzWindow* window, const DvzWindowExternalSurfaceInfo* info);

DVZ_EXPORT int dvz_window_wrap_update_surface(
    DvzWindow* window, const DvzWindowExternalSurfaceInfo* info);

DVZ_EXPORT void dvz_window_wrap_detach_surface(DvzWindow* window);
```

Semantics:

1. `attach`: initial external-surface binding.
2. `update`: replace/update surface handle/extent/scale (resize/recreate path).
3. `detach`: clear surface handles and mark surface unavailable.


## 2) Required Vulkan Instance Extension API

File: `include/datoviz/window/backend.h`

Add backend proc hooks:

```c
typedef uint32_t (*DvzWindowBackendRequiredExtensionCount)(
    DvzWindowBackend* backend, DvzWindowHost* host);
typedef const char* (*DvzWindowBackendRequiredExtensionAt)(
    DvzWindowBackend* backend, DvzWindowHost* host, uint32_t index);
```

Extend `DvzWindowBackendProcs`:

```c
DvzWindowBackendRequiredExtensionCount required_extension_count;
DvzWindowBackendRequiredExtensionAt required_extension_at;
```

Add host API:

```c
DVZ_EXPORT uint32_t dvz_window_host_required_extension_count(
    DvzWindowHost* host, DvzBackend backend);

DVZ_EXPORT int dvz_window_host_required_extensions(
    DvzWindowHost* host, DvzBackend backend, uint32_t capacity, const char** out_extensions);
```

Return contract:

1. `count`: number of extensions required by backend.
2. `extensions`: returns number written, `-1` on invalid input/backend unavailable.


## 3) Wrap Backend Configuration API (host-level extension injection)

File: `include/datoviz/window/backend.h`

Add:

```c
DVZ_EXPORT int dvz_window_wrap_set_required_extensions(
    DvzWindowHost* host, uint32_t count, const char* const* extensions);
```

Semantics:

1. Copy input strings into host-owned backend state.
2. Used by Python host before `dvz_instance_create_from_config()`.


## Internal Implementation Plan (file-level)

## A) Window module

1. Create `src/window/backend_wrap.c`.
2. Implement `dvz_window_register_wrap_backend(DvzWindowHost* host)` (new symbol).
3. Register wrap backend in `_window_register_builtins()` in `src/window/window_host.c`.
4. Add surface-ownership tracking in `DvzWindow`:
   - `bool backend_owns_surface`.
5. Update destroy path to respect ownership:
   - External surfaces are not destroyed by Datoviz when ownership is false.


## B) Backend proc extension-query plumbing

1. Extend `DvzWindowBackendProcs` in public header and backend initializers.
2. Implement host query APIs in `src/window/window_host.c`.
3. GLFW backend:
   - Implement query callbacks using `glfwGetRequiredInstanceExtensions`.
4. Headless backend:
   - Return zero required extensions.
5. Wrap backend:
   - Return host-injected extension list.


## C) Runtime tools/tests migration away from GLFW-only extension queries

1. `testing/dvz_live_canvas.c`:
   - Replace direct `glfwGetRequiredInstanceExtensions` logic with
     `dvz_window_host_required_extensions(...)`.
2. `src/vklite/tests/test_present.c`:
   - Same migration.
3. Keep GLFW-specific fallback paths only where strictly required for disabled-backend diagnostics.


## D) CMake wiring

1. No new Qt options/dependencies.
2. Ensure wrap backend source is compiled in `src/window/CMakeLists.txt` glob result.
3. Fix legacy compile-definition wiring in `src/window/CMakeLists.txt`:
   - Use `${DVZ_COMPILE_DEFINITIONS}` instead of `${COMPILE_DEFINITIONS}`.


## E) Python/ctypes compatibility requirements

1. Ensure newly added symbols are visible in public headers consumed by ctypes generation.
2. Regenerate ctypes wrappers (existing project tooling) after C API merge.
3. Validate from Python:
   - set required extensions
   - create wrap window
   - attach/update surface
   - drive `dvz_canvas_frame/submit` in present mode


## Data and Lifetime Rules

1. External surface lifecycle:
   - Host creates and destroys `VkSurfaceKHR` by default.
   - Datoviz only borrows handle when `owned_by_datoviz == false`.
2. Swapchain recreation trigger:
   - On `dvz_window_wrap_update_surface`, emit resize and mark handles dirty via existing path.
3. Null-handle transitions:
   - `update` with `VK_NULL_HANDLE` must behave like temporary surface loss.
4. Threading:
   - APIs are called on render/UI thread; no additional internal locking introduced in Phase 2.


## Tests to Add

Files:

1. `src/window/tests/test_window.c`
2. `src/vklite/tests/test_present.c` (extend)
3. Optional new: `src/window/tests/test_window_wrap.c` if preferred split

Test cases:

1. `test_window_wrap_create`:
   - Create host + wrap window and verify backend type.
2. `test_window_wrap_attach_detach`:
   - Attach synthetic surface info, verify `dvz_window_surface` fields, detach and verify cleared handles.
3. `test_window_required_extensions_headless`:
   - Count is zero for headless.
4. `test_window_required_extensions_wrap`:
   - Set required extensions and query roundtrip.
5. `test_vklite_present_extension_query_backend_neutral`:
   - Present fixture obtains required extensions via host API, not GLFW direct call.


## Acceptance Criteria

1. Datoviz builds and tests without Qt installed.
2. Existing GLFW path remains functional and unchanged from user perspective.
3. `dvz_live_canvas` can obtain required instance extensions via host API.
4. Wrap backend can carry an externally-provided valid surface to canvas present path.
5. External-surface ownership semantics are explicit and leak-safe.
6. No edits under `external/`.


## Suggested Implementation Sequence (small PRs)

1. PR1: Backend proc extension-query API + host query functions + GLFW/headless support.
2. PR2: Wrap backend implementation + surface attach/update/detach API.
3. PR3: Migrate `dvz_live_canvas` and `test_present` to backend-neutral extension query.
4. PR4: Add wrap/backend-neutral tests and fix regressions.
5. PR5: Python ctypes regeneration and smoke script (if desired in same repo phase).


## Risks and Mitigations

1. Risk: stale surface handle after host-side recreate.
   - Mitigation: explicit `update_surface()` contract; tests for replace/detach/reattach.
2. Risk: mismatch between extension list and actual surface creation.
   - Mitigation: host API requires extensions to be set before instance creation; add validation logs.
3. Risk: accidental surface destruction by Datoviz.
   - Mitigation: strict ownership flag default false for wrap; destroy path checks ownership.


## Decisions Locked

1. Keep separate `attach` and `update` APIs.
2. Wrap backend is registered by default as a built-in backend.
3. Wrap required-extension storage is a dynamically-allocated array owned by wrap backend state.
4. New APIs use `0/-1` or count/`-1` return conventions as defined in the locked return-code contract.


## Ready-to-Implement Checklist

1. Headers updated with API + Doxygen comments.
2. New wrap backend source added.
3. Window host built-in registration includes wrap backend.
4. Host extension query API used in runtime/tests.
5. Tests added/updated and passing with:
   - `just build`
   - `just test window`
   - `just test vklite`
6. Manual smoke (future Python consumer):
   - create wrap window
   - attach external surface
   - run present canvas frames
   - resize/update surface
   - detach and clean shutdown
