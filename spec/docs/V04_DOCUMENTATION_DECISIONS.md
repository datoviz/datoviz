# v0.4 Documentation Decisions

Decisions locked in June 2026 planning session. These override or extend INFORMATION_ARCHITECTURE.md
where they conflict. A future agent should reconcile the two files after implementing the
restructure described here.


## Tagline (landing page hero)

```
GPU scientific visualization at any scale — explore millions of data points in 2D and 3D
C and Python API · Vulkan and WebGPU · Desktop and browser
```

First line is the headline. Second line is the subtitle rendered as a dot-separated capability
strip below the headline.


## Top-Level Navigation Structure

Five sections, replacing the current seven-section Diataxis structure:

| Section | Purpose |
| --- | --- |
| **Get Started** | Install + first working example. Zero to rendering in one page. |
| **Examples** | The gallery — hero of the docs. Visual families, features, showcases, advanced. |
| **How-To** | Task-oriented guides. "How do I add a colorbar", "how do I go offscreen", etc. |
| **Reference** | API, visual family parameter tables, colormaps, build options. |
| **Advanced** | Lower-level layers for Vulkan/WebGPU/embedding developers. |

Rationale: simpler than Diataxis for the primary audience (Python scientists). The
explanation/architecture content folds into Advanced or How-To rather than a standalone section.
Contributors docs remain as a subsection of Advanced or a separate tab, not a top-level section.


## Primary Audience

Python scientists waiting for VisPy2, using datoviz now via raw ctypes with AI assistance.
They know NumPy, they know matplotlib, they are not C developers but can follow C examples.

Secondary audiences in priority order:
1. C application developers building desktop scientific tools
2. Browser/web app developers (scene → WASM)
3. Vulkan developers (vklite)
4. WebGPU engine developers (vklite-based renderer)
5. Embedding/integration developers (canvas + stream API)
6. ImGui-heavy interactive tool developers
7. Headless/offscreen/HPC users
8. Live data / real-time update users (v0.4 partial; v0.5 full)
9. Media pipeline / DAQ / timed sync users (v0.5+)

Note: datoviz is NOT a neuroscience-specific library. Neuroscience examples appear in the gallery
alongside geoscience, molecular, climate, and other domains. No single domain should dominate.


## Layer Model (public-facing)

Present four user-facing layers. DRP2 is internal and not a public surface.

| Layer | Audience | Status |
| --- | --- | --- |
| Scene (C + ctypes) | Scientists, app developers | First-class, desktop + browser |
| WebGPU renderer (vklite-based) | WebGPU engine developers | Advanced |
| vklite | Vulkan developers | Advanced |
| Canvas + Stream API | Embedding / integration developers | Advanced |

DRP2 is mentioned in architecture explanations as the internal transport between scene and
renderer, but is not documented as a user surface.


## Example Language Policy

All examples appear in both C and Python ctypes, side by side or in tabs. Raw ctypes only —
no wrapper classes, no helper abstractions except where unavoidable (event loop integration,
callback wiring). Ctypes code is auto-generated from C headers via the existing binding
generator toolchain; it is the source of truth for Python examples.


## Get Started Page

Single page. Zero external data dependencies — first example uses synthetic random data.
Structure:
1. Install (one command)
2. First example: scatter plot, 10k random 3D points, pan/zoom controller
   - C version first
   - Python ctypes version immediately below (or in a tab — TBD based on MkDocs capability)
3. Link to Examples gallery
4. Link to LLM entry point page (for AI-assisted workflow)


## AI-Assisted Workflow

### Philosophy
Docs are written for both human readers and LLMs. One version, not two. Precision and
consistency matter more than narrative prose. Every visual family page must be LLM-parseable:
consistent structure, unambiguous function signatures, minimal working examples.

### Prompt Widget
A static JavaScript widget embedded on the docs site (no backend, no LLM, fully deterministic).

- Free text input: user describes what they want in natural language
- Minimal optional hints (checkboxes) to help select injected context chunks
- Output: user's text wrapped in a structured header/footer containing:
  - One-paragraph datoviz v0.4 context description
  - Links to the LLM entry point page and relevant sub-pages
  - Instruction to use only documented v0.4 API and say so if uncertain
- Copy button + "Open in Claude" and "Open in ChatGPT" links
- No LLM on the docs side — the widget is pure template assembly

### LLM Entry Point Page
A dedicated page (`docs/llm.md` or similar) that serves as orientation for both humans and LLMs.
Structure:
- One paragraph: what datoviz v0.4 is, what it is not, primary API surface
- Capability → URL table: "I want to display X" → visual family page link
- Task → URL table: "I want to do Y" → how-to page link
- Layer → URL table: "I want to use layer Z" → advanced page link
- 3-4 minimal inline code patterns (create scene, set data, run event loop, update data)
- Written explicitly so an LLM can use it as context without reading the entire docs

The prompt widget links to this page as the primary context injection.


## Live Playground (RC Milestone)

A Pyodide-based Python editor embedded in the docs site. User writes Python code, it executes
via Pyodide calling the existing `datoviz_wasm_scene.mjs` Emscripten module through Pyodide's
JS FFI, renders via the existing WebGPU runtime in the browser.

Architecture:
```
Monaco/CodeMirror editor
  → Pyodide (Python in browser, web worker)
  → thin Python wrapper (~200 lines) calling WASM via Pyodide JS FFI
  → datoviz_wasm_scene.mjs (existing, unmodified)
  → DRP2 packet decoder (existing JS)
  → WebGPU runtime (existing JS)
  → HTMLCanvasElement (owned by JS harness)
```

Key constraints:
- Zero changes to the existing WASM build
- No ctypes — Pyodide calls JS calls WASM directly
- JS owns the canvas; Python is a guest
- Debounced re-execution on code change
- Python exceptions and WASM errors displayed in unified diagnostics panel

Intended workflow:
1. User uses prompt widget → pastes into external LLM → gets Python code back
2. User pastes Python code into playground → sees it render immediately in browser
3. User iterates

Status: RC milestone. Implementation is ~4-8 human-weeks but much faster with an agent given
the well-scoped architecture and existing WASM infrastructure.


## Hero Image

### Reference
Image 10 (generated June 2026) is the compositional reference. Four panels, organic overlapping
layout, no window chrome except functional UI elements, dark graphite background.

Panel contents:
1. **Wind globe** (dominant, ~55% of frame): Earth with streamlines and wind vectors, daytime
   or night-side texture. Basis: new `showcases/wind_globe.c` example (spec at
   `spec/scene/examples/scenarios/WIND_GLOBE.md`).
2. **Signal traces + ImGui** (medium): 128-channel dense time series with visible Dear ImGui
   controls panel. Label: "Signal Traces · 128 channels" (not EEG-specific).
3. **Protein structure** (medium): CPK sphere rendering with legend. Basis: existing protein
   arcball viewer example.
4. **Browser inset** (small): WebGPU scatter or point cloud in browser chrome showing
   "N points · 60 FPS". Communicates desktop→browser portability at low visual weight.

### Composition method
Real datoviz screenshots only — no AI-generated data or fake renders. Composited via a
reproducible Python/Pillow script that places screenshots on the graphite background.
No fake window chrome. Panel borders minimal or absent.

### Pending
- `wind_globe.c` implementation (see WIND_GLOBE.md spec)
- Signal traces showcase with ImGui (use existing `scientific_plotting_workflow` bottom panel
  as reference; may need a dedicated new example with visible ImGui controls)
- Night-side Earth texture decision (more dramatic but requires legal clearance on texture asset)
- Pillow composition script


## Feature Status for Documentation

### v0.4 (document fully)
- Scene layer: all visual families, controllers, axes, colorbars, labels, scale bars
- Offscreen/headless rendering
- Picking and query/readback
- Timer/animation API
- Partial buffer updates (`dvz_visual_set_data_range`)
- Scene compute shaders (GPU compute via WGSL/GLSL)
- Dear ImGui integration
- WASM/WebGPU browser path (experimental label)
- Canvas + stream API (advanced section)
- vklite (advanced section)

### v0.5 (mention as planned, link to spec)
- Ring buffers and streaming mutability hints
- Full DAQ/real-time multi-channel streaming
- Custom visual families with user shaders (`dvz_visual_custom`)
- Multi-texture mesh overlay (see MESH_MULTI_TEXTURE.md)
- Timed media sync / DvzMediaClock (see TIMED_MEDIA_SYNC.md)
- Jupyter integration (WebGPU widget, static PNG, or live PNG — TBD)

### Not in scope for datoviz docs
- High-level Python plotting API → VisPy2 / GSP
- Media pipeline integration (ffmpeg, GStreamer) → future roadmap


## Six-Panel Gallery Grid (below hero on landing page)

Six cards showing range across scientific domains and visual families. No domain should appear
twice. Candidates:
1. Wind globe (geoscience / climate)
2. Protein structure (molecular / life science)
3. Signal traces (instrument / DAQ)
4. Brain volume with slice (medical imaging — acceptable as one of six, not dominant)
5. Choropleth or geospatial map (data science / social science)
6. GPU particle simulation or LiDAR point cloud (physics / engineering)

Final selection depends on render quality of available screenshots. Priority: choose the six
that look best at thumbnail size on a dark background.
