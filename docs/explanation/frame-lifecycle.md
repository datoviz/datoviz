# Frame lifecycle

A view render step turns retained scene state into presentation, capture, or readback. The scene
decides *what* the frame means; the runtime executes the already-planned work. This page describes
the internal ordering, not a public event-loop recipe.

**Audience:** hosted-loop, scene-planning, runtime, and query contributors. **Prerequisite:**
[Architecture](architecture.md). The frame-plan and artifact APIs are advanced/internal surfaces;
ordinary applications use `DvzView`, `dvz.run(...)`, or documented capture helpers.


## End-to-end sequence

```text
1 input and app edits
2 controller/animation/derived-state update
3 dirty-scope resolution
4 validation
5 capability adaptation
6 one scene-level FramePlan
7 immutable DvzSceneFrameArtifact
8 DRP2 runtime execution
9 presentation/capture/recording
10 query/readback completion processing
```

Validation occurs after dirty-scope resolution so it can inspect the smallest correct scope.
Capability adaptation occurs after semantic validation and before planning. Planning therefore sees
validated intent plus an explicit capability decision; the runtime must not silently choose a
different scene fallback.


## What one `FramePlan` contains

One scene-level plan is built for a figure frame even when several panels contribute local nodes.
It records logical targets and stable resources plus ordered upload, compute, render, copy, and
readback nodes. Dependencies express upload-before-use, compute-write-before-render-read, render-
before-readback, panel ordering, and shared-resource ordering.

Uploads and lazy materialization belong in this plan. They must not appear later through an
execution-only side path. The plan contains no Vulkan, WebGPU, swapchain, command-buffer, or image-
view handles.


## Artifact snapshot and lifetime

Emission translates the already-built plan into an owned `DvzSceneFrameArtifact`:

- an immutable DRP2 stream snapshot;
- frozen upload payload bytes;
- zero or more setup/update/frame packet spans and payload arenas;
- status, diagnostics, resource version, and frame index.

Successful artifact creation ends the scene-mutation exclusion for that build. Later scene changes
are legal and affect later artifacts only. Stream, diagnostic, packet, and arena views returned from
the artifact are borrowed and must not outlive artifact destruction.


## View and runtime execution

`DvzView` drives repeated submissions for one figure. It synchronizes target size and input state,
requests the plan/artifact, executes the artifact stream through its reused `DvzDrp2Runtime`, then
presents, captures, records, or replays at the app/canvas boundary. A hosted loop calls render-once
when its surface is drawable; `dvz_app_run()` drives the same path from a Datoviz-owned loop.

External native UI may render in an app-owned post-scene slot before presentation. It may mutate
ordinary scene state before planning, but it does not become a parallel scene renderer.


## Completion and failure

Submission acceptance is distinct from later completion. Capture, query, and readback results need
frame/request identity and explicit lifetime. Browser completion is asynchronous; native execution
may also require polling. Runtime errors should map to scene-visible plan/resource/target identity,
with backend text retained only as diagnostic detail.


## Sources of truth

- [Frame lifecycle specification](https://github.com/datoviz/datoviz/blob/main/spec/scene/pipeline/FRAME_LIFECYCLE.md)
- [FramePlan specification](https://github.com/datoviz/datoviz/blob/main/spec/scene/pipeline/FRAME_PLAN.md)
- [Frame artifact public contract](../reference/c-api/scene.md)
- [Scene to runtime boundary](scene-to-runtime-boundary.md)
