# Using Datoviz in C

Datoviz is written in C and can be used directly from C applications. The v0.4
C examples are currently organized for development: they favor interactive
GLFW workbenches, realistic loads, and focused rendering utilities over tiny
tutorial programs.

---

## API layers

| Layer | Headers | Use case |
|-------|---------|----------|
| **Scene + App** | `datoviz/scene.h`, `datoviz/app.h` | Declare visuals, let Datoviz handle GPU execution |
| **vklite + Canvas** | `datoviz/vklite.h`, `datoviz/canvas.h` | Write Vulkan draw commands with Datoviz presentation plumbing |
| **DRP2 stream** | `datoviz/drp2.h` | Explore or test the backend-agnostic rendering protocol |

---

## Building the examples

The C examples live under grouped folders in `examples/c/` and are built as
part of the main CMake tree.

```bash
just build
just example-c visuals/point
just example-c visuals/mesh
just example-c features/technique_edl
just example-c showcases/point_cloud
just example-c advanced/raw_triangle_vklite
```

The executables land in matching build folders:

```text
build/examples/c/visuals/point
build/examples/c/features/technique_edl
build/examples/c/showcases/point_cloud
build/examples/c/advanced/raw_triangle_vklite
```

When a short example name is unique, `just example-c point` also works.

---

## Example groups

| Group | Purpose |
|-------|---------|
| `visuals/` | One public visual-family example per file |
| `features/` | One isolated scene feature, interaction, output workflow, or rendering technique per file |
| `composites/` | Semantic objects that lower to one or more visuals |
| `showcases/` | Composed workflows, scientific scenes, and gallery-facing demos |
| `advanced/` | Low-level runtime, DRP2, and host-integration examples |

Useful visual workbenches include:

```text
visuals/point
visuals/pixel
visuals/marker
visuals/primitive
visuals/segment
visuals/path
visuals/mesh
visuals/image
visuals/volume
visuals/sphere
visuals/text
```

---

## Key public APIs introduced in v0.4

| Function | Description |
|----------|-------------|
| `dvz_app()` | Create an app bound to a scene |
| `dvz_app_window_glfw()` | Register a GLFW canvas for a figure |
| `dvz_app_run()` | Render N frames, or run interactively with zero |
| `dvz_app_window_canvas()` | Access the underlying `DvzCanvas` |
| `dvz_app_window_capture_png()` | Save the last rendered frame as a PNG |
| `dvz_compile_glsl()` | Compile a GLSL string to SPIR-V at runtime |
| `dvz_drp2_runtime_download_buffer()` | Download GPU buffer bytes after execution |
| `dvz_drp2_stream_write_buffer_bytes()` | Append a write-buffer command from raw bytes |
| `dvz_primitive()` | Create a topology-parametric primitive visual |
| `dvz_image()` | Create a 2-D image visual |
| `dvz_sampled_field_set_data()` + `dvz_visual_set_field()` | Upload and bind 2-D texture data |

---

## See also

- `docs/architecture/drp2-overview.md` — DRP2 protocol design
- `examples/c/visuals/` — active visual examples
- `examples/c/advanced/` — raw runtime and host-integration examples
