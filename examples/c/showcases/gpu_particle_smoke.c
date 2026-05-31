/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gpu_particle_smoke - scene API compute-to-graphics particle smoke showcase.
 *
 * Scenario: showcase_gpu_particle_smoke
 * Style: dense transparent particle smoke, experimental scene compute, optional capture
 *
 * Build:  just example-c showcases/gpu_particle_smoke
 * Run:    ./build/examples/c/showcases/gpu_particle_smoke
 * Smoke:  ./build/examples/c/showcases/gpu_particle_smoke 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/input.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH                   1600u
#define HEIGHT                  1000u
#define PARTICLE_COUNT          1048576u
#define WORKGROUP_SIZE          128u
#define COMPUTE_SOURCE_CAPACITY 8192u
#define SIM_SPEED               0.65f
#define SIM_MAX_DT              (1.0f / 30.0f)

#define SMOKE_LIFETIME          4.6f
#define SMOKE_ALPHA_BASE        0.26f
#define SMOKE_ALPHA_YOUNG_BOOST 0.18f
#define SMOKE_SIZE_MIN          1.25f
#define SMOKE_SIZE_MAX          3.75f
#define SMOKE_VIOLET_MIX        0.20f
#define SMOKE_SOURCE_WIDTH      0.18f
#define SMOKE_SOURCE_HEIGHT     0.13f
#define SMOKE_TOP_FADE_START    0.90f

#define MOUSE_HOVER_RADIUS   0.18f
#define MOUSE_DRAG_STRENGTH  1.8f
#define MOUSE_HOVER_SWIRL    0.45f
#define MOUSE_DRAG_FOLLOW    0.9f
#define MOUSE_VELOCITY_LIMIT 3.0f
#define MOUSE_VELOCITY_DECAY 0.08f
#define MOUSE_FORCE_SCALE    8.0f
#define MOUSE_SPEED_LIMIT    2.4f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ParticleState
{
    DvzScene* scene;
    DvzSceneBuffer* params;
    float sim_time;
    bool mouse_valid;
    bool mouse_down;
    vec2 mouse_pos;
    vec2 mouse_prev;
    vec2 mouse_velocity;
    uint64_t mouse_timestamp;
} ParticleState;



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
    "layout(std430, set = 0, binding = 4) buffer Colors { uint rgba[]; } colors;\n"
    "layout(std430, set = 0, binding = 5) buffer Sizes { float size[]; } sizes;\n"
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
    "uint pack_rgba(vec4 color) {\n"
    "    uvec4 c = uvec4(clamp(color, 0.0, 1.0) * 255.0 + 0.5);\n"
    "    return c.r | (c.g << 8u) | (c.b << 16u) | (c.a << 24u);\n"
    "}\n"
    "vec3 palette(float u, float lane) {\n"
    "    vec3 amber = vec3(1.00, 0.62, 0.24);\n"
    "    vec3 rose = vec3(1.00, 0.44, 0.50);\n"
    "    vec3 pearl = vec3(0.96, 0.90, 0.70);\n"
    "    vec3 cyan = vec3(0.24, 0.80, 0.96);\n"
    "    vec3 violet = vec3(0.54, 0.42, 0.92);\n"
    "    vec3 a = mix(amber, rose, smoothstep(0.00, 0.34, u));\n"
    "    vec3 b = mix(pearl, cyan, smoothstep(0.28, 0.78, u));\n"
    "    vec3 c = mix(a, b, smoothstep(0.18, 0.70, u));\n"
    "    return mix(c, violet, SMOKE_VIOLET_MIX * lane * smoothstep(0.58, 1.0, u));\n"
    "}\n";

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
    "    float drag_strength = params.sim2.y;\n"
    "    float hover_swirl = params.sim2.z;\n"
    "    float drag_follow = params.sim2.w;\n"
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
    "        float drag = step(1.5, mouse_active);\n"
    "        float mouse_dt = clamp(dt * MOUSE_FORCE_SCALE, 0.0, 0.16);\n"
    "        vec2 follow = drag_follow * mouse_v - 0.28 * dir;\n"
    "        v += mouse_dt * influence * hover_swirl * tangent;\n"
    "        v += mouse_dt * influence * drag * drag_strength * follow;\n"
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
    "    float height = smoothstep(-1.05, 1.05, x.y);\n"
    "    float life = clamp(age / SMOKE_LIFETIME, 0.0, 1.0);\n"
    "    float lane = hash01(i * 747796405u + 2891336453u);\n"
    "    vec3 rgb = palette(max(height, life * 0.72), lane);\n"
    "    float alpha = (SMOKE_ALPHA_BASE + SMOKE_ALPHA_YOUNG_BOOST * (1.0 - life)) *\n"
    "                  smoothstep(0.0, 0.16, age);\n"
    "    alpha *= 1.0 - smoothstep(SMOKE_TOP_FADE_START, 1.0, height);\n"
    "    alpha *= 0.75 + 0.25 * lane;\n"
    "    float point_size = mix(SMOKE_SIZE_MIN, SMOKE_SIZE_MAX, smoothstep(0.0, 1.0, life));\n"
    "    point_size *= 0.82 + 0.36 * lane;\n"
    "    positions.x[j + 0u] = x.x;\n"
    "    positions.x[j + 1u] = x.y;\n"
    "    positions.x[j + 2u] = 0.0;\n"
    "    velocities.v[j + 0u] = v.x;\n"
    "    velocities.v[j + 1u] = v.y;\n"
    "    velocities.v[j + 2u] = 0.0;\n"
    "    ages.age[i] = age;\n"
    "    colors.rgba[i] = pack_rgba(vec4(rgb, alpha));\n"
    "    sizes.size[i] = point_size;\n"
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
        "#define SMOKE_LIFETIME %.8g\n"
        "#define SMOKE_ALPHA_BASE %.8g\n"
        "#define SMOKE_ALPHA_YOUNG_BOOST %.8g\n"
        "#define SMOKE_SIZE_MIN %.8g\n"
        "#define SMOKE_SIZE_MAX %.8g\n"
        "#define SMOKE_VIOLET_MIX %.8g\n"
        "#define SMOKE_SOURCE_WIDTH %.8g\n"
        "#define SMOKE_SOURCE_HEIGHT %.8g\n"
        "#define SMOKE_TOP_FADE_START %.8g\n"
        "#define MOUSE_FORCE_SCALE %.8g\n"
        "#define MOUSE_SPEED_LIMIT %.8g\n"
        "%s%s",
        (double)SMOKE_LIFETIME, (double)SMOKE_ALPHA_BASE, (double)SMOKE_ALPHA_YOUNG_BOOST,
        (double)SMOKE_SIZE_MIN, (double)SMOKE_SIZE_MAX, (double)SMOKE_VIOLET_MIX,
        (double)SMOKE_SOURCE_WIDTH, (double)SMOKE_SOURCE_HEIGHT, (double)SMOKE_TOP_FADE_START,
        (double)MOUSE_FORCE_SCALE, (double)MOUSE_SPEED_LIMIT, COMPUTE_GLSL_COMMON,
        COMPUTE_GLSL_MAIN);
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


static void _copy_vec2(const vec2 src, vec2 dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
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


static void _window_to_sim(float x, float y, vec2 out)
{
    const float u = _clamp01(x / (float)WIDTH);
    const float v = _clamp01(y / (float)HEIGHT);
    out[0] = 2.0f * u - 1.0f;
    out[1] = 1.0f - 2.0f * v;
}


static DvzColor _particle_color(float radius, float phase)
{
    const float t = _clamp01((radius - 0.18f) / 0.78f);
    const uint8_t r = (uint8_t)(115.0f + 65.0f * t + 18.0f * sinf(phase));
    const uint8_t g = (uint8_t)(150.0f + 70.0f * (1.0f - 0.30f * t));
    const uint8_t b = (uint8_t)(185.0f + 45.0f * sinf(0.6f + phase));
    const uint8_t a = (uint8_t)(34.0f + 26.0f * (1.0f - t));
    return dvz_color_rgba(r, g, b, a);
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
        const float a = TAU * _hash01(i * 3u + 1u);
        const float r = sqrtf(_hash01(i * 3u + 2u));
        const float jitter = _hash01(i * 3u + 3u);
        const float x = 0.16f * r * cosf(a) + 0.06f * sinf(0.013f * (float)i);
        const float y = -1.03f + 0.80f * r * r + 0.06f * sinf(a);
        positions[i][0] = x;
        positions[i][1] = y;
        positions[i][2] = 0.0f;
        velocities[i][0] = 0.04f * cosf(a);
        velocities[i][1] = 0.35f + 0.18f * jitter;
        velocities[i][2] = 0.0f;
        ages[i] = SMOKE_LIFETIME * _hash01(i * 5u + 4u);
        colors[i] = _particle_color(r, a);
        sizes[i] = 0.75f + 1.10f * jitter;
    }
}


static void _params_for_state(const ParticleState* state, float dt, vec4 params[3])
{
    ANN(state);
    ANN(params);
    const float active = state->mouse_valid ? (state->mouse_down ? 2.0f : 1.0f) : 0.0f;

    params[0][0] = state->sim_time;
    params[0][1] = dt;
    params[0][2] = (float)PARTICLE_COUNT;
    params[0][3] = active;

    params[1][0] = state->mouse_pos[0];
    params[1][1] = state->mouse_pos[1];
    params[1][2] = state->mouse_velocity[0];
    params[1][3] = state->mouse_velocity[1];

    params[2][0] = MOUSE_HOVER_RADIUS;
    params[2][1] = MOUSE_DRAG_STRENGTH;
    params[2][2] = MOUSE_HOVER_SWIRL;
    params[2][3] = MOUSE_DRAG_FOLLOW;
}


static void _particle_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    ParticleState* state = (ParticleState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type == DVZ_POINTER_EVENT_WHEEL || event->type == DVZ_POINTER_EVENT_NONE)
        return;

    vec2 pos = {0};
    _window_to_sim(event->pos[0], event->pos[1], pos);

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

    if (state->mouse_valid)
        _copy_vec2(state->mouse_pos, state->mouse_prev);
    else
        _copy_vec2(pos, state->mouse_prev);
    _copy_vec2(pos, state->mouse_pos);
    state->mouse_valid = true;
    state->mouse_timestamp = event->timestamp_ns;

    if (event->type == DVZ_POINTER_EVENT_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
        state->mouse_down = true;
    if (event->type == DVZ_POINTER_EVENT_RELEASE && event->button == DVZ_POINTER_BUTTON_LEFT)
        state->mouse_down = false;
}


static void _particle_frame(DvzView* win, void* user_data)
{
    ParticleState* state = (ParticleState*)user_data;
    if (state == NULL || state->scene == NULL || state->params == NULL)
        return;

    const float wall_dt = (float)dvz_scene_clock_dt(state->scene);
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
    dvz_view_request_frame(win);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int rc = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    vec3* positions = NULL;
    vec3* velocities = NULL;
    float* ages = NULL;
    DvzColor* colors = NULL;
    float* sizes = NULL;
    char compute_glsl[COMPUTE_SOURCE_CAPACITY] = {0};

    const uint32_t frames = example_frame_count(argc, argv);
    positions = (vec3*)dvz_calloc(PARTICLE_COUNT, sizeof(vec3));
    velocities = (vec3*)dvz_calloc(PARTICLE_COUNT, sizeof(vec3));
    ages = (float*)dvz_calloc(PARTICLE_COUNT, sizeof(float));
    colors = (DvzColor*)dvz_calloc(PARTICLE_COUNT, sizeof(DvzColor));
    sizes = (float*)dvz_calloc(PARTICLE_COUNT, sizeof(float));
    EXAMPLE_CHECK(
        positions != NULL && velocities != NULL && ages != NULL && colors != NULL && sizes != NULL,
        "gpu_particle_smoke: allocation failed");
    _init_particles(positions, velocities, ages, colors, sizes);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");
    DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzSceneBuffer* position_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec3),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(vec3),
               });
    DvzSceneBuffer* velocity_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec3),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(vec3),
               });
    DvzSceneBuffer* age_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(float),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(float),
               });
    DvzSceneBuffer* color_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(DvzColor),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(DvzColor),
               });
    DvzSceneBuffer* size_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_VERTEX | DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(float),
                   .byte_size = (uint64_t)PARTICLE_COUNT * sizeof(float),
               });
    DvzSceneBuffer* param_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_STORAGE,
                   .stride = sizeof(vec4),
                   .byte_size = 3 * sizeof(vec4),
               });
    EXAMPLE_CHECK(
        position_buffer != NULL && velocity_buffer != NULL && age_buffer != NULL &&
            color_buffer != NULL && size_buffer != NULL && param_buffer != NULL,
        "dvz_scene_buffer() failed");
    ParticleState state = {.scene = scene, .params = param_buffer, .sim_time = 0.0f};
    vec4 params[3] = {0};
    _params_for_state(&state, 0.0f, params);
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

    DvzVisual* points = dvz_point(scene, 0);
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
        _compute_shader_source(compute_glsl, sizeof(compute_glsl)),
        "compute shader source too long");

    DvzSceneCompute* compute = dvz_scene_compute(
        scene, &(DvzSceneComputeDesc){
                   .label = "gpu_particle_smoke",
                   .shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL,
                   .shader_source = compute_glsl,
                   .dispatch = {(PARTICLE_COUNT + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE, 1, 1},
               });
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
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 4, color_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(DvzColor)),
        "bind compute colors failed");
    EXAMPLE_CHECK(
        dvz_scene_compute_set_buffer(
            compute, 5, size_buffer, DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, 0,
            (uint64_t)PARTICLE_COUNT * sizeof(float)),
        "bind compute sizes failed");
    EXAMPLE_CHECK(dvz_figure_add_compute(figure, compute), "attach compute pass failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");
    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "gpu_particle_smoke");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");
    dvz_input_subscribe_pointer(router, _particle_pointer, &state);
    dvz_view_set_frame_callback(win, _particle_frame, &state);
    dvz_view_request_frame(win);

    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("gpu_particle_smoke");
    EXAMPLE_CHECK(dvz_view_capture_start(win, &capture) == 0, "dvz_view_capture_start() failed");
    dvz_app_run(app, frames);
    EXAMPLE_CHECK(dvz_view_capture_stop(win) == 0, "dvz_view_capture_stop() failed");

    if (frames == 0)
        printf(
            "gpu_particle_smoke: %u smoke particles, interactive scene compute path\n",
            PARTICLE_COUNT);
    else
        printf(
            "gpu_particle_smoke: %u smoke particles, %u frames, scene compute path\n",
            PARTICLE_COUNT, frames);
    rc = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    else if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(sizes);
    dvz_free(colors);
    dvz_free(ages);
    dvz_free(velocities);
    dvz_free(positions);
    return rc;
}
