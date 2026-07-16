# Advanced

Advanced documentation explains how Datoviz turns a retained scene into GPU work, how its native
and browser runtimes divide responsibilities, and how contributors validate those boundaries. It
also collects the operational procedures used to prepare a release.

You do not need this section to create an ordinary visualization. For scene construction, visual
data, interaction, and output, use the [How-To guides](../how-to/index.md). For exact attributes,
API contracts, and support status, use the [Reference](../reference/index.md).

## Who This Section Is For

Choose the path that matches your work. The paths are independent; you do not need to read the
whole section in order.

| If you are... | Start with | Useful background |
| --- | --- | --- |
| trying to understand the engine | [Architecture](../explanation/architecture.md) | The [scene building blocks](../explanation/figure-panel-visual-model.md) |
| changing scene planning or frame behavior | [Scene to runtime boundary](../explanation/scene-to-runtime-boundary.md) | C, retained scenes, and the [frame lifecycle](../explanation/frame-lifecycle.md) |
| working on Vulkan, WebGPU, capture, or replay | [Runtime internals](runtime-internals.md) | GPU resources, command submission, and C |
| embedding Datoviz or testing browser portability | [Portability and integration](#portability-and-integration) | The relevant host API and platform |
| contributing code or documentation | [Contributor workflows](#contributors) | A source checkout and the repository's `AGENTS.md` |
| preparing a release candidate | [Release maintainers](#release-maintainers) | Maintainer access and a reviewed release version |

!!! note "Status and stability"

    The scene API is the primary user surface. DRP2 and lower runtime layers are
    **advanced/unstable**; the WebGPU/WASM and compute-to-render paths are **experimental**.
    Contributor and release pages describe development procedures, not additional supported user
    APIs. Check [Project status](../reference/project-status.md) and
    [Feature status](../reference/feature-status.md) before depending on a lower-level surface.

## Concepts and Architecture

Start here when you need the mental model behind an API or want to understand where a behavior
belongs. These pages progress from user-visible objects to frame planning and resource reuse.

| Page | What it answers |
| --- | --- |
| [Scene building blocks](../explanation/figure-panel-visual-model.md) | How scenes, figures, panels, visuals, attributes, and views fit together. |
| [Architecture](../explanation/architecture.md) | Which layers make up Datoviz v0.4 and how data moves between them. |
| [Scene to runtime boundary](../explanation/scene-to-runtime-boundary.md) | Which decisions belong to scene planning and which belong to backend execution. |
| [Frame lifecycle](../explanation/frame-lifecycle.md) | How retained state becomes presentation, capture, or readback for one frame. |
| [Retained resources](../explanation/retained-resources.md) | What persists between frames and why it is retained. |
| [Invalidation and caching](../explanation/invalidation-and-caching.md) | How a scene change requests bounded work without rebuilding everything. |
| [GPU resource ownership](../explanation/gpu-resource-ownership.md) | Which layer owns or borrows concrete GPU resources and when they may be changed or destroyed. |

For adjacent user-facing concepts, see [coordinate systems](../explanation/coordinate-systems.md),
the [interaction model](../explanation/interaction-model.md), and
[queries, picking, and probing](../explanation/query-pick-probe-model.md). The
[Datoviz, GSP, and VisPy2](../explanation/gsp-vispy2-boundary.md) page explains why high-level
scientific plotting is outside Datoviz v0.4 itself.

## Runtime Layers

The runtime path is deliberately linear:

```text
scene state -> frame artifact -> DRP2 packets -> runtime -> output
```

Use [Runtime internals](runtime-internals.md) for an overview of the responsibilities and ownership
rules at each step. Continue to [DRP2 command streams](drp2-command-streams.md) only when you are
changing the backend-neutral command contract, packet transport, fixtures, validation, or replay.
DRP2 is an internal transport rather than an application-level scene API.

| Topic | Canonical page | Status |
| --- | --- | --- |
| Runtime responsibility and ownership | [Runtime internals](runtime-internals.md) | advanced/unstable |
| Backend-neutral commands and packets | [DRP2 command streams](drp2-command-streams.md) | advanced/unstable |
| GPU compute followed by rendering | [Compute and graphics](../reference/compute-graphics.md) | experimental, narrow slice |
| Recorded frame-stream diagnostics | [Record and replay](../how-to/record-replay.md) | advanced diagnostic workflow |
| Exact lower-level C symbols | [Runtime and utilities C API](../reference/c-api/runtime.md) | varies by API group |

## Portability and Integration

Use these pages when Datoviz must run in a non-default host or when a visualization needs to cross a
backend boundary. Support is feature-specific: confirm both the platform and the example or feature
you need.

| Goal | Start here | Important constraint |
| --- | --- | --- |
| Run a promoted example in a browser | [WebGPU subset](../reference/webgpu-subset.md) | Experimental subset; it is not native Vulkan parity. |
| Check browser support example by example | [WebGPU example matrix](../examples/webgpu-matrix.md) | Generated from current example metadata. |
| Deploy or diagnose a browser example | [Deploy to the web](../how-to/deploy-to-web.md) and [debug WebGPU](../how-to/debug-webgpu.md) | Browser routes reuse canonical C examples or portable scenarios. |
| Embed in a native C or C++ application | [C/C++ integration](../how-to/c-integration.md) | The host and Datoviz must agree on object and event-loop lifetimes. |
| Host Datoviz in Qt | [Embed in Qt](../how-to/embed-in-qt.md) | Requires the optional Qt provider. |
| Render without a window | [Render offscreen](../how-to/render-offscreen.md) | Capture output is raster RGBA8 unless an API states otherwise. |
| Diagnose platform or driver setup | [Platform diagnostics](../how-to/diagnose-platform.md) | Record the OS, GPU, driver, backend, and exact diagnostic. |

## Contributors

Contributor pages route changes to the current source of truth and provide focused validation
workflows. Read the repository `AGENTS.md` before editing; durable behavior belongs in `spec/`,
public guidance in `docs/`, and current execution status in `agents/`.

| Task | Page |
| --- | --- |
| Find the owning spec, source directory, or validation layer | [Architecture map](../contributors/architecture-map.md) |
| Add a backend-neutral command | [Adding a DRP2 command](../contributors/adding-a-drp2-command.md) |
| Add WebGPU contract coverage | [Adding a WebGPU fixture](../contributors/adding-a-webgpu-fixture.md) |
| Change generated example or API documentation | [Generated documentation](../contributors/generated-documentation.md) |
| Select a canonical example by feature or data capability | [Example selection by capability](../contributors/example-selection-by-capability.md) |

When a page disagrees with a durable spec, use the source-of-truth order documented by that
subsystem rather than copying the disagreement into another guide.

## Release Maintainers

These procedures are for maintainers preparing a release candidate or final release. They may
build artifacts and local evidence, but publication, tagging, uploading, and other external actions
remain separate approval-gated steps.

Follow this order:

1. [Release process](../contributors/release-process.md) defines the phases and local preparation
   flow.
2. [Release flight checklist](../contributors/release-flight-checklist.md) is the end-to-end
   checklist for a specific candidate.
3. [Release validation](../contributors/release-validation.md) explains local and cross-machine
   evidence.
4. [Release wheels](../contributors/release-wheels.md) covers platform wheel construction,
   inspection, and installed-artifact smoke tests.
5. [Agent release checklist](../contributors/agent-release-checklist.md) defines what automation
   may prepare and what still requires maintainer review.
6. [Generated documentation](../contributors/generated-documentation.md) and the
   [validation gallery](../examples/validation-gallery.md) cover generated public pages and visual
   evidence.

Start a release pass by choosing the exact version and reading the current repository status. Do
not infer that a successful local build authorizes publication.
