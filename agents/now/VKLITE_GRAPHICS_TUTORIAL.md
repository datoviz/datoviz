# Modern GPU Graphics In Vulkan Course Execution

Status: the complete 16-chapter implementation, epilogue, canonical programs and generated previews are present. The fuller explanatory revision landed on 2026-11-17; the 2026-11-18 Claude Opus 5.5 medium review and independent source check completed bounded documentation corrections. All 16 canonical programs pass the 2026-11-18 validation-enabled in-tree smoke with reproducible captures. The 2026-11-20 isolated-reader run passes the Linux source-install path for all 16 chapters. Maintainer proofreading and exact post-RC2 package/platform proof remain separate gates. Updated: 2026-11-21.

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

The 2026-11-16 review and 2026-11-17 explanatory/learner passes are complete at the revisions below. The maintainer requested a continuation on 2026-11-18 using Claude Opus 5.5 at medium effort. That continuation reviews the current overview, all 16 substantive chapters and epilogue against the canonical programs, shaders and implementation. It is a static factual/editorial review, independently checked against source; it does not claim a new isolated-reader, physical-interaction or installed-package pass.

`30ee8ea3a` implements the uniform-buffer/descriptor chapter and the 16-chapter sequence; `078bcb809` repairs the cumulative code path and mesh progression, and `fd65af119` makes the documented source setup independent of CUDA. On 2026-11-16, `just vulkan-course-check` verifies 16 chapters and 173 excerpts; `just vulkan-course-smoke` passes all 16 canonical steps with valid, reproducible captures. The strict documentation build generates all 18 media outputs.

1. Obtain maintainer proofreading of all chapters, diagrams, exercises, previews, ownership explanations and package-first instructions.
2. The 2026-11-16 Claude review covered a fresh-clone reader run of all 16 chapters (all built and ran with zero validation errors), voice, clarity and correctness, consistency, learner experience and pacing. Must-fix findings landed in `3d2397637`; a concise template, voice and learner pass follows. Deferred past v0.4: reordering part 4 (upload, sampling, then uniform buffers), moving viewport/scissor from chapter 3 to 4, and a canvas format query before the first frame. The fresh-clone reader run covers the isolated-reader contract for the revised course except live input on a physical display (exercised under Xvfb with lavapipe) and hosted platforms. Repeat the reader pass after the revision pass lands.
   On 2026-11-17 the maintainer judged the concise pass too terse. Chapter 12 (`7f7ea430c`) is the approved model for a fuller explanatory pass over every chapter; follow [../rules/VULKAN_COURSE_STYLE.md](../rules/VULKAN_COURSE_STYLE.md). The explanatory pass landed for every chapter on 2026-11-17 (`49ad528eb`, `8f1aede6b`, `4ef3e423e`, `032e94afc`), followed by a consistency review and a fresh learner review whose fixes landed in `159a18228`. Open learner suggestions not taken: chapter 13 carries both UVs and the upload (moving UVs to 14 would make 13 invisible again), repeated build-and-run blocks, and the animation removed in 4, restored in 8 and removed again in 11.
3. Validate exact installed artifacts and hosted/live behavior under the RC4 freeze below; retain the first official post-RC2 package gate for RC3.

Use generated geometry and a procedural asymmetric checkerboard. No committed Suzanne OBJ, PNG, `data` update, Blender recipe, or binary-asset approval is required for the course. Optional external-mesh polish is not a release blocker.

## 2026-11-18 Review And Maintainer Checklist

Claude Opus 5.5 at medium effort reviewed the overview, all 16 substantive chapters and the epilogue, then applied bounded corrections. An independent source check and parent review refined ownership, descriptor, OBJ and browser-support claims. The fuller teaching style remains; no canonical program, shader, public API or fenced C/GLSL excerpt changed. The corrections cover source/package setup, fragment semantics, the warm-up target, wrapper lifecycles, push-constant troubleshooting, depth configuration order, storage buffers, descriptor teardown, image views, OBJ normals and the WebGPU onward route. No blocking runtime defect was found in this review. Setup corrections landed in `36d29e8ac`; the remaining course corrections landed in `819a4d74a`.

The Windows source setup now includes the missing configure command with CUDA disabled and the Release vcpkg dependency DLL directory on `PATH`; the install rules do not install those dependency DLLs into the prefix. This correction is source-verified, not Windows execution proof. The package-only compiler shortcut and macOS loader diagnostics are also corrected.

Validation on Linux: `just vulkan-course-check` verifies 16 chapters and 178 excerpts; `just docs-status-check` passes. With the repository SDK environment loaded, `just vulkan-course-smoke` passes all 16 programs with zero validation errors and reproducible captures. The initial unconfigured-shell attempt could not find the validation layer; it is not counted as a passing run. `just docs-build-check` passes its 12 generator tests, strict build and all 18 generated course media outputs. The homepage selection's 24 focused tests pass and its technical snapshot is unchanged. Whitespace and local link checks pass. These are development-source checks, not exact-candidate, installed-package or physical-platform acceptance.

Two scratch exercise checks close the earlier uncertain observations: reversing chapter 9's final matrix multiplication produces a flat capture on the tested Linux provider, and recomputing chapter 16's sphere normals changes 9,622 of 480,000 pixels with maximum channel delta 8 and zero validation errors. The latter exercise now says “looks very similar” instead of promising no visible change. Validation-error exercises describe the error rather than promise an exact count or timing. Full review findings, implementation notes, logs and scratch artifacts remain ignored under `build/course-review/20261118T125214Z/`.

| Remaining work | Who or prerequisite | Recommended next action |
| --- | --- | --- |
| Changed prose and teaching voice | Maintainer | Start with [setup](../../docs/gpu-graphics/01-setup.md), [triangle and lifecycle](../../docs/gpu-graphics/04-triangle.md), [descriptors](../../docs/gpu-graphics/12-uniform-buffers.md), [mesh normals](../../docs/gpu-graphics/16-mesh.md) and [epilogue](../../docs/gpu-graphics/17-next.md), then review the remaining course voice, pacing, exercises and previews in the private WIP documentation. |
| Revised-course reader acceptance | Linux development-source reader run complete; maintainer assessment remains | The 2026-11-20 run follows frozen rendered instructions in an isolated installed-prefix project for all 16 chapters. Official packages, other platforms and physical input retain their separate gates. |
| Windows source/DLL instructions and platform behavior | Available Windows/macOS hardware or hosted candidate builders | Run the documented setup and relevant interaction/resize checks; preserve unavailable physical checks as exclusions. |
| First official package newer than RC2 | RC3 freeze and exact artifact preparation | Verify the installed CMake/shaderc/course route against the actual candidate; do not change the current development version early. |
| RC3 freeze and publication | Maintainer | Resolve remaining course/gallery/platform dispositions, then freeze. Release publication and website deployment need separate exact final approvals. |

Optional listing-format normalization and OBJ smoothing architecture are outside this correction pass. Previously deferred course reordering remains deferred; no new API work is required by these findings.

## Isolated Reader Follow-up, 2026-11-21

Claude Opus 5.5 at medium effort completed the 2026-11-20 reader run against frozen rendered course pages at `c2744ab2c`. It built a separate Release library and installed prefix, then incrementally authored, built and ran all 16 student programs using `find_package(datoviz CONFIG REQUIRED)`. No canonical step C file was opened and no full-listing recovery was needed; listings were used after successful authored runs to synchronize cosmetic differences. All primary programs pass, Vulkan steps report zero validation errors with an active layer, and repeated captures match. This is Linux development-source evidence, not fresh-clone dependency proof or an official candidate package.

Representative exercises and synthetic XTEST input under Xvfb cover malformed-shader filename/line diagnostics, failure-preserving **R** reload, repaired shader recovery, resize, chapter 11 drag/scroll and window close. Programmatic resize without a window manager and synthetic input are not physical interaction or HiDPI acceptance. Deliberately invalid descriptor/image GPU draws were checked statically; Windows, macOS and the official package route were not executed. No runtime defect requiring a code change was found.

Three verified troubleshooting corrections follow the run: chapter 8 now explains the actual `push-constant update failed` message; chapter 1 clarifies Vulkan configure-time discovery and the source environment's runtime shaderc dependency; chapter 4 names `provider-missing` and its remedy. No C/GLSL listing, canonical program, shader or API changed. Maintainer rereading is limited to chapter 1's source-install paragraph/troubleshooting table and the troubleshooting boxes in chapters 4 and 8; earlier prose remains unchanged. Cosmetic preferences from the reader are deferred.

The complete report, per-chapter results and command/provenance records are ignored under `build/course-reader/20261120T193632Z/`; the student snapshots, captures, exercises and live-session evidence remain in the isolated reader workspace. Maintainer course acceptance, exact post-RC2 artifacts, other platforms and final publication gates remain open. Apply focused validation to these factual prose corrections; do not repeat the entire reader run solely for the unchanged programs.

## RC4 Freeze

Build and run every chapter from exact candidate source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`. Require packaged runtime shaderc, deterministic previews, bounded GLFW resize/input/depth/repeated-frame/shutdown smoke, Vulkan validation, source synchronization, links, navigation, generated reference and binding checks, supported hosted-platform proof, and explicit physical exclusions.

Collect feedback on setup time, code progression, diagnostics, GPU and driver behavior, interaction, concepts, ownership, and cleanup. Freeze the tutorial-facing API except for recorded blockers.

## Final v0.4.0

Resolve or record RC4 feedback, regenerate all course media from exact final code, publish the release-pinned compatibility statement and known limitations, and include the course in final release documentation. Do not introduce a new API, runtime path, shader language, asset pipeline, or course structure during the final gate.
