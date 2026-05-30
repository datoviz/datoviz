# Datoviz v0.4 Release Plan

Status: active release roadmap. Updated: 2026-05-30.

This is the short route from the current branch to `v0.4.0`. Use [STATUS.md](STATUS.md) for current
blockers and [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation gates.


## Scope

Required before feature freeze:

1. Native scene/app path covers the declared v0.4 visual and interaction subset.
2. Retained textured mesh has deterministic example or fixture proof.
3. WebGPU/WASM has an honest experimental subset with unsupported-feature diagnostics.
4. Raw `ctypes` generation and smoke tests work for the intended public C surface.
5. v0.3 visible capability gaps are fixed, explicitly deferred, or external/GSP-owned.
6. Core examples compile and exercise the release feature set.

Not required for v0.4:

1. v0.3 source or ABI compatibility.
2. High-level object-oriented Python plotting wrappers.
3. Publication-quality PDF/SVG/vector export.
4. Full WebGPU parity with native Vulkan.
5. Complex text shaping, TeX/math layout, collision solving, dashboards, compute, custom shader
   APIs, CUDA interop, LOD/out-of-core policies, or full application APIs.

External ownership:

1. GSP/VisPy2 owns high-level OO Python and plotting APIs.
2. GSP/Matplotlib owns publication-oriented vector export.
3. Datoviz v0.4 owns the C engine, native scene/app path, raw/generated low-level Python binding
   surface, raster capture, and experimental WebGPU/WASM path.


## Release Sequence

### 1. Feature-Freeze Candidate

Exit criteria:

1. Feature/status table exists and uses `supported`, `experimental`, `advanced/unstable`,
   `deferred`, and `external/GSP`.
2. No feature-freeze blocker in [STATUS.md](STATUS.md) is unclassified.
3. Retained textured mesh remains in validation with deterministic proof.
4. Text, axes, ticks, colorbars, annotations, scale bars, and retained visuals are represented in
   examples or tests.

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

Ship an honest browser/backend subset, not native parity.

Exit criteria:

1. Supported subset is explicit, initially point, primitive, image, and preferably basic mesh.
2. DRP2/WebGPU runner and WGSL emission work for the subset.
3. Unsupported commands, visual families, shader variants, and runtime features have diagnostics.
4. A browser-visible demo, fixture dashboard, or runnable page exists.
5. Scene semantics are shared; there is no WebGPU-only scene contract.

### 4. Raw `ctypes` API Candidate

Exit criteria:

1. Generated bindings load the intended installed C API.
2. ABI/layout smoke checks pass.
3. Raw examples cover the supported low-level Python path.
4. The docs state that high-level Python plotting is GSP/VisPy2 scope.

### 5. RC1

Exit criteria:

1. RC1 tag and notes exist.
2. Build/test/spec validation is recorded.
3. Feature table, visible parity table, known gaps, raw `ctypes` scope, and WebGPU/WASM scope are
   published or linked.
4. Release examples are documented enough for early testers.

### 6. RC2

Exit criteria:

1. Documentation and gallery structure are mostly final.
2. Generated C reference or complete outline exists.
3. Captured artifacts prove the declared feature set.
4. RC1 feedback is triaged.

### 7. RC3

Exit criteria:

1. Only blocker fixes remain.
2. Packaging, licenses, generated artifacts, release notes, and docs are final candidates.

### 8. Final `v0.4.0`

Exit criteria:

1. `v0.4.0` is tagged and published with reproducible artifacts.
2. Documentation and release notes are public.
3. The active queue resets for v0.4 patch work and v0.5 planning.


## Post-Release Refactor Queue

Do not let these delay `v0.4.0` unless needed to fix a release blocker:

1. lower-risk `src/scene/scene.c` structural cleanup;
2. shader/visual registry cleanup that does not change public behavior;
3. broader runtime/backend modularity work;
4. long-horizon WebGPU parity and advanced visual work.
