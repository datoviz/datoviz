# How-To Documentation Writing Plan

Status: ready to execute. This is a self-contained briefing for a new agent session.

## Goal

Write all how-to documentation pages for the datoviz v0.4 website. There are 29 pages total: 3
already complete, 26 to write. All writing is **C tabs first**; Python tabs come later after user
review.

**Phase 1 (infrastructure) is already complete** — nav restructured, files renamed, new stubs
created, conversion script built. Your job is Phase 2 only: write the 26 how-to pages.

Pages within each batch can run fully in parallel via subagents.

---

## Background: key conventions

**Public API vs. internal runner pattern.** The C examples in `examples/c/features/` and
`examples/c/showcases/` use an internal `DvzScenarioSpec`/`DvzScenarioContext` runner. Do NOT copy
that pattern into documentation. Doc snippets must use the raw public API as shown in
`docs/start/quickstart.md`. Use C examples only as reference for what scene/visuals to construct;
rewrite the boilerplate using the quickstart pattern.

**Python tabs.** All pages currently need only C code. Leave Python tabs as `<!-- TODO: Python -->` 
comment stubs. Python will be added in a later pass after user review.

**Comments in code.** Per `CLAUDE.md`: comments go on their own line above the code they describe.
Never inline to the right of a line.

**Commits.** Per `CLAUDE.md`: no `Co-Authored-By:` trailers. Short subject line only. Group
related changes; do not make one commit per file.

**Style reference.** `docs/start/quickstart.md` is the canonical style and format reference for
both prose and code snippets.

---

## Agreed how-to structure (final)

All files live in `docs/how-to/`. Nav lives in `mkdocs.yml` lines ~103–135.

```
How-To
├── Scene Basics
│   ├── create-a-scene.md           (stub → write)
│   ├── add-a-visual.md             (stub → write)
│   ├── update-visual-data.md       (stub → write)
│   └── render-offscreen.md         (stub → write)
├── Coordinate Systems & Scales     (NEW group)
│   ├── coordinate-systems.md       (new file → write)
│   └── transforms-and-scales.md    (new file → write)
├── Visuals
│   ├── choose-a-visual-family.md   (stub → write)
│   ├── use-colormaps.md            (stub → write)
│   └── use-sampled-fields.md       (stub → write)
├── Layout
│   ├── create-multiple-panels.md   (stub → write)
│   ├── axes.md                     (renamed from add-axes.md → write)
│   └── adornments.md               (renamed from add-colorbars.md → write)
├── Interaction
│   ├── use-panzoom.md              (stub → write)
│   ├── 3d-navigation.md            (new file → write; covers arcball/turntable/fly)
│   └── input-events.md             (new file → write)
├── 3D Rendering                    (NEW group)
│   ├── lighting-and-materials.md   (new file → write)
│   └── rendering-techniques.md     (new file → write)
├── Animation & Video               (NEW group)
│   ├── animation.md                (new file → write)
│   └── video-export.md             (new file → write)
├── GUI                             (NEW group)
│   └── gui-controls.md             (new file → write)
├── Integration & Diagnostics
│   ├── c-integration.md            ✓ COMPLETE — do not touch
│   ├── embed-in-qt.md              ✓ COMPLETE — do not touch
│   ├── use-raw-ctypes.md           ✓ COMPLETE — do not touch
│   ├── deploy-to-web.md            (new file → write)
│   ├── debug-rendering.md          (stub → write; absorbs replay-dvzr.md)
│   └── profile-performance.md      (stub → write)
└── Walkthroughs
    ├── image-colorbar-probe.md     (stub → write; was "image, colorbar, and probe")
    ├── mesh-arcball.md             (stub → write)
    └── multi-panel-figure.md       (stub → write)
```

**Remove from nav but keep files** (they may be useful later):
`create-a-window.md`, `capture-an-image.md`, `add-annotations.md`,
`pick-and-probe.md`, `select-items.md`, `replay-dvzr.md`, `use-arcball.md`

---

## Phase 1 — Infrastructure (do this first, in sequence)

### 1a. Rename files

```sh
cd docs/how-to
git mv add-axes.md axes.md
git mv add-colorbars.md adornments.md
```

### 1b. Create new stub files

Create these files with a minimal stub (title + one-line description):
- `docs/how-to/coordinate-systems.md`
- `docs/how-to/transforms-and-scales.md`
- `docs/how-to/3d-navigation.md`
- `docs/how-to/input-events.md`
- `docs/how-to/lighting-and-materials.md`
- `docs/how-to/rendering-techniques.md`
- `docs/how-to/animation.md`
- `docs/how-to/video-export.md`
- `docs/how-to/gui-controls.md`
- `docs/how-to/deploy-to-web.md`

### 1c. Update mkdocs.yml nav

Replace the How-To section (lines ~103–135) with the new structure above.
The new nav block:

```yaml
  - How-To:
    - Scene Basics:
      - Create a scene: 'how-to/create-a-scene.md'
      - Add a visual: 'how-to/add-a-visual.md'
      - Update visual data: 'how-to/update-visual-data.md'
      - Render offscreen: 'how-to/render-offscreen.md'
    - Coordinate Systems & Scales:
      - Coordinate systems: 'how-to/coordinate-systems.md'
      - Transforms and scales: 'how-to/transforms-and-scales.md'
    - Visuals:
      - Choose a visual family: 'how-to/choose-a-visual-family.md'
      - Use colormaps: 'how-to/use-colormaps.md'
      - Use sampled fields: 'how-to/use-sampled-fields.md'
    - Layout:
      - Multiple panels: 'how-to/create-multiple-panels.md'
      - Axes: 'how-to/axes.md'
      - Adornments: 'how-to/adornments.md'
    - Interaction:
      - Panzoom: 'how-to/use-panzoom.md'
      - 3D navigation: 'how-to/3d-navigation.md'
      - Input events: 'how-to/input-events.md'
    - 3D Rendering:
      - Lighting and materials: 'how-to/lighting-and-materials.md'
      - Rendering techniques: 'how-to/rendering-techniques.md'
    - Animation & Video:
      - Animation: 'how-to/animation.md'
      - Video export: 'how-to/video-export.md'
    - GUI:
      - GUI controls: 'how-to/gui-controls.md'
    - Integration & Diagnostics:
      - Use from C or C++: 'how-to/c-integration.md'
      - Embed in Qt: 'how-to/embed-in-qt.md'
      - Use raw ctypes: 'how-to/use-raw-ctypes.md'
      - Deploy to web: 'how-to/deploy-to-web.md'
      - Debug rendering: 'how-to/debug-rendering.md'
      - Profile performance: 'how-to/profile-performance.md'
    - Walkthroughs:
      - Scatter with probe: 'how-to/image-colorbar-probe.md'
      - 3D mesh with arcball: 'how-to/mesh-arcball.md'
      - Multi-panel figure: 'how-to/multi-panel-figure.md'
```

### 1d. C→Python preprocessing script (already built)

`tools/c_to_python_skeleton.py` is already written. **Important:** run it on the clean
documentation C snippets you write (quickstart style), NOT on the internal `examples/c/features/`
files which use helper macros the script cannot handle. Workflow:

1. Write the C tab for a page
2. Save just the C code block to a temp file
3. `python3 tools/c_to_python_skeleton.py tmp.c` → skeleton
4. Complete the skeleton (numpy data, lifecycle) by hand

### 1e. Build the C→Python preprocessing script (SKIP — already done)

Create `tools/c_to_python_skeleton.py`. It takes a C code block (stdin or file) and outputs a
Python skeleton with mechanical substitutions applied, ready for an agent to complete the
non-mechanical parts (numpy data setup, lifecycle).

Mechanical substitutions:
- Strip `#include` lines
- Strip type declarations: `DvzFoo* var =` → `var =`, `DvzFoo var =` → `var =`
- Strip type casts: `(DvzFoo*)` → remove
- `NULL` → `None`
- `true` / `false` → `True` / `False`
- `dvz_foo(` → `dvz.dvz_foo(`
- `DVZ_FOO_BAR` → look up in a simple table; for enums, emit `dvz.DvzFooFlag.DVZ_FOO_BAR`
  (agent must verify the actual enum class name)
- `dvz_visual_set_data(visual, "attr", data, N)` → `dvz.dvz_visual_set_data(visual, "attr", data)`
  (drop trailing count arg when preceded by numpy array)
- `dvz_app(scene)` + `dvz_view_glfw(...)` + `dvz_app_run(app, 0)` + `dvz_app_destroy(app)` +
  `dvz_scene_destroy(scene)` → collapse to `dvz.run(scene, figure, title="...")`
- C-style array initializers (`float pos[N*3] = {0}`) → emit `# TODO: numpy` comment
- `int main(void) {` / closing `}` → strip

Mark non-mechanical sections with `# TODO: numpy` or `# TODO: verify`. The output is a skeleton
for an agent to complete, not a finished file.

### 1e. Commit Phase 1

Single commit: `docs: restructure how-to nav and add new page stubs`

---

## Phase 2 — Writing pages (parallel subagents, C tabs first)

Run the batches below in order (Batch 1 before Batch 2, etc.), but all pages *within* a batch can
run in parallel. Walkthroughs (Batch 6) depend on all earlier pages being done.

For each page, the subagent should:
1. Read the relevant C example(s) listed below
2. Read `docs/start/quickstart.md` for style/format reference
3. Read the existing stub file (for any intent notes)
4. Write the page following the template below
5. Commit when done (short message, no Co-Authored-By)

### Page template

```markdown
# Page Title

One-sentence description of what this page covers and when to use it.

## Overview (optional — only if setup context is needed)

Brief prose, 2–4 sentences max.

## Example

=== "C"

    ```c
    /* minimal complete example using the raw public API */
    ```

<!-- TODO: Python -->

## Step by step

Brief explanation of each key step in the example. One paragraph per step, not a bullet list.

## Common patterns / Variants (optional)

Short code snippets for important variants, without full boilerplate.

## See also

Links to related how-to pages or reference pages.
```

Rules:
- Never use the `DvzScenarioSpec` / `DvzScenarioContext` pattern. Use the raw API (quickstart
  style): `dvz_scene()`, `dvz_figure()`, `dvz_panel_full()`, `dvz_app()`, `dvz_view_glfw()`, etc.
- Keep examples self-contained and compilable. Minimal includes, no internal helpers.
- Comments go on their own line above the code they describe. No inline comments.
- Python tab: leave as `<!-- TODO: Python -->` for now.
- Page length: aim for 80–150 lines of markdown. How-to pages are not tutorials; they assume
  competence and get to the point.

---

## Page-to-C-example mapping

All C example files live in `examples/c/features/` unless noted as `showcases/`.

### Batch 1 — Scene Basics

| Page | C examples |
|------|-----------|
| `create-a-scene.md` | `features/basic_scene.c`, `features/app_glfw.c` |
| `add-a-visual.md` | `visuals/point.c`, `visuals/marker.c`, `features/basic_scene.c` |
| `update-visual-data.md` | `features/update_visual_data.c`, `features/update_partial.c`, `features/visibility.c` |
| `render-offscreen.md` | `features/offscreen_capture.c` + offscreen section of quickstart |

### Batch 2 — Coordinate Systems & Scales + Visuals

| Page | C examples |
|------|-----------|
| `coordinate-systems.md` | `features/coordinate_system.c`, `features/panel_view2d.c` |
| `transforms-and-scales.md` | `features/visual_transform.c`, `features/user_scale.c` |
| `choose-a-visual-family.md` | All `visuals/*.c` (read the directory listing + MANIFEST.yaml) |
| `use-colormaps.md` | `features/colormap_scale.c`, `features/colorbar.c` |
| `use-sampled-fields.md` | `features/sampled_field_2d.c`, `features/sampled_field_3d.c` |

### Batch 3 — Layout + Interaction

| Page | C examples |
|------|-----------|
| `create-multiple-panels.md` | `features/panel_multi.c`, `features/panel_grid.c`, `features/panel_linked.c` |
| `axes.md` | `features/axes_2d.c`, `features/axis_labels.c`, `features/datetime_axis.c` |
| `adornments.md` | `features/colorbar.c`, `features/overlay_card.c`, `features/text_block.c`, `features/annotation_readout.c`, `features/scalebar.c`, `features/scalebar_units.c`, `features/guide_lines.c`, `features/guide_spans.c`, `features/legend_categorical.c` |
| `use-panzoom.md` | `features/panzoom.c`, `features/panel_view2d.c` |
| `3d-navigation.md` | `features/controller_arcball.c`, `features/controller_turntable.c`, `features/controller_fly.c`, `features/controller_orbit_camera.c` |
| `input-events.md` | `features/input_events.c` |

### Batch 4 — 3D Rendering + Animation & Video + GUI

| Page | C examples |
|------|-----------|
| `lighting-and-materials.md` | `features/lighting.c`, `features/material_mesh.c`, `features/mesh_texture.c` |
| `rendering-techniques.md` | `features/technique_edl.c`, `features/technique_ssao.c`, `features/technique_msaa.c`, `features/technique_depth_cue.c`, `features/technique_transparency.c`, `features/alpha_blending.c` |
| `animation.md` | `features/timer_animation.c`, `features/animation_tracks.c`, `features/compute_buffer_animation.c` |
| `video-export.md` | `features/video_export.c` |
| `gui-controls.md` | `features/gui_controls.c`, `features/gui_cimgui.c`, `features/gui_viewport.c` |

### Batch 5 — Integration new pages + Diagnostics

| Page | C examples / sources |
|------|---------------------|
| `deploy-to-web.md` | Read `docs/explanation/portability-webgpu.md`, `docs/reference/webgpu-subset.md`; no C example — this is a deployment guide |
| `debug-rendering.md` | `features/record_replay.c`, `features/json_export.c`; also read `docs/contributors/docs-authoring.md` for existing validation commands |
| `profile-performance.md` | `features/timer_animation.c`; general frame timing guidance |

### Batch 6 — Walkthroughs (write last)

| Page | C examples |
|------|-----------|
| `image-colorbar-probe.md` | `showcases/linked_probe_colorbar.c`, `features/image_probe.c`, `features/colorbar.c` |
| `mesh-arcball.md` | `visuals/mesh.c`, `features/controller_arcball.c`, `features/material_mesh.c`, `features/technique_depth_test.c` |
| `multi-panel-figure.md` | `features/panel_linked.c`, `showcases/panel_linked_axes.c`, `showcases/scientific_plotting.c` |

---

## C→Python conversion notes (for the later Python pass)

When adding Python tabs, these patterns need special handling beyond the mechanical script:

- **Data arrays**: C flat arrays (`float pos[N*3]`) → numpy (`np.zeros((N, 3), dtype=np.float32)`).
  Always use explicit dtype.
- **App lifecycle**: C uses `dvz_app()` + `dvz_view_glfw()` + `dvz_app_run()` + `dvz_app_destroy()`
  + `dvz_scene_destroy()`. Python collapses to `dvz.run(scene, figure, title="...")`.
- **Offscreen**: C `dvz_view_offscreen()` + `dvz_view_capture_png()` → Python
  `dvz.capture(scene, figure, path="output.png")`.
- **Enum classes**: check actual Python enum class names in the generated binding; they may differ
  from a naive `DvzFooFlag` guess.
- **NULL controllers**: `dvz_panzoom(scene, NULL)` — the `NULL` is a parent/allocator arg; pass
  `None` in Python.
- **Struct literals**: C `DvzPointStyleDesc style = dvz_point_style_desc()` — check if Python
  exposes a constructor or uses keyword args.

---

## Validation

After each page is written, run:

```sh
git diff --check
git status --short
```

The full doctest (`just doctest`) will be wired up in a later infrastructure pass.

---

## Files to read before starting

1. `docs/start/quickstart.md` — canonical style and API pattern reference
2. `agents/now/START.md` — project context
3. `agents/now/STATUS.md` — current blockers
4. `CLAUDE.md` — commit rules and documentation comment style
5. `examples/c/MANIFEST.yaml` — visual family metadata
