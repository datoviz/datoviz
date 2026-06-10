# Documentation Information Architecture

Datoviz v0.4 uses a Diataxis-inspired documentation model, adapted for a low-level rendering engine
with a first-class executable example catalog.

The public documentation should make one positioning point immediately clear: Datoviz is the
engine layer. Most scientific users should eventually use VisPy2/GSP for high-level plotting.
Datoviz documentation is for native application developers, backend authors, advanced users of the
C/raw-binding surface, and contributors.

`mkdocs.yml` is the source of truth for the concrete public navigation. This document mirrors the
current MkDocs structure and records the intent behind each section. If a page is moved, added, or
removed in `mkdocs.yml`, update this file in the same change.


## Audiences

| Audience | Primary Need | Documentation Emphasis |
| --- | --- | --- |
| Native application developer | Embed Datoviz, open windows, render offscreen, capture frames | tutorials, how-to, runtime reference |
| Backend or adapter developer | Use Datoviz under GSP, VisPy2, hosted UIs, or WebGPU | explanations, reference, contributor docs |
| Advanced scientific developer | Use the C API or raw `ctypes` directly | examples, how-to, C reference |
| Contributor or coding agent | Modify visuals, scene, DRP2, runtime, docs, or tests | contributor docs, architecture explanations |
| Scientific end user | Plot data productively | redirected to VisPy2/GSP when high-level plotting is wanted |


## Top-Level Structure

Use these public documentation sections, as declared in `mkdocs.yml`:

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

Page roles should stay distinct:

| Page type | Purpose |
| --- | --- |
| Examples | executable truth and release proof |
| Reference | exact facts, status, signatures, attributes, and limits |
| How-to | practical task workflows that adapt examples to real code |
| Tutorials | narrative learning paths through stable composed workflows |
| Explanation | concepts, architecture, and tradeoffs |

Do not force a one-to-one relationship between features and how-to pages. The required coverage unit
for each public visual or feature is a minimal example. How-to pages should group related tasks, and
tutorials should wait until the examples they teach are stable.


## Live WebGPU Embeds

Live WebGPU examples are a separate runtime surface, not ordinary documentation content. They load
WASM, WebGPU JavaScript, GPU resources, canvas state, route-specific scenario data, and browser
event handling. Keep that runtime isolated from MkDocs pages.

Use this structure for public gallery examples:

1. gallery index pages stay screenshot-first and link to detail pages;
2. detail pages may embed one lazy live WebGPU iframe when the manifest marks the route
   `webgpu-live`;
3. every embedded iframe must also have a standalone link to the same route;
4. browser JavaScript remains host glue for the canonical C example or portable C scenario;
5. generated route checks must verify that iframe and standalone links resolve under the built site.

The canonical route shape is:

```text
examples/webgpu/live.html?id=<example-id>
```

Do not inline the WebGPU runtime directly into generated Markdown pages. Inline integration would
couple MkDocs rendering to the browser runtime, make ordinary documentation pages heavier, and make
runtime failures more likely to break page rendering. Iframes are the default boundary because they
give the WebGPU route its own document, scripts, canvas, query parameters, permissions, and
debuggable standalone URL.


## Legacy And Unlisted Docs

The current MkDocs configuration excludes legacy or out-of-scope files from the built v0.4 site:

```text
README.md
quickstart.md
architecture/*
blog/*
discussions/*
gallery/*
guide/*
tasks/*
visuals/*
reference/api_c.md
reference/api_py.md
reference/colormaps.md
```

MkDocs also marks these paths as intentionally outside the navigation:

```text
_old/*
architecture/*
blog/*
discussions/*
gallery/*
guide/*
tasks/*
visuals/*
```

Do not treat these excluded or unlisted legacy paths as the active v0.4 documentation structure.


## Start

Purpose: orient readers and prevent wrong expectations.

Current MkDocs navigation:

```text
start/
  Overview/
    what-is-datoviz.md
    choose-your-layer.md
    project-status.md
  Setup/
    install.md
    build-from-source.md
  First Steps/
    first-c-program.md
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

Current MkDocs navigation:

```text
tutorials/
  index.md
  Basics/
    first-scene.md
    interactive-window.md
    offscreen-capture.md
  Scene Workflows/
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

Purpose: provide minimal executable truth for visuals, features, composites, techniques, and
showcases.

Examples are a public documentation pillar, not an appendix. The rule is:

1. one public visual family gets one dedicated minimal C example;
2. one public feature gets one dedicated minimal C example;
3. one public semantic composite gets one dedicated minimal C example;
4. showcases may compose multiple visuals, features, and composites, but they must not replace
   minimal examples.

Current MkDocs navigation:

```text
examples/
  index.md
  Galleries/
    visual-gallery.md
    feature-gallery.md
    techniques.md
  Release Proof/
    showcases.md
    validation-gallery.md
```

Detailed coverage rules are in [EXAMPLE_COVERAGE.md](EXAMPLE_COVERAGE.md).


## How-To

Purpose: answer practical task questions.

How-to guides are still needed even when every feature has a minimal example. Examples show the
smallest working code. How-to guides explain which pattern to use, which ownership rules matter, and
how to adapt the example to real code.

Current MkDocs navigation:

```text
how-to/
  Scene And Runtime/
    create-a-scene.md
    create-a-window.md
    render-offscreen.md
    capture-an-image.md
  Visuals And Data/
    add-a-visual.md
    choose-a-visual-family.md
    update-visual-data.md
    use-sampled-fields.md
    use-colormaps.md
  Layout And Adornments/
    create-multiple-panels.md
    add-axes.md
    add-colorbars.md
    add-annotations.md
  Interaction/
    use-panzoom.md
    use-arcball.md
    pick-and-probe.md
    select-items.md
  Integration And Debugging/
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

Current MkDocs navigation:

```text
reference/
  index.md
  Status And Support/
    feature-status.md
    platform-support.md
    build-options.md
  API/
    c-api/index.md
    ctypes.md
    drp2/index.md
  Scene Reference/
    objects-and-lifetimes.md
    visual-families/index.md
    visual-attributes.md
    coordinate-systems.md
    controllers.md
    queries.md
    callbacks.md
    errors-and-logging.md
  Backends/
    webgpu-subset.md
```

Reference pages should prefer tables, status labels, signatures, constraints, and links to examples.
They should avoid tutorial prose, but visual reference pages should still contain short authored
judgment where it helps users choose correctly.

Visual reference pages should follow one template:

```text
Visual: <name>
Status:
Backends:
Use when:
Avoid when:
Data model:
Required attributes:
Optional attributes:
Controllers:
Picking/probing:
Backend notes:
Minimal example:
Related how-to:
```

The exact API facts in visual reference pages may be generated or table-driven. The choice guidance
such as "use when", "avoid when", and comparisons with neighboring visual families should be
authored prose.


## Explanation

Purpose: explain concepts, architecture, and tradeoffs.

Current MkDocs navigation:

```text
explanation/
  System/
    architecture.md
    why-datoviz.md
    gsp-vispy2-boundary.md
  Scene/
    scene-model.md
    figure-panel-visual-model.md
    coordinate-systems.md
    interaction-model.md
  Runtime/
    scene-to-drp2-runtime.md
    frame-lifecycle.md
    retained-resources.md
    invalidation-and-caching.md
    gpu-resource-ownership.md
  Advanced/
    query-pick-probe-model.md
    performance-model.md
    portability-webgpu.md
```

Explanation pages should answer why the system is shaped as it is. They should be explicit about
boundaries so contributors and coding agents do not create parallel renderers, presentation layers,
or runtime contracts.


## Contributors

Purpose: document how humans and agents change Datoviz safely.

Current MkDocs navigation:

```text
contributors/
  Orientation/
    architecture-map.md
    build-and-test.md
    coding-style.md
  Documentation/
    docs-authoring.md
    ai-agents.md
    adding-examples.md
  Development/
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
