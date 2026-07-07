# Documentation Information Architecture

Datoviz v0.4 uses a Diataxis-inspired documentation model, adapted for a low-level rendering engine
with a first-class executable example catalog.

The public documentation should make one positioning point immediately clear: Datoviz is the
engine layer. Most scientific users should eventually use VisPy2/GSP for high-level plotting.
Datoviz documentation is for native application developers, backend authors, advanced users of the
C/Python binding surface, and contributors.

`mkdocs.yml` is the source of truth for the concrete public navigation. This document records the
intent behind each section. If a page is moved, added, or removed in `mkdocs.yml`, update this file
in the same change.


## Audiences

| Audience | Primary Need | Documentation Emphasis |
| --- | --- | --- |
| Native application developer | Embed Datoviz, open windows, render offscreen, capture frames | tutorials, how-to, runtime reference |
| Backend or adapter developer | Use Datoviz under GSP, VisPy2, hosted UIs, or WebGPU | explanations, reference, contributor docs |
| Advanced scientific developer | Use the C API or `datoviz.raw` exact calls directly | examples, how-to, C reference |
| Contributor or coding agent | Modify visuals, scene, DRP2, runtime, docs, or tests | contributor docs, architecture explanations |
| Scientific end user | Plot data productively | redirected to VisPy2/GSP when high-level plotting is wanted |


## Top-Level Structure

Use these public documentation sections, as declared in `mkdocs.yml`:

```text
docs/
  index.md
  start/
  examples/
  how-to/
  reference/
  explanation/   # exposed under Advanced / Architecture
  advanced/      # exposed under Advanced / Runtime layers
  contributors/  # exposed under Advanced / Contributors
```

This is a compact, Datoviz-specific structure:

1. `start/` gives immediate orientation and first render.
2. `examples/` is a first-class pillar because examples are executable release proof, gallery
   content, screenshots, and the best input for coding agents.
3. `how-to/` explains task workflows and points to canonical examples instead of duplicating them.
4. `reference/` provides exact facts, tables, signatures, attributes, statuses, and limits.
5. `explanation/`, `advanced/`, and `contributors/` are exposed through **Advanced**, not as the
   main user path.
6. Contributor and release-maintainer docs remain reachable, but they are secondary to the public
   user path.

Page roles should stay distinct:

| Page type | Purpose |
| --- | --- |
| Examples | executable truth and release proof |
| Reference | exact facts, status, signatures, attributes, and limits |
| How-to | practical task workflows that adapt examples to real code |
| Explanation | concepts, architecture, and tradeoffs |

Do not force a one-to-one relationship between features and how-to pages. The required coverage unit
for each public visual or feature is a minimal example. How-to pages should group related tasks, and
composed walkthroughs should live in the examples/gallery layer unless they teach a distinct task
workflow.


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
3. use `datoviz.raw` exact calls only for low-level Python integration and smoke tests;
4. use DRP2/DVZR for command-stream portability and replay.


## Examples

Purpose: provide minimal executable truth for visuals, features, composites, techniques, and
showcases.

Examples are a public documentation pillar, not an appendix. The rule is:

1. one public visual family gets one dedicated minimal C example;
2. one public feature gets one dedicated minimal C example;
3. one public semantic composite gets one dedicated minimal C example;
4. showcases may compose multiple visuals, features, and composites, but they must not replace
   minimal examples.

Detailed coverage rules are in [EXAMPLE_COVERAGE.md](EXAMPLE_COVERAGE.md).


## How-To

Purpose: answer practical task questions.

How-to guides are still needed even when every feature has a minimal example. Examples show the
smallest working code. How-to guides explain which pattern to use, which ownership rules matter, and
how to adapt the example to real code.

Target MkDocs navigation:

```text
how-to/
  Core Workflow/
    create-a-scene.md
    create-a-window.md
    render-offscreen.md
    add-a-visual.md
    update-visual-data.md
    animation.md
  Data To Visuals/
    choose-a-visual-family.md
    coordinate-systems.md
    transforms-and-scales.md
    use-colormaps.md
    use-sampled-fields.md
  Layout/
    multiple-panels.md
    link-panels.md
    axes.md
    adornments.md
    add-annotations.md
  Interaction/
    use-panzoom.md
    3d-navigation.md
    input-events.md
    pick-and-probe.md
    probe-fields.md
    select-items.md
  Rendering/
    configure-cameras.md
    lighting-and-materials.md
    depth-blending.md
    profile-performance.md
  Output/
    screenshots.md
    video-export.md
    record-replay.md
  Integration/
    c-integration.md
    use-python.md
    use-raw-ctypes.md
    embed-in-qt.md
    deploy-to-web.md
  Diagnostics/
    debug-rendering.md
    debug-webgpu.md
    diagnose-platform.md
```

How-to rules:

1. stay task-focused;
2. link to minimal examples and generated gallery detail pages instead of duplicating full source;
3. include validation commands where relevant;
4. call out ownership, lifetime, coordinate, async, and backend limitations;
5. show screenshots by reusing generated gallery media when the task has a visual result;
6. keep composed recipes in Examples unless the page teaches a reusable task pattern.

Every how-to page should generally follow this contract, with shorter variants allowed when the
task does not need every section:

1. title and one-sentence task statement;
2. task workflow or "use this when" decision guidance;
3. minimal sequence of calls or steps, using snippets only for the calls being explained;
4. canonical examples, linking both generated gallery detail pages and `examples/c/...` sources;
5. important details covering ownership, lifetime, coordinates, async behavior, backend support,
   validation, or other task-specific limits;
6. common mistakes;
7. see-also links to related how-to, reference, explanation, or example pages.

Do not add `<!-- TODO: Python -->` markers or expose the old v0.3 Pythonic plotting API as current
v0.4 API. Python how-to pages should document the active top-level NumPy-adapted call form and
`datoviz.raw` exact-call boundaries.


## Reference

Purpose: provide exact, complete, dry facts.

Target MkDocs navigation:

```text
reference/
  index.md
  citation.md
  API/
    c-api/index.md
    c-api/scene.md
    c-api/visuals.md
    c-api/app.md
    c-api/runtime.md
    c-api/types.md
    ctypes.md
  Visual families/
    visual-families/index.md
    visual-families/*.md
  Core reference/
    objects-and-lifetimes.md
    coordinate-systems.md
    controllers.md
    callbacks.md
    visual-attributes.md
    queries.md
    errors-and-logging.md
  Compatibility/
    project-status.md
    feature-status.md
    v03-visible-parity.md
    platform-support.md
    build-options.md
  Backends/
    webgpu-subset.md
    compute-graphics.md
```

Reference pages should prefer tables, status labels, signatures, constraints, and links to examples.
They should avoid tutorial prose, but visual reference pages should still contain short authored
judgment where it helps users choose correctly.

`citation.md` is the public software-citation page. It should stay concise and should record the
current preferred citation, Zenodo DOI status, JOSS paper status, and links to repository metadata.
Do not put release diary notes or private publication plans there.

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
Advanced/
  Runtime layers/
    reference/drp2/index.md
    advanced/vklite.md
    advanced/canvas.md
    advanced/webgpu-renderer.md
  Release maintainers/
    releases/index.md
    releases/v0.4.0rc1.md
    contributors/release-process.md
    contributors/release-flight-checklist.md
    contributors/release-wheels.md
    contributors/release-validation.md
```

Explanation pages should answer why the system is shaped as it is. They should be explicit about
boundaries so contributors and coding agents do not create parallel renderers, presentation layers,
or runtime contracts. The retained explanation pages that are intentionally reachable by link but
omitted from navigation are listed in `mkdocs.yml` `not_in_nav`; consolidate or delete them in a
dedicated explanation cleanup pass.


## Contributors

Purpose: document how humans and agents change Datoviz safely.

Current MkDocs navigation exposes release-maintainer pages under `Advanced`, not a general
contributor section:

```text
Advanced/
  Release maintainers/
    release-process.md
    release-flight-checklist.md
    release-wheels.md
    release-validation.md
    examples/validation-gallery.md
```

Advanced contributor appendices may remain outside the navigation when they are linked from the
main contributor pages:

1. `contributors/architecture-map.md`;
2. `contributors/adding-a-drp2-command.md`;
3. `contributors/adding-a-webgpu-fixture.md`;
4. `contributors/example-selection-by-capability.md`.

Contributor pages may link to `spec/` and `agents/`, but public docs should not become execution
history or active task queues. `CONTRIBUTING.md` is the short repository entry point; detailed
procedures belong in `docs/contributors/`.


## Non-Goals

1. Do not document the v0.3 Pythonic API as a current Datoviz v0.4 surface.
2. Do not create a source-compatible migration guide from old Datoviz Python plotting to v0.4.
3. Do not duplicate VisPy2/GSP high-level plotting documentation.
4. Do not make DRP2, Vulkan, vklite, canvas, or stream internals the main user narrative.
5. Do not let showcase examples replace minimal examples.
