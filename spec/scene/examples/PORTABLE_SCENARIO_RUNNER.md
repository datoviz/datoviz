# Portable Scenario Runner

Execution Status:

- Status: RC architecture candidate for v0.4 portability work
- Updated on: 2026-06-08
- Purpose: make most scene examples compile as portable C scenarios and run through native,
  offscreen, capture, and browser hosts
- Scope: scenario/runner boundaries, native runner modes, WASM host expectations, capture
  ownership, migration sequence, and promotion rules

Current implementation checkpoint:

1. `examples/c/runner/` contains the native GLFW/offscreen/capture runner.
2. `DvzScenarioSpec.requirements` and runner-side requirement diagnostics are active in example
   support code.
3. `basic_scene`, `timer_animation`, `video_export`, `picking`, and `image_probe` carry
   first-slice requirement metadata in code and
   `examples/c/MANIFEST.yaml`.
4. Portable event and post-frame callbacks are active in the native runner; `picking` is the
   unified item-picking, hover, and selection proof on that bridge. `image_probe` proves the same
   bridge for sampled image pixel queries.
5. The WASM scenario host can drive `feature_timer_animation`, deliver browser pointer/wheel events
   to active scenario `event` callbacks, and report unsupported scenario requirements
   deterministically.
6. Browser-side query/readback packet emission and WebGPU readback plumbing exist for
   `feature_picking` and `feature_image_probe`. `wasm-picking` and `wasm-image-probe` are
   browser-live.

Current native escape hatches to retire or classify:

1. migrate next: `probe_labels.c`;
2. keep native-only or defer until compute/query browser support: `gpu_particle_smoke.c`,
   `textured_planet.c`, and `linked_probe_colorbar.c`;
3. add manifest classifications for every public C example once the query/probe migration pattern is
   stable.

Near-term implementation order:

1. use `probe_labels.c` as the categorical query follow-up after the image-probe pixel query proof;
2. classify live WebGPU vs native-only examples from manifest metadata.

Full visual and feature parity sequencing lives in
[../integration/WASM_WEBGPU_PARITY_PLAN.md](../integration/WASM_WEBGPU_PARITY_PLAN.md). This file
defines the example architecture used by that parity program.


## Problem

Most current C examples mix two responsibilities:

1. portable scene logic: scene, figures, panels, visuals, retained data, scene buffers, compute,
   controllers, and per-frame updates;
2. host/runtime logic: `dvz_app()`, `dvz_view_window()`, offscreen views, frame loops, capture,
   progress output, command-line modes, native window input, and browser lifecycle glue.

That shape is readable for small native examples, but it blocks direct reuse in WASM/WebGPU. A
browser host has different presentation and scheduling machinery:

```text
DOM canvas -> requestAnimationFrame -> browser input -> portable C scenario ->
scene/DRP2 packets -> WebGPU runtime
```

The goal is not to rewrite examples in JavaScript. The goal is to split examples so the scenario
logic remains ordinary C and only the runner changes.


## Architecture

The unit of portable example code is a **scenario**. The user-facing executable, browser page, test,
or capture job is a **runner**.

```text
portable C scenario
  -> native GLFW runner
  -> native offscreen runner
  -> native capture runner
  -> browser/WASM runner
  -> CI/evidence runner
```

The scenario owns:

1. scene-level construction;
2. figures, panels, visuals, cameras, controllers, retained buffers, and scene compute;
3. data initialization and retained updates;
4. abstract frame, resize, pointer, wheel, keyboard, and destroy callbacks;
5. capability requirements and shader-source selection;
6. diagnostics for unsupported scenario requirements.

The runner owns:

1. app, native window, offscreen target, or browser canvas creation;
2. frame scheduling and event-loop integration;
3. input normalization before passing portable events to the scenario;
4. capture, video, PNG, DVZR, browser screenshots, and evidence artifacts;
5. native/backend capability discovery;
6. CLI parsing and mode presets;
7. progress/status output;
8. runtime-specific teardown.

Scenario code must not directly call GLFW, native app, Vulkan, WebGPU, browser APIs, or capture
APIs unless the example is explicitly about that host/backend. Native-only host examples remain
allowed, but they are exceptions, not the default pattern for feature, visual, workflow, showcase,
and scientific examples.


## Scenario API Sketch

The first implementation should live in example-support code, not public `include/datoviz/`, until
several examples prove the shape.

```c
typedef struct DvzScenarioContext DvzScenarioContext;
typedef struct DvzScenarioSpec DvzScenarioSpec;
typedef struct DvzScenarioQueryResult DvzScenarioQueryResult;

typedef bool (*DvzScenarioInitFn)(DvzScenarioContext* ctx, void** out_user);
typedef void (*DvzScenarioFrameFn)(DvzScenarioContext* ctx, void* user);
typedef void (*DvzScenarioResizeFn)(
    DvzScenarioContext* ctx, uint32_t width, uint32_t height, void* user);
typedef void (*DvzScenarioPointerFn)(
    DvzScenarioContext* ctx, const DvzScenarioPointerEvent* event, void* user);
typedef void (*DvzScenarioWheelFn)(
    DvzScenarioContext* ctx, const DvzScenarioWheelEvent* event, void* user);
typedef void (*DvzScenarioQueryResultFn)(
    DvzScenarioContext* ctx, const DvzScenarioQueryResult* result, void* user);
typedef void (*DvzScenarioDestroyFn)(DvzScenarioContext* ctx, void* user);

struct DvzScenarioSpec
{
    const char* id;
    const char* title;
    uint32_t width;
    uint32_t height;
    double fps;
    uint64_t requirements;
    bool continuous_frames;

    DvzScenarioInitFn init;
    DvzScenarioFrameFn frame;
    DvzScenarioResizeFn resize;
    DvzScenarioPointerFn pointer;
    DvzScenarioWheelFn wheel;
    DvzScenarioQueryResultFn query_result;
    DvzScenarioDestroyFn destroy;
};
```

The context exposes portable scene state and frame timing. It deliberately omits `DvzApp`,
`DvzView`, GLFW handles, Vulkan handles, and capture objects.

```c
struct DvzScenarioContext
{
    DvzScene* scene;
    DvzFigure* figure;

    uint32_t width;
    uint32_t height;

    double time;
    double dt;
    uint64_t frame_index;

    DvzSceneShaderFormat shader_format;
    const DvzCapabilitySnapshot* capabilities;
};
```

Portable event payloads should be normalized by the runner:

```c
typedef struct DvzScenarioPointerEvent
{
    float x;
    float y;
    float content_scale;
    uint32_t button;
    uint32_t modifiers;
    double timestamp_ms;
} DvzScenarioPointerEvent;

typedef struct DvzScenarioWheelEvent
{
    float x;
    float y;
    float dx;
    float dy;
    uint32_t modifiers;
    double timestamp_ms;
} DvzScenarioWheelEvent;
```

Requirements should be explicit so unsupported browser runs fail before partial rendering:

```c
enum
{
    DVZ_SCENARIO_REQ_POINT_VISUAL = 1ull << 0,
    DVZ_SCENARIO_REQ_MARKER_VISUAL = 1ull << 1,
    DVZ_SCENARIO_REQ_MESH_VISUAL = 1ull << 2,
    DVZ_SCENARIO_REQ_IMAGE_VISUAL = 1ull << 3,
    DVZ_SCENARIO_REQ_TEXT_VISUAL = 1ull << 4,
    DVZ_SCENARIO_REQ_SCENE_BUFFERS = 1ull << 5,
    DVZ_SCENARIO_REQ_STORAGE_BUFFERS = 1ull << 6,
    DVZ_SCENARIO_REQ_SCENE_COMPUTE = 1ull << 7,
    DVZ_SCENARIO_REQ_QUERY_READBACK = 1ull << 8,
    DVZ_SCENARIO_REQ_FRAME_CALLBACKS = 1ull << 9,
    DVZ_SCENARIO_REQ_NATIVE_CAPTURE = 1ull << 10,
};
```

Browser/WASM runners must treat request/query/readback as asynchronous. Scenario code may enqueue a
query during pointer, wheel, or frame handling, but results arrive later through polling or
`query_result`. Native runners may keep blocking helpers where supported, but portable scenarios
must not depend on synchronous readback.


## Runner API Sketch

The runner is the host abstraction. It converts user-facing modes into native or browser runtime
behavior.

```c
typedef enum DvzRunnerPresentation
{
    DVZ_RUNNER_PRESENT_GLFW,
    DVZ_RUNNER_PRESENT_OFFSCREEN,
    DVZ_RUNNER_PRESENT_BROWSER,
} DvzRunnerPresentation;

typedef enum DvzRunnerCaptureKind
{
    DVZ_RUNNER_CAPTURE_NONE,
    DVZ_RUNNER_CAPTURE_VIDEO,
    DVZ_RUNNER_CAPTURE_PNG,
    DVZ_RUNNER_CAPTURE_DVZR,
} DvzRunnerCaptureKind;

typedef struct DvzRunnerConfig
{
    DvzRunnerPresentation presentation;

    uint32_t width;
    uint32_t height;
    uint32_t frame_count; // 0 = interactive
    double fps;

    DvzRunnerCaptureKind capture_kind;
    DvzAppCaptureConfig capture; // native runner only

    bool print_progress;
    bool pace_wall_time;
} DvzRunnerConfig;
```

The native entry point should be small:

```c
int dvz_scenario_run_native(
    const DvzScenarioSpec* spec, const DvzRunnerConfig* config);
```

Command-line parsing should be reusable runner code, not repeated in each example:

```c
int dvz_scenario_run_native_cli(
    const DvzScenarioSpec* spec, int argc, char** argv);
```

Mode presets are runner policy:

```text
--live
  presentation = GLFW
  frame_count = 0
  capture = none
  pace_wall_time = app scheduler/fps cap

--live-record 120
  presentation = GLFW
  frame_count = 120
  capture = video
  pace_wall_time = true

--offscreen-record 120
  presentation = OFFSCREEN
  frame_count = 120
  capture = video
  pace_wall_time = false

--png
  presentation = OFFSCREEN unless overridden
  frame_count = 1
  capture = png

--dvzr 120
  presentation = OFFSCREEN unless overridden
  frame_count = 120
  capture = dvzr
```

These presets should still allow environment overrides such as `DVZ_CAPTURE_DIR`,
`DVZ_CAPTURE_BASENAME`, `DVZ_CAPTURE_FPS`, `DVZ_CAPTURE_VIDEO_BACKEND`, and
`DVZ_CAPTURE_VIDEO_MODE`.


## Timing Policy

Scenario time, presentation pacing, and recording sampling are separate concerns.

The scenario receives logical frame time from the runner. For deterministic examples this should be
the scene clock at `config->fps`, not direct wall-clock deltas from the host event loop. That keeps
native, offscreen, capture, and browser hosts able to drive the same scenario callback contract.

Presentation pacing is host policy. Interactive GLFW runs should use the app scheduler and FPS cap
so visible motion matches the scenario's requested frame rate. Finite GLFW modes such as
`--live-record N` should be paced to wall time, because users are inspecting the live desktop window
while capture runs.

Recording sampling is output policy. `--offscreen-record N` should stay offline and unpaced: it
renders exactly `N` frames at the requested capture FPS and may finish faster than real time. The
encoded video duration comes from sample timing, not wall-clock export duration. `--live-record N`
records the same logical scenario frame stream while also pacing presentation, so it is useful for
diagnosing mismatches between what the user sees and what capture writes.

This timing policy should remain runner-local until it is proven across more examples. Do not add a
public `dvz_app_run_paced()` helper just for video export. If finite paced runs become useful for
applications outside examples, promote a broader app run-policy descriptor instead.


## Interaction And Controller Bridge

Scenarios must not receive `DvzView*`, GLFW handles, browser event objects, or host-specific input
objects. Those objects belong to the runner. This keeps the same scenario source usable by a native
GLFW runner, an offscreen runner, and a browser/WASM runner.

Interactive examples need more than builtin controller names. They need a bridge with three
separate concepts:

1. normalized host events: pointer, wheel, keyboard, resize, focus, and frame notifications;
2. runner-owned tools: panzoom, arcball, fly, turntable, picking/query dispatch, cursor policy, and
   platform key/button translation;
3. scenario callbacks: portable event handlers that mutate scene objects, retained visual data, or
   scenario state.

A scenario should request controller behavior without constructing it directly:

```c
typedef enum DvzScenarioControllerKind
{
    DVZ_SCENARIO_CONTROLLER_PANZOOM,
    DVZ_SCENARIO_CONTROLLER_ARCBALL,
    DVZ_SCENARIO_CONTROLLER_FLY,
    DVZ_SCENARIO_CONTROLLER_TURNTABLE,
} DvzScenarioControllerKind;

typedef struct DvzScenarioControllerRequest
{
    DvzPanel* panel;
    DvzScenarioControllerKind kind;
    vec2 initial_pan;
    vec2 initial_zoom;
} DvzScenarioControllerRequest;
```

The native runner can translate a panzoom request into `dvz_view_panzoom(view, panel, ...)`. A
browser runner can translate the same request into DOM pointer/wheel handlers and equivalent
controller state updates. The scenario only sees the panel, the request handle, and portable state.

Callbacks should receive panel-relative, normalized payloads:

```c
typedef enum DvzScenarioEventKind
{
    DVZ_SCENARIO_EVENT_POINTER,
    DVZ_SCENARIO_EVENT_WHEEL,
    DVZ_SCENARIO_EVENT_KEY,
    DVZ_SCENARIO_EVENT_RESIZE,
    DVZ_SCENARIO_EVENT_CONTROLLER_CHANGED,
    DVZ_SCENARIO_EVENT_PICK_RESULT,
} DvzScenarioEventKind;

typedef struct DvzScenarioPointerEvent
{
    DvzPanel* panel;
    float x_px;
    float y_px;
    double x_data;
    double y_data;
    uint32_t button;
    uint32_t modifiers;
    double timestamp_ms;
} DvzScenarioPointerEvent;

typedef void (*DvzScenarioEventFn)(
    DvzScenarioContext* ctx,
    const DvzScenarioEvent* event,
    void* user);
```

Picking and query readback should stay asynchronous and runner-owned. Scenario code can request a
pick against a panel and receive a later `DVZ_SCENARIO_EVENT_PICK_RESULT` callback. Native runners
can fulfill this through GPU readback and app-loop completion. Browser runners can fulfill it
through WebGPU readback promises or report unsupported requirements deterministically.

Examples that should wait for this bridge include `panel_multi.c`, `panzoom.c`,
controller examples, picking examples, hover/probe examples, and selection examples. Migrating them
before the bridge would either leak `DvzView*` into scenarios or silently drop important behavior.


## Native Runner Flow

The native runner owns the app/view/capture lifecycle:

```c
int dvz_scenario_run_native(
    const DvzScenarioSpec* spec, const DvzRunnerConfig* config)
{
    DvzScene* scene = dvz_scene();
    dvz_scene_set_clock_mode(scene, DVZ_SCENE_CLOCK_FIXED_STEP);
    dvz_scene_set_fps(scene, config->fps > 0 ? config->fps : spec->fps);

    DvzScenarioContext ctx = {
        .scene = scene,
        .width = config->width != 0 ? config->width : spec->width,
        .height = config->height != 0 ? config->height : spec->height,
    };

    void* user = NULL;
    if (!spec->init(&ctx, &user) || ctx.figure == NULL)
        return -1;

    DvzAppConfig app_config = dvz_app_config();
    if (config->presentation == DVZ_RUNNER_PRESENT_GLFW)
        app_config.fps_cap = config->fps;
    DvzApp* app = dvz_app_with_config(scene, &app_config);

    DvzView* view = NULL;
    if (config->presentation == DVZ_RUNNER_PRESENT_GLFW)
        view = dvz_view_window(app, ctx.figure, ctx.width, ctx.height, spec->title);
    else
        view = dvz_view_offscreen(app, ctx.figure, ctx.width, ctx.height);

    install_frame_bridge(scene, spec, &ctx, user);
    install_input_bridge(view, spec, &ctx, user);
    install_progress_bridge(view, config);

    DvzView* capture_view = view;
    if (config->presentation == DVZ_RUNNER_PRESENT_GLFW &&
        config->capture_kind == DVZ_RUNNER_CAPTURE_VIDEO)
    {
        capture_view = dvz_view_offscreen(app, ctx.figure, ctx.width, ctx.height);
    }

    if (config->capture_kind != DVZ_RUNNER_CAPTURE_NONE)
        dvz_view_capture_start(capture_view, &config->capture);

    if (config->pace_wall_time)
        run_paced(app, config->frame_count, config->fps);
    else
        dvz_app_run(app, config->frame_count);

    if (config->capture_kind != DVZ_RUNNER_CAPTURE_NONE)
        dvz_view_capture_stop(capture_view);

    if (spec->destroy != NULL)
        spec->destroy(&ctx, user);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
```

The frame bridge updates `ctx.time`, `ctx.dt`, and `ctx.frame_index`, then calls
`spec->frame(&ctx, user)`. The native implementation can use scene timer animations internally, but
the scenario should see the same logical frame callback on native and browser hosts.


## Capture And Evidence

Capture is host output, not scenario behavior.

Native runner outputs:

1. video MP4;
2. PNG;
3. DVZR;
4. combinations such as video plus DVZR.

Browser runner outputs:

1. canvas snapshots;
2. browser screenshots or headless-browser evidence;
3. possibly `MediaRecorder` video later.

Scenarios should never branch on “am I recording?” unless the scenario is specifically a capture
diagnostic. Recording must sample the same logical scenario frame stream as live display. A native
`--live-record` mode is therefore required in the first implementation because it shows a visible
GLFW preview while recording a synchronized capture view. The first native implementation may record
an offscreen view during live preview to avoid platform-specific GLFW framebuffer capture issues,
but the scenario callback stream must remain shared.


## Conceptual Scenario Example

```c
typedef struct
{
    DvzVisual* point;
    vec3 positions[64];
    DvzColor colors[64];
    float sizes[64];
} PointWave;

static bool point_wave_init(DvzScenarioContext* ctx, void** out_user)
{
    PointWave* wave = dvz_calloc(1, sizeof(PointWave));

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    DvzPanel* panel = dvz_panel_full(ctx->figure);

    wave->point = dvz_point(ctx->scene, 0);
    fill_initial_points(wave);
    upload_points(wave);
    dvz_panel_add_visual(panel, wave->point, NULL);

    *out_user = wave;
    return true;
}

static void point_wave_frame(DvzScenarioContext* ctx, void* user)
{
    PointWave* wave = user;
    fill_points_at_time(wave, ctx->time);
    upload_points(wave);
}

static void point_wave_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}

DvzScenarioSpec point_wave_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "point_wave",
        .title = "Point wave",
        .width = 1600,
        .height = 1200,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL,
        .init = point_wave_init,
        .frame = point_wave_frame,
        .destroy = point_wave_destroy,
    };
}
```

The native executable is host glue:

```c
int main(int argc, char** argv)
{
    return dvz_scenario_run_native_cli(&point_wave_scenario(), argc, argv);
}
```

The browser host instantiates the same spec and supplies browser frame/input calls:

```c
EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_scenario_create_point_wave(uint32_t width, uint32_t height)
{
    return dvz_wasm_scenario_create(point_wave_scenario(), width, height);
}
```


## File Layout

The first slice can stay under `examples/c`:

```text
examples/c/runner/
  scenario_runner.h
  scenario_runner.c
  scenario_cli.c

examples/c/features/
  timer_animation.c    # first runner-backed animated executable
  basic_scene.c        # candidate static scenario migration
```

If scenarios become shared by native and WASM builds, split portable scenario sources from native
entry points:

```text
examples/scenarios/
  features/timer_animation_scenario.c

examples/c/runtime/
  video_export.c       # direct native app-capture example

examples/c/features/
  timer_animation.c    # native main only

examples/wasm/
  scenario_host.c
```


## Low-Level Native Pattern

The repository should still keep one small low-level retained scene/app example for users who need
to see the underlying native host API without runner indirection:

```c
DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
DvzPanel* panel = dvz_panel_full(figure);
DvzVisual* points = dvz_point(scene, 0);

dvz_visual_set_data(points, "position", positions, count);
dvz_visual_set_data(points, "color", colors, count);
dvz_visual_set_data(points, "diameter_px", sizes, count);
dvz_panel_add_visual(panel, points, NULL);

DvzApp* app = dvz_app(scene);
DvzView* view = dvz_view_window(app, figure, 800, 600, "raw scene");
dvz_app_run(app, 0);
dvz_app_destroy(app);
dvz_scene_destroy(scene);
```

This low-level example documents retained scene/app construction. It is not a raw scene-emitted
stream example, and should not be the template for most gallery, feature, workflow, showcase, or
scientific examples.


## Shader Policy

Portable scenarios should prefer WGSL when possible. If a native path still needs GLSL/SPIR-V, the
scenario should provide both shader sources and select through `ctx->shader_format` or capability
policy.

Shader duplication is acceptable at this layer because shader languages are backend contracts. C
scenario duplication is not acceptable for scene behavior.


## Promotion Rules

Feature, visual, workflow, showcase, and scientific examples should use the portable scenario shape
by default once the runner is in place.

A scenario is portable when:

1. scene logic compiles without app/window/capture headers;
2. it exposes a `DvzScenarioSpec`;
3. native runner can run it live;
4. native runner can run it offscreen;
5. native runner can produce capture evidence when required;
6. WASM runner can instantiate it or report unsupported requirements deterministically.

Native-only examples are exceptions for examples whose subject is native integration itself:
Vulkan/vklite ownership, GLFW hosting, Qt/PyQt hosting, native video encoders, CUDA/Vulkan
external-memory interop, native GUI diagnostics, or platform packaging.

Browser-only examples are exceptions for examples whose subject is browser integration itself.

Examples are browser-live candidates by default once they use only scene-level APIs and scenario
callbacks. The burden is on metadata to explain why an example is native-only, deferred, or
unsupported on WebGPU.

Example manifests should distinguish:

1. `portable-scenario`: same C scenario can run under native and browser runners;
2. `webgpu-live`: browser route is implemented and validated;
3. `webgpu-planned`: intended for the RC live-example target, but not yet current;
4. `webgpu-deferred`: useful browser target after RC;
5. `native-only`: requires native app/window handles, GUI, video, CUDA, capture, or backend
   diagnostics;
6. `browser-only`: exists specifically to test browser integration.

Each `webgpu-live` or `webgpu-planned` row should list requirement tags such as
`point`, `mesh`, `panzoom`, `frame-callbacks`, `continuous-frames`, `scene-buffers`,
`scene-compute`, and `query-readback`.


## Implementation Plan

### Phase 1: Native Runner Proof

1. Add example-support scenario types and the native runner in `examples/c/runner/`.
2. Support `--live`, `--live-record N`, `--video N`, `--offscreen-record N`, `--png`, and
   `--dvzr N`.
3. Refactor `timer_animation.c` first because it proves frame-time portability and deterministic
   gallery video capture through the runner.
4. Refactor `basic_scene.c` or another static example to prove scenarios without frame callbacks.

### Phase 2: Low-Risk Feature Migration

Convert simple examples whose behavior is mostly scene construction or retained data updates:

1. static/layout examples: `panel_single.c`, `panel_grid.c`, simple explicit-panel layouts;
2. retained-data examples: `update_visual_data.c`, `update_partial.c`, `visibility.c`;
3. visual-state examples: `alpha_blending.c`, `depth_test.c`, `panel_background.c`;
4. simple feature proofs: `marker_symbols.c`, `colorbar.c`, `colormap_scale.c`.

This phase should keep the runner API intentionally boring. Do not add interaction abstractions or
public Datoviz APIs just to finish this group.

### Phase 3: Interaction, Controllers, And Resize

Add portable runner bridges for resize, pointer, wheel, keyboard modifiers, controller requests,
asynchronous picking/query results, and request-frame signals. Current status: native pointer/key/
resize event translation and post-frame callbacks exist; `picking.c` and `image_probe.c` are
migrated off `native_view`; browser pointer/wheel event delivery exists; picking/selection and
image-probe async query result delivery are browser-live. Then migrate representative interaction
examples before broad conversion:

1. `probe_labels.c` for the categorical query follow-up;
2. `panel_multi.c` and `panzoom.c` after controller request helpers are settled.

Only after these examples pass should the runner input API be treated as stable enough for broader
feature migration.

### Phase 4: WASM Scenario Host

1. Add the generic WASM scenario host beside the existing WASM scene ABI.
2. Expose missing WASM scene APIs needed by portable scenarios: scene buffers,
   buffer-backed visual attributes, scene compute, compute buffer binding, dispatch updates,
   frame/time updates, and diagnostics.
3. Compile migrated static/data scenarios to WASM first.
4. Add browser evidence for one static scenario and one animated scenario.
5. Ensure unsupported requirements produce deterministic diagnostics rather than partial browser
   rewrites.

### Phase 5: Compute, Query, And Showcase Migration

Refactor `gpu_particle_smoke.c` into a shared scenario plus native host once the runner supports
compute requirements. This is the first high-value proof that the same C scenario can drive native
Vulkan and WASM/WebGPU evidence for a compute-to-render workflow.

In parallel with compute particles, promote the query examples in the same runner shape. The first
point, marker, hover, selection, and image-probe browser-live proofs exist. These examples prove
async request/query/readback delivery without promising full query parity for every visual family.

### Promotion Gate

Promote helper pieces into public Datoviz API only after several examples prove they are general
runtime concepts rather than example-runner policy. In particular, do not promote a narrow
`dvz_app_run_paced()` helper just for video export. If finite paced loops are needed outside the
runner, promote a more general run-policy descriptor such as `DvzAppRunConfig`.


## Acceptance Criteria

The architecture is successful when:

1. `timer_animation` uses the same frame callback contract on native and WASM-capable hosts;
2. runner-backed examples can be recorded with `--video N` or `--offscreen-record N`;
3. `gpu_particle_smoke` can be migrated to one C scenario for native and WASM/WebGPU;
4. unsupported WebGPU requirements produce deterministic diagnostics;
5. native examples remain easy to read because host boilerplate is isolated;
6. new scene-level examples can be added once and registered for multiple hosts.
