# WASM/WebGPU Parity Plan

Execution Status:

- Status: canonical parity plan for future implementation
- Updated on: 2026-06-05
- Purpose: define the route from the current experimental browser subset to broad native
  Vulkan/WASM WebGPU scene parity
- Scope: scene features, visual families, WASM ABI expansion, DRP2/WebGPU runtime parity,
  examples, validation, diagnostics, and promotion rules


## Goal

Datoviz should support write-once scene code:

```text
portable C scene scenario
  -> native host: app/window/capture/Vulkan
  -> browser host: WASM scene ABI -> DRP2 packets -> WebGPU canvas
```

The parity boundary is the scene and DRP2 contract, not native host APIs. A scene-level example or
feature should be implemented once in C, then hosted by native Vulkan or browser WebGPU. Browser
JavaScript should load the WASM module, normalize browser input, execute DRP2 packets, report
diagnostics, and manage the WebGPU canvas. It should not reimplement Datoviz scene behavior.


## Non-Goals

WASM/WebGPU parity does not require browser equivalents for:

1. GLFW;
2. native app loops;
3. Vulkan or vklite handles;
4. native swapchains;
5. Qt/PyQt hosting;
6. native GUI/debug panels;
7. native video encoders;
8. CUDA/Vulkan external-memory interop;
9. platform packaging diagnostics.

Those remain native integration examples. Browser equivalents should use DOM canvas,
`requestAnimationFrame`, browser input events, WebGPU adapter/device/context setup, DRP2 packet
execution, and browser-native capture paths if needed.


## Source Of Truth

Use these documents together:

1. [WEBGPU_WASM.md](WEBGPU_WASM.md): browser integration contract.
2. [../../drp2/roadmap/WEBGPU.md](../../drp2/roadmap/WEBGPU.md): DRP2/WebGPU backend roadmap.
3. [../../drp2/PACKETS.md](../../drp2/PACKETS.md): split binary setup/update/frame packet
   transport.
4. [../examples/PORTABLE_SCENARIO_RUNNER.md](../examples/PORTABLE_SCENARIO_RUNNER.md): write-once
   C scenario architecture for examples.
5. [../../../docs/reference/webgpu-subset.md](../../../docs/reference/webgpu-subset.md): current
   public experimental subset.

This file owns the end-to-end parity program and implementation order. Backend command details stay
in DRP2 specs. Example-host structure stays in the portable scenario runner spec.


## Current Experimental Subset

The current browser path proves:

1. C/WASM scene state emitting split DRP2 setup/update/frame packets;
2. point visual;
3. pixel visual;
4. basic built-in marker visual;
5. segment visual with visual-wide cap controls;
6. path visual with visual-wide cap and join controls;
7. primitive triangle-list visual;
8. RGBA8 image visual;
9. low-level atlas-backed glyph visual;
10. basic, textured, and material-controlled mesh visual;
11. basic sphere visual;
12. 2D panzoom input;
13. one 3D sphere + textured mesh scene with camera and arcball input;
14. WebGPU runner execution for the committed DRP2 fixture subset;
15. semantic negative-fixture parity in the WebGPU runner;
16. compute and `ResourceBarrier` at DRP2 fixture level;
17. browser evidence through `examples/webgpu/examples.html` and `examples/webgpu/fixtures.html`.

This is an experimental subset, not native Vulkan feature parity.


## Architecture

The intended portable path is:

```text
scene retained state
  -> FramePlan
  -> DRP2 command stream
  -> split DRP2 setup/update/frame packets
  -> browser WebGPU runtime
```

Native Vulkan uses the same scene and DRP2 concepts through the native runtime path:

```text
scene retained state
  -> FramePlan
  -> DRP2 command stream
  -> vklite/canvas/stream/app
```

Rules:

1. scene APIs must not expose Vulkan, WebGPU, browser, GLFW, or app host types;
2. WebGPU must execute DRP2 packets, not scene visual-family-specific JavaScript;
3. browser demos should be content loaders and event glue only;
4. unsupported browser requirements must fail through deterministic diagnostics;
5. every promoted feature must keep native and browser behavior tied to shared scene semantics.


## Required WASM ABI Expansion

The current WASM scene ABI is intentionally narrow. Broad parity requires these additions:

1. generic portable example/session ABI based on `DvzExampleSpec`;
2. scene buffer creation and destruction;
3. scene buffer data upload and partial update;
4. buffer-backed visual attributes through `dvz_visual_set_attr_buffer()` semantics;
5. scene compute creation with shader format/source and dispatch;
6. compute buffer binding with read/read-write access, offset, and byte size;
7. dispatch update and frame parameter update hooks;
8. query/readback request creation and result polling;
9. selection and hover/update callback delivery;
10. diagnostics for unsupported requirements, bad handles, stale packets, and capability failures;
11. capability handoff for limits, formats, sample counts, shader formats, buffer alignment, and
    copy row pitch;
12. versioned reset/lifecycle behavior for retained browser runtime sessions.

The ABI should expose opaque handles and borrowed packet spans only. JavaScript must copy or execute
packet and arena spans before the next mutating WASM call.


## Shader Policy

WGSL is the portable shader language for browser execution. For each browser-supported visual or
technique:

1. provide WGSL shader sources;
2. keep vertex-buffer layouts and bind-group layouts explicit;
3. document unsupported variants;
4. prefer shared shader semantics over backend-specific visual behavior.

Native may keep GLSL/SPIR-V paths when useful. Portable C scenarios should select shader source by
capability. Shader-language duplication is acceptable; C scene/example behavior duplication is not.


## Promotion Rule

A visual family, feature, or example is not WebGPU-supported until the same change set or milestone
provides:

1. C scene path or portable scenario path;
2. WGSL shader path when shaders are involved;
3. DRP2 schema/fixture coverage when protocol behavior changes;
4. WebGPU runner execution or explicit unsupported diagnostics;
5. WASM scene smoke when the feature crosses the scene ABI;
6. browser evidence when the feature is user-visible;
7. public docs/status update;
8. clear unsupported-feature diagnostics;
9. validation commands recorded in the relevant spec or docs page.


## Visual Family Matrix

Use this table as the parity work tracker. `Current` means implemented in the active browser
subset. `Next` means the preferred near-term promotion target. `Deferred` means do not expand into
that family until earlier rows are stable.

| Family | Native Vulkan status | WASM/WebGPU target | Status | Required work |
| --- | --- | --- | --- | --- |
| point | retained scene visual | point visual with buffer-backed attrs current; particles next | current/next | keep buffer-backed attr evidence; prove compute-written positions |
| primitive | retained triangle-list visual | triangle-list visual | current | keep fixture/browser evidence and diagnostics |
| image | retained RGBA/scalar image paths | RGBA8 image first, scalar/colormap later | current | add scalar/colormap variants after query/colorbar path is stable |
| mesh | basic/textured/lit mesh paths | basic, textured, and material-controlled mesh current | current | keep texture-sampling and material-update evidence; broaden only with additional shader/material models |
| pixel | retained pixel visual | dense and buffer-backed 2D pixel visual | current | keep fixture/browser evidence |
| marker | retained marker visual | built-in marker subset current; glyph/atlas subset next | current/next | keep built-in marker evidence; settle atlas variants and picking diagnostics |
| segment/path/stroke | retained path and stroke-shaped paths | segment/path cap-join controls current; broader subpath/vector parity next | current/next | keep cap/join evidence; settle subpath/vector policy and browser proof |
| text/glyph | retained text and glyph visuals | low-level atlas glyph current; semantic text subset next | current/next | font/atlas packaging, WASM semantic text ABI, shaping limits |
| labels | label field and readback paths | label rendering and label probe subset | later | texture/label formats, query/readback, diagnostics |
| sphere | sphere impostor visual | basic sphere impostor current; raycast/depth/material parity next | current/next | keep WGSL/browser evidence; promote native-depth and material variants with parity proof |
| volume | volume visual and query paths | reduced volume/slice subset | later | 3D texture limits, sampling shaders, memory diagnostics |
| splat | experimental splat visual | explicit experimental subset | later | sorting/blending/WBOIT policy and memory limits |
| vector | planned/polish lane | vector arrows/glyphs | later | native family stabilization first |


## Annotation And Layout Matrix

Annotations should be promoted after core visual families and query/readback basics are stable.

| Feature | WASM/WebGPU target | Status | Required work |
| --- | --- | --- | --- |
| axes/ticks | 2D axes with ticks and labels | deferred | text/glyph subset, layout reserve semantics |
| colorbar | continuous scalar colorbar | deferred | scalar image/field color mapping, text labels |
| categorical legend | simple swatches and labels | deferred | legend semantics, text subset |
| scale bar | 2D scale bar with label | deferred | panzoom/domain update path and text subset |
| annotation/readout | anchored text/readout | deferred | text subset, picking/probe update path |
| overlay cards | basic panel-anchored overlays | deferred | layout and text subset |


## Interaction And Feature Matrix

| Feature | WASM/WebGPU target | Status | Required work |
| --- | --- | --- | --- |
| panzoom | browser pointer/wheel to C controller | current | keep smoke coverage |
| arcball | browser drag/wheel to C controller | current | keep smoke coverage |
| fly/turntable | browser input to C controllers | next | expose controller creation/binding variants and smoke |
| frame callbacks | portable example frame callback | next | `DvzExampleSpec` host runner and WASM frame entrypoint |
| resize/high-DPI | browser resize to C scene and packet replay | current | keep capability/resize diagnostics |
| picking/query | request/poll query results | later | WASM query ABI, WebGPU readback latency policy |
| selection | update retained selection state | later | query path, visual state update path |
| capture | browser artifact capture | later | browser screenshot/video policy separate from native capture |
| streaming/partial updates | same-shape updates and bounded growth | partial | buffer update ABI, diagnostics, memory limits |
| compute-to-render | scene compute writes render buffers | next | scene buffer/compute ABI and particle smoke browser proof |
| techniques | EDL/SSAO/WBOIT/depth peeling | deferred | per-technique WebGPU feasibility and fallback diagnostics |


## Implementation Order

### Phase 1: Make Examples Portable

1. Implement the portable scenario helper and native host runner.
2. Add a generic WASM example host beside the current scene ABI.
3. Keep one raw native scene/app example that shows the underlying API without helper indirection.
4. Convert small feature examples first, then showcases.
5. Mark native-integration examples explicitly as native-only.

### Phase 2: Particle Compute Parity

Promote `gpu_particle_smoke` as the first full write-once parity proof:

```text
same C particle scenario
  -> native Vulkan host
  -> WASM/WebGPU host
```

Required work:

1. refactor the example into shared C scenario plus native host;
2. provide GLSL and WGSL compute shader sources or use WGSL wherever accepted;
3. expose scene buffers through WASM;
4. expose buffer-backed point attributes through WASM;
5. expose scene compute and compute buffer binding through WASM;
6. update per-frame params through the shared scenario;
7. emit DRP2 packets with `ResourceBarrier`;
8. render particles in browser WebGPU;
9. add `wasm-scene-smoke` and `webgpu-browser-smoke` coverage.

Start with a browser particle count such as `32k` or `64k`, then raise after browser memory and
frame-time evidence is available.

### Phase 3: Core Visual Parity

Promote visuals in this order:

1. semantic text strings over the current low-level atlas glyph visual;
2. labels;
3. axes/colorbars/legends/scale bars/annotations;
4. sphere native-depth/material variants and reduced volume;
5. experimental splat and advanced techniques.

Each promotion must satisfy the promotion rule above.

### Phase 4: Query And Selection Parity

1. Define WASM query request/result ABI.
2. Implement WebGPU readback scheduling and latest-request-wins behavior.
3. Add point/image/label readback smokes.
4. Add selection state updates and browser-visible selection proof.
5. Document readback latency and unsupported formats.

### Phase 5: Larger Data And Reliability

1. Add browser memory-budget diagnostics.
2. Add long-run retained-session churn tests.
3. Add partial update/ring-buffer policies.
4. Add browser fixture dashboard rows for memory/lifecycle stress.
5. Add performance baselines for packet decode, upload, and render.


## Validation Commands

Browserless checks:

```bash
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
```

Browser evidence:

```bash
just webgpu-browser-smoke
```

DRP2 and scene checks:

```bash
just drp2-fixtures
./build/testing/dvztest scene/frame-plan-emit/drp2_compute_assisted
./build/testing/dvztest scene/scene-graph/compute_point_position_buffer_emits_drp2
```

Native GPU evidence, when Vulkan is available:

```bash
direnv exec . ./build/testing/dvztest scene/frame-plan-emit/runtime_compute_two_frames_glsl_executes
direnv exec . just example-c showcases/gpu_particle_smoke 120
```

Every parity milestone must end with:

```bash
git diff --check
```


## Documentation Requirements

Every promoted browser feature must update:

1. this parity plan if the matrix/status changes;
2. `docs/reference/webgpu-subset.md`;
3. `examples/webgpu/COMPAT.md` when fixture or browser evidence changes;
4. the relevant visual or feature reference page;
5. example metadata/manifest status when an example becomes portable or remains native-only.


## Risks

1. Shader language drift between GLSL/SPIR-V and WGSL.
2. Browser memory and buffer-size limits for dense scenes.
3. WebGPU readback latency and async result delivery.
4. Text shaping, font packaging, and glyph atlas portability.
5. Technique portability for WBOIT, depth peeling, EDL, and SSAO.
6. JavaScript demo shortcuts creeping back into scene semantics.
7. Example helper hiding too much of the public scene API.

Mitigation: keep one raw scene/app example, require WGSL/fixture/browser evidence for promoted
features, and keep browser runtime generic over DRP2 packets.


## Acceptance Criteria

Broad WASM/WebGPU parity is credible when:

1. scene-level examples are portable C scenarios by default;
2. `gpu_particle_smoke` runs as the same C scenario on native Vulkan and browser WebGPU;
3. the visual matrix has explicit status for every retained visual family;
4. unsupported browser features produce deterministic diagnostics;
5. native and browser fixture evidence is recorded for each promoted feature;
6. browser demos contain no scene-family-specific rendering logic;
7. new scene features cannot be marked browser-supported without tests, docs, diagnostics, and
   browser evidence.
