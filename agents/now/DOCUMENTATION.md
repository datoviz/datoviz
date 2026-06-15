# Datoviz v0.4 Documentation Plan

Status: active documentation roadmap. Updated: 2026-06-15.


## Next Steps (agent-ready, in priority order)

Decisions behind these tasks are in `spec/docs/V04_DOCUMENTATION_DECISIONS.md`. Read that first.

### 1. `wind_globe.c` showcase example
Spec: `spec/scene/examples/scenarios/WIND_GLOBE.md`. Self-contained, fully specced.
Earth mesh + wind vector arrows + streamlines in 3D, arcball controller, dark background.
Extends `showcases/textured_planet.c` and `showcases/wind_field.c`.
Output: 1600×900 PNG hero screenshot.

### 2. Doc restructure — MkDocs nav
Reorganize `mkdocs.yml` and `docs/` from the current 7-section Diataxis structure to the
5-section structure decided in `V04_DOCUMENTATION_DECISIONS.md`:
Get Started / Examples / How-To / Reference / Advanced.
Reconcile `spec/docs/INFORMATION_ARCHITECTURE.md` with `V04_DOCUMENTATION_DECISIONS.md`.
Move existing pages into the new structure; stub missing pages.

### 3. Start Here + Quickstart pages
Write two new pages:
- `docs/start/index.md` — orientation page serving both humans and LLMs: what datoviz is,
  capability→URL map, task→URL map, layer→URL map, 3-4 minimal inline code patterns.
  This is the page the prompt widget links to as primary context.
- `docs/start/quickstart.md` — "rendering in 10 minutes": scatter plot, 10k random 3D points,
  pan/zoom controller. C version first, Python ctypes version immediately below or in a tab.
  Zero external data dependencies — synthetic random data only.

### 4. Visual family pages
Bring each visual family page to the standard template:
description, parameter table, minimal C example, minimal ctypes example, screenshot.
Visual families: point, primitive, image, mesh, path, segment, marker, sphere, volume,
pixel, glyph, text, label, splat.
Screenshots come from existing `data/gallery/v0.4/visuals/` assets where available.

### 5. Prompt widget
Static JavaScript widget embedded in the docs site. No backend, no LLM, fully deterministic.
Free text input → structured header/footer wrapping the user's text → copy button +
"Open in Claude" / "Open in ChatGPU" links.
Depends on: Start Here page existing (its URL is injected into the prompt header).
Spec: see AI-Assisted Workflow section in `V04_DOCUMENTATION_DECISIONS.md`.

### 6. Pyodide live playground
Pyodide-based Python editor in the docs. User writes Python, it calls the existing
`datoviz_wasm_scene.mjs` WASM module via Pyodide JS FFI, renders via the existing WebGPU runtime.
Architecture and constraints: see Live Playground section in `V04_DOCUMENTATION_DECISIONS.md`.
Depends on: nothing — WASM build already exists and is unmodified.
Status: RC milestone.

### 7. Hero image composition
Pillow script that composites four real datoviz screenshots onto the graphite background.
Reference: `docs/assets/references/hero_reference_panels.jpg`.
Depends on: wind_globe.c render (task 1), signal traces + ImGui screenshot (TBD).
Signal traces: use existing `scientific_plotting_workflow` bottom panel as base, or create a
dedicated new showcase with visible ImGui controls.

Keep API, scene, DRP2, and documentation architecture contracts in `spec/`. Keep this file focused
on release documentation gates.


## Source-Of-Truth Boundaries

1. Release sequencing: [RELEASE.md](RELEASE.md).
2. Current blockers: [STATUS.md](STATUS.md).
3. Public API conventions: [../../spec/api/PUBLIC_API_CONVENTIONS.md](../../spec/api/PUBLIC_API_CONVENTIONS.md).
4. Python/GSP scope: [../../spec/api/PYTHON_GSP_SCOPE.md](../../spec/api/PYTHON_GSP_SCOPE.md).
5. Scene API and semantics: [../../spec/scene/](../../spec/scene/).
6. DRP2 commands, schemas, fixtures, and runtime contracts: [../../spec/drp2/](../../spec/drp2/).
7. Documentation IA, example coverage, and AI-friendly docs rules: [../../spec/docs/](../../spec/docs/).
8. Python binding import layers and array-aware facade:
   [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md).
9. Release readiness, RC process, communication, and gallery outreach:
   [../../spec/release/](../../spec/release/).

The public `docs/` tree may be rebuilt in place for v0.4. Do not recreate the v0.3 Python-first
guide as current Datoviz documentation.


## Required Public Docs

1. README that presents v0.4 as the current branch surface.
2. Install/build guide with supported platforms, dependencies, CMake, and `just` commands.
3. Feature/status table using `supported`, `experimental`, `advanced/unstable`, `deferred`, and
   `external/GSP`.
4. Known issues and unsupported variants.
5. Datoviz/GSP/VisPy2 positioning note.
6. RC and final release notes.
7. Example/gallery index with screenshots or captured artifacts where appropriate.
8. Generated C reference or complete API reference outline.
9. Python binding documentation covering top-level array-aware direct-engine use and exact
   `datoviz.raw` `ctypes` use.
10. WebGPU/WASM experimental-scope documentation.
11. Compute+graphics experimental-scope documentation, including the portable DRP2 subset, native
    synchronization boundary, and optional CUDA SDK example status.
12. WebGPU live-example matrix generated from example metadata, classifying every public example as
    `webgpu-live`, `webgpu-planned`, `webgpu-deferred`, or `native-only`, with live routes linked
    from gallery pages when available.
13. Gallery/data attribution policy for public datasets, generated media, and reuse in release
    communication.
14. Short contributor guidance for AI-assisted docs/example work: page type, source of truth,
    validation command, and unsupported-feature status.


## RC1 Gate

Required:

1. public header inventory;
2. public surface/status table;
3. ownership and destroy-rule notes for public objects;
4. callback, polling, and readback lifetime notes;
5. Python binding scope, including the planned top-level array-aware facade and exact
   `datoviz.raw` layer;
6. WebGPU/WASM experimental scope and known gaps;
7. compute+graphics experimental scope and CUDA/CuPy boundary;
8. WebGPU live-example matrix with requirement tags for compute, request/query/readback,
   native capture, GUI, video, CUDA, and desktop app/runtime dependencies;
9. clear statement that browser request/query/readback is asynchronous and that the RC slice is
   limited to promoted query families, not full native query parity;
10. clear statement that `webgpu-live` pages reuse the same canonical C example or portable C
    scenario as native validation, with browser JavaScript limited to host glue;
11. clear statement that old high-level Pythonic Datoviz API migration is outside v0.4 Datoviz
   scope.

The Python binding scope page may land before the generated API reference is complete. For RC1, it
needs to define the intended `import datoviz as dvz` array-aware facade, exact `datoviz.raw` import
style, generated-binding status, ownership and callback lifetime rules, validation commands, and
the boundary with GSP/VisPy2. Do not hand-maintain exhaustive C or raw binding symbol catalogs in
prose.


## RC2 Gate

Required:

1. documentation structure mostly final;
2. generated C reference or complete outline;
3. usable Python binding docs for the top-level facade and raw `ctypes`;
4. release examples with captured artifacts;
5. render-conformance or fixture result linked;
6. known issues updated from RC1 feedback.
7. real-dataset showcase candidates reviewed for license, attribution, scientific context, and
   whether the visualization would be useful to the dataset authors.
8. gallery media generation path documented for screenshots, animated GIFs, or short videos.

The generated C/raw API outline should come from parsed public headers. The first candidate source
is `build/bindings/datoviz_api.json`, produced by the raw-binding API extraction pipeline.


## Final Gate

Required:

1. final feature table;
2. final known issues and limitations;
3. install/build and positioning notes published;
4. Python binding and WebGPU/WASM scopes published;
5. website, gallery, and release announcement assets published.
6. public dataset examples include source links, license/citation notes, prepared-data provenance,
   and any required permissions.
7. direct outreach drafts for selected dataset authors are reviewed before sending.


## Validation

Documentation-only:

```sh
git diff --check
git status --short
```

Generated references, examples, screenshots, or inventories need the narrowest relevant generation,
build, or smoke command recorded in the final note.
