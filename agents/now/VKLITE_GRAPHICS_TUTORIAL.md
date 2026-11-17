# Modern GPU Graphics In Vulkan Course Execution

Status: the complete 16-chapter implementation, epilogue, canonical programs, synchronized excerpts and generated previews are present. The 2026-11-16 in-tree smoke passes all 16 steps; exact post-RC2 package proof and a fresh isolated-reader audit of the revised course remain release gates. The independent reader pass of the earlier 15-chapter baseline remains historical evidence. Updated: 2026-11-16.

Use [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for the durable contract, [../../docs/architecture/vulkan_course_plan.md](../../docs/architecture/vulkan_course_plan.md) for the working chapter outline, and [../../spec/architecture/SHADER_TOOLCHAIN.md](../../spec/architecture/SHADER_TOOLCHAIN.md) for shader policy.

## Implemented Baseline

- `docs/gpu-graphics/` contains the overview, substantive chapters 1-16, and epilogue 17; `examples/c/vulkan/step01.c` through `step16.c` are the canonical programs. Uniform buffers occupy chapter 12, texture upload/sampling 13-14, lighting 15, and the generated mesh 16.
- `just vulkan-course-check` verifies synchronized C and GLSL excerpts, and `just vulkan-course-smoke` builds and runs all current steps with deterministic captures and Vulkan validation.
- The documentation build generates chapter media from canonical programs, including deterministic animations and still fallbacks.
- Canvas-owned targets start with defined contents, making empty or load-based first frames reproducible.
- Installed loader discovery reports explicit search routes and works from a source install without manual runtime-directory arguments.
- `just vulkan-course-wheel-smoke <version>` tests the exact package-first instructions. It correctly reports that `0.4.0rc2` lacks the post-RC2 tutorial API.
- The earlier 15-chapter baseline received technical, prose, consistency and isolated-reader review. That reader followed rendered pages from a clean clone and external student directory, inspected captures, and exercised interaction and shader recovery. This result does not establish independent acceptance of the later 16-chapter revision.
- The chapter-5 Canvas input path is proven through focused input tests, synthetic hosted-view injection, and a physical X11 `r` injection that delivered one physical-key press, one committed-text event, and one release. The safe reload shape is callback-to-flag only, followed outside event dispatch by candidate shader/pipeline creation, failure-preserving rollback, device wait, pointer swap, and old-pipeline destruction; it requires no watcher or new public API.

The deleted `docs/tutorials/`, `examples/c/tutorial/`, `vulkan-tutorial-*` recipes, and old pilot previews are historical. Do not restore or reference them as current course content.

## Implemented Tutorial API

The reusable API work is complete: Canvas GPU-context augmentation and resolved frame format; borrowed command unwrap/detach; dynamic viewport and scissor helpers; shared build-time `glslc`; thread-safe runtime shaderc with typed status, diagnostics, profiles, file input, and owned results; optional Canvas depth; OBJ `vt` preservation; explicit image upload and sampling primitives; validated push constants; Canvas input routing; and direct camera/arcball composition.

Official-package shaderc proof on supported platforms, package installation proof for the rewritten course, and platform-delivered live resize remain release evidence rather than new API design work.

## Completed Revision And Remaining Review

The maintainer reports on 2026-11-16 that Claude is currently checking the course. Treat that review as in progress and collect its findings before scheduling another course audit. Its isolation, chapter coverage, execution evidence and recovery/interaction coverage are not yet recorded, so it does not automatically close the independent-reader or maintainer-review gates.

`30ee8ea3a` implements the uniform-buffer/descriptor chapter and the 16-chapter sequence; `078bcb809` repairs the cumulative code path and mesh progression, and `fd65af119` makes the documented source setup independent of CUDA. On 2026-11-16, `just vulkan-course-check` verifies 16 chapters and 173 excerpts; `just vulkan-course-smoke` passes all 16 canonical steps with valid, reproducible captures. The strict documentation build generates all 18 media outputs.

1. Obtain maintainer proofreading of all chapters, diagrams, exercises, previews, ownership explanations and package-first instructions.
2. The 2026-11-16 Claude review covered a fresh-clone reader run of all 16 chapters (all built and ran with zero validation errors), voice, clarity and correctness, consistency, learner experience and pacing. Must-fix findings landed in `3d2397637`; a concise template, voice and learner pass follows. Deferred past v0.4: reordering part 4 (upload, sampling, then uniform buffers), moving viewport/scissor from chapter 3 to 4, and a canvas format query before the first frame. The fresh-clone reader run covers the isolated-reader contract for the revised course except live input on a physical display (exercised under Xvfb with lavapipe) and hosted platforms. Repeat the reader pass after the revision pass lands.
   On 2026-11-17 the maintainer judged the concise pass too terse. Chapter 12 (`7f7ea430c`) is the approved model for a fuller explanatory pass over every chapter; follow [../rules/VULKAN_COURSE_STYLE.md](../rules/VULKAN_COURSE_STYLE.md). The explanatory pass landed for every chapter on 2026-11-17 (`49ad528eb`, `8f1aede6b`, `4ef3e423e`, `032e94afc`), followed by a consistency review and a fresh learner review whose fixes landed in `159a18228`. Open learner suggestions not taken: chapter 13 carries both UVs and the upload (moving UVs to 14 would make 13 invisible again), repeated build-and-run blocks, and the animation removed in 4, restored in 8 and removed again in 11.
3. Validate exact installed artifacts and hosted/live behavior under the RC4 freeze below; retain the first official post-RC2 package gate for RC3.

Use generated geometry and a procedural asymmetric checkerboard. No committed Suzanne OBJ, PNG, `data` update, Blender recipe, or binary-asset approval is required for the course. Optional external-mesh polish is not a release blocker.

## RC4 Freeze

Build and run every chapter from exact candidate source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`. Require packaged runtime shaderc, deterministic previews, bounded GLFW resize/input/depth/repeated-frame/shutdown smoke, Vulkan validation, source synchronization, links, navigation, generated reference and binding checks, supported hosted-platform proof, and explicit physical exclusions.

Collect feedback on setup time, code progression, diagnostics, GPU and driver behavior, interaction, concepts, ownership, and cleanup. Freeze the tutorial-facing API except for recorded blockers.

## Final v0.4.0

Resolve or record RC4 feedback, regenerate all course media from exact final code, publish the release-pinned compatibility statement and known limitations, and include the course in final release documentation. Do not introduce a new API, runtime path, shader language, asset pipeline, or course structure during the final gate.
