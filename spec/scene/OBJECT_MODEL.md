# Scene Object Model

This document defines the minimum stable concepts for the future scene layer.


## Core Objects

1. `Scene`
2. `Panel`
3. `Visual`
4. `Resource`
5. `Camera`
6. `Controller`
7. `FramePlan`
8. `Animation`
9. `RenderTarget`


## Scene

The scene is the top-level owner of:

1. panels,
2. shared resources,
3. global scheduling,
4. controller registration,
5. frame build and DRP2 emission.


## Panel

A panel is a logical viewport region with:

1. a camera,
2. a visual set,
3. a local interaction state,
4. one or more rendering targets,
5. panel-local configuration that contributes to the scene-level `FramePlan`.

Panels may be onscreen, offscreen, or virtual for composition.


## Visual

A visual is a high-level scientific renderable.

The broad concept should stay consistent with the local `v0.3` scene stack, but the current v0.4
direction should follow `VISUAL_FAMILIES.md`.

The current preferred first-class families are:

1. `primitive`
2. `pixel`
3. `point`
4. `marker`
5. `segment`
6. `path`
7. `glyph`
8. `image`
9. `mesh`
10. `sphere`
11. `volume`

Historical `v0.3` names such as `monoglyph`, `wiggle`, and `slice` remain useful background
vocabulary, but they should not be read here as the preferred v0.4 family set.

Minimum visual responsibilities:

1. declare its required resources,
2. declare its shader/material variant,
3. expose transform inputs,
4. participate in one or more frame stages,
5. support picking metadata when relevant.


## Resource

Scene resources are CPU-owned logical data objects that may map to DRP2 resources.

They should support:

1. dirty tracking,
2. subrange updates,
3. stable logical identity,
4. explicit usage role,
5. optional lifetime sharing across visuals.


## Camera

The first object model only needs two camera families:

1. 2D camera
2. 3D camera

Projection math and interaction policies belong to scene-side logic, not DRP2.


## Controller

Controllers are pure scene-side state machines that react to input/events and mutate scene state.

They should not emit backend commands directly.


## FramePlan

`FramePlan` is a preferred term over a strict render graph requirement at this stage.

Reason:

The scene layer clearly needs a structure that:

1. orders passes,
2. tracks read/write resources,
3. expresses clear/load/store behavior,
4. partitions visuals across stages.

But it is too early to hard-freeze a full public render-graph API.

The current spec direction is that one scene-level `FramePlan` is built per frame, while panels may
contribute panel-local targets, nodes, and ordering constraints inside that one plan.


## Animation

Animations should target scene properties rather than backend resources directly.


## RenderTarget

A `RenderTarget` is the scene-level logical description of where a frame is rendered.

It is a scene-visible handle that the DRP2 runtime resolves to actual backend resources.
The scene never holds a `VkImageView`, swapchain image, or backend framebuffer.

Two variants exist:

1. **Canvas target** — backed by the canvas swapchain; used for interactive display.
2. **Offscreen target** — backed by a readback-capable image; used for export or headless
   rendering.

The application creates a `RenderTarget` and passes it to the scene at creation time.
The scene uses it when building `FramePlan` nodes.
The DRP2 runtime handles the mapping to backend objects.

The scene does not own the canvas, window, stream, or sinks.
Those remain application-level and canvas-level concerns.
See `RUNTIME_BOUNDARY.md` for the full ownership model.
