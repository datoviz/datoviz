# Documentation Information Architecture

Datoviz v0.4 uses a Diataxis-inspired documentation model, adapted for a low-level rendering engine
with a first-class executable example catalog.

The public documentation should make one positioning point immediately clear: Datoviz is the
engine layer. Most scientific users should eventually use VisPy2/GSP for high-level plotting.
Datoviz documentation is for native application developers, backend authors, advanced users of the
C/raw-binding surface, and contributors.


## Audiences

| Audience | Primary Need | Documentation Emphasis |
| --- | --- | --- |
| Native application developer | Embed Datoviz, open windows, render offscreen, capture frames | tutorials, how-to, runtime reference |
| Backend or adapter developer | Use Datoviz under GSP, VisPy2, hosted UIs, or WebGPU | explanations, reference, contributor docs |
| Advanced scientific developer | Use the C API or raw `ctypes` directly | examples, how-to, C reference |
| Contributor or coding agent | Modify visuals, scene, DRP2, runtime, docs, or tests | contributor docs, architecture explanations |
| Scientific end user | Plot data productively | redirected to VisPy2/GSP when high-level plotting is wanted |


## Top-Level Structure

Use these public documentation sections:

```text
docs/
  index.md
  start/
  tutorials/
  examples/
  how-to/
  reference/
  explanation/
  contributors/
```

This is Diataxis plus two Datoviz-specific additions:

1. `start/` gives immediate orientation and layer selection.
2. `examples/` is a first-class pillar because examples are executable release proof and the best
   input for coding agents.


## Start

Purpose: orient readers and prevent wrong expectations.

Suggested pages:

```text
start/
  what-is-datoviz.md
  choose-your-layer.md
  install.md
  build-from-source.md
  first-c-program.md
  project-status.md
```

`choose-your-layer.md` is required. It should explain:

1. use VisPy2/GSP for high-level scientific plotting;
2. use Datoviz C APIs for low-level rendering, embedding, native apps, and backend integration;
3. use raw `ctypes` only for low-level Python integration and smoke tests;
4. use DRP2/DVZR for command-stream portability and replay.


## Tutorials

Purpose: teach a first-time user by walking through a small complete workflow.

Tutorials should be few, polished, narrative, and backed by runnable C examples. They should not try
to enumerate the whole API.

Suggested pages:

```text
tutorials/
  first-scene.md
  interactive-window.md
  offscreen-capture.md
  image-colorbar-probe.md
  mesh-arcball.md
  multi-panel-figure.md
```

Tutorial rules:

1. start from a complete C example;
2. explain the minimum mental model needed for that example;
3. avoid large API tables;
4. end with links to related examples, how-to pages, reference pages, and explanations.


## Examples

Purpose: provide minimal executable truth for visuals, features, techniques, and showcases.

Examples are a public documentation pillar, not an appendix. The rule is:

1. one public visual family gets one dedicated minimal C example;
2. one public feature gets one dedicated minimal C example;
3. showcases may compose multiple visuals and features, but they must not replace minimal examples.

Suggested pages:

```text
examples/
  index.md
  visual-gallery.md
  feature-gallery.md
  techniques.md
  showcases.md
  validation-gallery.md
```

Detailed coverage rules are in [EXAMPLE_COVERAGE.md](EXAMPLE_COVERAGE.md).


## How-To

Purpose: answer practical task questions.

How-to guides are still needed even when every feature has a minimal example. Examples show the
smallest working code. How-to guides explain which pattern to use, which ownership rules matter, and
how to adapt the example to real code.

Suggested pages:

```text
how-to/
  create-a-scene.md
  create-a-window.md
  render-offscreen.md
  capture-an-image.md
  add-a-visual.md
  update-visual-data.md
  choose-a-visual-family.md
  use-sampled-fields.md
  use-colormaps.md
  add-axes.md
  add-colorbars.md
  add-annotations.md
  use-panzoom.md
  use-arcball.md
  pick-and-probe.md
  select-items.md
  create-multiple-panels.md
  embed-in-qt.md
  use-raw-ctypes.md
  replay-dvzr.md
  debug-rendering.md
  profile-performance.md
```

How-to rules:

1. stay task-focused;
2. link to minimal examples instead of duplicating them;
3. include validation commands where relevant;
4. call out ownership, lifetime, and backend limitations.


## Reference

Purpose: provide exact, complete, dry facts.

Suggested pages:

```text
reference/
  index.md
  feature-status.md
  platform-support.md
  build-options.md
  c-api/
  objects-and-lifetimes.md
  visual-families/
  visual-attributes.md
  coordinate-systems.md
  controllers.md
  queries.md
  callbacks.md
  errors-and-logging.md
  ctypes.md
  webgpu-subset.md
  drp2/
```

Reference pages should prefer tables, status labels, signatures, constraints, and links to examples.
They should avoid tutorial prose.

Visual reference pages should follow one template:

```text
Visual: <name>
Status:
Backends:
Data model:
Attributes:
Controllers:
Picking/probing:
Limitations:
Minimal example:
Related API:
```


## Explanation

Purpose: explain concepts, architecture, and tradeoffs.

Suggested pages:

```text
explanation/
  architecture.md
  why-datoviz.md
  scene-model.md
  figure-panel-visual-model.md
  coordinate-systems.md
  scene-to-drp2-runtime.md
  retained-resources.md
  frame-lifecycle.md
  invalidation-and-caching.md
  gpu-resource-ownership.md
  interaction-model.md
  query-pick-probe-model.md
  performance-model.md
  portability-webgpu.md
  gsp-vispy2-boundary.md
```

Explanation pages should answer why the system is shaped as it is. They should be explicit about
boundaries so contributors and coding agents do not create parallel renderers, presentation layers,
or runtime contracts.


## Contributors

Purpose: document how humans and agents change Datoviz safely.

Suggested pages:

```text
contributors/
  architecture-map.md
  build-and-test.md
  coding-style.md
  docs-authoring.md
  ai-agents.md
  adding-examples.md
  adding-a-visual.md
  adding-a-drp2-command.md
  adding-a-webgpu-fixture.md
  release-validation.md
```

Contributor pages may link to `spec/` and `agents/`, but public docs should not become execution
history or active task queues.


## Non-Goals

1. Do not document the v0.3 Pythonic API as a current Datoviz v0.4 surface.
2. Do not create a source-compatible migration guide from old Datoviz Python plotting to v0.4.
3. Do not duplicate VisPy2/GSP high-level plotting documentation.
4. Do not make DRP2, Vulkan, vklite, canvas, or stream internals the main user narrative.
5. Do not let showcase examples replace minimal examples.
