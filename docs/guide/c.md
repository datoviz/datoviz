# Using Datoviz in C

Datoviz is written in C and can be used directly from C applications.  The
library is layered: you can work at whichever level of abstraction suits your
use case, from a high-level scene API all the way down to the raw DRP2
rendering protocol.

---

## API layers

| Layer | Headers | Use case |
|-------|---------|----------|
| **Scene + App** | `datoviz/scene.h`, `datoviz/app.h` | Most users — declare visuals, let the library handle GPU execution |
| **vklite + Canvas** | `datoviz/vklite.h`, `datoviz/canvas.h` | Power users who want to write their own Vulkan draw commands |
| **DRP2 stream** | `datoviz/drp2.h` | Library developers and researchers exploring the rendering protocol |

All three layers share the same build setup.

---

## Building the examples (from source)

The examples live under `examples/c/` and are built as part of the main CMake
tree — no separate install step needed during development.

```bash
# Build everything (first time)
just build

# Build and run a specific example
just example-c hello_point
just example-c raw_triangle
just example-c raw_triangle_drp2
```

The executables land in `build/examples/c/`.

---

## Example 1 — `hello_point.c` (scene + app)

The highest-level path.  You declare visuals and data; `DvzApp` takes care of
GPU context, runtime, and rendering.

```c
--8<-- "examples/c/hello_point.c"
```

After one `dvz_app_run()` call the frame is in the offscreen canvas.
`dvz_app_window_capture_png()` reads it back and saves a PNG.

---

## Example 2 — `raw_triangle.c` (vklite draw commands into DvzCanvas)

For power users who know Vulkan and want to write their own draw commands
while letting `DvzCanvas` manage all presentation plumbing (frame timing,
offscreen images, submission, video recording).

The draw callback receives a `VkCommandBuffer` (via `DvzStreamFrame`) that is
already allocated and begun.  You record your own commands into it; the canvas
handles the rest.

The **same draw callback** runs unchanged for every backend — only the canvas
configuration differs.

```
./raw_triangle           → raw_triangle.png  (one offscreen frame)
./raw_triangle video     → raw_triangle.mp4  (120 frames)
```

```c
--8<-- "examples/c/raw_triangle.c"
```

---

## Example 3 — `raw_triangle_drp2.c` (manual DRP2 command stream)

DRP2 (Datoviz Rendering Protocol 2) is the backend-agnostic IR that sits
between the scene layer and the GPU.  This example bypasses both `DvzScene`
and `DvzCanvas` and constructs a DRP2 stream by hand.

It is intentionally verbose — the goal is to show every step of the protocol
so that developers of other scientific-visualization libraries can understand
how DRP2 works and experiment with it directly.

```
./raw_triangle_drp2      → raw_triangle_drp2.png
```

```c
--8<-- "examples/c/raw_triangle_drp2.c"
```

---

## Key public APIs introduced in v0.4

| Function | Description |
|----------|-------------|
| `dvz_app()` | Create an app bound to a scene (owns GPU ctx + DRP2 runtime) |
| `dvz_app_window()` | Register an offscreen canvas for a figure |
| `dvz_app_run()` | Render N frames |
| `dvz_app_window_canvas()` | Access the underlying `DvzCanvas` |
| `dvz_app_window_capture_png()` | Save the last rendered frame as a PNG |
| `dvz_compile_glsl()` | Compile a GLSL string to SPIR-V at runtime (shaderc) |
| `dvz_drp2_runtime_download_buffer()` | Download GPU buffer bytes to CPU after execution |
| `dvz_drp2_stream_write_buffer_bytes()` | Append a WriteBuffer command from raw bytes |

---

## See also

- `docs/architecture/drp2-overview.md` — DRP2 protocol design
- `docs/architecture/next_raw_triangle_examples.md` — implementation notes for future contributors
