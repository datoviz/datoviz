# Interaction model

Interaction is retained scene mutation followed by another frame. Controllers handle standard
navigation; callbacks and application tools update semantic state; queries return rendered
identities or values. None of these paths should issue backend commands directly.

**Audience:** application developers building custom tools and contributors changing input or
controller routing. **Prerequisite:** understand panels and the [frame lifecycle](frame-lifecycle.md).
Exact events and controller functions are documented in [Callbacks](../reference/callbacks.md) and
[Controllers](../reference/controllers.md).


## One interaction cycle

```text
platform input
   -> host/UI filtering
   -> normalized scene event
   -> panel focus, hover, or capture routing
   -> controller/callback mutates retained state
   -> dirty scopes and redraw request
   -> validation, planning, emission, execution
   -> optional asynchronous query result
   -> freshness check and retained result update
```

The normalized event is authoritative above the runtime boundary. GLFW, Qt, and browser DOM events
may differ, but scene controllers consume semantic pointer, wheel, keyboard, resize, timer, or
query-result events.


## Controllers, callbacks, and application state

| Mechanism | Use it for | Avoid |
| --- | --- | --- |
| Controller | Panzoom, arcball, fly, turntable, and other reusable navigation state. | Updating visual buffers directly from backend-specific events. |
| Callback | Short application-specific reactions: shortcuts, tool modes, annotations, or request creation. | Heavy analysis, blocking I/O, or retaining borrowed event/result memory. |
| Application state | Semantic selection, domain objects, expensive prepared data, and host UI state. | Treating a raw GPU id as durable domain identity. |
| Query | Render-aware item identity, sample value, or GPU output. | Reimplementing panel and sample transforms in host code. |

Focus selects where keyboard/mode events go. Pointer hover selects a panel when no active capture
overrides it. Capture keeps a drag or gesture routed to its initiating controller until release or
cancel, even if the pointer leaves the panel.


## Invalidation consequences

A controller change is normally narrow. Panzoom or a 3D camera move marks panel transforms dirty and
requests a redraw. It should not upload the original data arrays. Axis layout becomes dirty only
when visible-domain coverage or readability requires new ticks/labels. Interaction mode or query
topology changes may require a frame-plan rebuild.

Linked views should share a controller only when they truly share its whole semantic state. Partial
links—such as X extent only or rotation only—need an explicit controller-state link so each panel
keeps its own viewport and unlinked state.


## Host and threading boundary

The render/app thread owns normal scene mutation and frame emission. A worker should hand prepared
data back through an application-controlled update path rather than mutate live retained objects
concurrently. Browser JavaScript may deliver input and display host UI, but the C/WASM scene remains
the durable owner of controllers, visuals, selections, and query policy.


## Sources of truth

- [Controller contract](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/interaction/CONTROLLERS.md)
- [Thread-safety contract](https://github.com/datoviz/datoviz/blob/v0.4-dev/spec/scene/integration/THREAD_SAFETY.md)
- [Queries, picking, and probing](query-pick-probe-model.md)
