# High-Precision Mandelbrot Deep Zoom

> **Example status:** informative pressure test
> **Target:** Python custom-shader example, with C/DRP fallback if needed
> **Data:** generated shader inputs, optional cached location JSON
> **Validation:** bounded smoke, deterministic zoom, screenshot/readback checks

## Summary

Build a fullscreen procedural Mandelbrot deep-zoom example with smooth animation, pan/zoom
interaction, high-precision view state, and a custom shader-backed visual or equivalent DRP path.
The first slice should use GPU double-single arithmetic for one hardcoded deep-zoom location; an
advanced perturbation path may be added later.

## User-Visible Result

- One window filled by a polished Mandelbrot rendering.
- Smooth automated logarithmic zoom into a named location such as Seahorse Valley.
- Mouse wheel zoom around cursor, left-drag pan, double-click recenter, pause/resume, reset, and
  optional location shortcuts.
- Continuous coloring, dark background, optional distance-estimation edge glow, and optional HUD
  with location, scale, iterations, precision mode, and FPS.
- Stable detail beyond ordinary `float32` zoom levels.

## Feature Pressure Points

- Custom fullscreen shader visual or direct render pipeline.
- Uniform resource updates for high-precision center, scale, viewport, iteration count, palette,
  time, and precision mode.
- Scene animation hook for deterministic zoom path.
- Event/controller routing for cursor-preserving zoom and pan.
- Optional storage buffer/texture for perturbation reference orbit.
- Optional compute-to-texture path if fullscreen fragment customization is unavailable.

## Required Data And Resources

No external data is required. Optional cached metadata may provide named locations:

```text
fractals/mandelbrot_deep_zoom_locations.json
```

Example location fields:

```text
name
center real/imag as decimal strings
initial_scale as decimal string
target_scale as decimal string
max_iter
palette
```

Built-in fallback locations must be present if the JSON is unavailable. Decimal values should stay
as strings until converted to high-precision CPU values.

Uniform state:

```text
viewport_size
center_hi, center_lo
scale_hi_lo or log_scale
aspect
max_iter
bailout
time
precision_mode
palette_mode
color_offset, color_scale
```

Baseline precision: represent high-precision floats as `hi + lo` `float32` pairs. CPU conversion
should use `decimal.Decimal` or `mpmath`:

```text
hi = float32(x)
lo = float32(x - Decimal(hi))
```

Advanced precision: CPU high-precision reference orbit plus GPU perturbation:

```text
Z[0] = 0
Z[n+1] = Z[n]^2 + C
dz[n+1] = 2 * Z[n] * dz[n] + dz[n]^2 + dc
```

## Scene Shape And Runtime Behavior

Scene shape:

```text
Scene
└── Panel
    ├── 2D/custom fractal controller
    ├── Fullscreen Mandelbrot visual
    └── Optional HUD overlay
```

Rendering:

- Draw one fullscreen triangle or quad.
- Convert pixel coordinates to complex coordinates.
- Iterate `z = z*z + c` in double-single baseline mode.
- Compute smooth iteration count:

```text
nu = iter + 1 - log2(log2(|z|))
t = color_scale * nu + color_offset
```

- Use a procedural palette, such as cosine palette, to avoid a required texture.
- Optional final/high-quality mode can use `2x2` supersampling; interaction mode should reduce
  iterations and samples.

Default exposed parameters:

```text
WIDTH = 1200
HEIGHT = 900
MAX_ITER_INTERACTIVE = 512
MAX_ITER_FINAL = 2000
SUPERSAMPLING_FINAL = 2
DEFAULT_LOCATION = "Seahorse Valley"
PRECISION_MODE = "double-single"
```

Animation should interpolate scale logarithmically from `initial_scale` to `target_scale`, using a
smooth easing function. User input pauses or overrides the automated zoom.

## Minimal Implementation Target

- Fullscreen fragment shader or equivalent direct DRP pipeline.
- Double-single center and scale.
- Procedural color palette.
- One hardcoded location.
- Wheel zoom, drag pan, reset, and automatic zoom.
- No perturbation, no external data, no required text overlay.

## Validation / Acceptance Criteria

- Runs out of the box and opens one interactive window.
- Renders a recognizable Mandelbrot set on the first frame.
- Scripted zoom is deterministic and nonblank.
- Cursor-centered zoom preserves the complex coordinate under the cursor.
- Drag pan and reset update shader uniforms immediately.
- The image remains visually stable past `float32`-only zoom limits.
- Optional downloaded location data degrades to built-in locations.
- Screenshot/readback validation catches blank output or obviously wrong coloring.

## Links

- [Shared example policies](../POLICIES.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
