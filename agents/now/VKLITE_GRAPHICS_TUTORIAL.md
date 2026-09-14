# Modern GPU Graphics In Vulkan Course Execution

Status: executable 15-chapter baseline, generated previews, automated checks, live validation, and an isolated-reader pass complete; conceptual-depth revision to 16 substantive chapters plus an epilogue is active. Exact post-RC2 package proof remains a release gate. Updated: 2026-09-14.

Use [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for the durable contract, [../../docs/architecture/vulkan_course_plan.md](../../docs/architecture/vulkan_course_plan.md) for the working chapter outline, and [../../spec/architecture/SHADER_TOOLCHAIN.md](../../spec/architecture/SHADER_TOOLCHAIN.md) for shader policy.

## Implemented Baseline

- `docs/gpu-graphics/` contains the overview, chapters 1-15, and the epilogue; `examples/c/vulkan/step01.c` through `step15.c` are their canonical programs.
- `just vulkan-course-check` verifies synchronized C and GLSL excerpts, and `just vulkan-course-smoke` builds and runs all current steps with deterministic captures and Vulkan validation.
- The documentation build generates chapter media from canonical programs, including deterministic animations and still fallbacks.
- Canvas-owned targets start with defined contents, making empty or load-based first frames reproducible.
- Installed loader discovery reports explicit search routes and works from a source install without manual runtime-directory arguments.
- `just vulkan-course-wheel-smoke <version>` tests the exact package-first instructions. It correctly reports that `0.4.0rc2` lacks the post-RC2 tutorial API.
- Technical, prose, and consistency reviews cover the current complete baseline. A fresh isolated worker followed the rendered course from a clean source clone and external student directory, built and ran every chapter, inspected captures, exercised live interaction and shader failure recovery, and reported no course-caused defect after the setup correction.
- The chapter-5 Canvas input path is proven through focused input tests, synthetic hosted-view injection, and a physical X11 `r` injection that delivered one physical-key press, one committed-text event, and one release. The safe reload shape is callback-to-flag only, followed outside event dispatch by candidate shader/pipeline creation, failure-preserving rollback, device wait, pointer swap, and old-pipeline destruction; it requires no watcher or new public API.

The deleted `docs/tutorials/`, `examples/c/tutorial/`, `vulkan-tutorial-*` recipes, and old pilot previews are historical. Do not restore or reference them as current course content.

## Implemented Tutorial API

The reusable API work is complete: Canvas GPU-context augmentation and resolved frame format; borrowed command unwrap/detach; dynamic viewport and scissor helpers; shared build-time `glslc`; thread-safe runtime shaderc with typed status, diagnostics, profiles, file input, and owned results; optional Canvas depth; OBJ `vt` preservation; explicit image upload and sampling primitives; validated push constants; Canvas input routing; and direct camera/arcball composition.

Official-package shaderc proof on supported platforms, package installation proof for the rewritten course, and platform-delivered live resize remain release evidence rather than new API design work.

## Active Conceptual-Depth Revision

1. Deepen first-introduction explanations and diagrams across chapters 3-11 without changing their completed behavior.
2. Insert chapter 12 on uniform buffers and descriptors: a static fragment `Material` tint at set 0 binding 0, with the MVP retained as a vertex-stage push constant.
3. Shift texture upload to chapter 13; put the combined image sampler at set 0 binding 1 in chapter 14; extend the material uniform with aligned lighting parameters in chapter 15; shift the generated mesh to chapter 16 and the epilogue to 17.
4. Extend canonical programs, shaders, navigation, checks, smokes, media, and installed-consumer validation through step 16.
5. Run conceptual and prose consistency review, then repeat the full isolated-reader audit with a fresh independent worker until one complete pass reports no course-caused failure or ambiguity.

Use generated geometry and a procedural asymmetric checkerboard. No committed Suzanne OBJ, PNG, `data` update, Blender recipe, or binary-asset approval is required for the course. Optional external-mesh polish may be considered only after the required course is complete.

## RC4 Freeze

Build and run every chapter from exact candidate source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`. Require packaged runtime shaderc, deterministic previews, bounded GLFW resize/input/depth/repeated-frame/shutdown smoke, Vulkan validation, source synchronization, links, navigation, generated reference and binding checks, supported hosted-platform proof, and explicit physical exclusions.

Collect feedback on setup time, code progression, diagnostics, GPU and driver behavior, interaction, concepts, ownership, and cleanup. Freeze the tutorial-facing API except for recorded blockers.

## Final v0.4.0

Resolve or record RC4 feedback, regenerate all course media from exact final code, publish the release-pinned compatibility statement and known limitations, and include the course in final release documentation. Do not introduce a new API, runtime path, shader language, asset pipeline, or course structure during the final gate.
