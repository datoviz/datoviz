# Why Datoviz?

Datoviz v0.4 is a C-first rendering engine for scientific visualization. It is not a plotting
library, a notebook widget toolkit, or a compatibility layer for the old Datoviz v0.3 Python API.
The purpose of the v0.4 rewrite is to make the rendering core explicit enough to validate,
embed, replay, port, and harden.

## The Short Version

Datoviz sits below plotting libraries and above low-level GPU APIs. A plotting library decides what
the user means by a scatter plot, image, mesh, colorbar, or axis. Datoviz owns the rendering engine
that turns those retained scientific scene objects into GPU work.

That position matters because modern GPU rendering is powerful but explicit. The application must
create buffers and textures, describe shaders and pipelines, schedule render passes, synchronize
data movement, and handle device-specific limits. Datoviz keeps those details in one engine instead
of spreading them through every plotting layer that wants fast scientific graphics.

## Engine First

The engine is designed around retained scene state. Users create figures, panels, visuals,
controllers, adornments, and resources. Datoviz keeps that state, validates it, and lowers it into
frame plans and backend-agnostic DRP2 command streams. The runtime then executes those streams
through the native Vulkan-backed path or, for the experimental browser subset, through WebGPU.

This separation gives Datoviz three jobs:

- provide a predictable native C API for scenes, app views, offscreen rendering, capture, and
  embedding;
- keep GPU work inspectable through frame plans, DRP2 streams, fixtures, and replay-oriented
  validation;
- act as a rendering backend that higher-level scientific plotting layers can target.

## What DRP2 Means

DRP2 means Datoviz Rendering Protocol v2. It is the command-stream layer between scene semantics and
GPU runtime execution.

A scene describes user-facing objects: panels, cameras, axes, point clouds, images, meshes, labels,
and their data. A frame plan is Datoviz's internal description of what one frame needs to draw. DRP2
is the lower-level stream of typed commands produced from that plan: create this buffer, upload these
bytes, create this shader module, begin this render pass, bind this pipeline, draw these vertices,
copy this texture, submit this work.

DRP2 exists so that Datoviz has one explicit boundary for rendering work:

```text
scene state -> frame plan -> DRP2 command stream -> runtime backend
```

The native runtime maps DRP2 commands to Vulkan, the low-level GPU API used by Datoviz on desktop.
`vklite` is Datoviz's internal Vulkan runtime layer: it owns the Vulkan objects and command-buffer
work needed to execute the supported DRP2 stream. The experimental browser runtime maps the
portable subset of the same command shape to WebGPU. That does not make DRP2 a public plotting API.
It is an advanced engine and contributor surface, useful for validation, fixtures, replay, backend
work, and portability checks.

The benefit is practical: scene code can stay about scientific visualization semantics, while DRP2
records the GPU work in a form that can be validated, serialized, replayed, and tested against more
than one backend.

## What WebGPU Is

WebGPU is a modern browser GPU API. It gives web applications controlled access to graphics and
compute features through concepts such as adapters, devices, buffers, textures, bind groups,
pipelines, command encoders, render passes, compute passes, and shader modules. Its shader language
is WGSL.

For Datoviz users, the important point is not the vocabulary. The important point is that WebGPU
brings explicit GPU rendering to the browser in a way that is much closer to Vulkan, Metal, and
Direct3D 12 than to older immediate-style web graphics APIs. It is designed around prepared
resources and recorded commands, not one high-level call per visual object.

Datoviz does not treat WebGPU as a separate plotting system. The browser path keeps the same upper
layers:

```text
C/WASM scene state -> frame plan -> DRP2 packets -> WebGPU runtime -> browser canvas
```

WASM, or WebAssembly, is the portable binary format that lets the Datoviz C scene engine run inside
the browser. JavaScript owns browser integration such as page loading, input events, canvas resize,
adapter selection, and status reporting. The Datoviz scene still owns visual semantics and emits
DRP2-shaped work.

## Why This Architecture

The v0.4 architecture is intentionally more layered than a direct renderer. Datoviz could have had
scene code call Vulkan directly, then separately add browser-specific WebGPU code. That would be
faster to prototype but harder to validate and maintain. It would also invite two different answers
to the same question: what does a Datoviz scene mean?

DRP2 keeps the answer shared. A supported scene feature should lower to a clear stream of rendering
commands. A backend can execute that stream, reject it with a diagnostic, or expose a missing
capability. This is especially important for WebGPU, where the v0.4 goal is an honest experimental
subset rather than full native Vulkan parity.

The result is a stricter contract:

- scene APIs describe scientific visualization objects and data contracts;
- frame planning decides how those objects should be rendered for a frame;
- DRP2 records backend-agnostic rendering commands;
- native Vulkan/vklite and browser WebGPU runtimes execute the supported command subset;
- unsupported features should fail explicitly instead of silently forking semantics.

## Deliberate Boundaries

Datoviz v0.4 deliberately does less than a full plotting environment. It does not own high-level
`plot()`, `scatter()`, or `imshow()` APIs, declarative chart grammars, notebook layout systems,
publication-oriented vector export, or Python-first object models. Those responsibilities belong to
GSP/VisPy2 or other libraries above Datoviz.

## Documentation Consequence

That boundary matters for the documentation. Datoviz pages should teach the engine, the retained
scene model, visual data contracts, runtime ownership, status labels, and low-level binding scope.
They should not promise migration compatibility with the v0.3 Python plotting API.

See also:

- [GSP and VisPy2 boundary](gsp-vispy2-boundary.md)
- [Project status](../reference/project-status.md)
- [Choose your layer](../start/choose-your-layer.md)
