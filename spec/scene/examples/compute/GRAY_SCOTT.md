# Gray-Scott Reaction-Diffusion Compute Example

> **Example status:** informative pressure test
> **Target:** Python compute/render example
> **Data:** deterministic generated simulation state
> **Validation:** bounded smoke, fixed seed, visual/readback checks

## Summary

Build a compute example where a Gray-Scott reaction-diffusion simulation updates ping-pong GPU
textures or buffers every frame and the render pass samples the newest state directly. No external
data is required.

## User-Visible Result

- One 2D panel filled by animated reaction-diffusion patterns: spots, coral, mitosis, worms, or
  mazes.
- High-contrast palette with dark background, cyan/teal mid-values, orange/gold high values, and
  optional white-hot highlights.
- Mouse injection into the field.
- Controls for pause, reset, presets, steps per frame, feed/kill rates, diffusion coefficients,
  `dt`, brush radius, brush strength, and color scale.

## Feature Pressure Points

- Compute pass every frame.
- Ping-pong resources used as simulation state.
- Render pass sampling compute output from the same frame.
- Multiple compute dispatches per displayed frame.
- Storage texture path preferred; storage-buffer fallback allowed.
- Parameter updates through uniforms or push constants.
- No CPU readback or CPU-side NumPy simulation during animation.

## Required Data And Resources

Simulation model:

```text
du/dt = Du * laplacian(u) - u*v*v + F*(1-u)
dv/dt = Dv * laplacian(v) + u*v*v - (F+K)*v
```

State stores `u` and `v` per grid cell:

```text
R = u
G = v
```

Default parameters:

```text
Du = 0.16
Dv = 0.08
F  = 0.035
K  = 0.065
dt = 1.0
resolution = 512 x 512
workgroup = 16 x 16
steps_per_frame = 4
```

Useful presets:

| Preset | F | K | Expected pattern |
|---|---:|---:|---|
| Spots | 0.035 | 0.065 | round spots and islands |
| Coral | 0.0545 | 0.062 | branching fronts |
| Mitosis | 0.0367 | 0.0649 | splitting cells |
| Worms | 0.078 | 0.061 | elongated moving structures |
| Mazes | 0.029 | 0.057 | labyrinths |

Initial condition:

```text
u = 1.0 everywhere
v = 0.0 everywhere
central 32 x 32 seed: u = 0.5, v = 0.25
optional fixed-seed circular seeds near center
```

Preferred resources:

```text
state_a: 2D rg32float storage/read texture
state_b: 2D rg32float storage/write texture, sampled by render
params: uniform buffer or push constants
```

Fallback resources:

```text
rgba32float textures, ignoring B/A
or storage buffers with index = y * width + x
```

## Scene Shape And Runtime Behavior

Per frame:

```text
for each simulation step:
    COMPUTE reaction_diffusion_step reads state_a, writes state_b
    swap state_a/state_b

RENDER main_color samples newest state
```

The compute shader should:

- Use periodic boundary conditions by default.
- Prefer a 9-point Laplacian:

```text
lap = 0.2  * (north + south + east + west)
    + 0.05 * (north_east + north_west + south_east + south_west)
    - 1.0  * center
```

- Clamp `u` and `v` to `[0, 1]`.
- Apply brush injection when active.

The render shader should sample the newest state, use the `v` channel as the primary scalar, and
map it through the palette. If a storage texture cannot be sampled directly, any required copy or
alias must stay GPU-side.

## Minimal Implementation Target

- One canvas/window and one 2D panel.
- Fixed `512 x 512` simulation.
- Two ping-pong GPU textures or buffers.
- One compute shader and one render path.
- Space pause/resume, `R` reset, left-drag injection.
- At least three presets.
- No GUI polish required before the compute/render path works.

## Validation / Acceptance Criteria

- Simulation evolves continuously after startup.
- Rendered texture for frame `N` is the compute output for frame `N`.
- No CPU readback of the simulation field during animation.
- Mouse injection visibly changes the field.
- Preset switching changes morphology.
- Bounded smoke run keeps finite values and avoids blank output.
- Runs for several minutes without numerical blow-up under default settings.
- `steps_per_frame > 1` alternates resources correctly.

## Links

- [Shared example policies](../POLICIES.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
