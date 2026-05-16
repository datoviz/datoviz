# High-Precision Mandelbrot Deep Zoom

> **Agent Pickup**
> - **Category:** `compute`
> - **Implementation target:** Shader/compute-oriented example, preferably runnable with deterministic defaults.
> - **Data policy:** Generated or synthetic data by default; optional assets must have a cache and fallback.
> - **Preprocessing:** Document shader inputs, generated buffers/textures, and any optional Python preparation script.
> - **Validation:** Bounded smoke run, deterministic seed, and visual/readback criteria for the simulation state.


## Summary

Build a GPU Mandelbrot deep-zoom example that renders a fullscreen procedural fractal through
Datoviz scene/DRP resources, with smooth pan/zoom interaction and a deterministic animated path.
The example uses generated shader inputs only: uniforms for center, scale, aspect, iteration count,
precision mode, and palette parameters, plus any generated buffers needed by the chosen fullscreen
visual. The first practical slice should render one fixed deep-zoom location with double-single
arithmetic and a bounded scripted zoom before adding richer interaction or optional perturbation
math. Validate with a bounded smoke run using deterministic defaults, checking that the frame
renders, navigation updates shader state, and readback or screenshot criteria catch a blank or
obviously incorrect fractal.


## Goal

Create a Datoviz v0.4 example showing a high-precision, deeply zoomable Mandelbrot fractal rendered interactively on the GPU.

The example should demonstrate:

* custom shader integration;
* full-screen procedural rendering;
* smooth animated zooms into interesting Mandelbrot locations;
* interactive pan/zoom controls;
* high-precision numerical handling beyond standard `float32`;
* compatibility with the future Datoviz v0.4 Scene / DRP architecture, without depending on an exact finalized Python API.

The preferred implementation is a **Python example using the v0.4 Scene API**, with a custom shader-backed fullscreen visual. If the Python API is not yet mature enough for custom shaders, the example may instead be implemented in **C on top of the Scene C API or directly on top of DRP**.

Datoviz v0.4 scene is expected to remain GPU-backend agnostic and emit DRP commands rather than direct Vulkan/windowing calls, so this example should be specified at the Scene/DRP level rather than tied to a particular backend.  The Scene API already includes panels, cameras, visuals, resources, controllers, animation hooks, and DRP generation concepts that fit this example well.  DRP supports WebGPU-style shader modules, render pipelines, compute pipelines, buffers, textures, passes, and command encoding, which are enough for either fragment-shader or compute-shader Mandelbrot rendering.

---

## Suggested filename

```text
mandelbrot_deep_zoom.md
```

Suggested eventual example script names:

```text
examples/python/mandelbrot_deep_zoom.py
```

or, for a lower-level C implementation:

```text
examples/scene/mandelbrot_deep_zoom.c
```

---

## Visual result

The example should open a window showing a Mandelbrot fractal with:

* smooth anti-aliased rendering;
* rich continuous coloring;
* deep zoom animation;
* optional orbit-trap or distance-estimation style shading;
* interactive mouse wheel zoom;
* click-and-drag panning;
* optional keyboard shortcuts to jump between predefined deep zoom locations.

The visual should feel like a polished demo, not a minimal fractal toy.

Recommended default appearance:

* black or very dark background outside the set;
* glowing color gradients outside the set;
* smooth coloring using normalized iteration count;
* optional subtle temporal interpolation during navigation;
* no axes by default;
* optional small overlay text showing zoom level, iteration count, precision mode, and current center.

---

## Core rendering idea

Render a single fullscreen quad or triangle covering the panel. The fragment shader computes the Mandelbrot escape iteration for each pixel.

For each pixel:

```text
c = center + pixel_to_complex(pixel, zoom, aspect)
z = 0
repeat:
    z = z*z + c
    escape if |z| > bailout
```

The challenge is high precision during deep zoom. A naive `float32` shader fails quickly because neighboring pixels become indistinguishable when the zoom scale is much smaller than about `1e-7`. Even `float64`, if available, is not portable and not desirable for WebGPU-style backends.

The example should therefore use one of these two approaches.

---

## Precision strategy

### Baseline mode: shader-only double-single arithmetic

The first implementation should support a GPU-only mode using **double-single arithmetic**, representing each value as two `float32` values:

```text
hi + lo
```

Complex numbers are represented as:

```text
struct DSComplex {
    vec2 hi;
    vec2 lo;
}
```

This gives approximately 44–48 bits of effective mantissa, enough for visually impressive zooms beyond ordinary `float32`, while remaining portable to GPU APIs that lack true `float64`.

Use this mode for:

* moderate deep zooms;
* simple implementation;
* real-time interaction;
* robust portability.

Expected useful zoom range:

```text
~1e8 to ~1e12, depending on shader quality and viewport size
```

This is already enough to visibly demonstrate the need for enhanced precision.

### Advanced mode: CPU high-precision reference orbit + GPU perturbation

For true deep zoom, use the standard Mandelbrot deep-zoom technique:

1. The CPU computes a high-precision reference orbit for the center point `C`.
2. The GPU computes only the perturbation `dc` for each pixel around that reference center.
3. The shader iterates the perturbation recurrence using lower precision.

Let:

```text
c = C + dc
z_n(c) = Z_n + dz_n
```

where `Z_n` is the high-precision reference orbit for the center `C`.

The perturbation recurrence is:

```text
dz_{n+1} = 2 * Z_n * dz_n + dz_n^2 + dc
```

Escape is tested on:

```text
Z_n + dz_n
```

The CPU uploads the reference orbit `Z_n` as a buffer or texture each frame or whenever the zoom target changes.

This allows much deeper zooms than direct per-pixel iteration.

Expected useful zoom range:

```text
1e20 and beyond
```

The exact depth depends on implementation details, orbit length, glitch handling, and whether rebasing is implemented.

### Recommended implementation path

Start with double-single arithmetic in a fragment shader. Add perturbation mode as an optional advanced path.

The example should be useful even if perturbation mode is not implemented immediately, but the spec should be structured so that another agent can extend it naturally.

---

## Data source

This example does not require a large external dataset.

However, to satisfy the “download data from `datoviz/data` if not cached” convention, the example may optionally load a small JSON file containing predefined Mandelbrot locations and camera paths:

```text
https://github.com/datoviz/data/...
```

Suggested data file:

```text
fractals/mandelbrot_deep_zoom_locations.json
```

If the file is absent from the cache and cannot be downloaded, the example should fall back to built-in locations hardcoded in the script.

Example JSON structure:

```json
{
  "locations": [
    {
      "name": "Seahorse Valley",
      "center": ["-0.743643887037151", "0.131825904205330"],
      "initial_scale": "2.5",
      "target_scale": "1e-10",
      "max_iter": 1500,
      "palette": "inferno"
    },
    {
      "name": "Elephant Valley",
      "center": ["0.275", "0.0"],
      "initial_scale": "3.0",
      "target_scale": "1e-8",
      "max_iter": 1000,
      "palette": "turbo"
    },
    {
      "name": "Mini Mandelbrot",
      "center": ["-1.25066", "0.02012"],
      "initial_scale": "1e-2",
      "target_scale": "1e-9",
      "max_iter": 2000,
      "palette": "magma"
    }
  ]
}
```

All decimal values should be stored as strings to avoid precision loss when parsed.

---

## Scene structure

The example should create:

```text
Scene
└── Panel
    ├── 2D camera or custom fractal controller
    ├── Fullscreen Mandelbrot visual
    └── Optional overlay visual for text / HUD
```

The fractal visual is not a standard scatter, line, image, or mesh visual. It is best described as a **custom procedural fullscreen visual**.

The visual needs:

* one fullscreen triangle or quad;
* a custom fragment shader;
* a uniform buffer containing view parameters;
* optional storage buffer containing the high-precision reference orbit;
* optional 1D texture or uniform array for the color palette.

The Scene layer’s concepts of resources, channels, visuals, panels, controllers, animations, and framegraph passes map naturally to this. Resources are CPU-side arrays tracked for dirty regions and uploaded through DRP when needed.  Visuals can be assigned to opaque, transparent, overlay, or picking stages, although this example only needs one main render pass and possibly an overlay pass.

---

## Required runtime behavior

### Startup

On startup, the example should:

1. Create a Datoviz app/window.
2. Create one scene.
3. Create one panel filling the window.
4. Create a fullscreen custom visual.
5. Load or define a Mandelbrot location.
6. Initialize the fractal view state:

   * center;
   * scale;
   * aspect ratio;
   * max iterations;
   * bailout radius;
   * palette parameters;
   * precision mode.
7. Start a smooth zoom animation.

The first frame should show the full Mandelbrot set or a recognizable intermediate view.

### Animation

The default animation should smoothly zoom toward the selected location.

Use logarithmic interpolation for the scale:

```text
log_scale(t) = mix(log(initial_scale), log(target_scale), easing(t))
scale(t) = exp(log_scale(t))
```

The center may be constant or slowly interpolated toward the target.

Use easing such as:

```text
smoothstep
easeInOutCubic
easeInOutSine
```

The animation should remain interactive: user input should pause or override the automated zoom.

Scene-level animation hooks are planned in v0.4 through animation objects updated by `scene_update`, so the example should conceptually use that mechanism where available.

### Interaction

Minimum interaction:

| Input        | Behavior                                       |
| ------------ | ---------------------------------------------- |
| Mouse wheel  | Zoom around cursor                             |
| Left drag    | Pan                                            |
| Double click | Recenter on clicked point                      |
| Space        | Pause / resume animation                       |
| R            | Reset current location                         |
| 1–9          | Jump to predefined locations                   |
| + / -        | Increase / decrease max iterations             |
| P            | Toggle precision mode, if both are implemented |
| H            | Toggle HUD                                     |

Zooming around the cursor should keep the complex coordinate under the cursor fixed.

For a pixel position `p`, convert to normalized panel coordinates, then to complex coordinates:

```text
complex_before = screen_to_complex(p, center, scale)
scale *= zoom_factor
center = complex_before - screen_offset(p, new_scale)
```

The controller should update a CPU-side uniform resource and mark it dirty.

---

## Uniform state

The shader should receive a compact uniform block similar to:

```c
struct MandelbrotParams {
    vec2 viewport_size;
    vec2 center_hi;
    vec2 center_lo;
    vec2 scale_hi_lo;
    float aspect;
    uint max_iter;
    float bailout;
    float time;
    uint precision_mode;
    uint palette_mode;
    float color_offset;
    float color_scale;
};
```

The exact ABI should follow the actual v0.4 Python/C binding conventions.

For a simpler version:

```text
center: complex high precision, represented as either decimal strings on CPU or hi/lo float pairs on GPU
scale: high precision scalar, represented as hi/lo or log-scale
max_iter: uint
bailout: float
palette parameters
viewport size
```

---

## Shader design

### Vertex shader

Use a fullscreen triangle to avoid a vertex buffer if possible.

Conceptual WGSL-style vertex shader:

```wgsl
@vertex
fn vs_main(@builtin(vertex_index) i: u32) -> VertexOut {
    var pos = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
        vec2<f32>(-1.0,  3.0)
    );

    var out: VertexOut;
    out.position = vec4<f32>(pos[i], 0.0, 1.0);
    out.uv = 0.5 * (pos[i] + vec2<f32>(1.0));
    return out;
}
```

If the backend requires a vertex buffer, use a fullscreen quad with two triangles.

### Fragment shader

The fragment shader should:

1. Convert pixel coordinates to complex coordinates.
2. Run Mandelbrot iterations.
3. Compute smooth iteration count.
4. Map the result to color.
5. Return RGBA.

Smooth coloring:

```text
nu = iter + 1 - log2(log2(|z|))
```

Color phase:

```text
t = color_scale * nu + color_offset
```

Use a compact procedural palette, for example cosine palette:

```text
color = a + b * cos(2π * (c * t + d))
```

This avoids requiring a texture palette.

### Distance-estimation option

Optionally compute derivative:

```text
dz = 2*z*dz + 1
```

Then estimate distance:

```text
dist = 0.5 * log(|z|) * |z| / |dz|
```

Use this for subtle edge shading or glow.

This is optional; the core example should not depend on it.

---

## Precision details

### Double-single representation

Represent a high-precision float as:

```text
struct DS {
    float hi;
    float lo;
}
```

Required operations:

```text
ds_add(a, b)
ds_sub(a, b)
ds_mul(a, b)
ds_square(a)
```

For complex numbers:

```text
ds_complex_add
ds_complex_mul
ds_complex_square
```

The shader should not try to be a general arbitrary-precision library. It only needs enough operations for:

```text
z = z*z + c
```

### CPU-side conversion

The CPU should store center and scale using Python `decimal.Decimal` or `mpmath.mpf`.

For upload to GPU:

```text
hi = float32(x)
lo = float32(x - Decimal(hi))
```

This preserves more precision than a single float.

### Perturbation mode

If implemented, the CPU should compute:

```text
Z[0] = 0
Z[n+1] = Z[n]^2 + C
```

using `mpmath` or Python `decimal`.

Upload `Z[n]` to the GPU as either:

* `vec4<f32>` values storing `(real_hi, real_lo, imag_hi, imag_lo)`;
* or a texture buffer / storage buffer depending on backend support.

The shader then computes `dz` per pixel.

Pseudo recurrence:

```text
dz = 0
dc = pixel_offset_from_center

for n in 0..max_iter:
    z_approx = Z[n] + dz
    if norm(z_approx) > bailout:
        escape

    dz = 2 * Z[n] * dz + dz * dz + dc
```

The first implementation may skip glitch correction. If artifacts appear at extreme zoom, document that this is expected and that rebasing/glitch detection is future work.

---

## DRP / Scene implementation options

### Option A: Scene custom visual, preferred

The example should define a custom fullscreen visual whose material/shader is supplied by the user.

Conceptually:

```python
scene = app.scene()
panel = scene.panel()
visual = scene.visual("custom_fullscreen")
visual.shader(fragment=mandelbrot_shader)
visual.uniform("params", params_buffer)
panel.add(visual)
```

Do not require this exact API. The implementing agent should adapt to the finalized v0.4 Python API.

This option is best if v0.4 exposes:

* custom shader visuals;
* uniform resources;
* per-frame update callbacks;
* event controllers;
* fullscreen primitives.

### Option B: Scene C API

If Python custom shaders are not ready, implement the example in C using the Scene API.

The C version should:

* create a `DvzScene`;
* create a panel;
* create a custom visual;
* create CPU-side resources for uniforms;
* attach an event controller;
* build DRP commands every frame.

The Scene C API already defines scene, panel, visual, resource, controller, animation, framegraph, and DRP builder structures that fit this design.

### Option C: Direct DRP implementation

If the custom Scene visual path is not available, implement directly with DRP:

1. Create uniform buffer.
2. Create shader module.
3. Create bind group layout.
4. Create bind group.
5. Create pipeline layout.
6. Create render pipeline.
7. Create command encoder.
8. Begin render pass.
9. Set pipeline.
10. Set bind group.
11. Draw fullscreen triangle.
12. End pass.
13. Submit.

DRP’s object model is intentionally WebGPU-aligned, with explicit buffer, shader, bind group, pipeline, render pass, draw, compute dispatch, and submit commands.

### Option D: Compute shader + texture

An alternative implementation is:

1. Compute shader fills an RGBA texture with fractal pixels.
2. Render pass displays the texture using a fullscreen image visual.

This is useful if:

* compute shaders are easier to customize than fragment shaders;
* the result should be reused as a texture;
* progressive rendering is implemented;
* tiled rendering is desired.

DRP supports compute pipelines and compute passes, including dispatch commands.

Fragment-shader rendering is simpler and should be the default.

---

## Progressive refinement

For responsiveness during interaction, use progressive quality:

```text
while user is actively dragging/zooming:
    max_iter = low value, e.g. 128–512
    antialiasing = 1 sample

after interaction stops:
    max_iter = high value, e.g. 1000–5000
    antialiasing = 2x2 or 3x3 samples
```

Optional:

* render at half resolution during interaction;
* accumulate samples over frames;
* jitter subpixel samples for anti-aliasing;
* reset accumulation when camera changes.

---

## Anti-aliasing

Minimum requirement:

* use at least one sample per pixel;
* avoid visibly jagged edges through smooth coloring.

Preferred:

* 2x2 supersampling in shader for final/high-quality mode;
* disabled or reduced during interaction.

Pseudo-code:

```text
color = 0
for sample in samples:
    c = pixel_to_complex(pixel + sample_offset)
    color += mandelbrot(c)
color /= sample_count
```

---

## HUD overlay

Optional but recommended.

Display:

```text
Mandelbrot deep zoom
Location: Seahorse Valley
Scale: 1e-10
Iterations: 1500
Precision: double-single / perturbation
FPS: ...
Controls: wheel zoom, drag pan, space pause
```

If text rendering is not ready, print this information to the console and/or expose it in the window title.

---

## Performance expectations

At 1920×1080:

* baseline shader-only mode should remain interactive for `max_iter <= 1000` on a modern GPU;
* deep perturbation mode may be more expensive due to reference orbit buffer reads;
* interaction mode should reduce iterations dynamically;
* high-quality still mode may use more iterations and supersampling.

The example should expose these parameters near the top of the file:

```python
WIDTH = 1200
HEIGHT = 900
MAX_ITER_INTERACTIVE = 512
MAX_ITER_FINAL = 2000
SUPERSAMPLING_FINAL = 2
DEFAULT_LOCATION = "Seahorse Valley"
PRECISION_MODE = "double-single"
```

---

## Caching and downloads

The example should use the standard Datoviz data cache convention if available.

Pseudo-code:

```python
path = datoviz_data("fractals/mandelbrot_deep_zoom_locations.json")
if not path.exists():
    download_from_datoviz_data_repo(...)
```

Fallback:

```python
LOCATIONS = [...]
```

The example must work offline after the first successful run.

Since the external data is only optional metadata, failure to download must not prevent the example from running.

---

## Acceptance criteria

The example is successful if:

1. It runs out of the box from the Datoviz repository.
2. It opens a single interactive window.
3. It renders a recognizable Mandelbrot fractal.
4. It smoothly animates a zoom into a predefined location.
5. Mouse wheel zoom and drag pan work.
6. The fractal remains stable beyond ordinary `float32` zoom levels.
7. The implementation uses a custom shader or custom DRP pipeline.
8. It does not depend on finalized details of the v0.4 Python API in the markdown spec.
9. It degrades gracefully if optional downloaded location data is unavailable.
10. The code is clean enough to serve as a test of custom shaders, uniform updates, event handling, and frame rendering in v0.4.

---

## Minimal implementation checklist

A first implementation can be limited to:

* Python example;
* fullscreen fragment shader;
* double-single center/scale;
* procedural color palette;
* one hardcoded deep zoom location;
* wheel zoom;
* drag pan;
* automatic zoom animation;
* no perturbation;
* no external data.

A more advanced implementation can add:

* `datoviz/data` JSON location download;
* perturbation mode;
* CPU high-precision reference orbit with `mpmath`;
* progressive rendering;
* HUD overlay;
* multiple locations;
* export screenshot or short video.

---

## Notes for the implementing agent

Avoid hardcoding assumptions about the final v0.4 Python API.

The agent should adapt this specification to whichever of these APIs exists at implementation time:

* high-level Python Scene API;
* lower-level Python DRP bindings;
* C Scene API;
* direct C DRP API.

The example should be treated as a stress test for:

* custom shader support;
* uniform buffer updates;
* event/controller routing;
* animation scheduling;
* framegraph / render-pass generation;
* backend-independent rendering;
* optional compute/render interop.

The result should be visually impressive enough to serve as a Datoviz v0.4 showcase example, while remaining technically focused and maintainable.
