# MkDocs Navigation Restructure

Status: ready for agent implementation.
Decisions: `spec/docs/V04_DOCUMENTATION_DECISIONS.md`.
Current nav source of truth: `mkdocs.yml`.


## Goal

Simplify the current 7-section nav (Start, Tutorials, Examples, How-To, Reference, Explanation,
Contributors) to 5 sections (Get Started, Examples, How-To, Reference, Advanced).

Primary audience is Python scientists. Top-level nav should never surface contributor or
architecture content unless the user actively seeks it.


## Section Mapping

### Get Started (new, absorbs Start + Tutorials)

```yaml
- Get Started:
  - Start Here: 'start/index.md'               # NEW — write from scratch, see below
  - Install: 'start/install.md'                # keep, light edit
  - Build from source: 'start/build-from-source.md'  # keep
  - Quickstart: 'start/quickstart.md'          # NEW — replaces first-c-program.md
  - First scene (C + Python): 'start/first-c-program.md'  # rename/rewrite as quickstart
  - Interactive window: 'start/interactive-window.md'     # moved from tutorials/
  - Offscreen capture: 'start/offscreen-capture.md'       # moved from tutorials/
  - Project status: 'start/project-status.md'             # keep
  - AI-assisted workflow: 'start/ai-workflow.md'          # NEW — prompt widget page
```

Drop from Start: `what-is-datoviz.md` (content absorbed into `start/index.md`),
`choose-your-layer.md` (content absorbed into `start/index.md`).


### Examples (unchanged)

Keep exactly as-is. No changes needed.


### How-To (unchanged + absorb tutorial walkthroughs)

Add the longer tutorial walkthroughs as new How-To entries:

```yaml
  - Scene And Runtime:
    - ...existing entries...
  - Visuals And Data:
    - ...existing entries...
  - Layout And Adornments:
    - ...existing entries...
  - Interaction:
    - ...existing entries...
  - Guides:                                    # NEW subsection
    - Mesh with arcball: 'how-to/mesh-arcball.md'           # moved from tutorials/
    - Multi-panel figure: 'how-to/multi-panel-figure.md'    # moved from tutorials/
    - Image, colorbar, probe: 'how-to/image-colorbar-probe.md'  # moved from tutorials/
  - Integration And Debugging:
    - ...existing entries...
```


### Reference (unchanged, minus DRP2)

Remove the DRP2 entry from the public nav — DRP2 is an internal transport layer, not a user
surface (see `V04_DOCUMENTATION_DECISIONS.md`, Layer Model section).

```yaml
  - API:
    - C API: ...keep...
    - Raw ctypes: 'reference/ctypes.md'    # keep
    # Remove: DRP2: 'reference/drp2/index.md'
```

Everything else in Reference stays exactly as-is.


### Advanced (new, absorbs Explanation + Contributors)

```yaml
- Advanced:
  - Architecture:
    - Architecture overview: 'explanation/architecture.md'
    - Why Datoviz: 'explanation/why-datoviz.md'
    - GSP and VisPy2 boundary: 'explanation/gsp-vispy2-boundary.md'
    - Scene model: 'explanation/scene-model.md'
    - Figure, panel, visual model: 'explanation/figure-panel-visual-model.md'
    - Coordinate systems: 'explanation/coordinate-systems.md'
    - Interaction model: 'explanation/interaction-model.md'
  - Internals:
    - Scene to DRP2 runtime: 'explanation/scene-to-drp2-runtime.md'
    - Frame lifecycle: 'explanation/frame-lifecycle.md'
    - Retained resources: 'explanation/retained-resources.md'
    - Invalidation and caching: 'explanation/invalidation-and-caching.md'
    - GPU resource ownership: 'explanation/gpu-resource-ownership.md'
    - Query, pick, and probe model: 'explanation/query-pick-probe-model.md'
    - Performance model: 'explanation/performance-model.md'
    - Portability and WebGPU: 'explanation/portability-webgpu.md'
  - Lower Layers:
    - vklite: 'advanced/vklite.md'                    # stub if not written
    - Canvas and stream API: 'advanced/canvas.md'     # stub if not written
    - WebGPU renderer: 'advanced/webgpu-renderer.md'  # stub if not written
  - Contributors:
    - Architecture map: 'contributors/architecture-map.md'
    - Build and test: 'contributors/build-and-test.md'
    - Coding style: 'contributors/coding-style.md'
    - Docs authoring: 'contributors/docs-authoring.md'
    - AI agents: 'contributors/ai-agents.md'
    - Agent quickstart: 'contributors/agent-quickstart.md'
    - Adding examples: 'contributors/adding-examples.md'
    - Adding a visual: 'contributors/adding-a-visual.md'
    - Release validation: 'contributors/release-validation.md'
```


## New Pages To Write

### `docs/start/index.md` — Start Here

This page serves both humans and LLMs. It is the page the prompt widget links to.

Structure (strict — do not deviate):

1. **One paragraph** describing datoviz v0.4: what it is, what it is not, primary API surface,
   relationship to VisPy2/GSP. Written so an LLM can use it as context verbatim.

2. **Capability map** — "I want to display X":

   | I want to display... | Go to |
   | --- | --- |
   | Scatter / point cloud | [Point visual](...) |
   | Line / path / trajectory | [Path visual](...) |
   | Mesh / surface | [Mesh visual](...) |
   | Volume / 3D scalar field | [Volume visual](...) |
   | Image / texture | [Image visual](...) |
   | Text / labels | [Text visual](...) |
   | Markers / symbols | [Marker visual](...) |
   | Spheres (3D impostor) | [Sphere visual](...) |
   | ... | ... |

3. **Task map** — "I want to do Y":

   | I want to... | Go to |
   | --- | --- |
   | Pan and zoom | [Panzoom controller](...) |
   | Rotate in 3D | [Arcball controller](...) |
   | Render offscreen / headless | [Offscreen capture](...) |
   | Add a colorbar | [Add colorbars](...) |
   | Pick / probe data | [Pick and probe](...) |
   | Add ImGui controls | [GUI integration](...) |
   | Run in the browser | [WebGPU / WASM](...) |
   | Update data in real time | [Update visual data](...) |
   | ... | ... |

4. **Layer map** — "I want to use layer Z":

   | Layer | Use when | Go to |
   | --- | --- | --- |
   | Scene (C / Python) | Building apps or visualizations | [Quickstart](...) |
   | vklite | Writing a Vulkan renderer | [vklite](...) |
   | WebGPU renderer | Embedding the renderer | [WebGPU renderer](...) |
   | Canvas + stream | Custom renderer, GLFW/Qt, video | [Canvas and stream](...) |

5. **Four minimal code patterns** (inline, no explanation):
   - Create a scene and run
   - Add a visual and set data
   - Update data in a timer callback
   - Capture offscreen to PNG

   Show Python ctypes version for each. C version linked to quickstart.

6. **Prompt widget** — embedded JS widget for AI-assisted code generation.
   (Widget implementation: see `V04_DOCUMENTATION_DECISIONS.md`, Prompt Widget section.)


### `docs/start/quickstart.md` — Quickstart

"Rendering in 10 minutes."

Structure:
1. Prerequisites (one line: install done, see install.md)
2. The example: scatter plot, 10k random 3D points, pan/zoom, dark background
3. C version: complete, runnable, ~40 lines, no boilerplate comments
4. Python ctypes version: complete, runnable, immediately below or in a tab
5. What you should see: one sentence + screenshot
6. Next: link to Examples gallery and Start Here capability map

Rules:
- Synthetic random data only — no file loading, no external dependencies
- Raw ctypes in Python — no wrapper classes
- Both versions must be kept in sync with the actual example in `examples/c/visuals/point.c`
  and its ctypes equivalent
- No explanatory comments in the code — the page prose explains; code is clean


### `docs/start/ai-workflow.md` — AI-Assisted Workflow

Short page explaining the intended AI-assisted workflow:
1. Brief: datoviz is designed to be used with AI coding assistants
2. The prompt widget (embedded, same widget as on Start Here)
3. The live playground (link, brief description)
4. Tips for prompting: mention the LLM entry point URL, ask for ctypes not C if you want Python,
   ask for synthetic data first then adapt to your real data


## Pages To Delete Or Archive

- `start/what-is-datoviz.md` — content absorbed into `start/index.md`
- `start/choose-your-layer.md` — content absorbed into `start/index.md`
- `tutorials/index.md` — section removed; content redistributed
- `tutorials/first-scene.md` — replaced by `start/quickstart.md`
- `tutorials/interactive-window.md` — moved to `start/`
- `tutorials/offscreen-capture.md` — moved to `start/`
- `tutorials/image-colorbar-probe.md` — moved to `how-to/`
- `tutorials/mesh-arcball.md` — moved to `how-to/`
- `tutorials/multi-panel-figure.md` — moved to `how-to/`

Do not delete files until the new pages are written and `mkdocs.yml` is updated. Move content,
then delete originals.


## Implementation Order

1. Write `start/index.md` (Start Here) — highest priority, unblocks prompt widget
2. Write `start/quickstart.md` — second priority
3. Update `mkdocs.yml` nav — do this after new pages exist to avoid broken links
4. Move tutorial pages to new locations, update internal links
5. Stub `advanced/vklite.md`, `advanced/canvas.md`, `advanced/webgpu-renderer.md`
6. Delete old pages
7. Update `spec/docs/INFORMATION_ARCHITECTURE.md` to reflect the new structure


## Validation

```sh
mkdocs build --strict   # zero warnings, zero broken links
mkdocs serve            # visual check of nav and pages
```
