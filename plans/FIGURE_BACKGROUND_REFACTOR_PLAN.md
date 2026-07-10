# Figure Background Ownership Refactor Plan

Status: proposed v0.4-dev pre-RC refactor. Public API changes are allowed. This plan covers the
clear/background color shared by native presentation, offscreen capture, and WebGPU/WASM.


## Problem

Figure background color is currently host policy rather than retained scene state:

1. The native app path overrides frame emission with linear `(0.05, 0.05, 0.08, 1.0)`.
2. The generic FramePlan emission configuration defaults to opaque black.
3. WASM/WebGPU inherits that generic default, so the same scenario renders differently by host.
4. Panel backgrounds are retained visuals, but they do not define uncovered figure regions or
   gaps between panels.
5. Clear-color encoding depends on the target format and color pipeline, making an accidental
   numeric match insufficient proof of pixel parity.

The result violates the intended boundary: scene state should define appearance, while app,
offscreen, Vulkan, and WebGPU runtimes should execute the emitted DRP2 contract.


## Target Semantics

1. Every figure owns one retained background color.
2. The figure background covers the entire render target, including space outside panels and gaps
   between panels.
3. A panel background remains an optional retained visual for panel-local colors, gradients, and
   images. It draws over the figure background and does not replace it.
4. Scene-to-FramePlan emission copies the resolved figure background into the final target clear
   value.
5. Hosts and backends do not select a visual default or override an authored figure background.
6. The v0.4 default is the current Datoviz graphite color, linear
   `(0.05, 0.05, 0.08, 1.0)`.
7. Figure background input follows scene color semantics. DRP2 carries linear clear values; the
   target format and color pipeline determine final encoding exactly once.
8. Native window, native offscreen capture, and WebGPU/WASM must produce equivalent encoded pixels
   for the same figure background and target color space, within explicit quantization tolerance.


## Proposed Model

Add retained background state to `DvzFigure`, initialized when the figure is created:

```c
typedef struct DvzFigureBackground
{
    float color[4]; /* linear RGBA */
    uint64_t version;
} DvzFigureBackground;
```

Expose a small backend-neutral API. The preferred public shape is a fallible setter plus getter:

```c
DvzResult dvz_figure_set_background_color(DvzFigure* figure, DvzColor color);
bool dvz_figure_background_color(const DvzFigure* figure, DvzColor* out_color);
```

Before implementation, resolve whether `DvzColor` is the correct authored-color type for this API.
If its established contract is encoded sRGB rather than linear input, convert once at the scene API
boundary and retain linear RGBA internally. Do not expose target-format or WebGPU concepts.

The emission flow becomes:

```text
DvzFigure retained background
    -> scene frame-artifact preparation
    -> DvzFramePlanEmitConfig.clear_color
    -> DRP2 render-pass clear value
    -> Vulkan / offscreen / WebGPU execution
```

`DvzFramePlanEmitConfig.clear_color` remains an internal/runtime transport value. It must not be a
second source of visual policy. Its standalone default may remain opaque black for low-level tests
and fixture callers that do not emit a figure.


## Implementation Phases

### Phase 1 - Specify and Test Color Semantics

1. Document figure versus panel background ownership in the scene semantics specification.
2. Confirm the authored `DvzColor` transfer function and the linear representation expected by
   DRP2 clear values.
3. Add CPU emission tests proving that default and custom figure backgrounds become the final
   render-pass clear value.
4. Cover invalid input, null arguments, getter round trips, and figure lifetime.

Acceptance:

- One specification defines figure coverage, panel precedence, alpha, and color-space behavior.
- Tests distinguish linear clear values from encoded output pixels.


### Phase 2 - Add Retained Figure State

1. Add the background field and graphite initialization to figure construction.
2. Add the public setter/getter and Doxygen documentation.
3. Mark background changes as frame work without recreating runtime resources.
4. Make all figure emission entry points resolve `clear_color` from retained figure state.
5. Regenerate and validate bindings because this phase changes the public API.

Validation:

```sh
just build
just test scene
just ctypes
just ctypes-check
git diff --check
```

Acceptance:

- Default and custom figure colors survive repeated frames, resize, and runtime recovery.
- Changing only the background emits the minimum required frame work.


### Phase 3 - Remove Host Policy

1. Delete the hard-coded graphite override from the native app emission path.
2. Audit offscreen, scenario runner, WASM, fixture, and direct figure-emission callers for other
   clear-color overrides.
3. Keep host configuration limited to target identity, dimensions, format, scale, and runtime
   resource scope.
4. Ensure WASM runtime reset/recovery replays the retained figure color without recreating the C
   scene.

Acceptance:

- No app, window, offscreen, or WASM host chooses the default scene appearance.
- Low-level FramePlan fixtures may still choose explicit clear values for isolated runtime tests.


### Phase 4 - Cross-Backend Pixel Parity

1. Add a minimal scenario with no panels or visuals to test the full-target figure clear.
2. Add a layout scenario with margins and multiple panels to distinguish figure background from
   panel backgrounds.
3. Capture native offscreen and WebGPU output using matched dimensions and color formats.
4. Compare representative pixels for default graphite, a custom opaque color, and a translucent
   color if target alpha is supported by the public capture contract.
5. Assert encoded sRGB values, not the raw linear clear tuple, where the target is sRGB.

Validation:

```sh
just test scene
just wasm-scene-smoke
just webgpu-browser-smoke
git diff --check
```

Acceptance:

- Native window/offscreen and WebGPU agree for full-target and panel-gap pixels.
- Panel backgrounds continue to override only their own regions.
- Recovery and resize preserve the same pixels.


### Phase 5 - Examples and Documentation

1. Remove example-local background assignments whose only purpose was compensating for host
   defaults; retain assignments that are intentional example styling.
2. Document figure and panel background APIs together with their coverage and precedence.
3. Refresh only gallery media whose appearance changes intentionally.
4. Reconcile WebGPU compatibility notes and feature-status documentation.


## Required Audit Locations

At minimum, inspect these implementation boundaries during the refactor:

1. `src/scene/core/scene.c` and the internal figure representation;
2. `src/scene/core/figure_emit.c` and frame-artifact emission;
3. `src/scene/frame_plan/fixture.c` for transport defaults;
4. `src/app/app.c` for the current native override;
5. `src/wasm/scene_api_packets.c` for WebGPU emission configuration;
6. `src/scene/visuals/families.c` for panel background precedence;
7. `web/drp2/webgpu.js` for clear-value execution only;
8. offscreen capture and color-management tests.


## Non-Goals

1. Do not make CSS canvas background participate in rendered output.
2. Do not encode a background as a fullscreen visual solely to avoid a render-pass clear.
3. Do not merge figure and panel background APIs; their coverage differs.
4. Do not make Vulkan or WebGPU choose Datoviz theme colors.
5. Do not broaden this work into a general theme system.
6. Do not claim parity from matching raw floats across differently encoded target formats.


## Completion Gate

The refactor is complete when figure background is retained scene state, all figure emission paths
derive DRP2 clear values from it, host-specific visual defaults are removed, bindings and public
documentation are current, and automated native/offscreen/WebGPU checks prove encoded-pixel parity
for default and custom backgrounds.
