# Modern GPU Graphics In Vulkan Course Execution

Status: rewritten chapters 1-3 and enabling API implemented; replacement previews and exact post-RC2 package proof remain for RC3; chapters 4-15 remain for RC4. Updated: 2026-08-01.

Use [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for the durable contract, [../../docs/architecture/vulkan_course_plan.md](../../docs/architecture/vulkan_course_plan.md) for the working chapter outline, and [../../spec/architecture/SHADER_TOOLCHAIN.md](../../spec/architecture/SHADER_TOOLCHAIN.md) for shader policy.

## Implemented Foundation

- `docs/gpu-graphics/` contains the course overview and chapters 1-3: setup, first window, and recording commands.
- `examples/c/vulkan/step01.c` through `step03.c` are the canonical programs and build as `example_c_vulkan_stepNN`.
- `just vulkan-course-check` verifies that every chapter C excerpt occurs in its canonical program.
- `just vulkan-course-smoke` and `just vulkan-course-installed-smoke` build and run every current step with deterministic captures and Vulkan validation.
- Canvas-owned targets start with defined contents, making empty or load-based first frames reproducible.
- Installed loader discovery reports explicit search routes and works from a source install without manual runtime-directory arguments.
- `just vulkan-course-wheel-smoke <version>` tests the exact package-first instructions. It correctly reports that `0.4.0rc2` lacks the post-RC2 tutorial API.

The deleted `docs/tutorials/`, `examples/c/tutorial/`, `vulkan-tutorial-*` recipes, and old pilot previews are historical. Do not restore or reference them as current course content.

## Implemented Tutorial API

The reusable API work is complete: Canvas GPU-context augmentation and resolved frame format; borrowed command unwrap/detach; dynamic viewport and scissor helpers; shared build-time `glslc`; thread-safe runtime shaderc with typed status, diagnostics, profiles, file input, and owned results; optional Canvas depth; OBJ `vt` preservation; explicit image upload and sampling primitives; validated push constants; Canvas input routing; and direct camera/arcball composition.

Official-package shaderc proof on supported platforms, package installation proof for the rewritten course, and platform-delivered live resize remain release evidence rather than new API design work.

## RC3 Next Steps

1. Replace the old `tools/build_tutorial_media.py` pilot inputs with generated previews for chapters 1-3.
2. Generate chapter 1’s terminal card from captured stdout, validate chapter 2 against exact expected RGBA, and add deterministic fixed-time capture plus animated WebP for chapter 3.
3. Verify the Canvas keyboard subscription path needed by chapter 5 and record the smallest safe pipeline-reload shape; do not add a watcher or general hot-reload subsystem.
4. Run the rewritten chapters through source-install, exact official package newer than RC2, and supported hosted-platform smokes with validation.
5. Record any platform-delivered live resize limitation without redesigning the API around an unavailable event.
6. Obtain maintainer review of chapters 1-3 voice, pacing, ownership explanations, package instructions, and generated previews before broad RC4 prose.

## RC4 Chapter Queue

Implement one canonical program, synchronized chapter, generated preview, source-install smoke, and installed exact-artifact smoke per checkpoint:

1. Chapters 4-7: first triangle, external shaders and explicit reload, vertex buffers, and index buffers.
2. Chapters 8-11: push constants, matrices and perspective, depth and culling, and mouse control.
3. Chapters 12-15: texture upload, texture sampling, lighting, and a generated sphere or torus as the real mesh.
4. Epilogue: explain the hidden instance/device, swapchain, acquisition/presentation, synchronization, render-target, memory, and Datoviz runtime machinery.

Use generated geometry and a procedural asymmetric checkerboard. No committed Suzanne OBJ, PNG, `data` update, Blender recipe, or binary-asset approval is required for the course. Optional external-mesh polish may be considered only after the required course is complete.

## RC4 Freeze

Build and run every chapter from exact candidate source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`. Require packaged runtime shaderc, deterministic previews, bounded GLFW resize/input/depth/repeated-frame/shutdown smoke, Vulkan validation, source synchronization, links, navigation, generated reference and binding checks, supported hosted-platform proof, and explicit physical exclusions.

Collect feedback on setup time, code progression, diagnostics, GPU and driver behavior, interaction, concepts, ownership, and cleanup. Freeze the tutorial-facing API except for recorded blockers.

## Final v0.4.0

Resolve or record RC4 feedback, regenerate all course media from exact final code, publish the release-pinned compatibility statement and known limitations, and include the course in final release documentation. Do not introduce a new API, runtime path, shader language, asset pipeline, or course structure during the final gate.
