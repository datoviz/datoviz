# Vulkan course — rewrite plan

**Status: implemented. All 15 chapters, the epilogue, canonical programs, shader sources, source checks, execution smokes, and generated previews have landed. The final isolated-reader audit remains a separate release gate. Supersedes the deleted RC3 Vulkan tutorial pilot.**

This note records the full rewrite of the AI-generated Vulkan tutorial that lived at `docs/tutorials/vulkan/`. The rewrite changed the section, pedagogy, code delivery model, and end goal.

Implemented in `docs/gpu-graphics/` and `examples/c/vulkan/`:

| Chapter | Page | Step program | Result |
| --- | --- | --- | --- |
| 1 | `01-setup.md` | `step01.c` (9 lines) | Datoviz links; version prints |
| 2 | `02-window.md` | `step02.c` (148 lines) | resizable window in a chosen color, or a PNG |
| 3 | `03-frame.md` | `step03.c` (173 lines) | the frame anatomy, and a pulsing window |
| 4 | `04-triangle.md` | `step04.c` (267 lines) | an RGB triangle |
| 5 | `05-shader-files.md` | `step05.c` (350 lines) | external shaders with safe live reload |
| 6 | `06-vertex-buffers.md` | `step06.c` (390 lines) | a square from vertex data |
| 7 | `07-index-buffers.md` | `step07.c` (409 lines) | the square with shared vertices |
| 8 | `08-push-constants.md` | `step08.c` (444 lines) | a rotating, pulsing square |
| 9 | `09-matrices.md` | `step09.c` (527 lines) | a projected cube without depth |
| 10 | `10-depth-culling.md` | `step10.c` (539 lines) | depth-tested, culled cube |
| 11 | `11-mouse-control.md` | `step11.c` (519 lines) | mouse-controlled cube |
| 12 | `12-texture-upload.md` | `step12.c` (624 lines) | checkerboard uploaded to a GPU image |
| 13 | `13-texture-sampling.md` | `step13.c` (680 lines) | textured cube |
| 14 | `14-lighting.md` | `step14.c` (683 lines) | lit textured cube |
| 15 | `15-mesh.md` | `step15.c` (689 lines) | generated, textured, lit sphere |

`just vulkan-course-check` verifies every code excerpt against its step program and external shaders. `just vulkan-course-smoke` builds and renders every step offscreen, requires reproducible captures with zero validation errors, and rejects an unchanged image except where a chapter intentionally changes workflow or data representation without changing pixels.

The hidden machinery is taught through per-chapter asides and the closing chapter rather than dedicated synchronization and swapchain chapters. Chapter 15 uses generated geometry and a procedural texture rather than a committed Suzanne asset, and compute is named in the epilogue rather than given a chapter.

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

Conclusion: keep almost nothing. The `vklite` API surface it proved out is sound and is exactly what the rewrite builds on; the tutorial structure around it goes.

---

## 2. Audience and promise

**Audience.** A C programmer who wants to learn modern GPU graphics. They know C and a build system. They know no Vulkan, and they do not want to spend the first week writing instance, device, swapchain, and semaphore code before seeing a pixel.

**Promise (the prologue's job to state).** *You will write, from an empty file, one C program that ends as an interactive 3D mesh viewer: perspective camera, mouse-driven rotation, depth testing, a texture you upload yourself, and per-fragment lighting. About 680 lines. The same program written against raw Vulkan is roughly 2100 lines, and most of those lines handle platform and execution machinery rather than the mesh itself.*

**The pedagogical contract** — what stays hidden, what stays explicit:

| Datoviz does it for you | You write it |
| --- | --- |
| Instance, physical-device selection, logical device, queues | Shader source, both stages |
| Window, surface, swapchain, image acquisition, presentation | Pipeline state: topology, vertex layout, depth, and culling |
| Per-frame semaphores/fences, frame pacing, resize recovery | Vertex and index data, and their GPU buffers |
| Command-pool and command-buffer allocation | Command recording: pass begin/end, binds, draws |
| Depth image allocation, offscreen capture, PNG/video sinks | Textures: staging, layout transitions, samplers, descriptors |
| The memory allocator | Matrices, camera, light math |

The hidden column is *named and explained*: each chapter's "Under the hood" aside says what raw Vulkan would demand at that point. The running raw-Vulkan line figures are approximate comparisons of the API surface rather than measurements of one reference implementation. The reader finishes knowing what a swapchain and a fence are for without having written one.

---

## 3. Placement, naming, URLs

- **Section: `GPU Graphics`.** A dedicated top-level tab keeps this general graphics course distinct from the Datoviz-focused advanced documentation.
- **Title: `Modern GPU Graphics in Vulkan`** — fixes the stale `in C`.
- **Files: `docs/gpu-graphics/`**, numbered for a linear read: `index.md`, `01-setup.md`, `02-window.md`, … `15-mesh.md`, `16-next.md`.
- **Consequence to confirm:** moving this out empties the `Tutorials` tab — only a 6-line `tutorials/first-scene.md` stub remains, and it is not even in the nav. Recommendation: drop the `Tutorials` tab, delete the stub, and let `Get Started` / `How-To` / `Examples` carry that role.

---

## 4. Ground rules for the rewrite

1. **The reader types everything.** Chapter 1 starts with an empty `main.c` in *their* directory and a build file they write. Course commands never use repository-relative example paths. Before a compatible package is published, contributors may install Datoviz from source; public release of the completed course requires a compatible installation artifact.
2. **One file that grows.** `main.c` is appended to and edited chapter by chapter. Every chapter ends with a collapsible **full current listing** so a stuck reader can resync in one copy-paste. From chapter 5 on there are also `shader.vert` / `shader.frag`.
3. **Install for the published course.** A compatible `pip install datoviz` gives headers, `datoviz-config`, and a CMake package. This becomes the default public path when the next package is published. The source install remains an explicit contributor and prerelease fallback.
4. **Every chapter ends in a verified result.** Each chapter opens with the image the reader will have at the end. Shader-file reload, index reuse, and texture upload deliberately preserve the preceding pixels because their payoff is workflow, data representation, or a GPU resource prepared for the next step; every other graphics chapter has a visibly distinct result.
5. **One new concept per step, and it must pay off immediately.** No concept is introduced to be used three chapters later.
6. **Failure is taught, not avoided.** Each chapter has a "when it goes wrong" box: black window, nothing drawn, inside-out cube, upside-down texture — with the diagnosis.
7. **Validation layers on from chapter 2**, framed as the reader's safety net, not as CI plumbing.
8. **No release-validation vocabulary.** No frame contracts, no resource generations, no counter dumps.
9. **Keep the lesson's successful path visible.** Course prose shows a failure check when it teaches the current concept or prevents a confusing crash. Complete step programs remain safe to run, but routine checks should share a small, explained cleanup pattern instead of interrupting each API call. Do not use unchecked calls merely to shorten a listing.

---

## 5. Chapter map

Four parts, 15 chapters plus an epilogue. Program length is measured from each completed canonical step rather than treated as a design target.

### Prologue — `index.md`
Hero image (or short video) of the final mesh viewer. The promise, the contract table from §2, prerequisites, and how the course is structured. No code.

### Part 1 — A window and a frame

| # | Chapter | Reader adds | Concepts | Result |
| --- | --- | --- | --- | --- |
| 1 | **Setup** | `main.c` printing `dvz_version()`; a `CMakeLists.txt` and a one-line `cc` alternative via `datoviz-config` | toolchain, linking a native library, where the headers are | terminal output, and confidence the build works |
| 2 | **Your first window** | window host, GPU context, canvas, frame loop, teardown | what a GPU context/device is; render loop; event polling; what the swapchain does *for* you; clear color; offscreen mode + PNG capture as your screenshot tool | a resizable window in the color you chose |
| 3 | **How a frame works** | a draw callback; wrap the borrowed command buffer; begin/end a rendering pass; viewport and scissor; animate the clear color | CPU records, GPU executes; command buffers; render pass and attachments; load/store ops; borrowed vs owned objects | a pulsing background — proof your commands run |

### Part 2 — Triangles: shaders, pipeline, vertex data

| # | Chapter | Reader adds | Concepts | Result |
| --- | --- | --- | --- | --- |
| 4 | **Your first triangle** | inline GLSL strings, `dvz_compile_glsl`, shader modules, empty pipeline layout, graphics pipeline, `dvz_cmd_draw(0,3,0,1)` | vertex and fragment stages; `gl_VertexIndex`; clip space and Vulkan's y-down, z 0..1; primitive topology; rasterization; interpolation; a pipeline as frozen state | the RGB triangle |
| 5 | **Shaders in their own files** | move GLSL to `shader.vert`/`shader.frag`, load and compile at startup, print compiler diagnostics, press `R` to recompile and rebuild the pipeline live | GLSL → SPIR-V; runtime vs offline compilation; why a pipeline must be rebuilt when a shader changes; reading compiler errors | same triangle, but shader edits appear without recompiling C |
| 6 | **Vertex buffers** | a `Vertex` struct, a mapped GPU buffer, upload, vertex binding + attributes, bind and draw | host-visible vs device-local memory; stride and `offsetof`; attribute formats; the shader's `in` locations must match the pipeline's attributes | a triangle from *your* data, then a quad from 6 vertices |
| 7 | **Index buffers** *(short)* | 4 vertices + 6 indices, `dvz_cmd_draw_indexed` | vertex reuse; `uint16` vs `uint32`; how indices feed primitive assembly | the same quad, 4 vertices instead of 6 |

### Part 3 — Into 3D

| # | Chapter | Reader adds | Concepts | Result |
| --- | --- | --- | --- | --- |
| 8 | **Push constants** | a push-constant slot, per-frame elapsed time, use it in the shader | the three ways to get data to a shader (push constants, uniform buffers, storage buffers) and when each fits; pipeline layout; per-frame vs per-object data | the quad spins and pulses from shader-side math |
| 9 | **Matrices and perspective** | three small matrix helpers, a cube (8 vertices, 36 indices), MVP pushed each frame | homogeneous coordinates; model/view/projection; perspective divide; FOV, aspect, near/far; how Vulkan's clip space differs from OpenGL's; aspect on resize | a spinning cube that looks *wrong* — faces in the wrong order |
| 10 | **Depth and culling** | request a depth buffer from the canvas, attach it, enable depth test/write, then set cull mode and front face | why chapter 9 looked wrong; the depth buffer and depth range; z-fighting; winding order and back-face culling; wireframe mode as an experiment | a correct solid spinning cube |
| 11 | **Mouse control** | input router, arcball, camera, compose and push the matrices, handle resize | interaction as matrix state; separating camera from model; why projection depends on window size | drag to rotate, scroll to zoom |

### Part 4 — Surfaces: textures and light

| # | Chapter | Reader adds | Concepts | Result |
| --- | --- | --- | --- | --- |
| 12 | **Uploading a texture** | procedural checkerboard pixels, a staging buffer, an image, two layout transitions, the copy, a one-shot submit | images vs buffers; tiling and why a copy is needed; image layouts and barriers; sRGB vs linear | nothing visible yet — verified by validation staying silent |
| 13 | **Sampling the texture** | a sampler, a descriptor slot and set, a `texcoord` attribute, sampling in the fragment shader | descriptor sets vs push constants; filtering; address modes; UV orientation | a textured cube |
| 14 | **Lighting** | a `normal` attribute, the normal matrix, ambient + diffuse, then specular; light and eye position pushed | normals and the dot product; world vs view space; per-vertex vs per-fragment shading; gamma | a lit textured cube |
| 15 | **A real mesh** | `dvz_geometry_sphere`/`torus` (and `dvz_geometry_obj` for your own model), double→float conversion, analytic normals, and optional `dvz_geometry_compute_normals` for meshes without normals | separating mesh *data* from mesh *rendering*; index counts; why the GPU wants floats; when recomputed normals blur seams or creases | **the deliverable:** a rotatable, textured, lit mesh |

### Epilogue — `16-next.md`
What you never wrote, one paragraph each with a pointer to where Datoviz does it: instance and device creation, queue families, surface and swapchain, acquire/present, semaphores and fences, render passes vs dynamic rendering, descriptor pools, memory allocation. Then where to go: compute shaders, multiple pipelines, MSAA, blending and transparency, ImGui overlays, and the Scene API for when you want none of this.

**Merge candidates** if 15 chapters reads as too many: 6+7 (vertex and index buffers), 12+13 (texture upload and sampling). That lands at 13. Splitting 14 into diffuse and specular is the opposite move if the pacing needs it.

---

## 6. What makes it attractive

- **Result image at the top of every chapter**, and the final one animated.
- **A running line-count meter** in each chapter header. Chapter 1 gives the program length; chapters 2-15 also estimate the raw Vulkan equivalent. Progress stays visible without forcing a Vulkan comparison into the setup chapter.
- **"Try it" boxes**, 3–5 per chapter, each with a predicted outcome the reader can check: swap the topology to a line list, set `polygon_mode` to wireframe, flip the winding, disable depth write, clamp vs repeat the sampler.
- **"Under the hood" asides** — the raw-Vulkan cost of the step just taken.
- **"When it goes wrong" box** per chapter, with real symptoms and their causes.
- **Collapsible full listing** at every chapter's end.
- **Short checkpoint** — three or four questions, not a paragraph-long recital.
- **Ownership tables** only where they earn their place (chapters 3, 12).

---

## 7. Code, verification, and images

The released course reader needs no repository once a compatible package is available. The repository still needs the canonical code, or the docs rot.

- **`examples/c/vulkan/step01.c` … `step15.c`** plus reader-local `stepNN/shader.vert` and `stepNN/shader.frag` files from chapter 5 onward, all registered in the examples CMake file. Each `stepNN.c` is the honest state of the reader's file at the end of chapter NN — a real program, no `#ifdef` switches. `diff stepNN.c stepNN+1.c` is exactly the chapter's delta, which makes both authoring and review straightforward.
- **`just vulkan-course-check`** (rewrite of `tools/check_vulkan_tutorial.py`): every fenced C or GLSL block in chapter NN must appear verbatim (whitespace-normalized) in `stepNN`'s sources. This is a real guarantee, unlike the current token-presence check.
- **`just vulkan-course-smoke`** (rewrite of `tools/run_vulkan_tutorial.py`): build and run every in-tree step offscreen with validation, require reproducible captures, and enforce the expected visual relationship between adjacent chapters. Separate installed-prefix and wheel recipes compile copied course sources as standalone consumers.
- **Images**: every chapter gets one, generated into `build/` from the step programs at docs-build time rather than committed, so previews cannot drift from the code and the `data` submodule is not involved. Chapter 1 gets a terminal card rendered with Pillow from the program's real stdout; flat-result chapters use the framebuffer capture validated against an exact expected RGBA; chapter 3 gets an animated WebP assembled from captures at fixed times; chapters 4-15 use ordinary captures with a non-flat check. `png_is_nonblank` stays as it is — it guards ~104 gallery images and "not flat" is the right check there, just the wrong contract for a chapter whose correct output is one color.
- **Delete** `examples/c/tutorial/` including the unused `*_spike` targets and orphan shader directories. Done.

Everything the API needs already exists and is proven by the current spikes: `dvz_compile_glsl`, `dvz_graphics_*` (including `cull_mode`, `front_face`, `polygon_mode`, `depth`, `blend`), `dvz_slots_push` / `dvz_cmd_push_constants`, `dvz_descriptors_image` / `_buffer`, canvas depth attachments, `dvz_arcball_*` / `dvz_camera_*`, and `dvz_geometry_*`. **No new public API is required.**

RC3 verification confirmed the Canvas input route for chapter 5 through focused input tests, synthetic hosted-view injection, and a physical X11 `r` injection with distinct key-press, committed-text, and key-release delivery. The reload callback only sets a request flag; before the next frame, the main loop builds a separate candidate pipeline, keeps the current pipeline on failure, and on success waits for the device before swapping and destroying the old pipeline. No watcher or new public API is required. The installed CMake target already exposes the Vulkan headers required by `vklite` signatures.

---

## 8. Migration status

1. Done: add `docs/gpu-graphics/`, the prologue, chapters 1-15, and the epilogue.
2. Done: add the top-level `GPU Graphics` navigation group and remove the pilot `Tutorials` section.
3. Done: add canonical programs and reader-local shader files under `examples/c/vulkan/` in lockstep with every chapter.
4. Done: source synchronization, execution smokes, stale-executable protection, installed-consumer validation, and generated preview tooling are wired into the docs build.
5. Done: delete `examples/c/tutorial/`, `docs/tutorials/`, and `data/tutorials/vulkan/`.
6. Done: the old `tutorials/vulkan/*` pages and navigation are absent from the v0.4 site.

---

## 9. Decisions (settled 2026-07-28)

1. **Matrix math**: the reader writes ~40 lines of `mat4` helpers in chapter 9 and sees the math, then adopts `dvz_camera_*` / `dvz_arcball_*` in chapter 11 for interaction — by then they know what those objects produce.
2. **Verification**: per-chapter `stepNN` programs in the repo, with CI asserting every code block in chapter NN appears verbatim in `stepNN`. The course text never points the reader at the repo.
3. **Granularity**: 15 chapters as mapped. No merges.
4. **`Tutorials` tab**: dropped, along with `docs/tutorials/`.
5. **Shaders**: inline strings through chapter 4, external files with hot reload from chapter 5.
6. **Texture source**: procedural checkerboard — zero assets, fully self-contained.

---

## 10. Pre-generation editorial gate (settled 2026-09-14)

Before writing chapters 4-15:

1. Keep the foundational object-model, coordinate, interaction, query, and project-boundary explanations under **Get Started → Concepts**. Reserve **Advanced** for scene planning, retained-resource machinery, GPU ownership, runtime layers, contributors, and release work.
2. In teaching excerpts, keep checks that explain a failure mode or protect a boundary the chapter introduces. Consolidate routine constructor and teardown failures in complete programs so error handling does not obscure the graphics sequence.
3. Distinguish CPU input memory from GPU resources precisely. Ordinary set-data calls copy CPU data before returning; Vulkan resources referenced by submitted commands must remain valid until GPU execution completes.
4. Review each new chapter with the same voice and structure as chapters 1-3, then run a separate cross-course prose and consistency audit before declaring the course complete.

## 11. Final isolated-reader audit

After all chapters pass normal repository validation, test the published course from beginning to end in a fresh isolated environment. Give an independent agent only the course, the supported installation artifacts, and the platform prerequisites stated by the course. Do not provide repository history, internal plans, unpublished source examples, or prior Datoviz guidance.

The agent must follow the instructions literally, create the project from an empty directory, build and run every chapter result, and record every command, failure, ambiguity, undocumented assumption, workaround, and elapsed time. It must not silently repair course code. Its deliverables are an execution transcript, a chapter-by-chapter audit, and a prioritized improvement plan. Any failed or ambiguous step must be corrected and rerun before the course is considered complete.
