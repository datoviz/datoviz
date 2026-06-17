# Deploy to Web

How to run a datoviz scene in a browser using the experimental WebAssembly/WebGPU path.

## Overview

Datoviz v0.4 includes an experimental browser path that compiles your C scene to WebAssembly and
renders it through the browser's WebGPU API. The pipeline is:

```text
C/WASM scene state → scene frame plan → WGSL DRP2 packets → browser WebGPU runtime → canvas
```

This is a portability proof for the v0.4 scene and DRP2 contract, not native Vulkan feature
parity. Treat it as an unstable preview. Unsupported features fail explicitly with diagnostics.

## Supported visual families

The browser subset covers:

- point, pixel, marker, segment, path, primitive, image, glyph, text, labels;
- basic, textured, and material-controlled mesh; basic sphere;
- 2D axes with ticks, grid lines, and bitmap text labels;
- adornment examples (colorbar, scalebar, categorical legend, annotation readout);
- panzoom, arcball, fly, turntable, and orbit-camera controllers.

App/window, video export, and GUI modules are unsupported in WASM.

## Steps

**1. Build the WASM target.**

Build datoviz with Emscripten to produce the WASM scene ABI:

```bash
just build-wasm
```

This compiles `src/wasm/scene_api.c` and outputs the WASM module and JS glue under `build/wasm/`.

**2. Run browserless smoke checks.**

Before opening a browser, validate the emitted DRP2 streams offline:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
just webgpu-browser-smoke
```

These checks catch scene-emission and DRP2 issues without requiring a browser.

**3. Serve the repository locally.**

The WASM module and HTML pages must be served over HTTP (not `file://`):

```bash
python3 -m http.server 8765
```

**4. Open the example pages.**

Navigate to the pre-built demo pages to verify the WASM scene renders:

```text
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-2d
http://localhost:8765/examples/webgpu/examples.html?demo=wasm-3d
```

The 2D page exercises point, pixel, marker, segment, path, image, text, mesh, and panzoom. The 3D
page exercises sphere, textured mesh, and arcball.

**5. Check the fixture dashboard.**

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

All committed fixture rows should pass. Any failure here indicates a DRP2 regression.

**6. Open a live gallery route (optional).**

Single-example browser routes are available for promoted C examples:

```text
http://localhost:8765/examples/webgpu/live.html?id=feature_timer_animation
```

Replace `feature_timer_animation` with any promoted example id from `examples/webgpu/COMPAT.md`.

## Using the WASM scene ABI

The WASM module exposes handle-based `dvz_wasm_api_*` functions. JavaScript loads the module,
calls `dvz_wasm_api_emit_packets()` each frame, and feeds the returned binary DRP2 packets into
the browser WebGPU runner in `web/wasm/scene.js`.

JavaScript must pass handles back unchanged and must not inspect internal scene state. Packet spans
are borrowed from the current frame artifact — decode or copy them immediately; do not retain WASM
views after packet release.

## Diagnostics

A failed scene emit returns a non-zero status. Retrieve diagnostics with:

```c
dvz_wasm_api_diagnostic_count()
dvz_wasm_api_diagnostic()
```

Successful emits leave the diagnostic report empty. Unsupported features fail explicitly rather
than silently falling back.

## See also

- [Explanation: Portability and WebGPU](../explanation/portability-webgpu.md)
- [Reference: WebGPU subset](../reference/webgpu-subset.md)
- [Render offscreen](render-offscreen.md)
