# Interaction Model

Interaction is a scene update loop. Input events change controller, visual, selection, or
application state; those changes request another frame; the next frame renders the retained result.

## Controllers

Controllers are the preferred path for standard navigation. Panzoom controls 2D domains. Arcball,
fly, turntable, and orbit-camera controllers control 3D view state where supported. A controller is
bound to a panel and may be shared across panels to link their view state.

## Callbacks

Callbacks are for application-specific behavior: key bindings, pointer tools, custom hover state,
selection policies, annotation updates, or host integration. Callback work should stay small. Heavy
data generation, file I/O, or expensive analysis should be deferred to a controlled update path so
the presentation loop remains responsive.

## Frame Requests

A changed scene needs a frame request. Native app paths can request and schedule presentation
through the window/app layer. Browser paths translate DOM input into the WASM scene ABI and schedule
WebGPU work through browser animation and async GPU APIs. The input events differ by host, but the
scene update principle is the same.

## Retained Interaction State

Interaction should update retained objects, not bypass them. A hover highlight should update visual
state or selection state. A probe should request query/readback work and display the retained
result. A linked pan should update shared controller/domain state. Browser JavaScript may deliver
input and display host UI, but it should not reimplement Datoviz scene semantics.

## Animation

Frame callbacks and timers are part of the same model. They mutate retained data or controller
state, request another frame, and let the frame planner decide which uploads and draws are needed.

See also:

- [Frame lifecycle](frame-lifecycle.md)
- [Query, pick, and probe model](query-pick-probe-model.md)
- [Controllers reference](../reference/controllers.md)
