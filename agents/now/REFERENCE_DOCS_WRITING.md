# Reference, Start, and Explanation Documentation Writing Plan

Status: ready to execute. Self-contained briefing — no prior conversation context needed.

## Goal

Write three batches of documentation pages in parallel using subagents:

- **15 visual family reference pages** (`docs/reference/visual-families/`)
- **3 start section pages** (`docs/start/`)
- **5 explanation pages** (`docs/explanation/`)

Total: 23 pages. All batches are independent and can run fully in parallel.
Phase 1 infrastructure is already done. Your job is writing only.

---

## Background: key conventions

**Public API pattern.** Code snippets use the raw public API as shown in
`docs/start/quickstart.md`. Do NOT use the internal `DvzScenarioSpec`/`DvzScenarioContext` runner
pattern found in `examples/c/features/`. Use C examples only as reference for what to render;
write boilerplate from the quickstart pattern.

**C tabs only.** All pages need only C code for now. Leave Python as `<!-- TODO: Python -->`.

**Comments in code.** Per `CLAUDE.md`: comments go on their own line above the code. Never inline.

**Commits.** Per `CLAUDE.md`: no `Co-Authored-By:` trailers. Short subject line. Group related
changes — commit per batch (not per file).

**Style reference.** `docs/start/quickstart.md` is the canonical format reference.

**Do not touch** `docs/how-to/` — a separate agent is writing those pages concurrently.

---

## Batch A — Visual family reference pages (15 pages)

All files in `docs/reference/visual-families/`. All are currently 3-line stubs.
C examples are in `examples/c/visuals/`.

### Template (from `docs/reference/visual-families/index.md`)

```markdown
# Visual Name

One-sentence description of what this visual renders and its primary use case.

| Property | Value |
|----------|-------|
| Status | supported / experimental |
| Backends | native, WebGPU |
| Primitive | points / lines / triangles |

## When to use

Prose: data shape and task where this visual is the best fit.

## Avoid when

Prose: neighboring visual families or deferred features to use instead.

## Attributes

| Name | Type | Required | Description |
|------|------|----------|-------------|

## Minimal example

=== "C"

    ```c
    /* minimal self-contained example using the raw public API */
    ```

<!-- TODO: Python -->

## See also

Links to related how-to pages and other visual families.
```

### Page-to-C-example mapping

| Page | C example | Notes |
|------|-----------|-------|
| `point.md` | `visuals/point.c` | |
| `marker.md` | `visuals/marker.c` | |
| `path.md` | `visuals/path.c` | |
| `segment.md` | `visuals/segment.c` | |
| `primitive.md` | `visuals/primitive.c` | triangle/line primitives |
| `mesh.md` | `visuals/mesh.c` | |
| `sphere.md` | `visuals/sphere.c` | |
| `image.md` | `visuals/image.c` | |
| `volume.md` | `visuals/volume.c` | |
| `text.md` | `visuals/text.c` | |
| `glyph.md` | `visuals/glyph.c` | signed-distance field glyphs |
| `pixel.md` | `visuals/pixel.c` | |
| `splat.md` | `visuals/splat.c` | Gaussian splats |
| `vector.md` | `visuals/vector.c` | arrow/vector field |
| `labels.md` | `visuals/labels.c` | |

Also read `examples/c/MANIFEST.yaml` for status, backend support, and requirement tags per visual.

Commit when done: `docs: write visual family reference pages`

---

## Batch B — Start section pages (3 pages)

Files in `docs/start/`. The other two start pages (`build-from-source.md`,
`choose-your-layer.md`) already have content — do not touch them.

### `first-c-program.md`

A C-focused companion to the quickstart. Target: someone who wants to write C, not Python.

Content:
- Brief intro (1 paragraph): what you'll build, what you need
- Full working C example: scene + figure + panel + point visual + panzoom + GLFW window
  Draw from `examples/c/visuals/point.c` for data setup, quickstart C tab for structure
- Build instructions per platform (Linux/macOS/Windows) — copy the build section from quickstart
- Brief "next steps" links to how-to pages

### `interactive-window.md`

Short bridge page. Target: someone who has the quickstart running and wants to understand
the window/run loop and add a controller.

Content:
- What `dvz_app()`, `dvz_view_glfw()`, `dvz_app_run()` do (2–3 sentences each)
- How to attach a controller to a panel (`dvz_panzoom`, `dvz_panel_bind_controller`)
- Short complete example
- Links to: quickstart, how-to/use-panzoom, how-to/3d-navigation

### `offscreen-capture.md`

Short bridge page. Target: someone who wants PNG output without a window (CI, scripts).

Content:
- What offscreen rendering is (1 paragraph)
- Show the offscreen path: `dvz_view_offscreen()`, `dvz_app_run(app, 1)`,
  `dvz_view_capture_png()`
- Short complete example (can draw from `examples/c/features/offscreen_capture.c` for the
  API calls, but write in raw public API style)
- Links to: quickstart, how-to/render-offscreen

Commit when done: `docs: write start section pages`

---

## Batch C — Explanation pages (5 pages)

Files in `docs/explanation/`. Write only these 5 — the others need deep internal knowledge
and should remain stubs.

These are conceptual pages, not code-heavy. Aim for 200–400 words each, minimal or no code.

### `why-datoviz.md`

Why does Datoviz exist? What gap does it fill?
- C rendering engine for GPU-accelerated scientific visualization
- Explicit control vs. high-level plotting (VisPy2/GSP handles that)
- WebGPU portability through DRP2
- Read `agents/now/START.md` and `docs/start/what-is-datoviz.md` for context

### `scene-model.md`

The retained scene hierarchy: scene → figure → panel → visual → attributes.
- What each object is responsible for
- Ownership and lifetime rules (who creates, who destroys)
- Read `docs/reference/objects-and-lifetimes.md` and any content in `spec/scene/README.md`

### `figure-panel-visual-model.md`

How figures, panels, and visuals compose:
- Figure = window-sized canvas, contains panels
- Panel = viewport with a controller and coordinate domain
- Visual = retained GPU object attached to a panel
- Data flows: attribute upload → GPU buffer → draw
- Read quickstart and `docs/start/what-is-datoviz.md` for surface-level; read
  `spec/scene/README.md` for depth

### `coordinate-systems.md`

The coordinate spaces in Datoviz and how data flows through them:
- Data coordinates (user domain, set via `dvz_panel_set_domain`)
- Normalized device coordinates (NDC, [-1, 1])
- Screen/pixel coordinates
- 3D convention: X right, Y up, Z toward viewer
- Read `examples/c/features/coordinate_system.c` header comment for the axis convention
- Cross-reference: how-to/coordinate-systems.md covers the practical side; this page is conceptual

### `gsp-vispy2-boundary.md`

Where Datoviz ends and GSP/VisPy2 begins:
- Datoviz v0.4 = engine + runtime + low-level Python ctypes binding
- High-level scientific plotting API = VisPy2/GSP (external project)
- Old Datoviz v0.3 Pythonic API is not part of v0.4 docs
- Read `docs/start/choose-your-layer.md` and `spec/api/PYTHON_GSP_SCOPE.md` if it exists

Commit when done: `docs: write explanation pages`

---

## Execution order

Run all three batches simultaneously. Within each batch, all pages run in parallel via subagents.

After all subagents complete, do a final `git diff --check && git status --short` check before
committing each batch.

---

## Files to read before starting

1. `docs/start/quickstart.md` — canonical API pattern and style reference
2. `docs/reference/visual-families/index.md` — visual family template
3. `examples/c/MANIFEST.yaml` — visual family status and backend support
4. `agents/now/START.md` — project context
5. `CLAUDE.md` — commit rules and comment style
