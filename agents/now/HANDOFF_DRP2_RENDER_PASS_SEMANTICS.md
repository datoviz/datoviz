# DRP2 Render-Pass Semantics Handoff

Status: imminent pre-RC refactor. Created: 2026-07-07.

This is the next runtime correctness cleanup before RC stabilization. API/ABI compatibility may be
broken in this lane when it removes ambiguous DRP2 semantics.


## Problem

Multi-panel examples can expose magenta frame/gutter pixels, observed in
`scientific_plotting_workflow`. The panel backgrounds are correct; the bad pixels are outside
panels.

Root cause: DRP2 currently overloads `BeginRenderPass.viewport` as:

1. the Vulkan/WebGPU render attachment area,
2. the initial draw viewport, and
3. the initial draw scissor.

The native vklite backend maps that field to the Vulkan dynamic rendering area. A render-pass
`LOAD_CLEAR` on a panel region therefore initializes only that panel region. When the final sRGB
encode path renders fullscreen from an intermediate texture, uninitialized pixels outside panel
regions are sampled and become visible.

Do not fix this by changing example colors. The protocol needs clearer render-pass semantics.


## Desired Contract

1. Render target initialization is explicit attachment/load-store state over a declared render area.
2. Draw viewport and draw scissor are independent draw-state concepts.
3. Frame, scene, and intermediate targets are fully initialized before fullscreen sampling,
   readback, resolve, or presentation.
4. Panel render passes may clip draws to panel rectangles, but panel clipping must not be the only
   mechanism that initializes frame pixels.
5. DRP2 traces, JSON, fixtures, native runtime, and WebGPU runtime should expose the same semantic
   split.


## Implementation Plan

Expected shape: five commits, with a sixth only if WebGPU migration grows.

1. **DRP2 protocol/API split**

   Add a descriptor-style render-pass begin API, tentatively `DvzDrp2RenderPassDesc`, with explicit
   pixel fields:

   - `render_area_px`: attachment/dynamic-rendering area,
   - `viewport_px`: initial draw viewport,
   - `scissor_px`: initial draw scissor,
   - color and depth/stencil attachment descriptors,
   - load/store ops, clear colors, resolve targets, and access metadata.

   Replace the public preference for `dvz_drp2_stream_begin_render_pass_region_clear()`. Thin
   wrappers may remain temporarily only if they map unambiguously into the descriptor.

2. **Native backend and serialization migration**

   Update DRP2 validation, recording, JSON/fixture serialization, and vklite execution so:

   - `render_area_px` maps to Vulkan dynamic rendering area,
   - `viewport_px` maps to viewport state,
   - `scissor_px` maps to scissor state,
   - no backend infers render area from viewport.

3. **Frame emitter migration**

   Update scene/frame-plan emission so each scene/intermediate/final target is initialized once
   over the full target when required. Subsequent panel passes should use `LOAD`, panel viewport,
   and panel scissor.

   The final sRGB encode path may stay fullscreen, but it must sample only fully initialized source
   pixels.

4. **Regression coverage**

   Add tests at three levels:

   - DRP2 semantic test proving render area, viewport, and scissor are stored and executed
     independently.
   - Native offscreen multi-panel margin/gutter readback test proving untouched panel gaps equal
     configured frame clear color, not magenta/uninitialized pixels.
   - sRGB/intermediate-path regression for the same behavior, because that path exposed the issue.

5. **Generated outputs, bindings, and docs**

   Since public DRP2/API headers change, run:

   ```sh
   just ctypes
   just ctypes-check
   ```

   Refresh generated C API docs, ctypes bindings, DRP2 fixtures, WebGPU metadata, and affected
   example/gallery docs as needed.

6. **Optional WebGPU parity commit**

   If WebGPU migration is larger than expected, split it into a separate commit. Preserve the same
   DRP2 contract even if WebGPU needs full-pass plus scissor/clear emulation for some cases.


## Validation

Use the narrow loop while migrating, then finish with:

```sh
just build
just test drp2
just test scene
just ctypes
just ctypes-check
git diff --check
```

For the original repro, keep a native screenshot/readback proof for:

```sh
./build/examples/c/showcases/scientific_plotting --png
```

The margin/gutter pixels must be deterministic frame clear color, not `#ff00ff`.


## Commit Expectations

Prefer logical checkpoint commits:

1. `drp2: split render area from viewport state`
2. `drp2: migrate native render pass execution`
3. `scene: initialize frame targets before panel passes`
4. `test: cover multi-panel target clears`
5. `docs: refresh render-pass API outputs`
6. optional `webgpu: align render pass area semantics`

Do not mix this with unrelated gallery, app input, or release-note changes.
