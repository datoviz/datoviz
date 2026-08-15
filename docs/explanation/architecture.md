# Architecture

Datoviz has one rendering path with a retained scene above a backend-neutral command boundary. This
page identifies the layers and the decisions each one owns; it does not replace their API reference
or implementation specifications.

**Audience:** embedding developers, backend authors, and contributors. **Prerequisite:** understand
[scene building blocks](figure-panel-visual-model.md) and run one scene example. Scene/app APIs are
the supported user path by feature; DRP2 and lower runtime APIs are **advanced/unstable**, and the
browser runtime is an **experimental** subset.


## One path, several owners

```text
retained scene state
    -> validation and capability adaptation
    -> one scene-level FramePlan
    -> immutable DvzSceneFrameArtifact
    -> DRP2 stream snapshot and setup/update/frame packets
    -> native vklite/canvas or browser WebGPU execution
    -> presentation, capture, recording, or readback
```

| Layer | Owns | Must not decide |
| --- | --- | --- |
| Scene | Figures, panels, visuals, data contracts, controllers, scales, adornments, semantic identity, validation, adaptation, planning. | Vulkan/WebGPU handles, swapchains, command recording, queue submission. |
| `FramePlan` | Logical targets, resources, upload/compute/render/copy/readback nodes, dependencies, and scene-level diagnostics for one build. | Backend handles or backend scheduling mechanics. |
| Frame artifact | An immutable snapshot of the emitted command stream, frozen payload bytes, packet spans, versions, and diagnostics. | Later scene mutations or long-lived backend resource ownership. |
| DRP2 | Typed logical GPU commands, object/pass lifetime rules, validation, capabilities, binary packets, fixtures, and recording shape. | Visual-family meaning, panel layout, navigation, or selection policy. |
| Native runtime | DRP2 execution through `vklite`, Vulkan resource ownership, synchronization, command recording, and submission. | Replanning a scene or inferring meaning from buffer ids. |
| Canvas/stream/app | Frame acquisition, offscreen/native targets, presentation, capture, sinks, views, and event-loop stepping. | A second scene or renderer. |
| Browser host/runtime | Adapter/device/canvas setup, input delivery, packet validation, and supported WebGPU execution. | Browser-only reimplementation of scene semantics. |


## Why the frame artifact exists

Planning and execution do not share mutable scene memory. Once `dvz_figure_emit_frame()` succeeds,
the returned `DvzSceneFrameArtifact` owns its stream snapshot and frozen upload bytes. User code may
mutate the retained scene immediately; those changes affect only later artifacts. Stream and packet
views borrowed from the artifact remain valid only until artifact destruction.

This snapshot boundary supports native execution, WASM transport, recording, and deterministic
inspection without making scene mutation wait for backend consumption.


## Native and browser execution

Native execution maps the artifact-owned DRP2 stream onto `vklite` and canvas resources. A
`DvzView` owns or borrows its concrete target and reuses the DRP2 runtime for repeated figure
submissions. The app layer drives a Datoviz-owned loop or a host calls render-once explicitly.

The browser uses the same retained C/WASM scene and receives binary `setup`, `update`, and `frame`
packets. JavaScript owns browser devices and canvas presentation but does not rebuild visuals,
controllers, or query semantics. Check the [WebGPU subset](../reference/webgpu-subset.md) before
assuming a native feature is portable.


## Extension rule

New user meaning belongs above the boundary in scene semantics. New portable execution work belongs
in `FramePlan`, DRP2, and each supporting runtime. Host-specific presentation stays in app/canvas or
the host provider. Do not add a parallel renderer, presentation path, frame stream, or Vulkan
wrapper to bypass a missing contract.


## Sources of truth

- [Scene runtime boundary](https://github.com/datoviz/datoviz/blob/main/spec/scene/core/RUNTIME_BOUNDARY.md)
- [FramePlan contract](https://github.com/datoviz/datoviz/blob/main/spec/scene/pipeline/FRAME_PLAN.md)
- [Frame lifecycle contract](https://github.com/datoviz/datoviz/blob/main/spec/scene/pipeline/FRAME_LIFECYCLE.md)
- [DRP2 authority](https://github.com/datoviz/datoviz/blob/main/spec/drp2/AUTHORITY.md)
- [Runtime internals](../advanced/runtime-internals.md)
