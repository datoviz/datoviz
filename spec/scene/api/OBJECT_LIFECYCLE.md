# Scene Object Lifecycle Plan

> **Execution Status**
> - **Status:** `API CLEANUP PLAN`
> - **Updated on:** `2026-06-03`
> - **Purpose:** record the v0.4 lifecycle policy for retained scene API objects before adding
>   missing destroy functions.


## Summary

The scene API should expose explicit destruction for retained, bounded objects that users can create
through the public API. This matters because the current scene implementation uses fixed-capacity
owner arrays for many handles. Without matching destroy/reuse semantics, long-running applications
can exhaust object slots even when objects are no longer conceptually alive.

The rule should be:

1. If a public function creates a retained scene-owned, figure-owned, or panel-owned object, the API
   should provide a matching destroy function unless the handle is explicitly documented as a
   borrowed singleton or accessor.
2. Destroy functions should detach all parent and dependent references before marking the object
   inactive.
3. Destroy functions should request a frame when they affect visible retained state.
4. Destroyed slots should be reusable where the owning array has bounded capacity.
5. Scene destruction remains the final cascading cleanup path, but it should not be the only way to
   release dynamic layout, interaction, or resource objects.


## Immediate Gaps

### Grid

`dvz_figure_grid()` creates a retained figure-owned `DvzGrid`, but there is no public
`dvz_grid_destroy()`.

This is the highest-priority gap because grid row/column topology should remain immutable in v0.4.
If users must create a new grid when topology changes, they also need a way to destroy the old grid.

Preferred API:

```c
void dvz_grid_destroy(DvzGrid* grid);
```

Required behavior:

1. Detach every grid-owned panel from the grid.
2. Clear the grid panel attachment list.
3. Mark the grid slot inactive/reusable.
4. Request a frame on the owning figure.
5. Leave the detached panels valid as free-placement panels at their last resolved descriptors, or
   explicitly destroy them only if a later API adds a destructive grid-destroy option.

The non-destructive default is preferred because destroying a grid should not silently destroy
visuals, controllers, cameras, axes, colorbars, legends, or interactions attached to panels.


### Controllers

`dvz_panzoom()`, `dvz_arcball()`, `dvz_fly()`, `dvz_turntable()`, and `dvz_orbit_camera()` create
scene-owned `DvzController` handles. There is no public controller destroy function.

Preferred API:

```c
void dvz_controller_destroy(DvzController* controller);
```

Required behavior:

1. Detach the controller from every panel dimension that references it.
2. Destroy any controller links that reference it as source or target.
3. Release the controller-family payload.
4. Mark the controller slot inactive/reusable.
5. Request frames for affected figures.

`dvz_controller_panzoom()`, `dvz_controller_arcball()`, `dvz_controller_fly()`,
`dvz_controller_turntable()`, and `dvz_controller_orbit_camera()` remain borrowed payload accessors;
they should not get matching destroy functions.


### Units And Date Formatting

`DvzUnitLadder`, `DvzUnits`, and `DvzDateTimeFormat` are scene-owned bounded objects with public
create APIs and no destroy APIs.

Preferred APIs:

```c
void dvz_unit_ladder_destroy(DvzUnitLadder* ladder);
void dvz_units_destroy(DvzUnits* units);
void dvz_datetime_format_destroy(DvzDateTimeFormat* format);
```

Required behavior:

1. Reject or ignore destruction of shared builtin singleton-style handles if they are intentionally
   cached for the scene lifetime.
2. Clear references from axes, annotations, scale bars, and other retained objects before marking
   custom objects inactive.
3. Mark custom slots inactive/reusable.

These APIs are lower priority than grid and controller destruction, but they should be resolved
before declaring the public v0.4 lifecycle model complete.


## Existing APIs To Keep As Borrowed Handles

These handles should stay non-destroyable through their accessor names:

1. `dvz_panel_axis()` returns a panel-owned axis singleton. Use visibility/configuration APIs rather
   than `dvz_axis_destroy()`.
2. `dvz_panel_camera()` returns the current panel-owned camera. A future `dvz_panel_clear_camera()`
   would be clearer than public camera destruction.
3. `dvz_panel_controller()` returns a borrowed controller reference. The controller itself should be
   destroyed only through `dvz_controller_destroy()`.
4. `dvz_composite_visual()` and `dvz_composite_visual_at()` return visuals owned by the composite
   realization. They should not imply independent visual ownership.


## Existing Destroy Semantics To Tighten

Some current destroy functions only mark owner pointers inactive. The lifecycle pass should make
them detach dependent references consistently:

1. `dvz_panel_destroy()` should remove the panel from any owning grid attachment list.
2. `dvz_figure_destroy()` should either cascade through panels, grids, and figure compute
   attachments, or be documented as a shallow slot invalidation API and renamed/deferred
   accordingly. A cascading destroy is preferred.
3. Destroy functions for panel-attached objects should clear the corresponding panel arrays and
   reserves. Colorbar and legend destruction already follow this better pattern.
4. Scene-owned fixed arrays should prefer reusable slot allocation over monotonic `count++`
   allocation when public destruction exists.


## Deferred Shape Mutation

Do not add `dvz_grid_resize()` for v0.4 unless a later implementation pass proves a complete
attachment-remapping policy.

The preferred v0.4 rule is:

> A grid's row/column count is fixed at creation. Dynamic layout changes are supported for figure
> size, margins, gutters, and row/column sizing. To change grid topology, create a new grid and
> destroy or detach the old grid explicitly.

This keeps topology mutation out of the first public grid API while still supporting dynamic
dashboards through explicit lifecycle operations.


## Implementation Order

1. Add `dvz_grid_destroy()` and tests covering panel detachment, slot reuse, and emit after destroy.
2. Add `dvz_controller_destroy()` and tests covering panel detachment and controller-link cleanup.
3. Tighten `dvz_panel_destroy()` grid detachment.
4. Decide whether `dvz_figure_destroy()` should be cascading and test the selected behavior.
5. Add unit/date formatting destroy APIs or explicitly document them as scene-lifetime objects.
6. Audit fixed-capacity owner arrays for reusable allocation after destruction.


## Validation

For each lifecycle API, add focused tests for:

1. destroying NULL handles;
2. destroying active handles;
3. repeated destruction;
4. parent detachment;
5. dependent reference cleanup;
6. slot reuse after destruction;
7. frame emission after destruction;
8. rejection while legacy raw borrowed emitted streams are live when visual/resource data may be
   affected, while artifact-backed emission snapshots allow later retained-scene mutation.
