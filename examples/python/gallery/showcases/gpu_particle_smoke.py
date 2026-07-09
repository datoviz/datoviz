#!/usr/bin/env python3
"""GPU-updated particle smoke using scene compute buffers."""

from __future__ import annotations

import ctypes
from dataclasses import dataclass

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


PARTICLE_COUNT = 32768
WORKGROUP_SIZE = 128
SIM_SPEED = 0.65
SIM_MAX_DT = 1.0 / 30.0
SMOKE_LIFETIME = 4.6
SMOKE_ALPHA_BASE = 0.26
SMOKE_ALPHA_YOUNG_BOOST = 0.18
SMOKE_SIZE_MIN = 1.25
SMOKE_SIZE_MAX = 3.75
SMOKE_TOP_FADE_START = 0.90
MOUSE_HOVER_RADIUS = 0.24
MOUSE_HOVER_SWIRL = 1.35
TAU = 2.0 * np.pi


@dataclass
class SmokeState:
    params: ctypes.POINTER(dvz.DvzSceneBuffer)
    shader_source: bytes
    label: bytes
    sim_time: float = 0.0
    last_elapsed: float | None = None


def _smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _mix_u8(a, b, t):
    return np.clip((1.0 - t) * a + t * b + 0.5, 0.0, 255.0).astype(np.uint8)


def _hash01(values):
    x = np.asarray(values, dtype=np.uint32).copy()
    x ^= x >> np.uint32(16)
    x *= np.uint32(0x7FEB352D)
    x ^= x >> np.uint32(15)
    x *= np.uint32(0x846CA68B)
    x ^= x >> np.uint32(16)
    return ((x & np.uint32(0x00FFFFFF)).astype(np.float32) / np.float32(16777216.0))


def _particle_colors(life, lane):
    warm = 1.0 - _smoothstep(0.10, 0.72, life)
    cool = _smoothstep(0.18, 0.95, life)
    r = _mix_u8(246, 92, cool)
    g = _mix_u8(126, 188, cool)
    b = _mix_u8(54, 218, cool)
    r = _mix_u8(r, 246, 0.18 * warm)
    g = _mix_u8(g, 222, 0.12 * warm)
    b = _mix_u8(b, 196, 0.10 + 0.12 * lane)

    alpha = SMOKE_ALPHA_BASE + SMOKE_ALPHA_YOUNG_BOOST * (1.0 - life)
    alpha *= _smoothstep(0.00, 0.08, life)
    alpha *= 1.0 - _smoothstep(SMOKE_TOP_FADE_START, 1.0, life)
    alpha *= 0.55 + 0.25 * lane
    a = np.clip(255.0 * np.clip(alpha, 0.0, 1.0) + 0.5, 0.0, 255.0).astype(np.uint8)
    return np.column_stack((r, g, b, a)).astype(np.uint8)


def _init_particles(count: int, time_s: float):
    idx = np.arange(count, dtype=np.uint32)
    a = TAU * _hash01(idx * np.uint32(7) + np.uint32(1))
    r = np.sqrt(_hash01(idx * np.uint32(7) + np.uint32(2)))
    lane = _hash01(idx * np.uint32(7) + np.uint32(3))
    life = _hash01(idx * np.uint32(7) + np.uint32(4))
    vertical = life**0.78
    width = 0.045 + 0.26 * vertical
    plume = 0.12 * np.sin(4.2 * vertical + 0.00011 * idx.astype(np.float32) + 0.54 * time_s)
    plume += 0.035 * np.sin(1.7 * time_s + TAU * lane)

    positions = np.zeros((count, 3), dtype=np.float32)
    positions[:, 0] = plume + width * r * np.cos(a)
    positions[:, 1] = -1.04 + 1.72 * vertical + 0.045 * r * np.sin(a)

    velocities = np.zeros((count, 3), dtype=np.float32)
    velocities[:, 0] = (
        0.04 * np.cos(a + 0.31 * time_s)
        - 0.03 * positions[:, 0]
        + 0.03 * np.sin(1.1 * time_s + TAU * lane)
    )
    velocities[:, 1] = 0.34 + 0.16 * lane

    ages = (SMOKE_LIFETIME * life).astype(np.float32)
    colors = _particle_colors(life, lane)
    sizes = (
        (SMOKE_SIZE_MIN + (SMOKE_SIZE_MAX - SMOKE_SIZE_MIN) * _smoothstep(0.0, 1.0, life))
        * (0.74 + 0.28 * lane)
    ).astype(np.float32)
    return positions, velocities, ages, colors, sizes


def _compute_shader_source() -> bytes:
    return f"""#version 450
#define SMOKE_LIFETIME {SMOKE_LIFETIME}
#define SMOKE_SOURCE_WIDTH 0.18
#define SMOKE_SOURCE_HEIGHT 0.13
#define MOUSE_FORCE_SCALE 8.0
#define MOUSE_SPEED_LIMIT 2.4
layout(local_size_x = {WORKGROUP_SIZE}) in;
layout(std430, set = 0, binding = 0) readonly buffer Params {{
    vec4 sim0;
    vec4 sim1;
    vec4 sim2;
}} params;
layout(std430, set = 0, binding = 1) buffer Positions {{ float x[]; }} positions;
layout(std430, set = 0, binding = 2) buffer Velocities {{ float v[]; }} velocities;
layout(std430, set = 0, binding = 3) buffer Ages {{ float age[]; }} ages;

uint hash_u32(uint x) {{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}}

float hash01(uint x) {{
    return float(hash_u32(x) & 0x00ffffffu) / 16777216.0;
}}

vec2 source_pos(uint i, float t) {{
    uint epoch = uint(floor(t * 18.0));
    float a = hash01(i * 1664525u + epoch * 1013904223u);
    float b = hash01(i * 22695477u + epoch * 1103515245u);
    float plume = 0.17 * sin(0.43 * t) + 0.06 * sin(1.31 * t);
    return vec2(plume + (a * 2.0 - 1.0) * SMOKE_SOURCE_WIDTH,
                -1.03 + b * SMOKE_SOURCE_HEIGHT);
}}

vec2 curl_flow(vec2 p, float t) {{
    float c1 = cos(3.0 * p.x + 2.1 * p.y + 0.63 * t);
    float c2 = cos(-2.4 * p.x + 3.7 * p.y - 0.41 * t);
    float c3 = cos(5.3 * p.x - 1.9 * p.y + 0.27 * t);
    vec2 grad = vec2(3.0 * c1 - 2.4 * c2 + 5.3 * c3,
                     2.1 * c1 + 3.7 * c2 - 1.9 * c3);
    vec2 curl = vec2(grad.y, -grad.x);
    vec2 rise = vec2(0.10 * sin(2.2 * p.y + 0.7 * t), 0.78);
    vec2 focus = vec2(-0.12 * p.x, -0.04 * p.y);
    return 0.18 * curl + rise + focus;
}}

void main() {{
    uint i = gl_GlobalInvocationID.x;
    uint count = uint(params.sim0.z);
    if (i >= count) return;
    float t = params.sim0.x;
    float dt = params.sim0.y;
    float mouse_active = params.sim0.w;
    vec2 mouse = params.sim1.xy;
    vec2 mouse_v = params.sim1.zw;
    float mouse_radius = params.sim2.x;
    float hover_swirl = params.sim2.z;
    uint j = 3u * i;
    vec2 x = vec2(positions.x[j + 0u], positions.x[j + 1u]);
    vec2 v = vec2(velocities.v[j + 0u], velocities.v[j + 1u]);
    float age = ages.age[i] + dt;
    vec2 flow = curl_flow(x, t);
    v = mix(v, flow, clamp(dt * 2.6, 0.0, 1.0));
    v += 0.020 * vec2(
        sin(17.0 * x.y + float(i & 255u) * 0.017 + t),
        cos(19.0 * x.x + float((i >> 8u) & 255u) * 0.013 - t));
    if (mouse_active > 0.5) {{
        vec2 d = x - mouse;
        float dist = length(d);
        float influence = 1.0 - smoothstep(0.0, mouse_radius, dist);
        vec2 dir = d / max(dist, 0.001);
        vec2 tangent = vec2(-dir.y, dir.x);
        float mouse_dt = clamp(dt * MOUSE_FORCE_SCALE, 0.0, 0.16);
        v += mouse_dt * influence * hover_swirl * (tangent + 0.25 * mouse_v);
        float speed = length(v);
        if (speed > MOUSE_SPEED_LIMIT) v *= MOUSE_SPEED_LIMIT / speed;
    }}
    v *= pow(0.986, dt / (1.0 / 120.0));
    x += dt * v;
    if (x.y > 1.10 || abs(x.x) > 1.18 || x.y < -1.12 || age > SMOKE_LIFETIME) {{
        x = source_pos(i, t);
        v = vec2(0.03 * sin(float(i) * 0.11), 0.42 + 0.10 * hash01(i + uint(t * 97.0)));
        age = 0.0;
    }}
    positions.x[j + 0u] = x.x;
    positions.x[j + 1u] = x.y;
    positions.x[j + 2u] = 0.0;
    velocities.v[j + 0u] = v.x;
    velocities.v[j + 1u] = v.y;
    velocities.v[j + 2u] = 0.0;
    ages.age[i] = age;
}}
""".encode()


def _params(state: SmokeState, dt: float) -> np.ndarray:
    return np.array(
        [
            [state.sim_time, dt, float(PARTICLE_COUNT), 0.0],
            [0.0, 0.0, 0.0, 0.0],
            [MOUSE_HOVER_RADIUS, 0.0, MOUSE_HOVER_SWIRL, 0.0],
        ],
        dtype=np.float32,
    )


def _update_params(state: SmokeState, wall_dt: float) -> None:
    sim_dt = float(np.clip(wall_dt, 0.0, SIM_MAX_DT) * SIM_SPEED)
    state.sim_time += sim_dt
    if dvz.dvz_scene_buffer_set_data(state.params, _params(state, sim_dt)) != 0:
        raise RuntimeError("dvz_scene_buffer_set_data(params) failed")


def _scene_buffer(scene, usage: int, stride: int, byte_size: int):
    desc = dvz.dvz_scene_buffer_desc()
    desc.usage = int(usage)
    desc.stride = int(stride)
    desc.byte_size = int(byte_size)
    buffer = dvz.dvz_scene_buffer(scene, ctypes.byref(desc))
    if not buffer:
        raise RuntimeError("dvz_scene_buffer() failed")
    return buffer


def _upload(buffer, data, name: str) -> None:
    if dvz.dvz_scene_buffer_set_data(buffer, data) != 0:
        raise RuntimeError(f"dvz_scene_buffer_set_data({name}) failed")


def _build_scene():
    positions, velocities, ages, colors, sizes = _init_particles(PARTICLE_COUNT, 0.0)
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(10, 13, 18, 255))

    vec3_size = 3 * np.dtype(np.float32).itemsize
    f32_size = np.dtype(np.float32).itemsize
    rgba_size = 4 * np.dtype(np.uint8).itemsize

    position_buffer = _scene_buffer(
        scene,
        dvz.DVZ_SCENE_BUFFER_USAGE_VERTEX | dvz.DVZ_SCENE_BUFFER_USAGE_STORAGE,
        vec3_size,
        positions.nbytes,
    )
    velocity_buffer = _scene_buffer(
        scene, dvz.DVZ_SCENE_BUFFER_USAGE_STORAGE, vec3_size, velocities.nbytes
    )
    age_buffer = _scene_buffer(scene, dvz.DVZ_SCENE_BUFFER_USAGE_STORAGE, f32_size, ages.nbytes)
    color_buffer = _scene_buffer(scene, dvz.DVZ_SCENE_BUFFER_USAGE_VERTEX, rgba_size, colors.nbytes)
    size_buffer = _scene_buffer(scene, dvz.DVZ_SCENE_BUFFER_USAGE_VERTEX, f32_size, sizes.nbytes)
    params_buffer = _scene_buffer(scene, dvz.DVZ_SCENE_BUFFER_USAGE_STORAGE, 4 * f32_size, 12 * f32_size)

    _upload(position_buffer, positions, "positions")
    _upload(velocity_buffer, velocities, "velocities")
    _upload(age_buffer, ages, "ages")
    _upload(color_buffer, colors, "colors")
    _upload(size_buffer, sizes, "sizes")

    shader_source = _compute_shader_source()
    state = SmokeState(params=params_buffer, shader_source=shader_source, label=b"gpu_particle_smoke")
    _upload(params_buffer, _params(state, 0.0), "params")

    points = dvz.dvz_point(scene, 0)
    if not points:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_attr_buffer(points, b"position", position_buffer, 0, PARTICLE_COUNT) != 0:
        raise RuntimeError("dvz_visual_set_attr_buffer(position) failed")
    if dvz.dvz_visual_set_attr_buffer(points, b"color", color_buffer, 0, PARTICLE_COUNT) != 0:
        raise RuntimeError("dvz_visual_set_attr_buffer(color) failed")
    if dvz.dvz_visual_set_attr_buffer(points, b"size", size_buffer, 0, PARTICLE_COUNT) != 0:
        raise RuntimeError("dvz_visual_set_attr_buffer(size) failed")
    ex.set_filled_point_style(points)
    if dvz.dvz_visual_set_depth_test(points, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")
    if dvz.dvz_visual_set_alpha_mode(points, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode() failed")
    ex.add_visual(panel, points)

    compute_desc = dvz.dvz_scene_compute_desc()
    compute_desc.label = state.label
    compute_desc.shader_format = dvz.DVZ_SCENE_SHADER_FORMAT_GLSL
    compute_desc.shader_source = state.shader_source
    compute_desc.dispatch[:] = ((PARTICLE_COUNT + WORKGROUP_SIZE - 1) // WORKGROUP_SIZE, 1, 1)
    compute = dvz.dvz_scene_compute(scene, ctypes.byref(compute_desc))
    if not compute:
        raise RuntimeError("dvz_scene_compute() failed")

    bindings = (
        (0, params_buffer, dvz.DVZ_SCENE_COMPUTE_ACCESS_READ, 3 * 4 * f32_size),
        (1, position_buffer, dvz.DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, positions.nbytes),
        (2, velocity_buffer, dvz.DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, velocities.nbytes),
        (3, age_buffer, dvz.DVZ_SCENE_COMPUTE_ACCESS_READ_WRITE, ages.nbytes),
    )
    for binding, buffer, access, byte_size in bindings:
        if dvz.dvz_scene_compute_set_buffer(compute, binding, buffer, access, 0, byte_size) != 0:
            raise RuntimeError(f"dvz_scene_compute_set_buffer({binding}) failed")
    if dvz.dvz_figure_add_compute(figure, compute) != 0:
        raise RuntimeError("dvz_figure_add_compute() failed")

    return scene, figure, panel, state


def _on_frame(state: SmokeState, _view, _frame_index: int, elapsed: float) -> None:
    if state.last_elapsed is None:
        wall_dt = 1.0 / 60.0
    else:
        wall_dt = elapsed - state.last_elapsed
    state.last_elapsed = elapsed
    _update_params(state, wall_dt)


def main() -> None:
    scene, figure, _panel, state = _build_scene()
    ex.run_with_frame_callback(
        scene,
        figure,
        "GPU Particle Smoke",
        lambda view, frame_index, elapsed: _on_frame(state, view, frame_index, elapsed),
    )


if __name__ == "__main__":
    main()
