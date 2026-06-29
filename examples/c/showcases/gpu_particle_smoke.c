/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gpu_particle_smoke - scene API compute-to-graphics particle smoke showcase.
 *
 * Scenario: showcase_gpu_particle_smoke
 * Style: showcase, graphite_cyan, 1280x720 window target, experimental scene compute
 *
 * Build:  just example-c showcases/gpu_particle_smoke
 * Run:    ./build/examples/c/showcases/gpu_particle_smoke --live
 * Smoke:  ./build/examples/c/showcases/gpu_particle_smoke --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT
#ifdef DVZ_EXAMPLE_NO_APP
#define PARTICLE_COUNT          32768u
#else
#define PARTICLE_COUNT          262144u
#endif
#define WORKGROUP_SIZE          128u
#define COMPUTE_SOURCE_CAPACITY 8192u
#define SIM_SPEED               0.65f
#define SIM_MAX_DT              (1.0f / 30.0f)

#define SMOKE_LIFETIME          4.6f
#define SMOKE_ALPHA_BASE        0.26f
#define SMOKE_ALPHA_YOUNG_BOOST 0.18f
#define SMOKE_SIZE_MIN          1.25f
#define SMOKE_SIZE_MAX          3.75f
#define SMOKE_SOURCE_WIDTH      0.18f
#define SMOKE_SOURCE_HEIGHT     0.13f
#define SMOKE_TOP_FADE_START    0.90f

#define MOUSE_HOVER_RADIUS   0.24f
#define MOUSE_HOVER_SWIRL    1.35f
#define MOUSE_VELOCITY_LIMIT 3.0f
#define MOUSE_VELOCITY_DECAY 0.08f
#define MOUSE_FORCE_SCALE    8.0f
#define MOUSE_SPEED_LIMIT    2.4f

#define GLSL_STR_(x) #x
#define GLSL_STR(x)  GLSL_STR_(x)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ParticleState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzSceneBuffer* params;
    float sim_time;
    bool mouse_valid;
    vec2 mouse_pos;
    vec2 mouse_velocity;
    uint64_t mouse_timestamp;
    char compute_glsl[COMPUTE_SOURCE_CAPACITY];
} ParticleState;



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

DvzScenarioSpec dvz_showcase_gpu_particle_smoke_scenario(void);



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static const char* COMPUTE_GLSL_COMMON =
    "layout(local_size_x = 128) in;\n"
    "layout(std430, set = 0, binding = 0) readonly buffer Params {\n"
    "    vec4 sim0;\n"
    "    vec4 sim1;\n"
    "    vec4 sim2;\n"
    "} params;\n"
    "layout(std430, set = 0, binding = 1) buffer Positions { float x[]; } positions;\n"
    "layout(std430, set = 0, binding = 2) buffer Velocities { float v[]; } velocities;\n"
    "layout(std430, set = 0, binding = 3) buffer Ages { float age[]; } ages;\n"
    "uint hash_u32(uint x) {\n"
    "    x ^= x >> 16;\n"
    "    x *= 0x7feb352du;\n"
    "    x ^= x >> 15;\n"
    "    x *= 0x846ca68bu;\n"
    "    x ^= x >> 16;\n"
    "    return x;\n"
    "}\n"
    "float hash01(uint x) {\n"
    "    return float(hash_u32(x) & 0x00ffffffu) / 16777216.0;\n"
    "}\n"
    "vec2 source_pos(uint i, float t) {\n"
    "    uint epoch = uint(floor(t * 18.0));\n"
    "    float a = hash01(i * 1664525u + epoch * 1013904223u);\n"
    "    float b = hash01(i * 22695477u + epoch * 1103515245u);\n"
    "    float plume = 0.17 * sin(0.43 * t) + 0.06 * sin(1.31 * t);\n"
    "    return vec2(plume + (a * 2.0 - 1.0) * SMOKE_SOURCE_WIDTH,\n"
    "                -1.03 + b * SMOKE_SOURCE_HEIGHT);\n"
    "}\n"
    "vec2 curl_flow(vec2 p, float t) {\n"
    "    float c1 = cos(3.0 * p.x + 2.1 * p.y + 0.63 * t);\n"
    "    float c2 = cos(-2.4 * p.x + 3.7 * p.y - 0.41 * t);\n"
    "    float c3 = cos(5.3 * p.x - 1.9 * p.y + 0.27 * t);\n"
    "    vec2 grad = vec2(\n"
    "        3.0 * c1 - 2.4 * c2 + 5.3 * c3,\n"
    "        2.1 * c1 + 3.7 * c2 - 1.9 * c3);\n"
    "    vec2 curl = vec2(grad.y, -grad.x);\n"
    "    vec2 rise = vec2(0.10 * sin(2.2 * p.y + 0.7 * t), 0.78);\n"
    "    vec2 focus = vec2(-0.12 * p.x, -0.04 * p.y);\n"
    "    return 0.18 * curl + rise + focus;\n"
    "}\n"
    "\n";

static const char* COMPUTE_GLSL_MAIN =
    "void main() {\n"
    "    uint i = gl_GlobalInvocationID.x;\n"
    "    uint count = uint(params.sim0.z);\n"
    "    if (i >= count) return;\n"
    "    float t = params.sim0.x;\n"
    "    float dt = params.sim0.y;\n"
    "    float mouse_active = params.sim0.w;\n"
    "    vec2 mouse = params.sim1.xy;\n"
    "    vec2 mouse_v = params.sim1.zw;\n"
    "    float mouse_radius = params.sim2.x;\n"
    "    float hover_swirl = params.sim2.z;\n"
    "    uint j = 3u * i;\n"
    "    vec2 x = vec2(positions.x[j + 0u], positions.x[j + 1u]);\n"
    "    vec2 v = vec2(velocities.v[j + 0u], velocities.v[j + 1u]);\n"
    "    float age = ages.age[i] + dt;\n"
    "    vec2 flow = curl_flow(x, t);\n"
    "    v = mix(v, flow, clamp(dt * 2.6, 0.0, 1.0));\n"
    "    v += 0.020 * vec2(\n"
    "        sin(17.0 * x.y + float(i & 255u) * 0.017 + t),\n"
    "        cos(19.0 * x.x + float((i >> 8u) & 255u) * 0.013 - t));\n"
    "    if (mouse_active > 0.5) {\n"
    "        vec2 d = x - mouse;\n"
    "        float dist = length(d);\n"
    "        float influence = 1.0 - smoothstep(0.0, mouse_radius, dist);\n"
    "        vec2 dir = d / max(dist, 0.001);\n"
    "        vec2 tangent = vec2(-dir.y, dir.x);\n"
    "        float mouse_dt = clamp(dt * MOUSE_FORCE_SCALE, 0.0, 0.16);\n"
    "        v += mouse_dt * influence * hover_swirl * (tangent + 0.25 * mouse_v);\n"
    "        float speed = length(v);\n"
    "        if (speed > MOUSE_SPEED_LIMIT) v *= MOUSE_SPEED_LIMIT / speed;\n"
    "    }\n"
    "    v *= pow(0.986, dt / (1.0 / 120.0));\n"
    "    x += dt * v;\n"
    "    if (x.y > 1.10 || abs(x.x) > 1.18 || x.y < -1.12 || age > SMOKE_LIFETIME) {\n"
    "        x = source_pos(i, t);\n"
    "        v = vec2(0.03 * sin(float(i) * 0.11), 0.42 + 0.10 * hash01(i + uint(t * 97.0)));\n"
    "        age = 0.0;\n"
    "    }\n"
    "    positions.x[j + 0u] = x.x;\n"
    "    positions.x[j + 1u] = x.y;\n"
    "    positions.x[j + 2u] = 0.0;\n"
    "    velocities.v[j + 0u] = v.x;\n"
    "    velocities.v[j + 1u] = v.y;\n"
    "    velocities.v[j + 2u] = 0.0;\n"
    "    ages.age[i] = age;\n"
    "}\n";

static const char* COMPUTE_WGSL =
    "struct Params {\n"
    "    sim0: vec4f,\n"
    "    sim1: vec4f,\n"
    "    sim2: vec4f,\n"
    "}\n"
    "@group(0) @binding(0) var<storage, read> params: Params;\n"
    "@group(0) @binding(1) var<storage, read_write> positions: array<f32>;\n"
    "@group(0) @binding(2) var<storage, read_write> velocities: array<f32>;\n"
    "@group(0) @binding(3) var<storage, read_write> ages: array<f32>;\n"
    "fn hash_u32(x_in: u32) -> u32 {\n"
    "    var x = x_in;\n"
    "    x = x ^ (x >> 16u);\n"
    "    x = x * 0x7feb352du;\n"
    "    x = x ^ (x >> 15u);\n"
    "    x = x * 0x846ca68bu;\n"
    "    x = x ^ (x >> 16u);\n"
    "    return x;\n"
    "}\n"
    "fn hash01(x: u32) -> f32 {\n"
    "    return f32(hash_u32(x) & 0x00ffffffu) / 16777216.0;\n"
    "}\n"
    "fn source_pos(i: u32, t: f32) -> vec2f {\n"
    "    let epoch = u32(floor(t * 18.0));\n"
    "    let a = hash01(i * 1664525u + epoch * 1013904223u);\n"
    "    let b = hash01(i * 22695477u + epoch * 1103515245u);\n"
    "    let plume = 0.17 * sin(0.43 * t) + 0.06 * sin(1.31 * t);\n"
    "    return vec2f(plume + (a * 2.0 - 1.0) * 0.18, -1.03 + b * 0.13);\n"
    "}\n"
    "fn curl_flow(p: vec2f, t: f32) -> vec2f {\n"
    "    let c1 = cos(3.0 * p.x + 2.1 * p.y + 0.63 * t);\n"
    "    let c2 = cos(-2.4 * p.x + 3.7 * p.y - 0.41 * t);\n"
    "    let c3 = cos(5.3 * p.x - 1.9 * p.y + 0.27 * t);\n"
    "    let grad = vec2f(3.0 * c1 - 2.4 * c2 + 5.3 * c3,\n"
    "                     2.1 * c1 + 3.7 * c2 - 1.9 * c3);\n"
    "    let curl = vec2f(grad.y, -grad.x);\n"
    "    let rise = vec2f(0.10 * sin(2.2 * p.y + 0.7 * t), 0.78);\n"
    "    let focus = vec2f(-0.12 * p.x, -0.04 * p.y);\n"
    "    return 0.18 * curl + rise + focus;\n"
    "}\n"
    "@compute @workgroup_size(128)\n"
    "fn main(@builtin(global_invocation_id) global_id: vec3u) {\n"
    "    let i = global_id.x;\n"
    "    let count = u32(params.sim0.z);\n"
    "    if (i >= count) { return; }\n"
    "    let t = params.sim0.x;\n"
    "    let dt = params.sim0.y;\n"
    "    let mouse_active = params.sim0.w;\n"
    "    let mouse = params.sim1.xy;\n"
    "    let mouse_v = params.sim1.zw;\n"
    "    let mouse_radius = params.sim2.x;\n"
    "    let hover_swirl = params.sim2.z;\n"
    "    let j = 3u * i;\n"
    "    var x = vec2f(positions[j + 0u], positions[j + 1u]);\n"
    "    var v = vec2f(velocities[j + 0u], velocities[j + 1u]);\n"
    "    var age = ages[i] + dt;\n"
    "    let flow = curl_flow(x, t);\n"
    "    v = mix(v, flow, clamp(dt * 2.6, 0.0, 1.0));\n"
    "    v += 0.020 * vec2f(sin(17.0 * x.y + f32(i & 255u) * 0.017 + t),\n"
    "                       cos(19.0 * x.x + f32((i >> 8u) & 255u) * 0.013 - t));\n"
    "    if (mouse_active > 0.5) {\n"
    "        let d = x - mouse;\n"
    "        let dist = length(d);\n"
    "        let influence = 1.0 - smoothstep(0.0, mouse_radius, dist);\n"
    "        let dir = d / max(dist, 0.001);\n"
    "        let tangent = vec2f(-dir.y, dir.x);\n"
    "        let mouse_dt = clamp(dt * 8.0, 0.0, 0.16);\n"
    "        v += mouse_dt * influence * hover_swirl * (tangent + 0.25 * mouse_v);\n"
    "        let speed = length(v);\n"
    "        if (speed > 2.4) { v *= 2.4 / speed; }\n"
    "    }\n"
    "    v *= pow(0.986, dt / (1.0 / 120.0));\n"
    "    x += dt * v;\n"
    "    if (x.y > 1.10 || abs(x.x) > 1.18 || x.y < -1.12 || age > 4.6) {\n"
    "        x = source_pos(i, t);\n"
    "        v = vec2f(0.03 * sin(f32(i) * 0.11), 0.42 + 0.10 * hash01(i + u32(t * 97.0)));\n"
    "        age = 0.0;\n"
    "    }\n"
    "    positions[j + 0u] = x.x;\n"
    "    positions[j + 1u] = x.y;\n"
    "    positions[j + 2u] = 0.0;\n"
    "    velocities[j + 0u] = v.x;\n"
    "    velocities[j + 1u] = v.y;\n"
    "    velocities[j + 2u] = 0.0;\n"
    "    ages[i] = age;\n"
    "}\n";



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static float _hash01(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return (float)(x & 0x00ffffffu) / (float)0x01000000u;
}


static bool _compute_shader_source(char* out, size_t size)
{
    ANN(out);
    const int n = dvz_snprintf(
        out, size,
        "#version 450\n"
        "#define SMOKE_LIFETIME " GLSL_STR(SMOKE_LIFETIME) "\n"
        "#define SMOKE_SOURCE_WIDTH " GLSL_STR(SMOKE_SOURCE_WIDTH) "\n"
        "#define SMOKE_SOURCE_HEIGHT " GLSL_STR(SMOKE_SOURCE_HEIGHT) "\n"
        "#define MOUSE_FORCE_SCALE " GLSL_STR(MOUSE_FORCE_SCALE) "\n"
        "#define MOUSE_SPEED_LIMIT " GLSL_STR(MOUSE_SPEED_LIMIT) "\n"
        "%s%s",
        COMPUTE_GLSL_COMMON, COMPUTE_GLSL_MAIN);
    return n >= 0 && (size_t)n < size;
}


static float _clamp01(float x)
{
    if (x < 0.0f)
        return 0.0f;
    if (x > 1.0f)
        return 1.0f;
    return x;
}


static float _clampf(float x, float lo, float hi)
{
    if (x < lo)
        return lo;
    if (x > hi)
        return hi;
    return x;
}


static float _vec2_norm(const vec2 v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1]);
}


static void _limit_vec2(vec2 v, float max_norm)
{
    const float norm = _vec2_norm(v);
    if (norm <= max_norm || norm <= 0.0f)
        return;

    const float scale = max_norm / norm;
    v[0] *= scale;
    v[1] *= scale;
}


static void _panel_to_sim(double x, double y, double width, double height, vec2 out)
{
    if (width <= 0.0)
        width = (double)WIDTH;
    if (height <= 0.0)
        height = (double)HEIGHT;

    const float u = _clamp01((float)(x / width));
    const float v = _clamp01((float)(y / height));
    out[0] = 2.0f * u - 1.0f;
    out[1] = 1.0f - 2.0f * v;
}


static float _smoothstepf(float edge0, float edge1, float x)
{
    const float t = _clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}


static uint8_t _mix_u8(uint8_t a, uint8_t b, float t)
{
    const float u = _clamp01(t);
    return (uint8_t)((1.0f - u) * (float)a + u * (float)b + 0.5f);
}


static DvzColor _particle_color(float life, float lane)
{
    const float warm = 1.0f - _smoothstepf(0.10f, 0.72f, life);
    const float cool = _smoothstepf(0.18f, 0.95f, life);
    uint8_t r = _mix_u8(246, 92, cool);
    uint8_t g = _mix_u8(126, 188, cool);
    uint8_t b = _mix_u8(54, 218, cool);
    r = _mix_u8(r, 246, 0.18f * warm);
    g = _mix_u8(g, 222, 0.12f * warm);
    b = _mix_u8(b, 196, 0.10f + 0.12f * lane);

    float alpha = SMOKE_ALPHA_BASE + SMOKE_ALPHA_YOUNG_BOOST * (1.0f - life);
    alpha *= _smoothstepf(0.00f, 0.08f, life);
    alpha *= 1.0f - _smoothstepf(SMOKE_TOP_FADE_START, 1.0f, life);
    alpha *= 0.55f + 0.25f * lane;
    return dvz_color_rgba(r, g, b, (uint8_t)(255.0f * _clamp01(alpha) + 0.5f));
}


static void
_init_particles(vec3* positions, vec3* velocities, float* ages, DvzColor* colors, float* sizes)
{
    ANN(positions);
    ANN(velocities);
    ANN(ages);
    ANN(colors);
    ANN(sizes);

    for (uint32_t i = 0; i < PARTICLE_COUNT; i++)
    {
        const float a = TAU * _hash01(i * 7u + 1u);
        const float r = sqrtf(_hash01(i * 7u + 2u));
        const float lane = _hash01(i * 7u + 3u);
        const float life = _hash01(i * 7u + 4u);
        const float vertical = powf(life, 0.78f);
        const float width = 0.045f + 0.26f * vertical;
        const float plume = 0.12f * sinf(4.2f * vertical + 0.00011f * (float)i);
        const float x = plume + width * r * cosf(a);
        const float y = -1.04f + 1.72f * vertical + 0.045f * r * sinf(a);
        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;
        velocities[i][0] = 0.04f * cosf(a) - 0.03f * x;
        velocities[i][1] = 0.34f + 0.16f * lane;
        velocities[i][2] = 0.0f;
        ages[i] = SMOKE_LIFETIME * life;
        colors[i] = _particle_color(life, lane);
        sizes[i] =
            (SMOKE_SIZE_MIN + (SMOKE_SIZE_MAX - SMOKE_SIZE_MIN) * _smoothstepf(0.0f, 1.0f, life)) *
            (0.74f + 0.28f * lane);
    }
}


static DvzSceneBuffer*
_scene_buffer(DvzScene* scene, uint32_t usage, uint32_t stride, uint64_t byte_size)
{
    DvzSceneBufferDesc desc = dvz_scene_buffer_desc();
    desc.usage = usage;
    desc.stride = stride;
    desc.byte_size = byte_size;
    return dvz_scene_buffer(scene, &desc);
}


static void _params_for_state(const ParticleState* state, float dt, vec4 params[3])
{
    ANN(state);
    ANN(params);
    const float active = state->mouse_valid ? 1.0f : 0.0f;

    params[0][0] = state->sim_time;
    params[0][1] = dt;
    params[0][2] = (float)PARTICLE_COUNT;
    params[0][3] = active;

    params[1][0] = state->mouse_pos[0];
    params[1][1] = state->mouse_pos[1];
    params[1][2] = state->mouse_velocity[0];
    params[1][3] = state->mouse_velocity[1];

    params[2][0] = MOUSE_HOVER_RADIUS;
    params[2][1] = 0.0f;
    params[2][2] = MOUSE_HOVER_SWIRL;
    params[2][3] = 0.0f;
}


static void _particle_pointer(ParticleState* state, const DvzScenarioPointerEvent* event)
{
    if (state == NULL || event == NULL)
        return;
    if (event->type == DVZ_SCENARIO_POINTER_WHEEL || event->type == DVZ_SCENARIO_POINTER_NONE)
        return;

    double x = 0.0;
    double y = 0.0;
    double panel_pos[2] = {0};
    if (!dvz_panel_transform_point(
            state->panel, DVZ_PANEL_COORD_FIGURE_PX, DVZ_PANEL_COORD_PANEL_PX,
            (const double[2]){event->x, event->y}, panel_pos))
    {
        state->mouse_valid = false;
        state->mouse_velocity[0] = 0.0f;
        state->mouse_velocity[1] = 0.0f;
        return;
    }
    x = panel_pos[0];
    y = panel_pos[1];

    DvzRect rect = {0};
    if (!dvz_panel_inner_rect_px(state->panel, &rect) || rect.width <= 0.0f || rect.height <= 0.0f)
        return;

    vec2 pos = {0};
    _panel_to_sim(x, y, (double)rect.width, (double)rect.height, pos);

    vec2 raw_velocity = {0};
    if (state->mouse_valid && event->timestamp_ns > state->mouse_timestamp)
    {
        const double seconds = (double)(event->timestamp_ns - state->mouse_timestamp) * 1e-9;
        const float dt = _clampf((float)seconds, 1.0f / 240.0f, 0.10f);
        raw_velocity[0] = (pos[0] - state->mouse_pos[0]) / dt;
        raw_velocity[1] = (pos[1] - state->mouse_pos[1]) / dt;
        _limit_vec2(raw_velocity, MOUSE_VELOCITY_LIMIT);
        state->mouse_velocity[0] = 0.45f * state->mouse_velocity[0] + 0.55f * raw_velocity[0];
        state->mouse_velocity[1] = 0.45f * state->mouse_velocity[1] + 0.55f * raw_velocity[1];
        _limit_vec2(state->mouse_velocity, MOUSE_VELOCITY_LIMIT);
    }
    else
    {
        state->mouse_velocity[0] = 0.0f;
        state->mouse_velocity[1] = 0.0f;
    }

    state->mouse_pos[0] = pos[0];
    state->mouse_pos[1] = pos[1];
    state->mouse_valid = true;
    state->mouse_timestamp = event->timestamp_ns;
}


static void _scenario_frame(DvzScenarioContext* ctx, void* user_data)
{
    ParticleState* state = (ParticleState*)user_data;
    if (ctx == NULL || state == NULL || state->params == NULL)
        return;

    float wall_dt = (float)ctx->dt;
    if (wall_dt <= 0.0f)
        wall_dt = 1.0f / 60.0f;
    const float sim_dt = _clampf(wall_dt, 0.0f, SIM_MAX_DT) * SIM_SPEED;
    state->sim_time += sim_dt;
    if (state->mouse_valid)
    {
        const float decay = powf(MOUSE_VELOCITY_DECAY, _clampf(wall_dt, 0.0f, 0.10f));
        state->mouse_velocity[0] *= decay;
        state->mouse_velocity[1] *= decay;
        _limit_vec2(state->mouse_velocity, MOUSE_VELOCITY_LIMIT);
    }

    vec4 params[3] = {0};
    _params_for_state(state, sim_dt, params);
    (void)dvz_scene_buffer_set_data(state->params, params, sizeof(params));
}


static void _scenario_event(DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user)
{
    if (ctx == NULL || event == NULL || event->kind != DVZ_SCENARIO_EVENT_POINTER)
        return;
    _particle_pointer((ParticleState*)user, &event->content.pointer);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    vec3* positions = NULL;
    vec3* velocities = NULL;
    float* ages = NULL;
    DvzColor* colors = NULL;
    float* sizes = NULL;
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    bool ok = false;
    ParticleState* state = (ParticleState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    state->scene = ctx->scene;
    if (out_user != NULL)
        *out_user = state;

    positions = (vec3*)dvz_calloc(PARTICLE_COUNT, sizeof(vec3));
    velocities = (vec3*)dvz_calloc(PARTICLE_COUNT, sizeof(vec3));
    ages = (float*)dvz_calloc(PARTICLE_COUNT, sizeof(float));
    colors = (DvzColor*)dvz_calloc(PARTICLE_COUNT, sizeof(DvzColor));
    sizes = (float*)dvz_calloc(PARTICLE_COUNT, sizeof(float));
    EXAMPLE_CHECK(
        positions != NULL && velocities != NULL && ages != NULL && colors != NULL && sizes != NULL,
        "gpu_particle_smoke: allocation failed");
    _init_particles(positions, velocities, ages, colors, sizes);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    EXAMPLE_CHECK(ctx->figure != NULL, "dvz_figure() failed");
    DvzPanel* panel = dvz_panel(ctx->figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");
    state->panel = panel;

    DvzSceneBuffer* position_buffer = _scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec3),
        (uint64_t)PARTICLE_COUNT * sizeof(vec3));
    DvzSceneBuffer* velocity_buffer = _scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec3),
        (uint64_t)PARTICLE_COUNT * sizeof(vec3));
    DvzSceneBuffer* age_buffer = _scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(float),
        (uint64_t)PARTICLE_COUNT * sizeof(float));
    DvzSceneBuffer* color_buffer = _scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_VERTEX, sizeof(DvzColor),
        (uint64_t)PARTICLE_COUNT * sizeof(DvzColor));
    DvzSceneBuffer* size_buffer = _scene_buffer(
        ctx->scene, DVZ_SCENE_BUFFER_USAGE_VERTEX, sizeof(float),
        (uint64_t)PARTICLE_COUNT * sizeof(float));
    DvzSceneBuffer* param_buffer =
        _scene_buffer(ctx->scene, DVZ_SCENE_BUFFER_USAGE_STORAGE, sizeof(vec4), 3 * sizeof(vec4));
    EXAMPLE_CHECK(
        position_buffer != NULL && velocity_buffer != NULL && age_buffer != NULL &&
            color_buffer != NULL && size_buffer != NULL && param_buffer != NULL,
        "dvz_scene_buffer() failed");
    state->params = param_buffer;
    state->sim_time = 0.0f;
    vec4 params[3] = {0};
    _params_for_state(state, 0.0f, params);
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(
            position_buffer, positions, (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "position buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(
            velocity_buffer, velocities, (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "velocity buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(age_buffer, ages, (uint64_t)PARTICLE_COUNT * sizeof(float)),
        "age buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(
            color_buffer, colors, (uint64_t)PARTICLE_COUNT * sizeof(DvzColor)),
        "color buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(size_buffer, sizes, (uint64_t)PARTICLE_COUNT * sizeof(float)),
        "size buffer upload failed");
    EXAMPLE_CHECK(
        dvz_scene_buffer_set_data(param_buffer, params, sizeof(params)),
        "param buffer upload failed");

    DvzVisual* points = dvz_point(ctx->scene, 0);
    EXAMPLE_CHECK(points != NULL, "dvz_point() failed");
    EXAMPLE_CHECK(
        dvz_visual_set_attr_buffer(points, "position", position_buffer, 0, PARTICLE_COUNT),
        "bind point position buffer failed");
    EXAMPLE_CHECK(
        dvz_visual_set_attr_buffer(points, "color", color_buffer, 0, PARTICLE_COUNT),
        "bind point color buffer failed");
    EXAMPLE_CHECK(
        dvz_visual_set_attr_buffer(points, "size", size_buffer, 0, PARTICLE_COUNT),
        "bind point size buffer failed");
    EXAMPLE_CHECK(dvz_visual_set_depth_test(points, false) == 0, "disable depth test failed");
    EXAMPLE_CHECK(
        dvz_visual_set_alpha_mode(points, DVZ_ALPHA_BLENDED) == 0, "enable alpha blending failed");
    EXAMPLE_CHECK(dvz_panel_add_visual(panel, points, NULL) == 0, "add point visual failed");
    EXAMPLE_CHECK(
        _compute_shader_source(state->compute_glsl, sizeof(state->compute_glsl)),
        "compute shader source too long");

    DvzSceneComputeDesc compute_desc = dvz_scene_compute_desc();
    compute_desc.label = "gpu_particle_smoke";
#ifdef DVZ_EXAMPLE_NO_APP
    compute_desc.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    compute_desc.shader_source = COMPUTE_WGSL;
#else
    compute_desc.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    compute_desc.shader_source = state->compute_glsl;
#endif
    compute_desc.dispatch[0] = (PARTICLE_COUNT + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE;
    compute_desc.dispatch[1] = 1;
    compute_desc.dispatch[2] = 1;
    DvzSceneCompute* compute = dvz_scene_compute(ctx->scene, &compute_desc);
    EXAMPLE_CHECK(compute != NULL, "dvz_scene_compute() failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 0, param_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ, 0, 3 * sizeof(vec4)),
        "bind compute params failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 1, position_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "bind compute positions failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 2, velocity_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(vec3)),
        "bind compute velocities failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 3, age_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(float)),
        "bind compute ages failed");
    EXAMPLE_CHECK(dvz_figure_add_compute(ctx->figure, compute), "attach compute pass failed");

    ok = true;
cleanup:
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(ages);
    dvz_free(velocities);
    dvz_free(positions);
    return ok;
}



static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



DvzScenarioSpec dvz_showcase_gpu_particle_smoke_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcase_gpu_particle_smoke",
        .title = "gpu_particle_smoke",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_POINT_VISUAL | DVZ_SCENARIO_REQ_SCENE_BUFFERS |
                        DVZ_SCENARIO_REQ_STORAGE_BUFFERS | DVZ_SCENARIO_REQ_SCENE_COMPUTE |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .event = _scenario_event,
        .destroy = _scenario_destroy,
    };
}



#ifndef DVZ_EXAMPLE_NO_MAIN
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_showcase_gpu_particle_smoke_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
