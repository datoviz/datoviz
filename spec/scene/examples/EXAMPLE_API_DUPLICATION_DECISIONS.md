# Example API Duplication Decisions

> **Execution Status**
> - **Status:** `DECISION NOTE`
> - **Decided on:** `2026-05-21`
> - **Scope:** current `examples/c` duplication and API pressure points
> - **Purpose:** record which repeated example patterns should become shared helpers or public API,
>   and which proposals are intentionally deferred or rejected.


## Context

The current C examples repeat several patterns that make examples harder to maintain and reveal API
pressure points in the scene/app layer. The most visible duplication is local `_frame_count()`
parsing, but the broader scan also found repeated panel construction, controller/input binding,
stringly typed visual uploads, app/window lifecycle boilerplate, and pick/probe request polling.

This note records the accepted direction so future cleanup does not re-open the same questions.


## Decisions

### 1. Add a local C example helper layer

Accepted.

Create an `examples/c`-local helper layer for non-public conveniences used by many examples. Initial
scope should include:

1. frame-count parsing with the current convention that `0` means interactive run;
2. small numeric parsing helpers such as unsigned `uint32_t` parsing;
3. common path helpers such as output paths relative to the executable or example assets;
4. optional cleanup conventions for examples with local allocations.

This is not a public Datoviz API requirement. It is example infrastructure.


### 2. Do not add a public app-session wrapper for now

Rejected for now.

Do not add a public `DvzAppSession` or similar wrapper that owns `DvzScene`, `DvzFigure`, `DvzApp`,
and `DvzAppWindow`. The explicit low-level ownership model remains the public app path for now.

Examples may still use local helpers to reduce cleanup repetition, but those helpers should not
promote a new public ownership abstraction.


### 3. Add `dvz_panel_full`; defer grid helpers to the grid spec

Partially accepted.

Add a small public helper for the common full-panel case:

```c
DvzPanel* dvz_panel_full(DvzFigure* figure);
```

Do not add ad hoc grid helpers as part of the example-duplication cleanup. A grid specification is
already planned, and grid panel APIs should follow that design instead of being introduced from the
example cleanup alone.


### 4. Add controller/input binding helpers

Accepted.

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

Final names and signatures may change during implementation, but the accepted direction is to make
controller-plus-input setup a first-class ergonomic path.


### 5. Add typed visual upload helpers

Accepted.

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
```

Exact helper coverage should follow the active visual families and their required/optional
attributes.


### 6. Defer pick/probe callback dispatch

Deferred.

Do not add `dvz_panel_on_pick()` / `dvz_panel_on_probe()` style callback dispatch as part of this
cleanup. The current request/polling path can remain while richer picking, probing, and selection
payload design continues separately.


## Implementation Order

1. Add `examples/c` helper infrastructure and migrate `_frame_count()` first.
2. Add `dvz_panel_full()`.
3. Add controller/input binding helpers.
4. Add typed visual upload helpers family by family.
5. Revisit pick/probe callbacks after the richer interaction API settles.


## Related Documents

1. [EXAMPLE_GAP_REPORT.md](EXAMPLE_GAP_REPORT.md)
2. [EXAMPLE_ORGANIZATION.md](EXAMPLE_ORGANIZATION.md)
3. [../interaction/CONTROLLERS.md](../interaction/CONTROLLERS.md)
4. [../visuals/SHARED_ATTRIBUTES.md](../visuals/SHARED_ATTRIBUTES.md)
