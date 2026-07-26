# Modern GPU Graphics in C

Status: required final-v0.4 tutorial; RC3 owns the enabling API and three-chapter pilot, and RC4 owns the complete course and installed-artifact proof.


## Purpose

Create a beginner-oriented computer graphics tutorial that teaches the modern GPU and Vulkan execution model through Datoviz's low-level `vklite` and Canvas layers.

The tutorial should give readers a live GLFW result quickly, then progressively expose shaders, graphics pipelines, GPU resources, command buffers, queue submission, synchronization, and presentation without beginning with the full structural verbosity of raw Vulkan.

The intended result is not a comprehensive Vulkan course, a general C course, or an introduction to the high-level Datoviz scene API. It is a practical path into modern GPU graphics for readers who find raw Vulkan onboarding overwhelming and do not want to begin with the older OpenGL state-machine model.

The public title should lead with the user benefit rather than the unfamiliar `vklite` name. The preferred presentation is "Modern GPU Graphics in C" with a subtitle such as "Learn Vulkan concepts with Datoviz and vklite."


## Release Contract

The tutorial is required for the final v0.4 documentation surface. RC3 must land and validate the tutorial-facing API improvements plus the first three polished chapters. RC4 must complete the course through a textured, lit, mouse-rotatable Suzanne mesh, validate the exact installed packages, and freeze the tutorial-facing API and assets. Final v0.4.0 should contain only feedback-driven fixes, regenerated final media, and publication work.

This promotion is intentional release scope, not optional visual polish. Agents must not restore the earlier "future proposal" or "not an RC3 requirement" status without an explicit maintainer decision that also updates the active and durable release plans.


## Audience

The primary reader is comfortable with basic programming and a terminal, possibly in a language other than C, but may have little or no experience with computer graphics, Vulkan, C memory management, pointers, callbacks, or common C resource-lifetime idioms.

The tutorial should not require prior Vulkan or OpenGL knowledge. It should introduce C constructs only when the reader needs them to understand or change visible behavior.

The primary success criterion is that a programmer new to graphics can understand the execution of a modern GPU frame. Datoviz contributor and low-level-user onboarding is a secondary benefit. The course is not aimed primarily at existing Vulkan developers or complete programming beginners.


## Teaching Contract

The main path should follow this loop:

```text
visible result
    -> small experiment
    -> observed change
    -> one new GPU or Vulkan concept
    -> only the necessary C explanation
    -> another visible experiment
```

The first chapter should reach a live, resizable GLFW triangle as quickly as practical. It may begin from supplied working infrastructure rather than asking the reader to construct every Vulkan object before the first image appears.

Each later chapter should open one more part of the system. The tutorial should preserve Vulkan terminology and relationships even when vklite removes repetitive configuration, allocation, platform, and lifecycle code.

The landing page should show the final textured, lit, rotatable Suzanne result so readers understand the destination before beginning with the triangle.


## Delivery Contract

The intended public path is a small standalone CMake consumer using an installed Datoviz package:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(gpu_tutorial PRIVATE datoviz::datoviz)
```

Canonical sources, shaders, assets, captures, and validation remain in the Datoviz repository. Source-tree convenience commands may exist, but understanding or rebuilding the entire Datoviz repository must not be a reader prerequisite.

Each lesson should use external shader files compiled at application startup. Editing a shader and restarting the example must not require recompiling C. Live shader hot-reload is optional after the core course and must not complicate the pilot.

The same teaching code should support live GLFW execution and deterministic offscreen validation. Prefer one program with a presentation selection or two thin entry points over duplicated pipeline, resource, and draw implementations.


## Abstraction Boundary

Vklite is the main pedagogical API. It should compress Vulkan representation and setup without replacing the Vulkan mental model.

The tutorial should use:

- Datoviz window and Canvas support for GLFW hosting, surfaces, frame acquisition, swapchain recreation, submission, presentation, and other platform-sensitive frame plumbing during the early lessons.
- Vklite for shader modules, graphics and compute pipelines, buffers, images, samplers, descriptors, render scopes, commands, and synchronization where its API gives a direct conceptual mapping to Vulkan.
- Datoviz CPU-side geometry and controller helpers when they avoid unrelated asset-parsing or arcball-mathematics detours; GPU buffers, images, descriptors, pipelines, attachments, and commands remain explicit.
- Native Vulkan types and handles when they clarify the real object being used, including `VkCommandBuffer`, `VkImageView`, formats, stage masks, access masks, layouts, and queue-related concepts.
- Selected raw `vkCmd*` calls when they are clearer than a wrapper or when comparing the vklite operation with its Vulkan equivalent is itself useful.

The main narrative should not repeatedly expand every vklite operation into full raw Vulkan code. Short "Vulkan underneath" notes may name the corresponding native objects and calls. A complete raw Vulkan triangle may appear as an optional final comparison after the reader understands the purpose of its parts.

Every important wrapper should make ownership explicit. In particular, tutorial code must distinguish owned vklite resources from borrowed device, frame, attachment, and command-buffer handles.

`DvzStreamFrame.resource_generation` identifies a concrete borrowed frame-resource set or slot, not a global recreation epoch. Equality confirms stable matching handles; inequality may indicate ordinary frame-slot rotation or resource recreation. Consumers must use `handles_dirty`, extent, format, and generation together when refreshing dependent state rather than treating every generation difference as resize or recreation.

API improvements motivated by the tutorial must be generally useful Canvas, vklite, file-I/O, geometry, or controller improvements. Do not introduce a tutorial-only renderer, frame stream, application runtime, Vulkan wrapper, or opaque ownership layer.


## Runtime Shader Contract

Tutorial shaders are external GLSL files read as null-terminated text and compiled with shaderc through a public Datoviz API. The resulting heap-owned SPIR-V is used to create a vklite shader module and released with `dvz_memory_free()`.

The public path must distinguish the compiler roles defined in [../architecture/SHADER_TOOLCHAIN.md](../architecture/SHADER_TOOLCHAIN.md):

- `glslc` is the single build-time command-line compiler for Datoviz-owned native scene, Canvas, test, and example shaders.
- the shaderc library is the runtime compiler used by tutorial applications.
- `spirv-val` validates generated SPIR-V in CI and release lanes.
- `glslangValidator` is optional cross-check or specialized-workflow tooling, not a normal native build requirement.

Datoviz does not launch build-time command-line tools from the runtime API. The tutorial-facing API must use a typed stage and target profile, preserve the source filename in compiler diagnostics, expose a clear availability or preflight result, distinguish provider and compilation failures, obey the public allocator contract, and fail with actionable diagnostics when runtime shaderc is unavailable. Exact signatures remain spike-driven.


## Progressive C Guidance

The tutorial should not front-load a C primer. It should explain language and memory concepts at their first meaningful use and connect them directly to graphics behavior.

Examples include:

| Graphics step | C concept introduced in context |
| --- | --- |
| Changing the first triangle | scalar values, arrays, functions, and `const` |
| Defining vertex data | structs, aggregate initialization, `sizeof`, and `offsetof` |
| Passing vklite objects | pointers, opaque handles, `NULL`, and borrowed versus owned values |
| Drawing a frame | callbacks, function pointers, `void*` user data, and casts |
| Uploading resources | byte sizes, CPU memory addresses, read-only input, and lifetime |
| Creating resources | return codes and output parameters |
| Releasing resources | creation/destruction symmetry and reverse-order cleanup |
| Configuring Vulkan state | enums, flags, bitmasks, and the `|` operator |
| Growing the application | headers, source files, compilation, and modular state |

The recurring questions should be:

1. Is this data in CPU memory or GPU memory?
2. Who owns the resource?
3. How long is it valid?

Tutorial code should favor named intermediate variables, straightforward control flow, consistent error checks, explicit cleanup, and strong compiler warnings. It should avoid unexplained macros, compressed expressions, and advanced C idioms that do not serve the current graphics concept.


## Proposed Learning Path

The sequence should remain adjustable after testing the first chapters, but the conceptual progression is:

### 1. First Live Triangle

Build and run a supplied program, see a live resizable GLFW triangle, change its color and shape, and learn the high-level anatomy of one frame. Positions and colors begin in the vertex shader through `gl_VertexIndex`; Datoviz initially owns device, surface, swapchain, resize, and submission plumbing.

### 2. Shaders and the Graphics Pipeline

Edit external vertex and fragment shaders, observe interpolation, and relate shader stages, inputs and outputs, clip space, GLSL, SPIR-V, primitive topology, rasterization, viewport, scissor, blending, attachment formats, and pipeline layout to a vklite graphics pipeline. End with a visually rewarding shader playground rather than only the unchanged RGB triangle.

### 3. Vertex Data and GPU Buffers

Move positions and colors from shader-generated values into a C vertex array and GPU buffer. Introduce structs, vertex bindings, attributes, formats, strides, offsets, uploads, and resource lifetime.

These three chapters are the RC3 pilot.

### 4. Per-Frame State and Transforms

Animate the 2D geometry and pass changing application state to shaders. Introduce model transforms, push constants, uniform buffers, pipeline layouts, descriptor layouts and sets, and per-frame resource lifetime without attempting the full 3D scene at once.

### 5. Indexed 3D Geometry and Depth

Build an indexed 3D mesh and introduce index buffers, indexed drawing, model-view-projection transforms, perspective, depth attachments, depth testing, culling, coordinate conventions, and resize-dependent attachment recreation.

### 6. Images, Textures, and Samplers

Load the supplied Suzanne OBJ and Datoviz-owned UV diagnostic texture while keeping image storage, image views, samplers, descriptors, layouts, upload commands, transfer synchronization, UV coordinates, filtering, and sRGB-to-linear sampling conceptually distinct.

### 7. Mouse-Driven Arcball

Connect the public CPU-side `DvzArcball` to the Canvas input router, replace automatic rotation with direct mouse interaction, and update the model-view-projection state without turning the lesson into an arcball-mathematics course.

### 8. Normals and Basic Lighting

Finish the textured, lit, mouse-rotatable Suzanne. Visualize interpolated normals, introduce normalization, the dot product, object and world space, the inverse-transpose normal matrix, Lambert diffuse lighting, ambient fill, and linear-space lighting of an sRGB albedo texture. A short optional Blinn-Phong specular extension is appropriate; a general BRDF or physically based rendering course is not.

### 9. Queue Submission and Synchronization

Reveal more of the frame lifecycle after the reader knows the work being synchronized: acquire, wait, record, submit, signal, and present. Introduce fences, binary and timeline semaphores, stages, access masks, barriers, hazards, and frames in flight.

### 10. Swapchains and Presentation

Inspect the presentation machinery that was hidden at the beginning: surfaces, formats, present modes, image acquisition, out-of-date handling, minimized windows, resize, and swapchain-dependent resource recreation.

### 11. Compute Commands

Use a particle update or image-processing exercise to introduce compute pipelines, workgroups, dispatch, storage buffers or images, and synchronization between compute and graphics work.

### 12. Datoviz Runtime Layers

Conclude by relating the learned concepts to the active Datoviz runtime path:

```text
scene frame plans -> DRP2 command streams -> vklite runtime ->
Canvas/stream frame execution -> optional app presentation
```

This is an architectural epilogue, not a prerequisite for the low-level course.

### Optional WebGPU Comparison

After the native concepts are established, selected notes may compare Vulkan pipelines, descriptors, command recording, resources, and synchronization with the corresponding WebGPU model. WebGPU should not expand the initial tutorial into a second full implementation track.


## Chapter Format

Each chapter should be ordinary Markdown accompanied by complete, runnable code. A consistent chapter should contain:

1. A screenshot or short visual preview of the intended result.
2. One principal GPU or Vulkan concept.
3. A working starting point based on the preceding lesson.
4. One or more small code changes with focused explanations.
5. A diagram or command trace only when it materially clarifies the execution model.
6. Brief C reminders exactly where unfamiliar constructs appear.
7. A run-and-observe checkpoint.
8. Small experiments that produce visible changes.
9. A focused exercise.
10. A complete reference program and validation command.

The prose should emphasize causality: what the CPU records, what the GPU later executes, which resource a command uses, and why ordering or lifetime matters.

Code snippets in prose should come from or be validated against complete examples wherever practical. The complete examples, rather than copied snippets, should be the executable source of truth.


## Initial Repository Placement

The first implementation should remain in the Datoviz repository:

```text
docs/tutorials/vulkan/
    index.md
    first-triangle.md
    shaders-and-pipeline.md
    vertex-buffers.md

examples/c/tutorial/
    first_triangle.c
    shaders_and_pipeline.c
    vertex_buffers.c
    transforms.c
    indexed_depth.c
    texture.c
    arcball.c
    lighting.c
    shaders/
    assets/
        suzanne.obj
        suzanne_albedo.png
        ASSETS.md
```

The exact executable and shader naming should be finalized during the RC3 pilot. The tutorial should not create a parallel renderer, presentation runtime, frame stream, or Vulkan wrapper.


## Suzanne and Texture Assets

Distribute a small triangulated, UV-unwrapped, smooth-normal Suzanne as a human-inspectable ASCII OBJ in the main repository, not the `data` submodule. Record the Blender version, export and triangulation recipe, source history, license, coordinate convention, vertex and face counts, and any transformations in `ASSETS.md`. Do not require Blender at build or runtime.

The OBJ path requires the public geometry loader to preserve `vt` coordinates and correctly resolve independent OBJ position, normal, and texture-coordinate indices. The loader must retain its bounds, overflow, malformed-input, negative-index, polygon-triangulation, and cleanup guarantees.

Use a small deterministic Datoviz-owned PNG with an asymmetrical UV grid, labeled or otherwise recognizable regions, and the Datoviz palette. Commit the source or generation script, provenance, license, and deterministic generation or checksum record with the PNG. The texture should expose UV orientation, seams, mirroring, filtering, and mip behavior while remaining attractive on the final mesh. Readers should be able to replace it with their own image without changing the rendering architecture.

All shipped tutorial assets require the normal v0.4 asset-license and provenance review. Adding the generated PNG or other binary asset requires the repository-mandated approval for the exact file in the implementation turn.


## Pilot

The initial pilot should contain only three polished chapters:

1. First live triangle.
2. Shaders and the graphics pipeline.
3. Vertex data and GPU buffers.

The pilot should test:

- whether the first visible result arrives quickly enough;
- whether the assumed programming knowledge is appropriate;
- whether C explanations are timely without interrupting the graphics narrative;
- whether vklite hides the correct amount of Vulkan machinery;
- whether examples remain short, readable, runnable, and honest about ownership;
- whether screenshots and exercises add useful feedback;
- whether the API subset is stable enough to support durable teaching material.

Broader course work belongs to RC4 and should begin only after the RC3 pilot establishes a successful voice, abstraction boundary, public API profile, and validation workflow.


## Tutorial-Enabling API Requirements

RC3 must use executable chapter spikes to design, implement, and validate generally reusable improvements for these outcomes:

1. Configure a GPU context for a selected Canvas window backend without making readers manually reproduce backend instance-extension discovery and the standard Canvas Vulkan feature set.
2. Read null-terminated shader text or compile a shader file while preserving its real source name, returning actionable compiler availability and failure diagnostics, and honoring Datoviz allocation ownership.
3. Record commands into the Canvas frame's borrowed command buffer without repeated accidental wrapper-allocation ceremony, while preserving the prohibition on destroying, resetting, submitting, transitioning, or retaining borrowed handles.
4. Set frame-sized dynamic viewport and scissor state through a coherent vklite or Canvas contract.
5. Resolve and inspect the Canvas render-target color format before pipeline creation, distinguish the requested configuration from the actual frame format, and detect later format changes without assuming a particular RGBA/BGRA ordering.
6. Request and inspect an optional Canvas-owned depth attachment with explicit format, borrowed frame handles, resize recreation, resource-generation reporting, and ownership.
7. Load OBJ texture coordinates and independent OBJ indices safely into `DvzGeometry`.
8. Upload sampled images and transition them for shader use without hiding layouts, barriers, ownership, or synchronization; add convenience only where the chapter spike demonstrates recurring accidental complexity.
9. Connect the public low-level arcball to the Canvas input router and obtain the model-view-projection state without depending on the retained scene layer.

Exact APIs must not be invented in this specification. Each public change requires a narrow chapter spike, ownership review, focused tests, generated binding refresh where applicable, installed-consumer proof, and documentation before it is accepted. Prefer improving an existing subsystem boundary over adding tutorial-local helpers.

The accepted first-result profile consists of `dvz_canvas_configure_gpu_ctx()`, `dvz_canvas_frame_format()`, `dvz_commands_wrap_borrowed_recording()` followed by `dvz_commands_unwrap()`, `dvz_cmd_set_viewport()`, `dvz_cmd_set_scissor()`, and `dvz_cmd_set_viewport_scissor()`. GPU configuration augments caller policy instead of replacing it. A present Canvas without an explicit color format reports `VK_FORMAT_UNDEFINED` until swapchain creation resolves the actual format. Unwrapping detaches only borrowed-recording wrappers and never ends, resets, submits, frees, or otherwise touches the Vulkan command buffer. The combined dynamic-state helper means full-frame zero-origin viewport and scissor state; the separate helpers remain available for non-default rectangles.

The accepted depth profile uses `DvzCanvasConfig.depth_format`: `VK_FORMAT_UNDEFINED` disables depth, while a depth or depth-stencil format requests one Canvas-owned attachment per frame resource set. `DvzStreamFrame.depth_image`, `depth_view`, `depth_format`, `depth_layout`, `depth_image_borrowed`, `depth_view_borrowed`, and `depth_valid` describe the callback-duration attachment. Canvas creates, transitions, recreates, and destroys it with the matching color resource; `resource_generation`, `handles_dirty`, format, and extent describe the coupled resource set. The callback may attach the reported view in the reported layout but must not destroy, transition, or retain the borrowed handles.

The accepted image-upload profile uses the existing vklite resource and command boundaries rather than an opaque upload helper. Initialization creates an owned host-visible staging buffer, sampled image, image view, sampler, descriptor slots, and descriptors; records the explicit `UNDEFINED` to `TRANSFER_DST_OPTIMAL` barrier, buffer-to-image copy, and `TRANSFER_DST_OPTIMAL` to `SHADER_READ_ONLY_OPTIMAL` barrier; then performs one blocking owned-command submission before releasing staging storage. The sampled image, view, sampler, and descriptors remain owned by the renderer and are destroyed in dependency-safe order after submitted drawing completes. Convenience may reduce wrapper allocation in future only if it keeps layouts, stages, access scopes, ownership, and synchronization visible at the call site.

The accepted direct-controller profile obtains the Canvas-owned borrowed router through `dvz_canvas_input()`, connects an owned standalone `DvzArcball`, combines its model and view contribution with an owned standalone `DvzCamera`, refreshes camera projection from each framebuffer extent, and disconnects before destroying the controller or Canvas window. `dvz_cmd_push_constants()` writes the resulting combined transform through a range declared by `dvz_slots_push()` while validating non-empty four-byte-aligned bounds and shader-stage visibility; this avoids raw Vulkan commands and shared per-frame uniform mutation in the enabling spike. The final chapter may use ordinary frame-safe uniform buffers when teaching persistent per-frame state.


## Validation Direction

Every complete tutorial example should build in automated validation. Where practical, deterministic offscreen variants should produce reference captures, while the reader-facing path remains live GLFW rendering.

Documentation validation should detect stale or missing example links and should prefer code inclusion or synchronization mechanisms that prevent prose snippets from silently diverging from compiled examples.

The tutorial must label vklite honestly as advanced/unstable unless its release status changes. Published tutorial versions should pin or clearly declare a compatible Datoviz version rather than implicitly tracking an unstable development tip.

The RC3 and RC4 gates additionally require:

- standalone `find_package(datoviz CONFIG REQUIRED)` builds against installed packages;
- runtime compilation of the external tutorial shaders through the packaged shaderc path;
- deterministic offscreen captures with basic nonblank or reference-image validation;
- bounded live GLFW smoke for resize, frame recreation, depth, input, and shutdown;
- Vulkan validation coverage for attachment layouts, barriers, descriptors, resource lifetime, and resize;
- focused malformed-OBJ and UV/normal/index tests;
- exact-artifact Linux, macOS, and Windows consumer proof consistent with the release validation policy;
- asset-license, provenance, install-location, and runtime-discovery checks;
- `just ctypes` and `just ctypes-check` after public headers, exported API, binding policy, or generator changes;
- strict documentation builds, link checks, example synchronization checks, and `git diff --check`.


## Versioning

Do not promise general vklite stability for v0.4. Define a small tutorial profile consisting of the API and behavior exercised by the compiled chapters, test it continuously, and declare compatibility with an exact Datoviz release series. Changes to the profile require deliberate tutorial updates and release notes.


## Possible Standalone Repository

A standalone repository may become useful after final v0.4 if the tutorial needs an independent release cadence or a deliberately linear learning history.

One possible later format is a sequence of small, runnable commits or stable lesson tags in which each change represents the result the reader is expected to reproduce. A lightweight progression tool could run, check, compare, and advance between lessons without requiring strong Git knowledge or discarding learner edits.

This is a future delivery option, not an initial requirement. Ordinary Markdown, complete code, screenshots, instructions, and exercises should establish the course before custom tooling or repository extraction is considered.


## Non-Goals

- Comprehensive coverage of the Vulkan API.
- Exhaustive treatment of GPU hardware architecture.
- A general-purpose C programming course.
- Raw Vulkan bootstrap before the first rendered result.
- Replacement of Vulkan terminology with Datoviz-specific concepts.
- A high-level scene, plotting, or scientific visualization tutorial.
- Complete native/WebGPU parity.
- A new runtime or wrapper layer built only for the tutorial.
- Custom course tooling before the Markdown pilot is proven.


## Resolved Decisions

1. Optimize for programmers who know basic programming but are new to graphics; do not target complete programming beginners or existing Vulkan experts as the primary reader.
2. Lead public positioning with modern GPU graphics and Vulkan concepts, using Datoviz and vklite as the mechanism.
3. Treat the installed standalone CMake consumer as the public path and the Datoviz repository as the canonical source and validation home.
4. Start with shader-generated positions and colors, expose the borrowed command buffer and draw sequence early, and defer detailed acquisition, submission, and presentation machinery.
5. Compile external GLSL files at runtime with shaderc; do not require a C rebuild for shader edits or require hot reload in the pilot.
6. Share rendering code between GLFW and deterministic offscreen execution.
7. Use Datoviz CPU-side geometry and arcball helpers while keeping GPU resources and commands explicit.
8. Use a supplied Suzanne OBJ and a deterministic Datoviz-owned texture as the final course asset.
9. Teach basic lighting mathematics and linear-space texture lighting, but not general BRDF or PBR theory.
10. Provide a tested, release-pinned tutorial profile rather than a general vklite stability promise.
11. Land tutorial-enabling API and the three-chapter pilot in RC3, complete and freeze the course in RC4, and reserve final v0.4.0 for feedback fixes and publication.


## Spike-Dependent Decisions

1. What exact API signatures satisfy the tutorial-enabling outcomes without introducing a tutorial-only abstraction?
2. Which live GLFW program shape gives the shortest copy-safe result while preserving explicit ownership and installed-package use?
3. Which current image-upload and barrier operations need general convenience versus clearer documentation?
4. Which vklite operations are clearer as selected native `vkCmd*` calls in the main text?
5. What exact Suzanne export, unwrap, texture-generation recipe, and asset license pass the final provenance review?
