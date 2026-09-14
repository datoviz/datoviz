# Modern GPU Graphics In Vulkan Tutorial Contract

Status: required final-v0.4 tutorial. The executable 15-chapter baseline is implemented and independently proven; the approved conceptual-depth revision expands it to 16 substantive chapters plus an epilogue and requires a fresh isolated-reader proof. Updated: 2026-09-14.

Use [../../docs/architecture/vulkan_course_plan.md](../../docs/architecture/vulkan_course_plan.md) for the working chapter outline and [../../agents/now/VKLITE_GRAPHICS_TUTORIAL.md](../../agents/now/VKLITE_GRAPHICS_TUTORIAL.md) for current execution status. This document owns the durable educational, abstraction, delivery, versioning, and validation contract.

## Purpose

Deliver a beginner-oriented course for C programmers who are new to modern GPU graphics. A reader starts from an empty program and builds one Vulkan-backed application through explicit shaders, pipelines, resources, commands, transforms, interaction, textures, and lighting.

Datoviz Canvas and vklite remove platform bootstrap and presentation boilerplate without hiding Vulkan terminology, command recording, resource state, synchronization, or ownership. The course must not create a tutorial-only runtime, renderer, frame stream, wrapper, or ownership model.

## Release Contract

RC3 owns the reusable tutorial-enabling API, shader-toolchain implementation, and rewritten chapters 1-3: setup, first window, and frame command recording. These chapters must work as standalone installed CMake consumers and establish the course voice, code-growth model, ownership vocabulary, validation path, and deterministic capture infrastructure.

RC4 owns the complete 16-chapter course, preview media for every chapter, exact installed-package validation, the frozen tutorial-facing API profile, and reader feedback. Final v0.4.0 owns feedback-driven fixes, regenerated final media, release-pinned compatibility wording, and publication; it is not a new API-design phase.

## Audience And Promise

The primary reader knows ordinary C and a build system but may know no Vulkan. The course promises a fast visible result followed by one growing program that ends as an interactive textured and lit 3D mesh viewer.

The landing page must state what Datoviz supplies and what the reader writes:

| Datoviz supplies | Reader writes |
| --- | --- |
| Instance, device, queues, window, surface, swapchain, acquisition, presentation, frame synchronization, command allocation, depth allocation, capture, and memory allocation | Shader source, pipeline state, vertex and index data, GPU buffers, command recording, transforms, texture upload and sampling, descriptors, interaction, and lighting |

Hidden machinery must be named accurately in focused “under the hood” explanations and the epilogue. It must not dominate the first-result path or be presented as if it does not exist.

## Delivery Contract

1. The public path starts from an installed package and `find_package(datoviz CONFIG REQUIRED)`; a source install is a fallback, not the assumed reader environment.
2. The reader writes one C file that grows chapter by chapter. Each chapter has a complete canonical step program in `examples/c/vulkan/` and a full current listing for recovery.
3. Every prose C or GLSL excerpt must be mechanically synchronized with its canonical source.
4. Live GLFW and deterministic offscreen modes share one rendering implementation.
5. Every chapter has a distinct generated preview derived from its canonical program. Generated previews live under `build/`, not in the `data` submodule.
6. Validation layers are introduced as a reader tool, and each chapter includes a focused failure-diagnosis section.
7. Public explanations distinguish owned, borrowed, callback-duration, frame-duration, and resource-generation lifetimes where they matter.
8. The course labels vklite as advanced/unstable and pins compatibility to the exact Datoviz release rather than promising general low-level stability.
9. Every major concept is defined at first introduction, placed in the CPU-to-GPU flow, connected to its concrete representation in the running program, given an explicit copy and lifetime rule, and reinforced with an experiment whose result the reader can predict.
10. The course uses stable terms for shader stages and invocations, vertex records and attributes, primitive assembly, rasterization, fragments, resources, attachments, pipeline layouts, descriptor set layouts, descriptor sets, sets, bindings, push constants, uniform buffers, and storage buffers. `DvzSlots` is identified as the Datoviz layout-declaration wrapper rather than used as a substitute for those Vulkan concepts.

## Abstraction Boundary

The reader remains responsible for shader stages, pipeline state, buffers, images, samplers, descriptors, push constants, draw commands, transforms, and resource transitions. Canvas may own the window, presentation resources, frame synchronization, command allocation, optional depth resources, capture targets, and event routing.

Borrowed Canvas frame commands, images, image views, depth views, devices, and allocators must never be destroyed, reset, submitted, transitioned outside the granted contract, or retained beyond their documented lifetime. Course code must use the same public Canvas, vklite, shader, file-I/O, geometry, camera, arcball, and input boundaries available to installed consumers.

## Shader Contract

GLSL remains the course shader language. External shader files compile at application startup through the typed runtime shader API and packaged shaderc provider. Editing a shader and restarting the program must not require recompiling C.

Build-time `glslc`, runtime `libshaderc`, optional `spirv-val`, and optional `glslangValidator` have distinct roles defined in [../architecture/SHADER_TOOLCHAIN.md](../architecture/SHADER_TOOLCHAIN.md). Runtime compilation diagnostics must expose the real filename, stage, status, and useful line information. The course must include a deliberate malformed-shader exercise.

Live hot reload is a chapter-level teaching feature only if the existing Canvas input route and safe pipeline replacement make it small and reliable. It must not introduce a general watcher, cache, or virtual shader filesystem.

## Chapter Sequence

The required course has sixteen substantive chapters plus an epilogue:

1. Setup: link Datoviz, print the version, and verify the toolchain.
2. First window: create the GPU context and Canvas, run the loop, choose a clear color, and capture offscreen output.
3. Recording commands: use a draw callback, borrowed command wrapper, rendering pass, viewport, scissor, and animated clear color.
4. First triangle: compile inline GLSL, create shader modules and a graphics pipeline, and draw through `gl_VertexIndex`.
5. External shaders: load source files, report diagnostics, and rebuild the pipeline after an explicit reload action when the verified input path supports it.
6. Vertex buffers: upload an interleaved vertex structure, declare attributes, bind, and draw.
7. Index buffers: reuse vertices and issue indexed draws.
8. Push constants: send small per-frame state and animate the result.
9. Matrices and perspective: introduce model, view, projection, aspect, and a first indexed 3D mesh.
10. Depth and culling: request Canvas-owned depth, configure depth testing, winding, culling, and wireframe experiments.
11. Mouse control: connect Canvas input, camera, and arcball state without the retained scene layer.
12. Uniform buffers and descriptors: keep the MVP in its vertex-stage push range; add a static, 16-byte-aligned fragment `Material` tint in a host-visible uniform buffer; declare it at set 0 binding 0; populate and bind a descriptor set; distinguish layouts, sets, bindings, copied values, referenced resources, alignment, and lifetime.
13. Texture upload: create procedural pixels, staging resources, an image, transitions, copy commands, and one-shot submission while retaining the material uniform.
14. Texture sampling: add image views, a sampler, UVs, filtering, and address-mode experiments; declare the combined image sampler at set 0 binding 1 beside the material uniform at binding 0.
15. Lighting: add normals, normal transforms, ambient, diffuse, and specular light; extend the existing aligned material uniform with light and material parameters while keeping projection and model-view matrices in the 128-byte push range.
16. Real mesh: use generated sphere or torus geometry and show `dvz_geometry_obj()` as the path for readers’ own models.

The epilogue explains the instance/device, swapchain, acquisition/presentation, synchronization, render-target, descriptor-pool, memory, and runtime-layer machinery that Datoviz supplied. It closes with a compact account of state that changes per frame, per draw, or rarely. Compute and WebGPU are onward routes, not required chapters in the v0.4 course.

## Conceptual Progression

The course uses five focused diagrams: the CPU-recording and asynchronous-GPU-execution timeline in chapter 3; the programmable and fixed-function graphics pipeline in chapter 4; the object-to-world-to-view-to-clip-to-normalized-device-to-framebuffer coordinate chain in chapter 9; the C-structure-to-uniform-buffer-to-descriptor-set-to-GLSL-binding relationship in chapter 12; and the CPU-pixels-to-staging-buffer-to-optimal-image transfer in chapter 13.

Chapter 4 defines a shader as a GPU program and distinguishes shader source, SPIR-V intermediate representation, shader module, pipeline, shader invocation, primitive assembly, rasterization, fragment generation, and attachment writes. SPIR-V must not be called GPU-native machine code, and fragment invocations must not be described as a one-to-one synonym for pixels.

Chapter 6 distinguishes shader locations from buffer bindings and explains format, offset, and stride as the contract that interprets vertex-record bytes. Chapter 7 states that indices select complete vertex records and explains why seams and creases sometimes prevent reuse.

Chapter 8 presents push constants as small command-recorded values with device limits and stage visibility, without universal performance claims. Chapter 12 completes the comparison: push constants copy small values into recorded commands, uniform-buffer descriptors refer to structured read-only buffer ranges, and storage buffers support broader shader access and larger or variable data. The choice depends on access, capacity, update frequency, sharing, and device limits rather than a single size rule.

The chapter 12 canonical program uses a static material value so a single uniform buffer is never overwritten while an earlier frame may still read it. A future course change that writes a uniform buffer every frame must first provide a safe per-frame allocation, offset, or synchronization contract. Descriptor updates retain resource references rather than copying buffer or image contents, so the referenced objects remain valid until submitted GPU work completes.

The chapter 15 lighting explanation keeps every vector in one named coordinate space, renormalizes interpolated normals, describes inverse-transpose normal transforms for nonuniform scaling, and ties linear-light calculations to the concrete sRGB texture and attachment formats used by the program.

## Assets And Media

The required course uses generated Datoviz geometry and a procedurally generated asymmetric checkerboard texture. It requires no committed Suzanne OBJ, binary PNG, `data` submodule update, Blender recipe, or external asset-provenance gate.

Suzanne or another external mesh may appear as optional polish only after the generated-geometry course is complete, and it must not become a reader prerequisite or release blocker. Any exact binary proposed for the repository still requires explicit approval and normal license and provenance review.

Every chapter receives generated media:

1. chapter 1 uses a terminal card rendered from captured program output;
2. flat-result chapters use framebuffer captures checked against exact expected RGBA values;
3. time-varying chapter 3 uses deterministic captures at fixed times assembled into animated WebP with the standard still fallback;
4. chapters 4-16 use deterministic non-flat framebuffer captures;
5. media generation must run from canonical chapter programs during the documentation build and must not depend on deleted pilot assets.

## Validation

The repository must provide:

1. source synchronization for every C and GLSL excerpt;
2. in-tree build and deterministic offscreen execution for every step;
3. standalone installed-prefix and exact-wheel CMake-consumer execution;
4. runtime shaderc availability and diagnostic proof through official packages;
5. Vulkan validation with zero unexpected messages;
6. reproducible captures and chapter-specific image checks;
7. bounded live GLFW smoke for resize, input, depth recreation, repeated frames, and shutdown where the host supports those actions;
8. supported hosted-platform proof, with unavailable physical hardware recorded as an exclusion rather than inferred from hosted results;
9. public-header, generated binding, strict documentation, link, navigation, and compatibility checks at release checkpoints.
10. a fresh isolated-reader pass using only the rendered public course, a fresh source clone or exact supported package, and stated platform prerequisites; the student project must live outside the source tree, every graphical chapter must produce inspected capture evidence, every documented interaction and recovery path must be exercised, and the complete pass must report no course-caused failure or ambiguity.

RC3 must prove chapters 1-3 and the enabling API from installed development artifacts. The first official package newer than RC2 must pass the wheel course smoke before package-first instructions lose their version warning. RC4 repeats every chapter against exact candidate artifacts and freezes the API profile. Any course-caused failure or ambiguity in the isolated-reader pass requires a correction followed by another complete pass with a fresh independent reader. Final accepts only blocker or feedback-driven changes.

## Non-Goals

- Raw Vulkan bootstrap before the first result.
- Comprehensive Vulkan, synchronization, presentation, compute, or hardware-architecture coverage.
- A second shader language or full WebGPU lesson track.
- A tutorial-only runtime or wrapper.
- General asset pipelines, glTF, PBR, multiple lights, shadows, or advanced materials.
- A standalone tutorial repository or commit-by-commit progression tool before the in-repository course is complete.
