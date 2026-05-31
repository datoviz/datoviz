Execution Status:

- Status: future roadmap informed by the v0.4 experimental compute-to-graphics slice
- Updated on: 2026-05-31
- Purpose: define the staged path from figure-attached scene compute to retained scene compute
  tasks without creating a second execution graph beside `DvzFramePlan`
- Scope: scene resources, compute tasks, scheduling policy, FramePlan lowering, and long-term
  compute/render synchronization

# Scene Compute Task Roadmap

Scene compute should evolve from the current narrow compute-to-graphics experiment into a retained
scene task model. The executable graph remains `DvzFramePlan`; scene tasks are persistent user
intent that lower into one or more FramePlan nodes for a particular tick or frame.


## Layering

The long-term architecture has three layers:

1. scene retained state: resources, visuals, compute tasks, and schedules;
2. `DvzFramePlan`: executable per-frame or per-tick IR with nodes, graph passes, resource accesses,
   dependencies, and barriers;
3. DRP2/runtime: backend command emission and execution.

Do not introduce a second runtime graph for compute. A retained scene task may be graph-shaped at
the user-intent layer, but execution must be expressed through the existing FramePlan machinery.

The intended lowering is:

```text
Scene resources + tasks + schedules
  -> selected work for this tick/frame
  -> DvzFramePlan nodes, graph resources, graph passes, access declarations, barriers
  -> DRP2 command stream
  -> vklite/WebGPU/native runtime
```


## Current Slice

The v0.4 experimental slice is intentionally narrow:

1. `DvzSceneCompute` stores shader source, dispatch shape, and buffer bindings;
2. `DvzSceneBuffer` may be shared between compute and visuals;
3. `dvz_figure_add_compute()` schedules compute before the figure render path;
4. FramePlan/DRP2 lowering emits compute before render and inserts the required
   compute-write-to-render-read synchronization;
5. `examples/c/lab/gpu_particle_advection.c` proves storage-buffer compute feeding point rendering.

This is a frame-coupled convenience surface, not the final general model.


## Stage 1: Harden Frame-Coupled Compute

Before broadening the API, stabilize the current slice:

1. use real app/frame timing in examples instead of synthetic frame counters;
2. expose or standardize frame callback timing metadata: elapsed time, delta time, and frame index;
3. clamp demo `dt` values so stalls do not destabilize visual simulations;
4. test compute-before-render ordering;
5. test storage-write to vertex-read barriers;
6. test multiple compute buffers and read-only versus read-write bindings;
7. test dirty CPU uploads feeding compute buffers;
8. document `dvz_figure_add_compute()` as a convenience API.

The default scheduling model remains one compute dispatch per rendered frame.


## Stage 2: Introduce Retained Scene Tasks

Promote compute into scene-level retained work:

```c
DvzSceneTask* task = dvz_scene_task_compute(scene, &desc);
dvz_scene_task_bind_buffer(task, binding, buffer, access, offset, size);
dvz_scene_task_schedule(task, &schedule);
```

A compute task should describe:

1. shader format, shader source, and optional entry point;
2. dispatch shape or dispatch source;
3. buffer/texture bindings;
4. declared access for every binding;
5. schedule policy;
6. optional explicit dependencies;
7. optional scope, such as before a figure render.

Figure-attached compute should become shorthand for:

```text
scene compute task
  schedule: frame-coupled
  scope: before this figure renders
  dependencies: inferred from bound resources and visual consumers
```


## Stage 3: Add Scheduling Policies

Scheduling should be independent from shader and resource binding state.

Initial policies:

1. `FRAME`: run once before a dependent render frame;
2. `ON_DIRTY`: run when declared input resources changed;
3. `ONCE`: run once, then become clean;
4. `MANUAL`: run only when explicitly triggered;
5. `FIXED_STEP`: run zero or more fixed simulation steps during one app tick.

The first implementation should remain conservative:

1. execute through the same FramePlan path;
2. use one queue/runtime submission model unless the runtime already owns stronger semantics;
3. avoid async overlap;
4. infer barriers from declared resource accesses.


## Stage 4: Support Multi-Pass Compute Workflows

Multiple retained tasks should lower into ordered FramePlan work:

```text
task A writes buffer X
task B reads X and writes Y
task C reads Y
render reads Y
```

The scheduler selects tasks for the current tick. FramePlan lowering then emits:

1. upload nodes for dirty inputs;
2. compute nodes or graph passes for selected tasks;
3. resource barriers between incompatible accesses;
4. render/readback/copy nodes for consumers.

This supports:

1. particle simulation;
2. fluid or field update steps;
3. generated geometry;
4. GPU layout;
5. reductions and histograms;
6. preprocessing before render;
7. compute-only workflows with readback.


## Stage 5: Fixed-Step Simulation

Fixed-step scheduling decouples simulation time from display refresh.

The scheduler owns an accumulator:

```text
accumulator += wall_dt
while accumulator >= sim_dt and steps < max_steps:
    emit one compute step
    accumulator -= sim_dt
render latest complete state
```

A single retained task may therefore emit multiple compute nodes into one FramePlan. Required
policy knobs:

1. simulation `dt`;
2. maximum steps per app tick;
3. catch-up behavior when the app stalls;
4. parameter update strategy for each substep;
5. whether render interpolation is supported.


## Stage 6: Timer, Async, And Versioned Resources

Timer-driven and async compute are later work because they require stronger synchronization.

Use cases:

1. compute every fixed wall-clock interval;
2. render the latest complete resource version;
3. background GPU preprocessing;
4. streaming data transforms;
5. long-running compute with eventual readback.

Likely requirements:

1. resource versioning;
2. double or triple buffering helpers;
3. explicit fence/timeline tracking;
4. latest-complete semantics for render consumers;
5. optional async compute queue support where available;
6. diagnostics when the backend cannot provide the requested scheduling mode.

This stage should extend `DvzSceneTask` and `DvzSceneSchedule`; it should not bypass FramePlan.


## Non-Goals

This roadmap does not require v0.4 to ship:

1. a broad public custom-shader framework;
2. async compute queues;
3. timer-driven compute independent from rendering;
4. automatic parallel scheduling;
5. backend-specific synchronization details in the public scene API.


## Design Rule

Use one executable graph:

```text
DvzFramePlan
```

Scene tasks are retained declarations. Schedules decide when those declarations are realized.
FramePlan remains the runtime-facing contract that carries the executable order, accesses, and
barriers.
