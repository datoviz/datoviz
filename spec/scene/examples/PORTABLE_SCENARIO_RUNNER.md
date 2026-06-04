# Portable Scenario Runner

Execution Status:

- Status: proposed example architecture for v0.4/v0.5 portability work
- Updated on: 2026-06-04
- Purpose: make scene examples write-once C scenarios that can run through native Vulkan hosts or
  browser WASM/WebGPU hosts
- Scope: example structure, host boundaries, helper API shape, and promotion rules

Full visual and feature parity sequencing lives in
[../integration/WASM_WEBGPU_PARITY_PLAN.md](../integration/WASM_WEBGPU_PARITY_PLAN.md). This file
only defines the example/scenario architecture used by that parity program.


## Problem

Most current C examples mix two concerns:

1. portable scene logic: data creation, retained visuals, scene buffers, compute, controllers, and
   per-frame updates;
2. native host logic: `dvz_app()`, `dvz_view_glfw()`, app callbacks, capture, window input, and
   native runtime setup.

That shape is clear for native users, but it prevents examples from compiling directly into the
browser path. A browser host has different presentation and lifecycle machinery:

```text
DOM canvas -> requestAnimationFrame -> browser input -> C/WASM scene -> DRP2 packets -> WebGPU
```

The goal is not to rewrite examples in JavaScript. The goal is to split examples so the scenario
logic remains ordinary C and only the host changes.


## Target Shape

Scene-level examples should be portable by default:

```text
portable C scenario
  -> native example host: app/window/capture/Vulkan
  -> WASM example host: browser lifecycle/input/DRP2 packets/WebGPU
```

The portable scenario owns:

1. scene, figure, panel, visual, scene-buffer, and scene-compute construction;
2. data initialization and retained updates;
3. abstract frame, resize, pointer, wheel, and destroy callbacks;
4. capability requirements and shader-source selection;
5. diagnostics for unsupported requirements.

The host owns:

1. native window or browser canvas creation;
2. frame scheduling;
3. input normalization;
4. capture or browser evidence;
5. backend capability discovery;
6. runtime-specific execution.

Scenario code must not directly call GLFW, native app, Vulkan, WebGPU, or browser APIs unless the
example is explicitly about that host/backend.


## Helper API Sketch

The helper should be small and boring. It exists to remove host boilerplate from examples, not to
hide the scene API.

```c
typedef struct DvzExampleContext DvzExampleContext;

typedef struct DvzExamplePointerEvent
{
    float x;
    float y;
    float content_scale;
    uint32_t button;
    uint32_t modifiers;
    double timestamp_ms;
} DvzExamplePointerEvent;

typedef bool (*DvzExampleInitFn)(DvzExampleContext* ctx, void** out_user);
typedef void (*DvzExampleFrameFn)(DvzExampleContext* ctx, void* user);
typedef void (*DvzExampleResizeFn)(DvzExampleContext* ctx, uint32_t width, uint32_t height, void* user);
typedef void (*DvzExamplePointerFn)(
    DvzExampleContext* ctx, const DvzExamplePointerEvent* event, void* user);
typedef void (*DvzExampleDestroyFn)(DvzExampleContext* ctx, void* user);

typedef struct DvzExampleSpec
{
    const char* id;
    const char* title;
    uint32_t width;
    uint32_t height;
    uint64_t requirements;
    DvzExampleInitFn init;
    DvzExampleFrameFn frame;
    DvzExampleResizeFn resize;
    DvzExamplePointerFn pointer;
    DvzExampleDestroyFn destroy;
} DvzExampleSpec;
```

The context should expose portable scene state and capabilities:

```c
struct DvzExampleContext
{
    DvzScene* scene;
    uint32_t width;
    uint32_t height;
    double time;
    float dt;
    uint64_t frame_index;
    DvzSceneShaderFormat shader_format;
    const DvzCapabilitySnapshot* capabilities;
};
```

Example requirements should be explicit so unsupported browser runs fail before partial rendering:

```c
enum
{
    DVZ_EXAMPLE_REQ_POINT_VISUAL = 1ull << 0,
    DVZ_EXAMPLE_REQ_MESH_VISUAL = 1ull << 1,
    DVZ_EXAMPLE_REQ_IMAGE_VISUAL = 1ull << 2,
    DVZ_EXAMPLE_REQ_SCENE_BUFFERS = 1ull << 3,
    DVZ_EXAMPLE_REQ_STORAGE_BUFFERS = 1ull << 4,
    DVZ_EXAMPLE_REQ_SCENE_COMPUTE = 1ull << 5,
    DVZ_EXAMPLE_REQ_QUERY_READBACK = 1ull << 6,
};
```


## Conceptual Scenario Example

The GPU particle smoke example should become a shared C scenario:

```c
typedef struct
{
    DvzFigure* figure;
    DvzPanel* panel;
    DvzSceneBuffer* params;
    DvzSceneBuffer* positions;
    DvzSceneBuffer* velocities;
    DvzSceneCompute* compute;
    float sim_time;
    vec2 mouse;
    bool mouse_valid;
} ParticleSmoke;

static bool particle_init(DvzExampleContext* ctx, void** out_user)
{
    ParticleSmoke* smoke = dvz_calloc(1, sizeof(ParticleSmoke));

    smoke->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    smoke->panel = dvz_panel_full(smoke->figure);

    smoke->positions = make_scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
        sizeof(vec3), PARTICLE_COUNT * sizeof(vec3));
    smoke->velocities = make_scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec3),
        PARTICLE_COUNT * sizeof(vec3));
    smoke->params = make_scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec4), 3 * sizeof(vec4));

    upload_initial_particles(smoke);

    DvzVisual* points = dvz_point(ctx->scene, 0);
    dvz_visual_set_attr_buffer(points, "position", smoke->positions, 0, PARTICLE_COUNT);
    dvz_visual_set_attr_buffer(points, "color", make_color_buffer(ctx), 0, PARTICLE_COUNT);
    dvz_visual_set_attr_buffer(points, "diameter", make_size_buffer(ctx), 0, PARTICLE_COUNT);
    dvz_panel_add_visual(smoke->panel, points, NULL);

    DvzSceneComputeDesc desc = dvz_scene_compute_desc();
    desc.label = "particle_smoke";
    desc.shader_format = ctx->shader_format;
    desc.shader_source = particle_shader_source(ctx->shader_format);
    desc.dispatch[0] = (PARTICLE_COUNT + 127) / 128;
    desc.dispatch[1] = 1;
    desc.dispatch[2] = 1;

    smoke->compute = dvz_scene_compute(ctx->scene, &desc);
    dvz_scene_compute_set_buffer(
        smoke->compute, 0, smoke->params, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, PARAM_BYTES);
    dvz_scene_compute_set_buffer(
        smoke->compute, 1, smoke->positions, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
        POSITION_BYTES);
    dvz_scene_compute_set_buffer(
        smoke->compute, 2, smoke->velocities, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
        VELOCITY_BYTES);
    dvz_figure_add_compute(smoke->figure, smoke->compute);

    *out_user = smoke;
    return true;
}

static void particle_frame(DvzExampleContext* ctx, void* user)
{
    ParticleSmoke* smoke = user;
    smoke->sim_time += ctx->dt;

    vec4 params[3] = {0};
    fill_particle_params(smoke, ctx, params);
    dvz_scene_buffer_set_data(smoke->params, params, sizeof(params));
    dvz_example_request_frame(ctx);
}

static void particle_pointer(
    DvzExampleContext* ctx, const DvzExamplePointerEvent* event, void* user)
{
    ParticleSmoke* smoke = user;
    smoke->mouse[0] = 2.0f * event->x / (float)ctx->width - 1.0f;
    smoke->mouse[1] = 1.0f - 2.0f * event->y / (float)ctx->height;
    smoke->mouse_valid = true;
}

static void particle_destroy(DvzExampleContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}

DvzExampleSpec particle_smoke_spec(void)
{
    return (DvzExampleSpec){
        .id = "gpu_particle_smoke",
        .title = "GPU particle smoke",
        .width = 1600,
        .height = 1200,
        .requirements = DVZ_EXAMPLE_REQ_POINT_VISUAL | DVZ_EXAMPLE_REQ_SCENE_BUFFERS |
                        DVZ_EXAMPLE_REQ_STORAGE_BUFFERS | DVZ_EXAMPLE_REQ_SCENE_COMPUTE,
        .init = particle_init,
        .frame = particle_frame,
        .pointer = particle_pointer,
        .destroy = particle_destroy,
    };
}
```

The native executable should be only host glue:

```c
int main(int argc, char** argv)
{
    return dvz_example_run_native(particle_smoke_spec(), argc, argv);
}
```

The WASM host should instantiate the same spec, then expose generic browser calls:

```c
EMSCRIPTEN_KEEPALIVE
uint32_t dvz_wasm_example_create_particle_smoke(uint32_t width, uint32_t height)
{
    return dvz_wasm_example_create(particle_smoke_spec(), width, height);
}
```

The browser JavaScript remains loader and event glue:

```js
const session = await WasmExampleSession.create(canvas, { example: "gpu_particle_smoke" });

function frame(now) {
  session.frame(now);
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
```


## Raw Pattern Example

The repository should still keep one small raw scene/app example for users who need to see the
underlying native pattern without helper indirection:

```c
DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
DvzPanel* panel = dvz_panel_full(figure);
DvzVisual* points = dvz_point(scene, 0);

dvz_visual_set_data(points, "position", positions, count);
dvz_visual_set_data(points, "color", colors, count);
dvz_visual_set_data(points, "diameter", sizes, count);
dvz_panel_add_visual(panel, points, NULL);

DvzApp* app = dvz_app(scene);
DvzView* view = dvz_view_glfw(app, figure, 800, 600, "raw scene");
dvz_app_run(app, 0);
dvz_app_destroy(app);
```

This raw example is documentation for the native host API. It should not be the template for most
gallery, feature, or scientific examples.


## Shader Policy

Portable examples should prefer WGSL when possible. If a native path still needs GLSL/SPIR-V, the
scenario should provide both shader sources and select through `ctx->shader_format` or capability
policy.

Shader duplication is acceptable at this layer because shader languages are backend contracts. C
scenario duplication is not acceptable for scene behavior.


## Promotion Rules

1. All scene, visual, showcase, scientific, workflow, and feature examples should use the portable
   scenario shape by default.
2. Native-only examples are exceptions for examples whose subject is native integration itself:
   Vulkan/vklite ownership, GLFW hosting, Qt/PyQt hosting, native video encoders, CUDA/Vulkan
   external-memory interop, native GUI diagnostics, or platform packaging.
3. Browser-only examples are exceptions for examples whose subject is browser integration itself.
4. If WebGPU lacks a scenario requirement, the WASM host must report an unsupported-feature
   diagnostic instead of needing a JavaScript rewrite.
5. A scenario is browser-supported only when the same C scenario builds for WASM, emits DRP2
   packets, passes browserless smoke, and has browser evidence.
6. Example manifests should distinguish `portable-scenario`, `native-only`, `browser-only`, and
   `unsupported-on-webgpu` status.


## Implementation Plan

1. Add the helper types and native runner in a small example-support module.
2. Add the generic WASM example host beside the existing WASM scene ABI.
3. Expose missing WASM scene APIs needed by portable scenarios: scene buffers, buffer-backed visual
   attributes, scene compute, compute buffer binding, dispatch updates, frame/time updates, and
   diagnostics.
4. Refactor `examples/c/showcases/gpu_particle_smoke.c` into shared scenario plus native host.
5. Add a WebGPU/WASM particle smoke demo that loads the shared C scenario, not a JavaScript rewrite.
6. Keep one minimal raw native scene/app example as the explicit underlying API pattern.
7. Convert remaining scene-level examples incrementally, starting with feature examples and
   compute/interaction showcases.


## Acceptance Criteria

The helper architecture is successful when:

1. `gpu_particle_smoke` uses one C scenario for native and WASM/WebGPU;
2. browser particle smoke uses the same C scene-buffer and scene-compute setup as native;
3. unsupported WebGPU requirements produce deterministic diagnostics;
4. native examples still remain easy to read because host boilerplate is isolated;
5. new scene-level examples can be added once and registered for both hosts.
