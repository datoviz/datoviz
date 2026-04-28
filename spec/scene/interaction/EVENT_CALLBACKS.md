# Scene Event Callbacks

This document defines the scene-level event callback system — the mechanism by which
the application observes scene-produced events without polling.


## Purpose

Controllers and picking already define how input triggers scene mutations.
This document defines the reverse direction: how the scene notifies the application
when scene-level state changes occur.

Examples:
- the user changed the selection — the app updates a status bar,
- a pick result arrived — the app shows a tooltip,
- an animation completed — the app advances to the next step,
- the window was resized — the app updates an external layout.


## Registration

```text
dvz_scene_on(scene, event_type, callback, user_data)
```

Multiple callbacks may be registered for the same event type.
They are called in registration order.

Callbacks fire synchronously on the render thread at the point in the frame lifecycle
where the event occurs (see per-event notes below).
They may read scene state freely.
They may mutate scene state through normal scene setters.
They must not call GPU or DRP2 functions directly.

To unregister:

```text
dvz_scene_off(scene, event_type, callback)
```


## Event Types

### `DVZ_EVENT_SELECTION_CHANGED`

Fires after `DvzSelection` state is updated — either from user interaction (click, box,
lasso) or from a programmatic `dvz_selection_set` / `dvz_selection_clear` call.

Fires during stage 3 (State Update) of the frame lifecycle.

Callback signature:

```text
void on_selection(DvzScene* scene, DvzSelection* sel, void* user_data)
```

The callback may call `dvz_selection_get` to read the current index set.


### `DVZ_EVENT_PICK_RESULT`

Fires when a pick readback result is available.

Fires during stage 11 (Post-Frame Readback) of the frame lifecycle.

Callback signature:

```text
void on_pick(DvzScene* scene, DvzPickResult* result, void* user_data)
```

`DvzPickResult` carries the panel, visual, item index, and screen position of the pick,
or a flag indicating an empty pick (no item under the cursor).

This event subsumes the polling model described in `PICKING.md` and is the preferred
way to react to pick results.


### `DVZ_EVENT_HOVER`

Fires each frame when the cursor is over a pickable item and the hover pick result
changes (different item, or transition from item to empty or vice versa).

Fires during stage 11 (Post-Frame Readback).

Callback signature:

```text
void on_hover(DvzScene* scene, DvzPickResult* result, void* user_data)
```

`result` is null-like when the cursor is over empty space.
When the cursor stays over the same item across frames, the callback does not re-fire.


### `DVZ_EVENT_ANIM_STEP`

Fires each frame that an animation advances, after the animation state is updated.

Fires during stage 3 (State Update).

Callback signature:

```text
void on_anim_step(DvzScene* scene, DvzAnimation* anim, double t, void* user_data)
```

`t` is the current normalized animation time in `[0, 1]` (or the raw clock time for
open-ended animations).


### `DVZ_EVENT_ANIM_COMPLETE`

Fires once when a bounded animation reaches `t_end`.

Fires during stage 3 (State Update), in the same frame as the final step.

Callback signature:

```text
void on_anim_complete(DvzScene* scene, DvzAnimation* anim, void* user_data)
```


### `DVZ_EVENT_RESIZE`

Fires when the figure or panel size changes (window resize, DPI change, or programmatic
panel resize).

Fires during stage 1 (Input Update) after the new size is known.

Callback signature:

```text
void on_resize(DvzScene* scene, DvzFigure* figure, float width, float height,
               void* user_data)
```


### `DVZ_EVENT_DPI_CHANGED`

Fires when the device pixel ratio changes (window moved to a different display).

Fires during stage 1 (Input Update).

Callback signature:

```text
void on_dpi(DvzScene* scene, float dpi_scale, void* user_data)
```


## Callbacks From Background Threads

Background threads must not call `dvz_scene_on` directly.

If a background thread needs to react to a scene event, it should:

1. register the callback from the render thread (e.g., at startup),
2. have the callback enqueue a `DVZ_TRANSFER_CALLBACK` if it needs to trigger further
   background work.

This keeps the callback itself on the render thread while still allowing background
coordination.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `PICKING.md` | `DVZ_EVENT_PICK_RESULT` and `DVZ_EVENT_HOVER` replace the polling model |
| `SELECTION.md` | `DVZ_EVENT_SELECTION_CHANGED` fires after selection state update |
| `ANIMATION.md` | `DVZ_EVENT_ANIM_STEP` and `DVZ_EVENT_ANIM_COMPLETE` fire during animation update |
| `FRAME_LIFECYCLE.md` | each event notes which lifecycle stage it fires in |
| `THREAD_SAFETY.md` | background threads react via transfer callbacks, not direct registration |
| `HIGH_DPI.md` | `DVZ_EVENT_DPI_CHANGED` fires on pixel ratio change |
