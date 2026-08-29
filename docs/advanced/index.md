# Advanced

Use Advanced to understand Datoviz architecture, change runtime or backend code, contribute to the
repository, or prepare a release. You do not need this section to create an ordinary visualization;
use the [How-To guides](../how-to/index.md) for tasks and [Reference](../reference/index.md) for exact
contracts.

!!! note "Status and stability"

    The scene API is the primary user surface. DRP2 and lower runtime layers are
    **advanced/unstable**; WebGPU/WASM and compute-to-render paths are **experimental**.

<div class="dvz-audience-grid">
  <a class="dvz-audience-card" href="../explanation/architecture/">
    <span class="dvz-audience-card__label">Understand</span>
    <strong>Understand the engine</strong>
    <span>Learn the object model, architecture, frame lifecycle, and retained-resource model.</span>
  </a>
  <a class="dvz-audience-card" href="runtime-internals/">
    <span class="dvz-audience-card__label">Build</span>
    <strong>Work on runtime and backends</strong>
    <span>Follow scene output through DRP2 to native Vulkan or browser WebGPU execution.</span>
  </a>
  <a class="dvz-audience-card" href="../contributors/architecture-map/">
    <span class="dvz-audience-card__label">Contribute</span>
    <strong>Contribute</strong>
    <span>Find subsystem owners, source-of-truth specifications, and validation workflows.</span>
  </a>
  <a class="dvz-audience-card" href="../contributors/release-process/">
    <span class="dvz-audience-card__label">Release</span>
    <strong>Prepare a release</strong>
    <span>Use the maintainer process, flight checklist, artifact checks, and release evidence.</span>
  </a>
</div>

To learn the GPU foundations hands-on, follow [Modern GPU Graphics in Vulkan](../gpu-graphics/index.md), which builds a renderer from an empty C file.

## From scene to output

Retained scene state becomes a bounded frame plan, an immutable frame artifact, and a DRP2 command
stream. The selected runtime executes that stream through native Vulkan or browser WebGPU and
produces presentation, capture, or readback output.

<div class="dvz-layer-diagram" role="list" aria-label="Datoviz scene to output architecture">
  <div class="dvz-layer-diagram__step" role="listitem">
    <strong>Retained scene</strong>
    <span>Semantic objects</span>
  </div>
  <div class="dvz-layer-diagram__step" role="listitem">
    <strong>FramePlan</strong>
    <span>Bounded work</span>
  </div>
  <div class="dvz-layer-diagram__step" role="listitem">
    <strong>Frame artifact</strong>
    <span>Immutable snapshot</span>
  </div>
  <div class="dvz-layer-diagram__step" role="listitem">
    <strong>DRP2</strong>
    <span>Command stream</span>
  </div>
  <div class="dvz-layer-diagram__step" role="listitem">
    <strong>Runtime</strong>
    <span class="dvz-layer-diagram__branches"><span>Native Vulkan</span><span>Browser WebGPU</span></span>
  </div>
  <div class="dvz-layer-diagram__step" role="listitem">
    <strong>Output</strong>
    <span>Present, capture, readback</span>
  </div>
</div>

## Concepts

- [Figure, panel, and visual model](../explanation/figure-panel-visual-model.md)
- [Coordinate systems](../explanation/coordinate-systems.md)
- [Interaction model](../explanation/interaction-model.md)
- [Queries, picking, and probing](../explanation/query-pick-probe-model.md)
- [Datoviz, GSP, and VisPy2](../explanation/gsp-vispy2-boundary.md)

## Scene planning

- [Architecture](../explanation/architecture.md)
- [Scene-to-runtime boundary](../explanation/scene-to-runtime-boundary.md)
- [Retained resources](../explanation/retained-resources.md)
- [Invalidation and caching](../explanation/invalidation-and-caching.md)
- [Frame lifecycle](../explanation/frame-lifecycle.md)
- [GPU resource ownership](../explanation/gpu-resource-ownership.md)

## Learn GPU graphics

- [Modern GPU Graphics in Vulkan](../gpu-graphics/index.md) — a course that builds a renderer from an empty
  C file up to an interactive textured and lit 3D mesh, using `vklite` and the canvas.

## Runtime and portability

- [Runtime internals](runtime-internals.md)
- [DRP2 command streams](drp2-command-streams.md) — advanced/unstable
- [WebGPU subset](../reference/webgpu-subset.md) — experimental
- [WebGPU example matrix](../examples/webgpu-matrix.md)
- [Compute and graphics](../reference/compute-graphics.md) — experimental
- [Share Datoviz buffers with CUDA](cuda-external-memory.md) — experimental, Linux/NVIDIA only
- [Deploy to the web](../how-to/deploy-to-web.md)
- [Embed in Qt](../how-to/embed-in-qt.md)
- [Record and replay](../how-to/record-replay.md)

## Contributors

- [Architecture map](../contributors/architecture-map.md)
- [Add a DRP2 command](../contributors/adding-a-drp2-command.md)
- [Add a WebGPU fixture](../contributors/adding-a-webgpu-fixture.md)
- [Add a public gallery example](../contributors/adding-examples.md)
- [Select examples by capability](../contributors/example-selection-by-capability.md)
- [Generated documentation](../contributors/generated-documentation.md)

Read the repository `AGENTS.md` before editing. Durable behavior belongs in `spec/`, public guidance
in `docs/`, and current execution status in `agents/`.

## Release maintainers

- [Release process](../contributors/release-process.md)
- [Release flight checklist](../contributors/release-flight-checklist.md)
- [Release wheels](../contributors/release-wheels.md)
- [Release validation](../contributors/release-validation.md)
- [Physical release validation](../contributors/release-physical-validation.md)
- [Agent release checklist](../contributors/agent-release-checklist.md)
- [Validation gallery](../examples/validation-gallery.md)
