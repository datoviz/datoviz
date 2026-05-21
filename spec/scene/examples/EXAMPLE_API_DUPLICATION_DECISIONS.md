# Example API Duplication Decisions

> **Execution Status**
> - **Status:** `DECISION NOTE`
> - **Decided on:** `2026-05-21`
> - **Implementation status updated on:** `2026-05-21`
> - **Scope:** current `examples/c` duplication and API pressure points
> - **Purpose:** record which repeated example patterns should become shared helpers or public API,
>   which proposals are intentionally deferred or rejected, and what remains open.


## Context

The current C examples repeat several patterns that make examples harder to maintain and reveal API
pressure points in the scene/app layer. The most visible duplication is local `_frame_count()`
parsing, but the broader scan also found repeated panel construction, controller/input binding,
stringly typed visual uploads, app/window lifecycle boilerplate, and pick/probe request polling.

This note records the accepted direction so future cleanup does not re-open the same questions.


## Current Implementation Status

The core API decisions and current C example migration are complete for the stable visual-family
bundles covered by this note. Future agents should treat the remaining work below as explicit
deferrals rather than re-litigating the decisions.

Done:

1. `examples/c` now has a local helper layer with shared frame-count parsing
   (`example_frame_count()` in `examples/c/example_common.*`).
2. No public app-session ownership wrapper was added; explicit app/scene/window ownership remains
   the public path.
3. `dvz_panel_full()` exists for the common full-panel case.
4. App-window controller helpers exist with final names under the `dvz_app_window_panel_*()` family:
   `dvz_app_window_panel_panzoom()`, `dvz_app_window_panel_arcball()`,
   `dvz_app_window_panel_fly()`, and `dvz_app_window_panel_turntable()`.
5. Typed visual upload helpers exist for the stable repeated bundles:
   `dvz_point_data()`, `dvz_point_selection()`, `dvz_mesh_data()`, and
   `dvz_mesh_instances()`, plus `dvz_pixel_data()`, `dvz_primitive_data()`, and
   `dvz_sphere_data()`.
6. Pick/probe callback dispatch remains intentionally deferred.
7. C examples have been migrated to the landed helpers where the replacement is behavior-preserving.
8. GUI-viewport examples intentionally stay on the lower-level controller/input primitives where
   the router comes from `DvzGuiViewport` rather than `DvzAppWindow`.

Still open:

1. Image should wait until the retained image helper shape is clear because the current image path
   has multiple valid input forms. A narrow quad helper may become worthwhile later, but the public
   shape is intentionally deferred.
2. Decide whether an index-buffer helper such as `dvz_mesh_indices()` is actually worth adding.
   The current scene-buffer path is explicit and still acceptable.
3. Broaden the local example helper layer only for repeated non-public chores that remain noisy
   after the current cleanup, such as common output-path handling. Do not add generic lifecycle or
   cleanup wrappers unless repeated error paths become a real maintenance problem.
4. Revisit pick/probe callback dispatch only after richer picking/probing payload semantics settle.


## Decisions

### 1. Add a local C example helper layer

Accepted; partially implemented.

Create an `examples/c`-local helper layer for non-public conveniences used by many examples. Initial
scope should include:

1. frame-count parsing with the current convention that `0` means interactive run;
2. small numeric parsing helpers such as unsigned `uint32_t` parsing;
3. common path helpers such as output paths relative to the executable or example assets;
4. optional cleanup conventions for examples with local allocations.

This is not a public Datoviz API requirement. It is example infrastructure.

Remaining work: shared frame-count parsing is done. Add more local helpers only when repeated
example code remains both noisy and non-public after the current cleanup.


### 2. Do not add a public app-session wrapper for now

Rejected for now; implemented by omission.

Do not add a public `DvzAppSession` or similar wrapper that owns `DvzScene`, `DvzFigure`, `DvzApp`,
and `DvzAppWindow`. The explicit low-level ownership model remains the public app path for now.

Examples may still use local helpers to reduce cleanup repetition, but those helpers should not
promote a new public ownership abstraction.


### 3. Add `dvz_panel_full`; defer grid helpers to the grid spec

Partially accepted; implemented for full panels.

Add a small public helper for the common full-panel case:

```c
DvzPanel* dvz_panel_full(DvzFigure* figure);
```

Do not add ad hoc grid helpers as part of the example-duplication cleanup. A grid specification is
already planned, and grid panel APIs should follow that design instead of being introduced from the
example cleanup alone.

Remaining work: use `dvz_panel_full()` for new exact full-figure panels. Do not convert real
multi-panel layouts or deliberately inset/custom panels to `dvz_panel_full()`.


### 4. Add controller/input binding helpers

Accepted; implemented for app windows.

Add public convenience helpers that create a controller, bind it to a panel, and connect it to the
target window input router. The repeated low-level sequence should remain available:

1. create `dvz_panzoom()` / `dvz_arcball()` / `dvz_fly()` / `dvz_turntable()`;
2. unwrap the typed payload with `dvz_controller_*()`;
3. bind through `dvz_panel_bind_controller()`;
4. route input through `dvz_panel_connect_input()`.

The new helpers should sit above that sequence and preserve the lower-level primitives. Candidate
shape:

```c
DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel, DvzAppWindow* win, const DvzPanzoomDesc* desc);
DvzArcball* dvz_panel_arcball(DvzPanel* panel, DvzAppWindow* win, const DvzArcballDesc* desc);
DvzFly* dvz_panel_fly(DvzPanel* panel, DvzAppWindow* win, const DvzFlyDesc* desc);
DvzTurntable* dvz_panel_turntable(
    DvzPanel* panel, DvzAppWindow* win, const DvzTurntableDesc* desc);
```

Final names use the `dvz_app_window_panel_*()` prefix because these helpers bind through an
app-window input router. Keep GUI-viewport and other non-app-window routers on the lower-level
controller/input primitives unless a separate helper is justified.

Remaining work: use the app-window helpers for new app-window examples. Do not force this helper
into GUI-viewport examples.


### 5. Add typed visual upload helpers

Accepted; implemented for stable repeated bundles.

Keep `dvz_visual_set_data()` as the generic visual data path, but add family-level helpers for common
attribute bundles. The goal is to reduce repeated string attributes, centralize count validation, and
make user code less error-prone.

Candidate examples:

```c
int dvz_point_data(
    DvzVisual* visual, const float* positions, const DvzColor* colors,
    const float* diameters, uint32_t count);

int dvz_mesh_data(
    DvzVisual* visual, const float* positions, const float* normals,
    const DvzColor* colors, uint32_t vertex_count);

int dvz_mesh_indices(DvzVisual* visual, const uint32_t* indices, uint32_t index_count);

int dvz_pixel_data(
    DvzVisual* visual, const void* positions, const void* colors,
    const float* pixel_sizes, uint32_t item_count);

int dvz_primitive_data(
    DvzVisual* visual, const void* positions, const void* colors,
    const void* normals, uint32_t vertex_count);

int dvz_sphere_data(
    DvzVisual* visual, const void* positions, const void* colors,
    const float* radii, uint32_t item_count);
```

Current implemented helpers cover point, mesh, pixel, primitive, and sphere. Exact future helper
coverage should follow the active visual families and their required/optional attributes.

Remaining work: keep `dvz_visual_set_data()` as the generic path and avoid adding helpers for
one-off or unstable attribute combinations. Image, path, marker, segment, and text examples remain
explicit until their standard bundles are stable enough to justify a public helper.


### 6. Defer pick/probe callback dispatch

Deferred; unchanged.

Do not add `dvz_panel_on_pick()` / `dvz_panel_on_probe()` style callback dispatch as part of this
cleanup. The current request/polling path can remain while richer picking, probing, and selection
payload design continues separately.


## Implementation Order

1. Done: add `examples/c` helper infrastructure and migrate `_frame_count()` first.
2. Done: add `dvz_panel_full()`.
3. Done: add app-window controller/input binding helpers.
4. Done: add typed visual upload helpers for point, mesh, pixel, primitive, and sphere.
5. Done: migrate examples to the landed helpers without changing example behavior.
6. Later: revisit pick/probe callbacks after the richer interaction API settles.


## Related Documents

1. [EXAMPLE_GAP_REPORT.md](EXAMPLE_GAP_REPORT.md)
2. [EXAMPLE_ORGANIZATION.md](EXAMPLE_ORGANIZATION.md)
3. [../interaction/CONTROLLERS.md](../interaction/CONTROLLERS.md)
4. [../visuals/SHARED_ATTRIBUTES.md](../visuals/SHARED_ATTRIBUTES.md)
