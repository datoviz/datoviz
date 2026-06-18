# Nav Restructure Plan

Status: completed in `mkdocs.yml`. Durable nav policy now lives in
`spec/docs/NAV_RESTRUCTURE.md`; this file is retained only as historical execution context.

## Goal

Restructure `mkdocs.yml` to reflect the agreed final documentation architecture and keep the nav
aligned with the v0.4 documentation contract:

1. **Examples** are the executable source of truth, screenshots, and release proof.
2. **How-To** pages are task workflows that explain how to adapt canonical examples.
3. **Reference** pages are exact facts: API, statuses, attributes, lifetimes, and backend limits.
4. **Internals** is intentionally small and only exposes lower-layer architecture for advanced
   runtime/backend work.
5. **Contributing** is a top-level maintainer/developer section.

Read `CLAUDE.md` before starting: short commit messages, no Co-Authored-By trailers.

---

## Agreed final nav structure

```
Home  |  Get Started  |  Examples  |  How-To  |  Reference  |  Internals  |  Contributing
```

### Get Started — unchanged
Keep exactly as-is:
- Install: `start/install.md`
- Quickstart: `start/quickstart.md`
- AI-assisted workflow: `start/ai-workflow.md`

### Examples — unchanged
Keep exactly as-is.

### How-To — task-oriented rewrite

Do **not** keep the current How-To section unchanged. It contains hidden stubs, overlapping pages,
and stale hand-written examples. The final How-To nav should be:

```yaml
- How-To:
  - Core Workflow:
    - Create a scene: 'how-to/create-a-scene.md'
    - Open an interactive window: 'how-to/create-a-window.md'
    - Render offscreen and capture: 'how-to/render-offscreen.md'
    - Add visuals to a panel: 'how-to/add-a-visual.md'
    - Update visual data: 'how-to/update-visual-data.md'
    - Animate a scene: 'how-to/animation.md'
  - Data To Visuals:
    - Choose a visual family: 'how-to/choose-a-visual-family.md'
    - Use coordinate systems: 'how-to/coordinate-systems.md'
    - Transform visual data: 'how-to/transforms-and-scales.md'
    - Map scalar values with colormaps: 'how-to/use-colormaps.md'
    - Use sampled fields and textures: 'how-to/use-sampled-fields.md'
  - Layout:
    - Create multiple panels: 'how-to/create-multiple-panels.md'
    - Link panels and controllers: 'how-to/link-panels.md'
    - Add axes: 'how-to/axes.md'
    - Add colorbars, scale bars, and legends: 'how-to/adornments.md'
    - Add text, labels, and annotations: 'how-to/add-annotations.md'
  - Interaction:
    - Use panzoom: 'how-to/use-panzoom.md'
    - Use 3D controllers: 'how-to/3d-navigation.md'
    - Handle input events: 'how-to/input-events.md'
    - Pick items: 'how-to/pick-and-probe.md'
    - Probe image or field values: 'how-to/probe-fields.md'
    - Select and highlight data: 'how-to/select-items.md'
  - Rendering:
    - Configure cameras: 'how-to/configure-cameras.md'
    - Use lighting and materials: 'how-to/lighting-and-materials.md'
    - Control depth, blending, and transparency: 'how-to/rendering-techniques.md'
    - Profile rendering performance: 'how-to/profile-performance.md'
  - Output:
    - Save screenshots: 'how-to/capture-an-image.md'
    - Export videos: 'how-to/video-export.md'
    - Record and replay frame streams: 'how-to/replay-dvzr.md'
  - Integration:
    - Use from C or C++: 'how-to/c-integration.md'
    - Use from Python: 'how-to/use-python.md'
    - Use raw ctypes: 'how-to/use-raw-ctypes.md'
    - Embed in Qt: 'how-to/embed-in-qt.md'
    - Deploy WebGPU examples to the browser: 'how-to/deploy-to-web.md'
  - Diagnostics:
    - Debug rendering output: 'how-to/debug-rendering.md'
    - Diagnose WebGPU support: 'how-to/debug-webgpu.md'
    - Diagnose build and platform issues: 'how-to/diagnose-platform.md'
```

How-To pages should link to canonical examples and gallery pages instead of duplicating full source
programs. Use short call sequences and snippets only when they explain the task.

### Reference — restructured

```yaml
- Reference:
  - Overview: 'reference/index.md'
  - API:
    - C API:
      - Overview: 'reference/c-api/index.md'
      - Scene: 'reference/c-api/scene.md'
      - Visuals and composites: 'reference/c-api/visuals.md'
      - App, window, and I/O: 'reference/c-api/app.md'
      - Runtime and utilities: 'reference/c-api/runtime.md'
      - C types: 'reference/c-api/types.md'
    - Python binding: 'reference/ctypes.md'
  - Visual families:
    - Overview: 'reference/visual-families/index.md'
    - Point: 'reference/visual-families/point.md'
    - Path: 'reference/visual-families/path.md'
    - Mesh: 'reference/visual-families/mesh.md'
    - Volume: 'reference/visual-families/volume.md'
    - Image: 'reference/visual-families/image.md'
    - Text: 'reference/visual-families/text.md'
    - Marker: 'reference/visual-families/marker.md'
    - Sphere: 'reference/visual-families/sphere.md'
    - Segment: 'reference/visual-families/segment.md'
    - Vector: 'reference/visual-families/vector.md'
    - Pixel: 'reference/visual-families/pixel.md'
    - Glyph: 'reference/visual-families/glyph.md'
    - Splat: 'reference/visual-families/splat.md'
    - Primitive: 'reference/visual-families/primitive.md'
    - Labels: 'reference/visual-families/labels.md'
  - Core reference:
    - Object lifetimes: 'reference/objects-and-lifetimes.md'
    - Coordinate systems: 'reference/coordinate-systems.md'
    - Controllers: 'reference/controllers.md'
    - Callbacks and events: 'reference/callbacks.md'
  - Compatibility:
    - Feature status: 'reference/feature-status.md'
    - Platform support: 'reference/platform-support.md'
    - Build options: 'reference/build-options.md'
  - Backends:
    - WebGPU subset: 'reference/webgpu-subset.md'
    - Compute and graphics: 'reference/compute-graphics.md'
```

Changes from current:
- Sub-group "Status And Support" → **Compatibility**
- Sub-group "Scene Reference" → **Core reference** (drop `visual-attributes`, `queries`,
  `errors-and-logging` — add to `not_in_nav`)
- Sub-group "API" gains `Python binding` (was "Raw ctypes" outside a sub-group)
- Add `reference/visual-families/labels.md` to visual families (was missing from nav)
- `project-status.md` moved to `not_in_nav` — it duplicates `feature-status.md`
  (verify this before dropping; if they cover different things keep both under Compatibility)

### Internals — renamed from "Advanced", restructured

```yaml
- Internals:
  - Architecture:
    - Why Datoviz: 'explanation/why-datoviz.md'
    - Scene model: 'explanation/scene-model.md'
    - Performance model: 'explanation/performance-model.md'
    - Portability and WebGPU: 'explanation/portability-webgpu.md'
  - Lower layers:
    - vklite: 'advanced/vklite.md'
    - Canvas and stream API: 'advanced/canvas.md'
    - WebGPU renderer: 'advanced/webgpu-renderer.md'
```

Changes from current "Advanced":
- Rename section "Advanced" → **Internals**
- Architecture sub-group: keep only 4 pages (why-datoviz, scene-model, performance-model,
  portability-webgpu)
- Drop from nav → add to `not_in_nav`:
  - `explanation/architecture.md`
  - `explanation/figure-panel-visual-model.md`
  - `explanation/coordinate-systems.md`  (covered in Reference)
  - `explanation/interaction-model.md`   (covered in How-To)
  - `explanation/gsp-vispy2-boundary.md` (covered in start/choose-your-layer.md)
  - `explanation/frame-lifecycle.md`
  - `explanation/retained-resources.md`
  - `explanation/invalidation-and-caching.md`
  - `explanation/gpu-resource-ownership.md`
  - `explanation/query-pick-probe-model.md`
  - `explanation/scene-to-drp2-runtime.md`
- Remove entire "Contributors" sub-group from here (moved to top-level Contributing section)

### Contributing — new top-level section (promoted from Advanced > Contributors)

```yaml
- Contributing:
  - Getting started:
    - Build and test: 'contributors/build-and-test.md'
    - Coding style: 'contributors/coding-style.md'
  - Adding content:
    - Adding examples: 'contributors/adding-examples.md'
    - Adding a visual: 'contributors/adding-a-visual.md'
    - Docs authoring: 'contributors/docs-authoring.md'
  - AI workflows:
    - AI agents: 'contributors/ai-agents.md'
    - Agent quickstart: 'contributors/agent-quickstart.md'
  - Release:
    - Release process: 'contributors/release-process.md'
    - Release flight checklist: 'contributors/release-flight-checklist.md'
    - Release wheels: 'contributors/release-wheels.md'
    - Release validation: 'contributors/release-validation.md'
    - Validation gallery: 'examples/validation-gallery.md'
    - WebGPU matrix: 'examples/webgpu-matrix.md'
```

Note: `contributors/architecture-map.md` dropped → add to `not_in_nav`.

---

## not_in_nav additions

Add these lines to the `not_in_nav:` block in `mkdocs.yml`:

```
  explanation/architecture.md
  explanation/figure-panel-visual-model.md
  explanation/coordinate-systems.md
  explanation/interaction-model.md
  explanation/gsp-vispy2-boundary.md
  explanation/frame-lifecycle.md
  explanation/retained-resources.md
  explanation/invalidation-and-caching.md
  explanation/gpu-resource-ownership.md
  explanation/query-pick-probe-model.md
  explanation/scene-to-drp2-runtime.md
  reference/visual-attributes.md
  reference/queries.md
  reference/errors-and-logging.md
  contributors/architecture-map.md
```

Also verify whether `reference/project-status.md` is genuinely redundant with
`reference/feature-status.md` — if so add to `not_in_nav`, otherwise keep under Compatibility.

---

## Verification

After updating `mkdocs.yml`, run:

```sh
python -m mkdocs build --strict 2>&1 | head -50
```

Fix any "file not found" or "not in nav" warnings before committing.
If `mkdocs` is not available, run `git diff --check && git status --short` at minimum.

Commit: `docs: restructure public docs nav`
