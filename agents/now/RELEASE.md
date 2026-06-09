# Datoviz v0.4 Release Plan

Status: active release roadmap. Updated: 2026-06-09.

This is the short route from the current branch to `v0.4.0`. Use [STATUS.md](STATUS.md) for current
blockers and [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation gates.

Durable release policy lives in [../../spec/release/](../../spec/release/):
[READINESS.md](../../spec/release/READINESS.md),
[RC_PROCESS.md](../../spec/release/RC_PROCESS.md),
[COMMUNICATION.md](../../spec/release/COMMUNICATION.md), and
[GALLERY_OUTREACH.md](../../spec/release/GALLERY_OUTREACH.md).


## Scope

Required before feature freeze:

1. Native scene/app path covers the declared v0.4 visual and interaction subset.
2. Retained textured mesh has deterministic example or fixture proof.
3. WebGPU/WASM has an honest experimental RC subset broad enough to host most non-desktop scene
   examples live on the website, including core visuals, animation/frame callbacks,
   compute-to-render particles, and a narrow request/query/readback slice.
4. Raw `ctypes` generation and smoke tests work for the intended public C surface.
5. v0.3 visible capability gaps are fixed, explicitly deferred, or external/GSP-owned.
6. Core examples compile and exercise the release feature set.
7. Minimal compute+graphics interop has an experimental C-first proof, including explicit DRP2
   synchronization and a gallery-oriented particle-advection plan or example.
8. Low-level Qt/PyQt hosted rendering works through the optional Qt bridge provider, without adding
   Qt as a dependency of `libdatoviz`.

Not required for v0.4:

1. v0.3 source or ABI compatibility.
2. High-level object-oriented Python plotting wrappers.
3. Publication-quality PDF/SVG/vector export.
4. Full WebGPU parity with native Vulkan, full query parity across every visual family, browser
   equivalents for native desktop runtime examples, and advanced WebGPU technique parity.
5. Complex text shaping, TeX/math layout, collision solving, dashboards, general custom shader
   APIs, CUDA interop beyond an optional native advanced example, CuPy/Python interop,
   LOD/out-of-core policies, or full application APIs.

External ownership:

1. GSP/VisPy2 owns high-level OO Python and plotting APIs.
2. GSP/Matplotlib owns publication-oriented vector export.
3. Datoviz v0.4 owns the C engine, native scene/app path, raw/generated low-level Python binding
   surface, raster capture, experimental WebGPU/WASM path, and experimental compute+graphics
   proof.


## Release Sequence

### 1. Feature-Freeze Candidate

Exit criteria:

1. Feature/status table exists and uses `supported`, `experimental`, `advanced/unstable`,
   `deferred`, and `external/GSP`.
2. No feature-freeze blocker in [STATUS.md](STATUS.md) is unclassified.
3. Retained textured mesh remains in validation with deterministic proof.
4. Text, axes, ticks, colorbars, annotations, scale bars, and retained visuals are represented in
   examples or tests.
5. Qt/PyQt hosting is either proven with the optional bridge or tracked as the remaining release
   blocker with the implementation handoff in `spec/scene/integration/QT_HOST_BRIDGE.md`.

Suggested validation:

```sh
git diff --check
just build
just test scene
just spec-check
```

### 2. v0.3 Visible Parity Audit

Audit visible capabilities, not old APIs: retained scene workflow, offscreen/GLFW app, screenshot
capture, frame callbacks, multi-panel figures, text, axes, colorbars, panzoom, arcball/fly/turntable,
point, pixel, marker, primitive, segment/path, image, mesh/textured mesh, sphere, and volume.

Exit criteria: every visible gap is fixed, deferred, or external/GSP, and the table is linked from
release docs.

### 3. WebGPU/WASM Experimental Slice

Ship an honest browser/backend subset with broad live-example coverage, not native parity.

Exit criteria:

1. Portable scenario host exists for native and browser runners.
2. Example manifest marks each public example as `webgpu-live`, `webgpu-planned`,
   `webgpu-deferred`, or `native-only`.
3. Most non-desktop scene examples have live browser gallery routes or explicit `webgpu-planned`
   gaps.
4. DRP2/WebGPU runner and WGSL emission work for the declared subset.
5. Compute-to-render particle showcase runs in browser WebGPU at a documented particle budget.
6. Point/marker picking plus one probe/readback example works through async browser readback.
7. Unsupported commands, visual families, query targets, shader variants, native-only runtime
   features, and capability failures have diagnostics.
8. Scene semantics are shared; there is no WebGPU-only scene contract.
9. Browser runtime traffic uses frame artifact packet spans; DRP2 JSON remains debug/fixture-only.
10. `webgpu-live` gallery examples reuse the same canonical C example or portable C scenario as the
    native route; browser JavaScript remains host glue and does not reimplement example behavior.

### 4. Compute+Graphics Experimental Slice

Ship one real GPU compute-to-render path without widening v0.4 into a general compute framework.

Exit criteria:

1. Minimal DRP2 synchronization semantics are active, schema-backed, fixture-covered, and mapped by
   native vklite execution.
2. A portable compute-to-render command stream covers storage-buffer write followed by render-time
   vertex or instance consumption.
3. Native validation proves compute, synchronization, graphics consumption, and readback or captured
   visual evidence.
4. WebGPU accepts the portable subset or emits explicit unsupported-feature diagnostics.
5. A C gallery example, preferably GPU particle advection, is planned or landed with a release
   artifact target.
6. Optional CUDA SDK interop remains native-only, capability-gated, and advanced/unstable.

### 5. Qt/PyQt Hosted Path

Exit criteria:

1. `datoviz_qtbridge` builds only when Qt development headers and libraries are available.
2. `libdatoviz` has no Qt link dependency.
3. `datoviz.qt` dynamically loads the bridge when PyQt hosting is requested.
4. PyQt6 hosting no longer calls missing Python bindings for
   `QVulkanInstance::setVkInstance()` or `QVulkanInstance::vkInstance()` directly.
5. Unsupported PyQt/PySide bindings, missing bridge libraries, and Qt runtime mismatches fail with
   clear diagnostics.
6. The native Qt smoke and Python PyQt hosted example are proven locally or recorded as blocked by
   environment constraints.

### 6. Raw `ctypes` API Candidate

Exit criteria:

1. Generated bindings load the intended installed C API.
2. ABI/layout smoke checks pass.
3. Raw examples cover the supported low-level Python path.
4. The docs state that high-level Python plotting is GSP/VisPy2 scope.
5. C API docs state that `dvz_figure_emit()` and `dvz_figure_emit_ex()` are gone and that
   `DvzSceneFrameArtifact` is the scene emission product.

### 7. RC1

Exit criteria:

1. RC1 tag and notes exist.
2. Build/test/spec validation is recorded.
3. Feature table, visible parity table, known gaps, raw `ctypes` scope, and WebGPU/WASM scope are
   published or linked.
4. Release examples are documented enough for early testers.
5. Required RC note fields from
   [../../spec/release/RC_PROCESS.md](../../spec/release/RC_PROCESS.md) are present.

### 8. RC2

Exit criteria:

1. Documentation and gallery structure are mostly final.
2. Generated C reference or complete outline exists.
3. Captured artifacts prove the declared feature set.
4. RC1 feedback is triaged.
5. Gallery/data attribution and outreach candidates satisfy
   [../../spec/release/GALLERY_OUTREACH.md](../../spec/release/GALLERY_OUTREACH.md).

### 9. RC3

Exit criteria:

1. Only blocker fixes remain.
2. Packaging, licenses, generated artifacts, release notes, and docs are final candidates.
3. Packaging and quality checks from
   [../../spec/release/READINESS.md](../../spec/release/READINESS.md) are clean or recorded as
   known issues.

### 10. Final `v0.4.0`

Exit criteria:

1. `v0.4.0` is tagged and published with reproducible artifacts.
2. Documentation and release notes are public.
3. Launch screenshots, short clips, README/website assets, and announcement text are generated from
   current gallery examples.
4. Direct feedback channels are open for early users, especially scientists whose public datasets
   are used in showcase examples.
5. The active queue resets for v0.4 patch work and v0.5 planning.


## Post-Release Refactor Queue

Do not let these delay `v0.4.0` unless needed to fix a release blocker:

1. lower-risk `src/scene/scene.c` structural cleanup;
2. shader/visual registry cleanup that does not change public behavior;
3. broader runtime/backend modularity work;
4. long-horizon WebGPU parity and advanced visual work.
