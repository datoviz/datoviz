# Gray-Scott Reaction-Diffusion Compute Example

> **Agent Pickup**
> - **Category:** `compute`
> - **Implementation target:** Shader/compute-oriented example, preferably runnable with deterministic defaults.
> - **Data policy:** Generated or synthetic data by default; optional assets must have a cache and fallback.
> - **Preprocessing:** Document shader inputs, generated buffers/textures, and any optional Python preparation script.
> - **Validation:** Bounded smoke run, deterministic seed, and visual/readback criteria for the simulation state.


## Summary

Build a Datoviz v0.4 compute example where a Gray-Scott reaction-diffusion simulation updates a
GPU ping-pong state texture or buffer every frame and the render pass samples that state directly.
No external data is required; the initial concentrations, perturbation patch, palette, and parameter
presets should be generated deterministically, with any optional palette asset cached and backed by
a fallback. The first practical slice should run a fixed 512x512 simulation with one preset, a
single render panel, and no CPU readback during animation. Validate with a bounded smoke run that
advances a deterministic number of steps and checks visual output or readback statistics for a
non-static, finite concentration field.


## Purpose

This example demonstrates how a Datoviz v0.4 scene can combine a GPU compute simulation with rendering in the same frame, without any GPU-to-CPU readback or CPU-mediated texture updates.

The example runs a Gray-Scott reaction-diffusion simulation in a compute shader and renders the evolving chemical concentration field directly from the GPU resource written by the compute pass.

The primary goal is to test and demonstrate:

- compute pass execution at every frame;
- ping-pong GPU resources used as simulation state;
- render passes sampling or reading compute outputs directly;
- framegraph dependencies between compute and render passes;
- dynamic per-frame uniforms or push constants;
- interactive parameter updates;
- interactive mouse injection into the simulation;
- no CPU copy of the simulation state after initialization.

This should be a visually attractive, standalone Python example that works out of the box on Datoviz v0.4 once the exact Python API is finalized.

The example should not depend on external data. It may optionally download a small palette image or preset file from `datoviz/data`, but this is not necessary.

Suggested filename for the future example:

```text
examples/features/compute_reaction_diffusion.py
```

Suggested documentation/example name:

```text
Gray-Scott reaction-diffusion compute shader
```

---

## High-level visual result

The window shows a full-screen animated reaction-diffusion pattern: coral-like, worm-like, maze-like, or spot-like structures evolving smoothly over time.

The simulation should be rendered as a colorized scalar field, preferably using a high-contrast palette:

- dark navy or black background;
- cyan/teal mid-values;
- orange/gold high-values;
- optional white-hot highlights.

The image should fill a 2D panel. Users should be able to pan/zoom only if this is trivial with the current scene API, but this is not the focus. The main interaction should be parameter control and mouse-based injection into the field.

---

## Scientific model

Use the classic Gray-Scott reaction-diffusion equations:

```text
du/dt = Du * laplacian(u) - u*v*v + F*(1-u)
dv/dt = Dv * laplacian(v) + u*v*v - (F+K)*v
```

Where:

- `u` and `v` are scalar chemical concentrations;
- `Du` and `Dv` are diffusion coefficients;
- `F` is the feed rate;
- `K` is the kill rate.

The simulation state stores both fields per grid cell:

```text
R = u
G = v
```

Recommended initial parameters:

```text
Du = 0.16
Dv = 0.08
F  = 0.035
K  = 0.065
dt = 1.0
```

Useful presets:

| Preset | F | K | Expected pattern |
|---|---:|---:|---|
| Spots | 0.035 | 0.065 | round spots and islands |
| Coral | 0.0545 | 0.062 | branching coral-like fronts |
| Mitosis | 0.0367 | 0.0649 | cell-like splitting structures |
| Worms | 0.078 | 0.061 | thin elongated moving structures |
| Mazes | 0.029 | 0.057 | maze-like labyrinths |

The default preset should be visually interesting within a few seconds. `Spots` or `Coral` are good defaults.

---

## Simulation resolution

Use a fixed simulation grid, independent from the window size:

```text
width  = 512
height = 512
```

This is large enough to look good and small enough to run comfortably.

Optional quality settings:

```text
low    = 256 x 256
medium = 512 x 512
high   = 1024 x 1024
```

The first implementation should use `512 x 512` only.

---

## GPU resources

The simulation requires two ping-pong resources.

Preferred representation:

```text
state_a: 2D texture, format rg32float, storage/read usage
state_b: 2D texture, format rg32float, storage/write usage + sampled/render read usage
```

Alternative if storage textures are not yet exposed in the Python API:

```text
state_a: storage buffer containing width * height * vec2<f32>
state_b: storage buffer containing width * height * vec2<f32>
```

The texture version is preferred because it maps naturally to rendering as an image and tests compute-to-texture-to-render data flow.

Each frame:

```text
compute reads  state_a
compute writes state_b
render samples state_b
swap(state_a, state_b)
```

The swap may be implemented either by swapping Python-side object references or by alternating bind groups.

No CPU readback should occur after initialization.

---

## Initial condition

Initialize the whole grid as:

```text
u = 1.0
v = 0.0
```

Then seed several small square or circular regions with:

```text
u = 0.5
v = 0.25
```

Recommended deterministic initialization:

- one central square, about `32 x 32` pixels;
- several smaller random circular seeds distributed near the center;
- fixed random seed for reproducibility.

Example CPU initialization logic:

```python
state = np.zeros((height, width, 2), dtype=np.float32)
state[..., 0] = 1.0  # u
state[..., 1] = 0.0  # v

# central seed
cx, cy = width // 2, height // 2
r = 16
state[cy-r:cy+r, cx-r:cx+r, 0] = 0.5
state[cy-r:cy+r, cx-r:cx+r, 1] = 0.25

# optional deterministic random seeds
rng = np.random.default_rng(0)
for _ in range(20):
    x = int(cx + rng.normal(0, width * 0.12))
    y = int(cy + rng.normal(0, height * 0.12))
    rr = int(rng.integers(4, 10))
    # write disk or square around (x, y)
```

Upload this initial state once to both `state_a` and `state_b`, or upload to `state_a` and clear/initialize `state_b` as appropriate.

---

## Framegraph structure

The example should explicitly exercise a compute-to-render dependency.

Per frame, the scene/framegraph should conceptually contain:

```text
Pass 1: COMPUTE reaction_diffusion_step
    read:  state_a
    write: state_b
    dispatch: ceil(width / 16), ceil(height / 16), 1

Pass 2: RENDER main_color
    read:  state_b as sampled texture or image input
    draw:  full-screen quad, image visual, or equivalent panel-filling visual
```

After the frame is built or submitted:

```text
swap(state_a, state_b)
```

If the scene API supports persistent framegraphs, the implementation may instead keep two sets of bind groups and toggle an index every frame.

The key invariant is:

```text
rendered texture for frame N = compute output for frame N
```

---

## Compute shader behavior

The compute shader performs one or more Gray-Scott update steps.

Recommended workgroup size:

```text
16 x 16
```

The shader should use periodic boundary conditions for simplicity:

```text
x_left  = (x - 1 + width)  % width
x_right = (x + 1)          % width
y_up    = (y - 1 + height) % height
y_down  = (y + 1)          % height
```

Use either a 5-point or 9-point Laplacian.

Simple 5-point Laplacian:

```text
lap = north + south + east + west - 4 * center
```

Better 9-point Laplacian:

```text
lap = 0.2  * (north + south + east + west)
    + 0.05 * (north_east + north_west + south_east + south_west)
    - 1.0  * center
```

The 9-point version usually looks better and should be preferred.

The shader should clamp the output:

```text
u = clamp(u, 0.0, 1.0)
v = clamp(v, 0.0, 1.0)
```

---

## Compute shader pseudocode

The exact shader language may be WGSL, GLSL, or SPIR-V depending on the current Datoviz v0.4 backend state. The implementation should use the shader source format most natural for the current branch.

WGSL-like pseudocode:

```wgsl
struct Params {
    width: u32,
    height: u32,
    Du: f32,
    Dv: f32,
    F: f32,
    K: f32,
    dt: f32,
    brush_x: f32,
    brush_y: f32,
    brush_radius: f32,
    brush_strength: f32,
    brush_active: u32,
};

@group(0) @binding(0)
var src: texture_storage_2d<rg32float, read>;

@group(0) @binding(1)
var dst: texture_storage_2d<rg32float, write>;

@group(0) @binding(2)
var<uniform> params: Params;

fn wrap_coord(p: vec2<i32>) -> vec2<i32> {
    let w = i32(params.width);
    let h = i32(params.height);
    return vec2<i32>((p.x + w) % w, (p.y + h) % h);
}

fn load_state(p: vec2<i32>) -> vec2<f32> {
    return textureLoad(src, wrap_coord(p));
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (gid.x >= params.width || gid.y >= params.height) {
        return;
    }

    let p = vec2<i32>(i32(gid.x), i32(gid.y));

    let c  = load_state(p);
    let n  = load_state(p + vec2<i32>( 0, -1));
    let s  = load_state(p + vec2<i32>( 0,  1));
    let e  = load_state(p + vec2<i32>( 1,  0));
    let w  = load_state(p + vec2<i32>(-1,  0));
    let ne = load_state(p + vec2<i32>( 1, -1));
    let nw = load_state(p + vec2<i32>(-1, -1));
    let se = load_state(p + vec2<i32>( 1,  1));
    let sw = load_state(p + vec2<i32>(-1,  1));

    let lap = 0.2 * (n + s + e + w) + 0.05 * (ne + nw + se + sw) - c;

    var u = c.x;
    var v = c.y;

    let uvv = u * v * v;

    u = u + params.dt * (params.Du * lap.x - uvv + params.F * (1.0 - u));
    v = v + params.dt * (params.Dv * lap.y + uvv - (params.F + params.K) * v);

    if (params.brush_active != 0u) {
        let q = vec2<f32>(f32(gid.x), f32(gid.y));
        let b = vec2<f32>(params.brush_x, params.brush_y);
        let d = distance(q, b);
        if (d < params.brush_radius) {
            let a = 1.0 - smoothstep(0.0, params.brush_radius, d);
            v = mix(v, 1.0, a * params.brush_strength);
            u = mix(u, 0.0, a * params.brush_strength);
        }
    }

    textureStore(dst, p, vec4<f32>(clamp(u, 0.0, 1.0), clamp(v, 0.0, 1.0), 0.0, 1.0));
}
```

If the chosen backend does not support `rg32float` storage textures, use `rgba32float` and ignore `B/A`, or use storage buffers.

---

## Rendering shader behavior

The render stage should draw a full-screen quad or use an image visual that samples the output simulation texture.

The fragment shader reads the `v` concentration and maps it to color.

WGSL-like pseudocode:

```wgsl
@group(0) @binding(0)
var sim_texture: texture_2d<f32>;

@group(0) @binding(1)
var sim_sampler: sampler;

fn palette(t: f32) -> vec3<f32> {
    let a = vec3<f32>(0.02, 0.03, 0.08);  // dark navy
    let b = vec3<f32>(0.00, 0.65, 0.75);  // cyan/teal
    let c = vec3<f32>(1.00, 0.45, 0.08);  // orange
    let d = vec3<f32>(1.00, 0.95, 0.75);  // warm white

    let t1 = smoothstep(0.00, 0.35, t);
    let t2 = smoothstep(0.25, 0.75, t);
    let t3 = smoothstep(0.70, 1.00, t);

    return mix(mix(a, b, t1), mix(c, d, t3), t2);
}

@fragment
fn fs_main(in: VertexOut) -> @location(0) vec4<f32> {
    let s = textureSample(sim_texture, sim_sampler, in.uv).y;
    let t = pow(clamp(s * 2.5, 0.0, 1.0), 0.75);
    return vec4<f32>(palette(t), 1.0);
}
```

The render pipeline should use linear sampling if the simulation texture is sampled as a regular texture. If storage texture output cannot be sampled directly, create a sampled view or copy/alias as required by the backend. This copy, if needed by the backend, must remain GPU-side.

---

## Python example structure

The exact Datoviz v0.4 Python API is not finalized, so the implementation should follow this conceptual structure rather than a fixed API.

```python
import numpy as np
import datoviz as dvz

WIDTH = 512
HEIGHT = 512

# 1. Create app/canvas/scene/panel.
app = ...
scene = ...
panel = ...

# 2. Create GPU resources for ping-pong simulation state.
state_a = create_storage_texture_or_buffer(WIDTH, HEIGHT, format="rg32float")
state_b = create_storage_texture_or_buffer(WIDTH, HEIGHT, format="rg32float")

# 3. Initialize simulation state on CPU and upload once.
initial = make_initial_state(WIDTH, HEIGHT)
state_a.upload(initial)
state_b.upload(initial)

# 4. Create dynamic parameter buffer/uniform/push constants.
params = ReactionDiffusionParams(...)
params_resource = create_uniform_or_push_constants(params)

# 5. Create compute shader/pipeline.
compute_pipeline = create_compute_pipeline(compute_shader_source, bindings=[state_a, state_b, params])

# 6. Create render shader/pipeline or image visual sampling the output texture.
image_visual = create_fullscreen_image_or_quad(texture=state_b)

# 7. Build framegraph or register per-frame passes.
#    COMPUTE pass writes state_b, RENDER pass samples state_b.

# 8. Event handlers.
#    - mouse drag updates brush position and brush_active.
#    - key press changes presets / pause / reset.
#    - GUI sliders update F/K/etc.

# 9. Per-frame callback.
def on_frame(dt):
    update_params(dt, mouse_state, gui_state)

    for _ in range(steps_per_frame):
        dispatch_compute(read=state_a, write=state_b, params=params)
        render_from(state_b)
        state_a, state_b = state_b, state_a

app.run()
```

The actual implementation may use scene-level animation/update callbacks, frame tick events, or explicit app callbacks depending on the final v0.4 API.

---

## Interaction requirements

Minimum required controls:

| Control | Behavior |
|---|---|
| Left mouse drag | Inject `v` into the simulation at the cursor position |
| Space | Pause/resume simulation |
| R | Reset initial condition |
| 1-5 | Switch presets |
| Up/down or GUI slider | Adjust number of simulation steps per frame |

Optional GUI controls:

- `F` feed rate slider;
- `K` kill rate slider;
- `Du` diffusion coefficient slider;
- `Dv` diffusion coefficient slider;
- `dt` slider;
- brush radius slider;
- brush strength slider;
- color scale slider.

Recommended slider ranges:

```text
F:              0.000 .. 0.100
K:              0.000 .. 0.080
Du:             0.000 .. 0.250
Dv:             0.000 .. 0.150
dt:             0.1   .. 2.0
steps/frame:    1     .. 32
brush radius:   2     .. 64 pixels
brush strength: 0.0   .. 1.0
```

The example should remain visually stable under normal slider values.

---

## Performance expectations

At `512 x 512`, the simulation should run smoothly on any discrete GPU and on most integrated GPUs.

Recommended defaults:

```text
simulation resolution: 512 x 512
workgroup size:        16 x 16
steps per frame:       4
```

The example should expose `steps_per_frame` because reaction-diffusion patterns often evolve slowly with a single step per rendered frame.

Avoid CPU-side per-frame NumPy updates. CPU work per frame should be limited to updating a small uniform buffer or push constants.

---

## Validation criteria

The example is successful if all of the following are true:

1. The simulation evolves continuously after startup.
2. The render pass displays the output of the compute pass from the same frame.
3. No CPU readback of the simulation field occurs during animation.
4. Mouse interaction modifies the simulation field visibly.
5. Preset switching changes the qualitative morphology of the pattern.
6. The example keeps running for several minutes without numerical blow-up.
7. The code remains reasonably short and readable.
8. The implementation is compatible with the intended v0.4 scene/DRP architecture.

---

## Edge cases and implementation notes

### Multiple compute steps per frame

If `steps_per_frame > 1`, the implementation should perform multiple compute dispatches before rendering or interleave dispatch/swap operations correctly.

Conceptually:

```text
for i in range(steps_per_frame):
    compute state_a -> state_b
    swap state_a/state_b

render state_a
```

After this loop, `state_a` contains the newest state.

### Bind group alternation

If bind groups cannot be modified cheaply every frame, create two compute bind groups:

```text
bind_group_0: src=state_a, dst=state_b
bind_group_1: src=state_b, dst=state_a
```

Then alternate them every dispatch.

Similarly, the render bind group must sample whichever resource contains the newest state.

### Storage texture format compatibility

Preferred:

```text
rg32float
```

Fallback:

```text
rgba32float
```

Fallback storage-buffer layout:

```text
index = y * width + x
state[index] = vec2<f32>(u, v)
```

The storage-buffer version can still render directly if the render shader reads the buffer using UV-to-index conversion, or if a GPU-side conversion pass writes into a sampled texture.

### Boundary conditions

Use periodic wrapping by default. It avoids special edge handling and makes the image tileable.

Clamp-to-edge boundaries are also acceptable but may create visible boundary artifacts.

### Numerical stability

Recommended safe values:

```text
dt <= 1.0
steps_per_frame <= 8 by default
u/v clamped to [0, 1]
```

If a user selects unstable parameters, the simulation may converge to blank output. This is acceptable, but reset/preset controls must recover quickly.

---

## Optional visual polish

The following additions are optional and should only be implemented if the core compute/render example is already working:

1. Add a subtle vignette in the fragment shader.
2. Add palette presets.
3. Add a small overlay text showing current `F`, `K`, and preset name.
4. Add a screenshot key.
5. Add automatic slow interpolation between presets.
6. Add a short warm-up phase before showing the first frame.

Do not add complexity that obscures the main purpose of the example: compute output feeding rendering directly.

---

## Minimal implementation target

A minimal acceptable version contains:

- one canvas/window;
- one 2D panel;
- two ping-pong GPU simulation textures or buffers;
- one compute shader implementing Gray-Scott update;
- one render path displaying the latest simulation texture;
- space pause/resume;
- R reset;
- left mouse injection;
- at least three parameter presets.

This minimal version should be prioritized over GUI polish.

---

## Suggested final example title and metadata

```yaml
title: Gray-Scott reaction-diffusion compute shader
tags:
  - compute
  - shader
  - texture
  - simulation
  - animation
  - interaction
summary: Run a Gray-Scott reaction-diffusion simulation in a compute shader and render the GPU output directly without CPU readback.
```
