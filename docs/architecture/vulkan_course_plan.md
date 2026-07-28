# Vulkan course — rewrite plan

**Status: in progress. Part 1 (chapters 1-3) has landed; chapters 4-15 are pending. Supersedes the
RC3 "Vulkan tutorial pilot", which is deleted.**

This note plans a full rewrite of the AI-generated Vulkan tutorial that lived at
`docs/tutorials/vulkan/`. The rewrite changes the section, the pedagogy, the code delivery model,
and the end goal.

Landed so far, in `docs/advanced/vulkan/` and `examples/c/vulkan/`:

| Chapter | Page | Step program | Result |
| --- | --- | --- | --- |
| 1 | `01-setup.md` | `step01.c` (9 lines) | Datoviz links; version prints |
| 2 | `02-window.md` | `step02.c` (123 lines) | resizable window in a chosen color, or a PNG |
| 3 | `03-frame.md` | `step03.c` (139 lines) | the frame anatomy, and a pulsing window |

`just vulkan-course-check` verifies every code excerpt against its step program;
`just vulkan-course-smoke` renders every step offscreen and requires reproducible captures with zero
validation errors.

All structural decisions are settled — see the spec's "Rewrite Direction" section. In short: the
hidden machinery is taught through per-chapter asides and the closing chapter rather than dedicated
synchronization and swapchain chapters; chapter 15 uses generated geometry and a procedural texture
rather than a committed Suzanne asset; and compute is named in the epilogue rather than given a
chapter. Chapters 4-15 can be written without further input.

---

## 1. What is wrong with the current pilot

| Problem | Evidence |
| --- | --- |
| Wrong section | Sits in `Tutorials`, right after `Get Started`, as if it were a beginner path. It is the most advanced material in the docs. |
| Stale nav title | `mkdocs.yml` says *Modern GPU Graphics in C*; the page says *in Vulkan*. |
| Not self-contained | Chapter 1 opens with `./build/gpu-tutorial/first_triangle --live`. The reader runs someone else's binary and edits shaders inside a cloned checkout. Nothing is ever written from scratch. |
| Reference-first, not build-first | Chapters explain a finished 1000-line `triangle.c` driven by `#ifdef DVZ_TUTORIAL_USE_*` switches. The reader never sees a program grow. |
| Wrong altitude in places | Frame-contract counters, `invalid_frame_contract_count`, `resource_generation`, ownership audits. That is release-validation vocabulary, not a graphics course. |
| Stops far too early | Three chapters, ending at a vertex buffer holding a flat triangle. No 3D, no depth, no camera, no texture, no light. |
| Dead spikes in the repo | `examples/c/tutorial/shaders/{depth,texture,arcball}` and the `*_spike` targets exist but no chapter uses them. |

Conclusion: keep almost nothing. The `vklite` API surface it proved out is sound and is exactly
what the rewrite builds on; the tutorial structure around it goes.

---

## 2. Audience and promise

**Audience.** A C programmer who wants to learn modern GPU graphics. They know C and a build
system. They know no Vulkan, and they do not want to spend the first week writing instance,
device, swapchain, and semaphore code before seeing a pixel.

**Promise (the prologue's job to state).** *You will write, from an empty file, one C program that
ends as an interactive 3D mesh viewer: perspective camera, mouse-driven rotation, depth testing,
a texture you upload yourself, and per-fragment lighting. About 600 lines. The same program
written against raw Vulkan is roughly 2000 lines, and about 1400 of those have nothing to do with
graphics.*

**The pedagogical contract** — what stays hidden, what stays explicit:

| Datoviz does it for you | You write it |
| --- | --- |
| Instance, physical-device selection, logical device, queues | Shader source, both stages |
| Window, surface, swapchain, image acquisition, presentation | Pipeline state: topology, vertex layout, depth, culling, blending |
| Per-frame semaphores/fences, frame pacing, resize recovery | Vertex, index, and uniform data, and its GPU buffers |
| Command-pool and command-buffer allocation | Command recording: pass begin/end, binds, draws |
| Depth image allocation, offscreen capture, PNG/video sinks | Textures: staging, layout transitions, samplers, descriptors |
| The memory allocator | Matrices, camera, light math |

The hidden column is *named and explained* — one "Under the hood" aside per chapter says what raw
Vulkan would demand at that exact point and roughly how many lines it costs. The reader finishes
knowing what a swapchain and a fence are, without having written one. This is the OpenGL-tutorial
feel the course is after, with Vulkan concepts underneath.

---

## 3. Placement, naming, URLs

- **Section: `Advanced`.** Recommended as its own nav group named **Vulkan course**, placed
  directly after `Advanced > Overview` so it stays discoverable (alternative: after
  `Runtime layers`, which is thematically closer but buries it).
- **Title: `Modern GPU Graphics in Vulkan`** — fixes the stale `in C`.
- **Files: `docs/advanced/vulkan/`**, numbered for a linear read:
  `index.md`, `01-setup.md`, `02-window.md`, … `15-mesh.md`, `16-next.md`.
- **Consequence to confirm:** moving this out empties the `Tutorials` tab — only a 6-line
  `tutorials/first-scene.md` stub remains, and it is not even in the nav. Recommendation: drop the
  `Tutorials` tab, delete the stub, and let `Get Started` / `How-To` / `Examples` carry that role.

---

## 4. Ground rules for the rewrite

1. **The reader types everything.** Chapter 1 starts with an empty `main.c` in *their* directory
   and a build file they write. No `git clone`, no prebuilt binary, no repo-relative paths in any
   command the reader runs.
2. **One file that grows.** `main.c` is appended to and edited chapter by chapter. Every chapter
   ends with a collapsible **full current listing** so a stuck reader can resync in one copy-paste.
   From chapter 5 on there are also `shader.vert` / `shader.frag`.
3. **Install, don't build.** `pip install datoviz` gives headers, `datoviz-config`, and a CMake
   package. That is the default path, so the reader needs no source checkout. Source build is a
   fallback link, not the main road.
4. **Every chapter ends in a picture.** Each chapter opens with the image the reader will have at
   the end, and every chapter's result is visibly different from the previous one's.
5. **One new concept per step, and it must pay off immediately.** No concept is introduced to be
   used three chapters later.
6. **Failure is taught, not avoided.** Each chapter has a "when it goes wrong" box: black window,
   nothing drawn, inside-out cube, upside-down texture — with the diagnosis.
7. **Validation layers on from chapter 2**, framed as the reader's safety net, not as CI plumbing.
8. **No release-validation vocabulary.** No frame contracts, no resource generations, no counter
   dumps.

---

## 5. Chapter map

Four parts, 15 chapters plus an epilogue. `Δ` is the rough size of the reader's file at the end of
the chapter.

### Prologue — `index.md`
Hero image (or short video) of the final mesh viewer. The promise, the contract table from §2,
prerequisites, and how the course is structured. No code.

### Part 1 — A window and a frame

| # | Chapter | Reader adds | Concepts | Result | Δ |
| --- | --- | --- | --- | --- | --- |
| 1 | **Setup** | `main.c` printing `dvz_version()`; a `CMakeLists.txt` and a one-line `cc` alternative via `datoviz-config` | toolchain, linking a native library, where the headers are | terminal output, and confidence the build works | 15 |
| 2 | **Your first window** | window host, GPU context, canvas, frame loop, teardown | what a GPU context/device is; render loop; event polling; what the swapchain does *for* you; clear color; offscreen mode + PNG capture as your screenshot tool | a resizable window in the color you chose | 70 |
| 3 | **Recording commands** | a draw callback; wrap the borrowed command buffer; begin/end a rendering pass; viewport and scissor; animate the clear color | CPU records, GPU executes; command buffers; render pass and attachments; load/store ops; borrowed vs owned objects | a pulsing background — proof your commands run | 100 |

### Part 2 — Triangles: shaders, pipeline, vertex data

| # | Chapter | Reader adds | Concepts | Result | Δ |
| --- | --- | --- | --- | --- | --- |
| 4 | **Your first triangle** | inline GLSL strings, `dvz_compile_glsl`, shader modules, empty pipeline layout, graphics pipeline, `dvz_cmd_draw(0,3,0,1)` | vertex and fragment stages; `gl_VertexIndex`; clip space and Vulkan's y-down, z 0..1; primitive topology; rasterization; interpolation; a pipeline as frozen state | the RGB triangle | 190 |
| 5 | **Shaders in their own files** | move GLSL to `shader.vert`/`shader.frag`, load and compile at startup, print compiler diagnostics, press `R` to recompile and rebuild the pipeline live | GLSL → SPIR-V; runtime vs offline compilation; why a pipeline must be rebuilt when a shader changes; reading compiler errors | same triangle, but shader edits appear without recompiling C | 230 |
| 6 | **Vertex buffers** | a `Vertex` struct, a mapped GPU buffer, upload, vertex binding + attributes, bind and draw | host-visible vs device-local memory; stride and `offsetof`; attribute formats; the shader's `in` locations must match the pipeline's attributes | a triangle from *your* data, then a quad from 6 vertices | 270 |
| 7 | **Index buffers** *(short)* | 4 vertices + 6 indices, `dvz_cmd_draw_indexed` | vertex reuse; `uint16` vs `uint32`; how indices feed primitive assembly | the same quad, 4 vertices instead of 6 | 290 |

### Part 3 — Into 3D

| # | Chapter | Reader adds | Concepts | Result | Δ |
| --- | --- | --- | --- | --- | --- |
| 8 | **Push constants** | a push-constant slot, per-frame elapsed time, use it in the shader | the three ways to get data to a shader (push constants, uniform buffers, storage buffers) and when each fits; pipeline layout; per-frame vs per-object data | the quad spins and pulses from shader-side math | 320 |
| 9 | **Matrices and perspective** | three small matrix helpers, a cube (8 vertices, 36 indices), MVP pushed each frame | homogeneous coordinates; model/view/projection; perspective divide; FOV, aspect, near/far; how Vulkan's clip space differs from OpenGL's; aspect on resize | a spinning cube that looks *wrong* — faces in the wrong order | 380 |
| 10 | **Depth and culling** | request a depth buffer from the canvas, attach it, enable depth test/write, then set cull mode and front face | why chapter 9 looked wrong; the depth buffer and depth range; z-fighting; winding order and back-face culling; wireframe mode as an experiment | a correct solid spinning cube | 420 |
| 11 | **Mouse control** | input router, arcball, camera, compose and push the matrices, handle resize | interaction as matrix state; separating camera from model; why projection depends on window size | drag to rotate, scroll to zoom | 460 |

### Part 4 — Surfaces: textures and light

| # | Chapter | Reader adds | Concepts | Result | Δ |
| --- | --- | --- | --- | --- | --- |
| 12 | **Uploading a texture** | procedural checkerboard pixels, a staging buffer, an image, two layout transitions, the copy, a one-shot submit | images vs buffers; tiling and why a copy is needed; image layouts and barriers; sRGB vs linear | nothing visible yet — verified by validation staying silent | 510 |
| 13 | **Sampling the texture** | a sampler, a descriptor slot and set, a `texcoord` attribute, sampling in the fragment shader | descriptor sets vs push constants; filtering; address modes; UV orientation | a textured cube | 550 |
| 14 | **Lighting** | a `normal` attribute, the normal matrix, ambient + diffuse, then specular; light and eye position pushed | normals and the dot product; world vs view space; per-vertex vs per-fragment shading; gamma | a lit textured cube | 600 |
| 15 | **A real mesh** | `dvz_geometry_sphere`/`torus` (and `dvz_geometry_obj` for your own model), double→float conversion, `dvz_geometry_compute_normals` | separating mesh *data* from mesh *rendering*; index counts; why the GPU wants floats | **the deliverable:** a rotatable, textured, lit mesh | 640 |

### Epilogue — `16-next.md`
What you never wrote, one paragraph each with a pointer to where Datoviz does it: instance and
device creation, queue families, surface and swapchain, acquire/present, semaphores and fences,
render passes vs dynamic rendering, descriptor pools, memory allocation. Then where to go: compute
shaders, multiple pipelines, MSAA, blending and transparency, ImGui overlays, and the Scene API for
when you want none of this.

**Merge candidates** if 15 chapters reads as too many: 6+7 (vertex and index buffers), 12+13
(texture upload and sampling). That lands at 13. Splitting 14 into diffuse and specular is the
opposite move if the pacing needs it.

---

## 6. What makes it attractive

- **Result image at the top of every chapter**, and the final one animated.
- **A running line-count meter** in each chapter header: *your program: 380 lines · the raw Vulkan
  equivalent: ~1400*. Progress made visible, and the course's whole argument restated for free.
- **"Try it" boxes**, 3–5 per chapter, each with a predicted outcome the reader can check: swap the
  topology to a line list, set `polygon_mode` to wireframe, flip the winding, disable depth write,
  clamp vs repeat the sampler.
- **"Under the hood" asides** — the raw-Vulkan cost of the step just taken.
- **"When it goes wrong" box** per chapter, with real symptoms and their causes.
- **Collapsible full listing** at every chapter's end.
- **Short checkpoint** — three questions, not a paragraph-long recital.
- **Ownership tables** only where they earn their place (chapters 3, 12).

---

## 7. Code, verification, and images

The reader needs no repository. The repository still needs the code, or the docs rot.

- **`examples/c/vulkan-course/step01.c` … `step15.c`** plus `shaders/` and one `CMakeLists.txt`.
  Each `stepNN.c` is the honest state of the reader's file at the end of chapter NN — a real
  program, no `#ifdef` switches. `diff stepNN.c stepNN+1.c` is exactly the chapter's delta, which
  makes both authoring and review straightforward.
- **`just vulkan-course-check`** (rewrite of `tools/check_vulkan_tutorial.py`): every fenced C or
  GLSL block in chapter NN must appear verbatim (whitespace-normalized) in `stepNN`'s sources. This
  is a real guarantee, unlike the current token-presence check.
- **`just vulkan-course-smoke`** (rewrite of `tools/run_vulkan_tutorial.py`): compile every step
  against an installed prefix, run offscreen with validation, assert non-blank captures.
- **Images**: every chapter gets one, generated into `build/` from the step programs at docs-build
  time rather than committed, so previews cannot drift from the code and the `data` submodule is not
  involved. Chapter 1 gets a terminal card rendered with Pillow from the program's real stdout;
  flat-result chapters use the framebuffer capture validated against an exact expected RGBA;
  chapter 3 gets an animated WebP assembled from captures at fixed times; chapters 4-15 use ordinary
  captures with a non-flat check. `png_is_nonblank` stays as it is — it guards ~104 gallery images
  and "not flat" is the right check there, just the wrong contract for a chapter whose correct
  output is one color.
- **Delete** `examples/c/tutorial/` including the unused `*_spike` targets and orphan shader
  directories. Done.

Everything the API needs already exists and is proven by the current spikes: `dvz_compile_glsl`,
`dvz_graphics_*` (including `cull_mode`, `front_face`, `polygon_mode`, `depth`, `blend`),
`dvz_slots_push` / `dvz_cmd_push_constants`, `dvz_descriptors_image` / `_buffer`, canvas depth
attachments, `dvz_arcball_*` / `dvz_camera_*`, and `dvz_geometry_*`. **No new public API is
required.**

Two things to verify before chapter 1 is written:
1. `datoviz-config --cflags` exposes the Vulkan headers that `vklite` signatures need (the CMake
   target does, per `d94f72dd6`).
2. There is a usable key-press path at the canvas/input layer for the chapter 5 hot-reload key.

---

## 8. Migration checklist

1. Add `docs/advanced/vulkan/` and write the prologue plus part 1.
2. Renumber the nav: new `Advanced > Vulkan course` group; delete the `Tutorials` tab and
   `docs/tutorials/`.
3. Add `examples/c/vulkan-course/` step by step, in lockstep with each chapter.
4. Rewrite the two `tools/` scripts and the `justfile` recipes; wire the new media generation.
5. Delete `examples/c/tutorial/`, `docs/tutorials/`, `data/tutorials/vulkan/`.
6. Redirect or drop the three old `tutorials/vulkan/*` URLs (days old, RC-only, low risk).

---

## 9. Decisions (settled 2026-07-28)

1. **Matrix math**: the reader writes ~40 lines of `mat4` helpers in chapter 9 and sees the math,
   then adopts `dvz_camera_*` / `dvz_arcball_*` in chapter 11 for interaction — by then they know
   what those objects produce.
2. **Verification**: per-chapter `stepNN` programs in the repo, with CI asserting every code block
   in chapter NN appears verbatim in `stepNN`. The course text never points the reader at the repo.
3. **Granularity**: 15 chapters as mapped. No merges.
4. **`Tutorials` tab**: dropped, along with `docs/tutorials/`.
5. **Shaders**: inline strings through chapter 4, external files with hot reload from chapter 5.
6. **Texture source**: procedural checkerboard — zero assets, fully self-contained.
