# Vklite Vulkan Concepts Tutorial

Status: future documentation proposal; not an RC3 requirement.


## Purpose

Create a beginner-oriented computer graphics tutorial that teaches the modern GPU and Vulkan execution model through Datoviz's low-level `vklite` and Canvas layers.

The tutorial should give readers a live GLFW result quickly, then progressively expose shaders, graphics pipelines, GPU resources, command buffers, queue submission, synchronization, and presentation without beginning with the full structural verbosity of raw Vulkan.

The intended result is not a comprehensive Vulkan course, a general C course, or an introduction to the high-level Datoviz scene API. It is a practical path into modern GPU graphics for readers who find raw Vulkan onboarding overwhelming and do not want to begin with the older OpenGL state-machine model.


## Audience

The primary reader understands basic programming but may have little or no experience with computer graphics, Vulkan, C memory management, pointers, callbacks, or common C resource-lifetime idioms.

The tutorial should not require prior Vulkan or OpenGL knowledge. It should introduce C constructs only when the reader needs them to understand or change visible behavior.


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


## Abstraction Boundary

Vklite is the main pedagogical API. It should compress Vulkan representation and setup without replacing the Vulkan mental model.

The tutorial should use:

- Datoviz window and Canvas support for GLFW hosting, surfaces, frame acquisition, swapchain recreation, submission, presentation, and other platform-sensitive frame plumbing during the early lessons.
- Vklite for shader modules, graphics and compute pipelines, buffers, images, samplers, descriptors, render scopes, commands, and synchronization where its API gives a direct conceptual mapping to Vulkan.
- Native Vulkan types and handles when they clarify the real object being used, including `VkCommandBuffer`, `VkImageView`, formats, stage masks, access masks, layouts, and queue-related concepts.
- Selected raw `vkCmd*` calls when they are clearer than a wrapper or when comparing the vklite operation with its Vulkan equivalent is itself useful.

The main narrative should not repeatedly expand every vklite operation into full raw Vulkan code. Short "Vulkan underneath" notes may name the corresponding native objects and calls. A complete raw Vulkan triangle may appear as an optional final comparison after the reader understands the purpose of its parts.

Every important wrapper should make ownership explicit. In particular, tutorial code must distinguish owned vklite resources from borrowed device, frame, attachment, and command-buffer handles.


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

Build and run a supplied program, see a live GLFW triangle, change its color and shape, and learn the high-level anatomy of one frame. Datoviz initially owns device, surface, swapchain, resize, and submission plumbing.

### 2. Shaders and the Programmable Pipeline

Edit vertex and fragment shaders, observe interpolation, introduce shader stages, inputs and outputs, clip space, GLSL, SPIR-V, and shader modules.

### 3. The Graphics Pipeline

Relate shaders, primitive topology, rasterization, viewport, scissor, blending, attachment formats, and pipeline layout to a vklite graphics pipeline. Use small state changes with visible consequences.

### 4. Vertex Buffers and GPU Memory

Move positions and colors from shader-generated values into a C vertex array and GPU buffer. Introduce vertex bindings, attributes, formats, strides, offsets, uploads, and resource lifetime.

### 5. Command Buffers

Focus on recording rather than immediate execution, command ordering, render scopes, pipeline and buffer binding, multiple draw calls, reset and reuse, and the ownership of the frame-provided command buffer.

### 6. Uniforms, Push Constants, and Descriptors

Animate transforms and pass application state to shaders. Introduce pipeline layouts, descriptor layouts and sets, uniform buffers, per-frame resources, and push constants as related data-binding mechanisms.

### 7. Images, Textures, and Samplers

Render a textured quad while keeping image storage, image views, samplers, descriptors, layouts, upload commands, and transfer synchronization conceptually distinct.

### 8. Indexed Geometry, Transforms, and Depth

Build a rotating textured mesh. Introduce index buffers, indexed drawing, model-view-projection transforms, depth attachments, depth testing, culling, and coordinate conventions.

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
```

The exact example directory must be reconciled with the public example taxonomy before implementation. The tutorial should not create a parallel renderer, presentation runtime, frame stream, or Vulkan wrapper.


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

Broader course work should wait until the pilot establishes a successful voice, abstraction boundary, and validation workflow.


## Validation Direction

Every complete tutorial example should build in automated validation. Where practical, deterministic offscreen variants should produce reference captures, while the reader-facing path remains live GLFW rendering.

Documentation validation should detect stale or missing example links and should prefer code inclusion or synchronization mechanisms that prevent prose snippets from silently diverging from compiled examples.

The tutorial must label vklite honestly as advanced/unstable unless its release status changes. Published tutorial versions should pin or clearly declare a compatible Datoviz version rather than implicitly tracking an unstable development tip.


## Possible Standalone Repository

A standalone repository may become useful after the pilot if the tutorial needs an independent release cadence or a deliberately linear learning history.

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


## Open Decisions

1. Which exact Canvas and vklite API subset should be treated as the tutorial foundation?
2. Should the first triangle use shader-generated positions or begin immediately with a vertex buffer?
3. How much of the frame command buffer should the first chapter expose before the detailed command-buffer lesson?
4. Should early shader compilation happen at runtime for immediacy or through the normal build pipeline for reproducibility?
5. Which live GLFW example pattern gives the shortest copy-safe program while preserving explicit ownership?
6. How should live examples pair with deterministic offscreen captures without duplicating the teaching code?
7. Which vklite operations are clearer as native `vkCmd*` calls in the main text?
8. What compatibility and versioning promise, if any, should the educational vklite subset receive?
