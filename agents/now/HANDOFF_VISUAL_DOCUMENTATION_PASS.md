# Public Documentation Visual Pass Handoff

Status: approved direction; implementation has not started. Updated: 2026-07-16.

This handoff defines the next public-documentation visual pass. Implement the pilot checkpoint
first, render it locally, and ask for maintainer review before applying the system broadly.


## Mission

Make `Get Started` and `Advanced` easier to understand visually without turning the documentation
into a marketing site or decorating pages that are already clear. Use real Datoviz output to show
results and precise schematics to explain hierarchy, choice, ownership, and sequence.

The pass should answer these questions quickly:

1. What will a new user build?
2. Which Python, C, browser, or advanced path should they choose?
3. How do scene objects and runtime layers fit together?
4. How does retained state become GPU work and output?


## Read First

1. `AGENTS.md`;
2. `agents/now/START.md`;
3. `agents/now/DOCUMENTATION.md`;
4. `agents/now/HANDOFF_DOCS_AUDIT_REWRITE.md`;
5. `spec/docs/V04_DOCUMENTATION_DECISIONS.md`;
6. `spec/docs/INFORMATION_ARCHITECTURE.md`;
7. `spec/scene/README.md` before drawing scene or runtime semantics;
8. `spec/drp2/README.md` before drawing DRP2 packet, lifetime, or execution semantics.

Verify architectural labels against the current public pages and durable specs. A diagram is not
an excuse to simplify a boundary into something false.


## Current Baseline

The home page already establishes the desired dark graphite, cyan, mint, amber, and rose visual
language. `docs/stylesheets/extra.css` contains its richer homepage and gallery patterns.

The section pages have a smaller reusable foundation in `docs/stylesheets/content.css`:

- `dvz-section-intro`;
- `dvz-section-grid` and `dvz-section-card`;
- `dvz-context-strip`.

`docs/start/index.md` uses the section intro and routing cards. Several start pages use context
chips, but most conceptual content remains prose, tables, ASCII trees, and code. The Quickstart has
a real scatter screenshot, but it appears after much of the implementation. `docs/advanced/index.md`
is well routed but mostly prose and tables. Architecture, runtime, DRP2, frame, coordinate,
interaction, and query pages repeat text diagrams that do not yet form one consistent mental model.

No Mermaid, Graphviz, or PlantUML renderer is configured or installed. Do not add a diagram
dependency during the pilot.


## Approved Visual Approach

1. Use HTML/CSS for responsive flows, layer stacks, audience cards, decision paths, status blocks,
   and simple containment diagrams.
2. Use hand-authored SVG when spatial layout, branching, transforms, or annotations become awkward
   in HTML. Keep SVG source directly editable in the repository.
3. Use real existing gallery output for screenshots. Do not use mockups or AI-generated technical
   diagrams.
4. Prefer static WebP output on ordinary documentation pages. Reserve autoplay video for the home,
   blog, and gallery surfaces.
5. Give an important page at most one dominant teaching visual, with a second only when it explains
   a distinct concept.
6. Keep code and tables when they are the clearest representation. Pair code with output; do not
   replace exact reference material with pictures.
7. Use cyan for the supported primary path, amber for experimental surfaces, muted cyan for
   advanced/unstable surfaces, mint for optional paths, and rose for errors or stop conditions.
   Always include a text status label; color alone is insufficient.
8. Every screenshot needs useful alt text, a concise caption, and a link to the canonical example
   or source page. Every schematic needs a textual equivalent in nearby prose.


## Reusable Components

Extend `docs/stylesheets/content.css` rather than adding page-specific CSS repeatedly. Start with
these components and add only what the pilot proves necessary:

| Component | Purpose |
| --- | --- |
| `dvz-doc-hero` | Two-column introduction with text/actions and one real output image. |
| `dvz-output-example` | Rendered result with caption, source link, and optional adjacent code summary. |
| `dvz-step-flow` | Numbered workflow that becomes vertical on narrow screens. |
| `dvz-layer-diagram` | Supported/experimental/advanced layer or architecture stack. |
| `dvz-object-diagram` | Nested scene, figure, panel, visual, data, and controller relationships. |
| `dvz-sequence` | Event, packet, frame, query, or release sequence. |
| `dvz-audience-grid` | Reader-goal routing cards with Material icons. |

Use existing Material icons. Do not introduce an icon package. Keep component names and styles
generic enough for multiple pages, but do not build a framework beyond demonstrated needs.


## Pilot Checkpoint

Implement these four pages first:

### 1. Get Started overview

File: `docs/start/index.md`.

- Add a two-column hero using the existing Quickstart scatter output.
- Put Python Quickstart and First C Program actions beside the result.
- Replace the numbered prose under `Recommended first path` with a four-step visual path:
  `Install -> Create a scene -> Add data -> Interact or capture`.
- Keep the existing routing cards, adding small Material icons only where they improve scanning.

### 2. Core concepts

File: `docs/start/core-concepts.md`.

- Replace the ASCII object tree with a responsive nested schematic:
  `Scene -> Figure -> Panel -> Visual -> named data arrays`, with the controller attached to the
  panel and multiple panels visibly possible.
- Preserve the exact object table and explanatory prose as the text equivalent.
- If practical, annotate a real rendered panel to connect figure, panel, visual marks, and
  controller interaction to visible output.

### 3. Choose your layer

File: `docs/start/choose-your-layer.md`.

- Add a decision/layer diagram above the detailed table.
- Show GSP/VisPy2 as external work, the scene API as the recommended supported path, browser WebGPU
  as experimental, and app/canvas/stream/DRP2/vklite surfaces as advanced where appropriate.
- Do not present DRP2 as an ordinary user-facing entry point.

### 4. Advanced overview

File: `docs/advanced/index.md`.

- Add one canonical architecture map:
  `retained scene -> FramePlan -> frame artifact -> DRP2 -> native Vulkan or browser WebGPU -> output`.
- Add four audience cards: understand the engine, work on runtime/backend code, contribute, and
  prepare a release.
- Retain the status/stability warning and detailed routing tables below the visual overview.

After these four pages, run all validation below and ask the maintainer to review the local site at
desktop and mobile widths. Do not continue to the broad rollout until the visual density, colors,
diagram language, and media choices are approved.


## Get Started Rollout

After pilot approval:

| Page | Visual purpose |
| --- | --- |
| `start/what-is-datoviz.md` | Pair representative 2D and 3D output with a concise engine-position diagram. |
| `start/install.md` | Python wheel, installed C/C++, and source-build decision flow; no screenshots. |
| `start/build-from-source.md` | Dependencies, configure, build, and validate sequence. |
| `start/quickstart.md` | Move the actual output near the top and pair the result with the complete Python/C path. |
| `start/first-c-program.md` | Show output plus create, attach, run, and destroy lifecycle. |
| `start/core-concepts.md` | Extend the approved object model only if retained updates need a second schematic. |
| `start/choose-your-layer.md` | Apply pilot feedback to the final routing diagram. |
| `start/ai-workflow.md` | Documentation context, assistant, generated code, and local validation flow. |


## Advanced And Explanation Rollout

Use one stable architecture vocabulary across these pages. Do not create divergent pipeline labels
for each page.

| Page or cluster | Visual purpose |
| --- | --- |
| `explanation/architecture.md` | Canonical layer ownership and output path. |
| `explanation/scene-to-runtime-boundary.md` | Semantic decisions above the boundary, execution decisions below it. |
| `advanced/runtime-internals.md` | Native Vulkan and browser WebGPU execution branches with owners. |
| `advanced/drp2-command-streams.md` | Setup, update, frame phases and logical-resource lifetime. |
| `explanation/frame-lifecycle.md` | Planning, submission, presentation, capture, readback, and result timing. |
| `explanation/retained-resources.md` | Persistent state versus per-frame artifacts and target resources. |
| `explanation/invalidation-and-caching.md` | Mutation, dirty dependencies, and bounded rebuild. |
| `explanation/gpu-resource-ownership.md` | Owned, borrowed, artifact-scoped, and in-flight resources. |
| `explanation/coordinate-systems.md` | Data, world, panel, clip, logical-pixel, and framebuffer transforms. |
| `explanation/interaction-model.md` | Input, controller, scene update, and next-frame sequence. |
| `explanation/query-pick-probe-model.md` | Request, GPU work, asynchronous completion, and retained result. |
| `explanation/gsp-vispy2-boundary.md` | High-level plotting above the Datoviz engine boundary. |

Contributor and release pages may receive compact checkpoint flows after the conceptual pages are
complete. Leave short procedures textual when a diagram would only restate a list.


## Broader Audit Rule

After Get Started and Advanced, inspect How-To and Reference pages with this test:

1. Expected visible result: add a real screenshot.
2. Spatial or ownership relationship: add a schematic.
3. Ordered state transition: add a sequence or flow.
4. API, platform, or audience choice: add a decision map.
5. Exact lookup table or signature catalog: keep it primarily textual.

Do not decorate generated C API pages or exhaustive reference tables. Change generated pages only
through their generator or metadata source.


## Asset And Repository Rules

1. Reuse existing gallery poster assets through the current MkDocs asset-injection path.
2. Put new hand-authored diagrams under a focused path such as
   `docs/assets/diagrams/start/` or `docs/assets/diagrams/advanced/` only when an external SVG is
   preferable to inline semantic HTML.
3. Do not edit, stage, or commit the `data` submodule or new media there without explicit approval
   in the current turn.
4. Do not commit `site/`, build-local WebP output, videos, generated binaries, or unrelated user
   changes.
5. If a required screenshot does not already exist, record the missing canonical example or media
   need and ask before broadening into gallery capture work.


## Validation

For each checkpoint:

```sh
just docs-build-check
git diff --check
git status --short
```

Also:

1. inspect the rendered pages at a normal desktop width and at the mobile breakpoint;
2. confirm no component causes horizontal page overflow;
3. verify diagrams remain understandable without color and have nearby text equivalents;
4. verify image alt text, captions, source links, and lazy loading;
5. verify reduced-motion behavior when a reused component contains motion;
6. confirm internal links and all referenced media survive the strict MkDocs build;
7. inspect the staged set and commit one logical checkpoint at a time.

Run `just docs` only when a checkpoint intentionally changes generated documentation inputs. It is
not required for ordinary hand-authored page or CSS changes.


## Out Of Scope

1. Navigation restructuring beyond small links needed by the pilot.
2. Public API, scene, runtime, or binding semantic changes.
3. A Mermaid, Graphviz, PlantUML, or new JavaScript diagram dependency.
4. Decorative AI-generated imagery or bitmap architecture diagrams.
5. A broad rewrite of already-correct prose.
6. Gallery capture, media regeneration, or `data` submodule work without separate approval.
7. Visual decoration of generated API reference pages.


## Commit Sequence

Prefer these local checkpoints:

1. reusable visual components plus the four-page pilot;
2. remaining Get Started pages;
3. Advanced overview, architecture, and runtime foundation;
4. explanation diagrams by related concept cluster;
5. contributor/release flows and final consistency audit.

Run the relevant checks before each commit. Publication or push still requires the maintainer's
explicit instruction in that turn.
