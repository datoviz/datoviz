# Scene Requirements

This document defines what the active scene layer requires from DRP2 and its runtime.

It intentionally does not define the final public scene API.


## Normative Status

This document is normative.

Its hard requirements should be read as constraints on both:

1. scene-side architectural work,
2. future runtime-facing contract work.


## Architectural Position

The scene layer sits above DRP2.

It should:

1. own user-visible visualization state,
2. transform that state into rendering work,
3. emit backend-agnostic DRP2 commands,
4. avoid direct dependency on Vulkan, vklite, swapchain, or windowing internals.


## Hard Requirements

1. Scene must not expose backend handle types in its public API.
2. Scene must be able to target both native and browser runtimes through the same DRP2 semantics.
3. Scene must support deterministic offscreen rendering and readback.
4. Scene must support resource updates without forcing whole-scene rebuilds each frame.
5. Scene must support picking workflows.
6. Scene must support compute-assisted workflows if compute remains mandatory in DRP2 v1.
7. Scene must validate authored intent before planning execution work.
8. Scene must apply capability adaptation before planning rather than relying on backend best-effort
   fallback.
9. Scene must build one scene-level `FramePlan` per frame, even when the plan contains panel-local
   nodes or targets.


## Required Runtime Services

The scene layer should depend on a small runtime-facing service surface:

1. capability snapshot query,
2. error and diagnostics reporting,
3. shader module ingestion,
4. resource creation and update,
5. command-stream submission for already-planned work,
6. offscreen target creation and typed readback,
7. completion routing for picking and export results,
8. optional frame timing and debug hooks later.


## What Scene Should Own

1. panels and layout,
2. cameras and controllers,
3. visuals and materials,
4. CPU-side resources and dirty tracking,
5. transform logic,
6. validation and capability adaptation,
7. framegraph or equivalent render-work planning,
8. picking interpretation,
9. animation and scheduling logic.


## What Scene Should Not Own

1. Vulkan objects,
2. swapchain policy,
3. backend memory allocation details,
4. backend synchronization primitives,
5. platform windowing handles,
6. encoder command-buffer internals.


## Contract Pressure

If scene design seems to require low-level backend leakage, prefer improving DRP2 or the runtime
boundary rather than adding a scene-private escape hatch.
