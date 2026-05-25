# Datoviz v0.4 Release Readiness

> **Execution Status**
> - **Status:** `RELEASE READINESS REFERENCE`
> - **Updated on:** `2026-05-25`
> - **Purpose:** define the checks that make v0.4 coherent enough to publish after feature proof

Start with [`V0_4_RELEASE_MASTER_CHECKLIST.md`](V0_4_RELEASE_MASTER_CHECKLIST.md) for sequencing.
Use this file as the compact release-quality rubric for API review, documentation, bindings,
examples, packaging, and communication.


## Release Position

Datoviz v0.4 is an architectural C-engine release. Source and ABI compatibility with v0.3 are not
requirements, but visible capability regressions must be fixed, explicitly deferred, or moved to
GSP/VisPy2.

Native first slices are active for rendered text, linear 2D axes/ticks/labels, continuous
colorbars, label annotations, scale bars, retained visual families, app/offscreen/GLFW rendering,
capture, DRP2 emission, and several graph-backed techniques. RC1 should prove and classify that
surface rather than reopening broad feature construction.

Remaining feature-freeze blockers:

1. WebGPU/WASM experimental subset and smoke.
2. Raw generated `ctypes` scope, regeneration, and load smoke.
3. v0.3 visible parity audit.
4. Public API/status classification.
5. Compact release-example proof set.


## Scope

Required for v0.4:

1. C API and native scene/app path for the declared feature set.
2. Low-level/generated Python `ctypes` binding surface.
3. Raster screenshot/capture and enough video/frame-sequence support for release examples.
4. Experimental WebGPU/WASM path with explicit unsupported-feature diagnostics.
5. Public status labels for supported, experimental, advanced/unstable, deferred, and external
   features.

Not required for v0.4:

1. Exact v0.3 source or ABI compatibility.
2. High-level object-oriented Python plotting wrappers.
3. Publication-quality PDF/SVG/vector export from Datoviz.
4. Full WebGPU parity with native Vulkan.
5. Complex text shaping, TeX/math layout, rich labels, and label collision solving.
6. Rich rendered pinned readout UI beyond examples needed for v0.4.
7. Broad mesh/path/volume/text picking, lasso selection, custom shader APIs, scene-level compute,
   CUDA interop, LOD/out-of-core policy, or full dashboard/application APIs.

External scope:

1. GSP/VisPy2 owns high-level OO Python and plotting APIs.
2. GSP/Matplotlib owns publication-oriented vector export.
3. Datoviz owns the C engine, low-level binding surface, raster capture, native scene/app path, and
   experimental WebGPU/WASM portability lane.


## Visible Parity

Avoid surprising visible regressions in:

1. retained scene workflow, offscreen/GLFW app, resize, capture, and frame callbacks,
2. point, pixel, marker, primitive, segment/path, image, mesh, sphere, volume, and text/glyph
   visuals,
3. full and partial visual updates across live frames,
4. image/volume sampled fields and scalar color mapping where supported,
5. 2D axes, ticks, labels, colormaps, continuous colorbars, and scale bars,
6. panzoom, arcball, camera/fly/turntable, and input routing,
7. multi-panel figures, linked-panel behavior, per-panel viewport/scissor, and resize behavior,
8. depth, transparency, material, marker styling, path width/cap/join basics, image filtering,
   sphere modes, and volume slice/render controls,
9. offscreen screenshot/gallery capture and enough video/frame-sequence capture for examples.

Acceptable deferrals include old v0.3 API names, old Python OO ergonomics, niche v0.3 visuals,
demo helper APIs, Python-side data/download helpers, and Datoviz-native structural vector export.


## API Review

Before RC1:

1. audit every public header under `include/datoviz/`,
2. classify modules and symbols as public, experimental, advanced/unstable, internal leakage, or
   deferred,
3. confirm ownership, destroy rules, callback lifetimes, error/status behavior, and feature gates,
4. state that `scene` and `app` are the main public narrative,
5. label `drp2` as an advanced protocol surface,
6. label `vk`, `vklite`, `canvas`, `stream`, and `video` as lower-level or integration surfaces,
7. keep broad Python plotting and vector export outside Datoviz v0.4.


## Documentation

Before RC1:

1. write the feature/status table,
2. write v0.3 visible parity notes,
3. write known issues and explicit deferrals,
4. update README/build docs enough for a clean source build and example run,
5. document WebGPU/WASM as experimental with a supported subset,
6. document raw `ctypes` scope and smoke result,
7. keep active v0.4 design material out of legacy `docs/` unless public docs migration has
   explicitly started.

Public docs should eventually follow a task-oriented split:

1. tutorials for first experience,
2. how-to guides for common tasks,
3. reference for API facts,
4. explanation for architecture and tradeoffs.


## Examples

The RC1 proof set should be small and honest. Prefer deterministic C examples or fixtures over a
large gallery.

Recommended proof targets:

1. scatter with axes, markers, panzoom, and selection/pick if ready,
2. scalar image with colorbar, probe, and pinned/readout state,
3. linked panels with axes and panzoom,
4. text/label annotation and scale-bar examples,
5. mesh/sphere/material flagship,
6. volume/offscreen example,
7. dense point cloud with EDL,
8. WebGPU/WASM experimental page or fixture dashboard for the supported subset.

Each release example should have:

1. command to build/run,
2. required optional dependencies,
3. expected output class,
4. known gaps,
5. validation result.


## Quality Gates

Minimum RC1 checks:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. `just spec-check`
5. WebGPU preflight/browser smoke for the experimental subset
6. raw `ctypes` generation/load smoke
7. representative offscreen or bounded GLFW smokes for release examples

Broaden validation for changes touching allocation, object lifetimes, Vulkan resources,
command-buffer ownership, synchronization, shaders, DRP2 schemas, or public headers.


## Packaging And Release

Before final v0.4.0:

1. define supported platforms and optional dependency behavior,
2. verify build from clean checkout,
3. verify install or package artifacts for the supported platform set,
4. include license and third-party notices for vendored dependencies and assets,
5. produce release notes with scope, known issues, migration notes, and validation summary,
6. tag exact RC and final commits.


## Tracker Hygiene

Keep release tracking concise:

1. use `V0_4_RELEASE_MASTER_CHECKLIST.md` for phase sequencing,
2. use `IMPLEMENTATION.md` for feature gate status,
3. use this file for release-quality criteria,
4. put stable behavior in `spec/`,
5. put completed implementation history in `agents/done/`,
6. avoid duplicating long status narratives across files.
