# Modern GPU Graphics Tutorial Execution Plan

Status: required RC3 API-and-pilot work followed by required RC4 course completion. Updated: 2026-07-25.

Use [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for the durable educational, abstraction, asset, versioning, and validation contract and [../../spec/architecture/SHADER_TOOLCHAIN.md](../../spec/architecture/SHADER_TOOLCHAIN.md) for the agreed build-time, runtime, API, packaging, target-profile, and validation design. This file owns execution order, checkpoints, and agent handoff from RC3 through final v0.4.0.

## Objective

Deliver a beginner-oriented "Modern GPU Graphics in C" course that gives a programmer new to graphics a live Vulkan-backed result quickly, then progresses through explicit GPU resources and commands to a textured, lit, mouse-rotatable Suzanne mesh. Use Datoviz Canvas and vklite to remove platform boilerplate without hiding Vulkan terminology, ownership, command recording, resource state, or synchronization.

The tutorial is also an API quality gate. Chapter spikes must improve generally reusable Canvas, vklite, file-I/O, geometry, and controller boundaries when current ceremony obscures the graphics concept. Do not create a tutorial-only runtime, renderer, frame stream, wrapper layer, or ownership model.

## Milestone Boundary

RC3 owns the enabling public API and the three-chapter pilot. RC4 owns the complete course, tutorial assets, installed exact-artifact proof, and API freeze. Final v0.4.0 owns feedback fixes, regenerated final captures, release notes, and publication; it must not become a new tutorial API design phase.

## Execution Rules

1. Begin each public API change with the narrowest executable chapter spike that demonstrates the actual friction.
2. Before broadening an existing subsystem, state the general user benefit and keep the change inside the active runtime path.
3. Make owned, borrowed, callback-duration, frame-duration, and resource-generation lifetimes explicit in headers, examples, tests, and prose.
4. Do not destroy, reset, submit, transition, or retain a Canvas-borrowed command buffer, frame image, image view, depth view, device, or allocator unless a new reviewed contract explicitly grants ownership.
5. After public header, exported API, binding policy, or binding-generator changes, run `just ctypes` and `just ctypes-check` before Python, GSP, packaging, or installed-consumer validation.
6. Keep tutorial assets in the main repository only after exact-file approval, provenance, license review, and package/install-path design. Do not modify or stage the `data` submodule for this tutorial.
7. Keep one canonical rendering implementation shared by live GLFW and deterministic offscreen execution.
8. Keep Markdown prose unwrapped and run `git diff --check` at every checkpoint.

## RC3 Checkpoint 1: First-Result Spike

Create a source-tree-only spike that renders a shader-generated triangle in a live resizable GLFW Canvas and through the offscreen path. Measure source size, setup steps, first-result commands, ownership explanations, resize behavior, and shutdown behavior before accepting API changes.

The spike must expose only the early frame concepts needed by chapter one: a borrowed frame command buffer and image view, begin rendering, bind the pipeline, set viewport and scissor, draw three vertices, and end rendering. Device selection, surface creation, acquisition, submission, presentation, and swapchain recreation may remain in supplied infrastructure but must be named accurately.

Use the spike to resolve:

- a general GPU-context configuration path for a selected Canvas backend, including backend-required instance extensions and the standard Canvas Vulkan features;
- the smallest honest live-loop and cleanup shape for installed C consumers;
- coherent dynamic viewport and scissor handling;
- command recording into a borrowed Canvas frame without repeated accidental wrapper allocation;
- error paths for absent GLFW, Vulkan, shaderc, or a suitable device.

Acceptance requires focused unit or integration tests for every new contract, a successful resizable live smoke where available, deterministic offscreen capture, clean Vulkan validation, explicit ownership documentation, and no parallel runtime path.

Evidence commit `0be0fe350` adds the `597`-line source-tree spike at `examples/c/tutorial/vklite_triangle_spike.c`. One renderer submitted and drew all 30 validated offscreen frames into an inspected nonblank `800x600` capture and all 114 validated live GLFW frames before attended window closure, with no frame-contract, format, or Vulkan-validation errors. The full Canvas selection passed 26 tests with seven declared platform or capability skips; the external GLFW resize test skipped because macOS did not deliver the requested resize, so the live-resize gate remains open.

The spike demonstrated four general API needs: augment a caller-owned GPU-context configuration with the selected Canvas backend requirements; resolve the actual Canvas render-target format before ordinary pipeline creation; reuse an opaque borrowed-recording command wrapper without invoking owned-command lifecycle operations; and emit frame-sized dynamic viewport and scissor commands through the installed vklite surface rather than raw loader symbols. It also confirmed the separate typed shader-compilation requirement already owned by Checkpoint 2.

The four first-result outcomes are implemented by commits `f17e965f5`, `2e3627525`, `596f294af`, and `37e615627`. The accepted public surface is `dvz_canvas_configure_gpu_ctx()`, `dvz_canvas_frame_format()`, `dvz_commands_unwrap()`, `dvz_cmd_set_viewport()`, `dvz_cmd_set_scissor()`, and `dvz_cmd_set_viewport_scissor()`. Canvas GPU configuration is additive and atomic on failure; automatic present formats remain unresolved until swapchain creation; borrowed-command detach touches no Vulkan object and rejects owned wrappers without mutation; and the full-frame dynamic-state helper emits a zero-origin viewport and scissor with depth range `[0, 1]`.

The completed API slice passes the full build, Canvas selection with 28/35 passing and seven declared capability/platform skips, vklite/DRP2 selection with 80/80 passing, generated ctypes and C reference validation, raw ctypes smoke, the complete specification check, validated 10-frame offscreen and 60-frame live spike runs, the existing advanced raw-triangle run, and standalone installed-package plus FetchContent C consumers that link every accepted symbol. The remaining automated macOS live-resize skip is a pilot validation gate, not an API-slice blocker.

`DvzStreamFrame.resource_generation` is a frame-resource identity, not a global recreation epoch. Present mode rotates among independently allocated slots with distinct generations, so consecutive generation differences may be ordinary slot rotation. Equality confirms the same resource handles; refresh logic must consider `handles_dirty`, extent, format, and generation together. The tutorial and tests must distinguish slot rotation from resize, swapchain recreation, and depth-resource recreation.

## RC3 Checkpoint 2: External Shader Contract

Move vertex and fragment GLSL into external files and compile them at application startup. Shader edits must require only an example restart, not a C rebuild.

Execute this checkpoint in four reviewable slices:

1. Consolidate native build-time compilation behind one reusable `glslc` CMake helper for scene, Canvas, tests, and applicable examples; remove the normal Canvas `glslangValidator` requirement, preserve named graphics and compute profiles, and validate generated SPIR-V with `spirv-val` in CI and release lanes.
2. Move runtime shaderc discovery, loading, target selection, compilation, diagnostics, and ownership out of DRP2 pipeline code into a focused thread-safe shader-compilation module.
3. Implement a generally useful typed public API with explicit source size, source filename, entry point, stage, target profile, availability, status, diagnostics, and `dvz_memory_free()`-owned SPIR-V, plus a compile-file convenience or null-terminated text reader.
4. Prove source-build enabled and disabled configurations, packaged-provider discovery, installed CMake consumers, external vertex and fragment files, and supported-platform behavior.

Slices 1 through 3 are implemented by commits `f46ac75bd`, `3544a520a`, and `06aad322d`. Native scene, Canvas, and test shaders share the named-profile `glslc` helper with optional `spirv-val`; CI and release configurations require both precompilation and validation; Canvas no longer requires `glslangValidator`; and the runtime adapter now lives in `src/shader` with once-only provider initialization. The public `shader.h` contract provides typed stages, fixed graphics and compute profiles, explicit source sizes and names, stable availability and failure statuses, owned diagnostics and SPIR-V, idempotent cleanup, generated C and ctypes reference coverage, and compatibility access to the legacy narrow wrapper. The public text reader preserves byte size while adding null termination for compiler input.

Slice 3 proof covers successful vertex, fragment, and compute compilation; Vulkan 1.0/SPIR-V 1.0 graphics and Vulkan 1.3/SPIR-V 1.6 compute targets; malformed source with filename-bearing diagnostics; empty source and invalid stage/profile rejection; repeated cleanup; concurrent compilation; enabled and disabled adapters; missing and incompatible providers; core-only symbol export without DRP2; source and installed C consumers; bindings; generated reference docs; and full specification validation.

Commit `86bedc74c` implements the local external-file portion of slice 4. The same renderer reads vertex and fragment files through `dvz_read_text()`, compiles through the typed API, accepts an explicit shader directory, has no private-header dependency, and builds as both a repository target and a standalone `find_package(datoviz CONFIG REQUIRED)` consumer. Source-tree and installed-package macOS runs produced visually inspected nonblank `800x600` captures after three validated offscreen frames; a copied malformed fragment required no C rebuild and reported its real path and line. Official wheel-provider and Linux/Windows hosted proof remain open.

The implementation must distinguish build-time `glslc`, runtime shaderc, `spirv-val`, and optional `glslangValidator`; report malformed stages, empty source, missing files, compilation errors, absent or incompatible providers, and successful vertex, fragment, and compute compilation; and work from source builds and installed wheel/package layouts on Linux, macOS, and Windows.

Audit existing `dvz_compile_glsl()` callers during this checkpoint. Correct allocator mismatches and misleading hardcoded diagnostic names without preserving an inferior v0.3-era contract.

Runtime GLSL is a supported v0.4 user capability, not only a fallback for built-in scene shaders. Official packages must guarantee the runtime provider, while custom source builds may disable it and use precompiled SPIR-V.

## RC3 Checkpoint 3: Three-Chapter Pilot

Publish and validate:

1. First live triangle with shader-generated positions and colors.
2. External shaders and the graphics pipeline, ending with a visually rewarding shader experiment.
3. C vertex data and an explicit GPU vertex buffer.

Each chapter requires a preview, one principal concept, complete runnable source, external shaders, focused C guidance, a visible experiment, a deliberate failure or diagnostic exercise where appropriate, a checkpoint, an exercise, live GLFW execution, deterministic offscreen execution, and a validation command.

The public path must be a standalone CMake consumer using `find_package(datoviz CONFIG REQUIRED)`. Repository targets may provide contributor convenience but are not the reader prerequisite.

Commits `1a748def3`, `3b567370a`, and `b5a5b989f` implement and publish the pilot. `first_triangle`, `shaders_and_pipeline`, and `vertex_buffers` compile one canonical `examples/c/tutorial/triangle.c` renderer with chapter-specific defaults; chapter two uses a visibly distinct external shader pair, and chapter three uploads interleaved C positions and colors into an owned mapped vertex buffer while frame commands remain borrowed. Repository and standalone installed-package CMake builds each ran three validated offscreen frames at `800x600`; all captures were nonblank, chapter two differed from chapter one, and the source and installed smoke paths are repeatable through `just vulkan-tutorial-smoke` and `just vulkan-tutorial-installed-smoke`.

The public tutorial pages provide commands, one principal concept, complete-source and shader links, focused C and ownership guidance, visible experiments, deliberate failures, checkpoints, and exercises. `tools/check_vulkan_tutorial.py` keeps target names, shader mappings, required APIs, prose commands, preview references, and legacy-API exclusions synchronized. Approved data commit `63a0ba44` records the three canonical `800x600` PNGs, exact hashes, generation commit, commands, and license; main commit `c77448188` generates build-only WebP derivatives, includes them in MkDocs output, and rejects missing deployment media. The pilot passes the 80-test vklite selection, 98-page/113-C-block snippet validation, strict MkDocs link and build validation, real deployment staging, Git LFS integrity, the full specification check, Python syntax validation for its checkers, and `git diff --check`. Official packaged-provider and Linux/Windows hosted proof and a platform-delivered live-resize smoke remain open.

RC3 exit evidence must include:

- source-tree and installed-package builds;
- packaged runtime shaderc compilation;
- deterministic nonblank or reference captures;
- bounded GLFW resize and shutdown smoke;
- generated C reference and binding refresh for public changes;
- strict docs, links, source-snippet synchronization, and example checks;
- Linux, macOS, and Windows hosted proof consistent with the release validation matrix;
- recorded feedback questions covering first-result latency, assumed C knowledge, ownership clarity, shader diagnostics, API ceremony, and abstraction honesty.

Do not begin broad RC4 chapter production until the pilot voice, API profile, installed consumer, and validation workflow have maintainer approval.

## RC3 API Outcome Inventory

RC3 must either deliver each outcome below or record a maintainer-approved reason that the existing API already satisfies it:

| Outcome | Required proof |
| --- | --- |
| Canvas-aware GPU configuration | GLFW and offscreen examples configure the required instance extensions, device features, and Canvas extensions without copied backend bootstrap code. |
| Unified native shader build | Scene, Canvas, tests, and examples use one `glslc` helper; normal builds no longer require `glslangValidator`; release products carry validated precompiled SPIR-V. |
| External shader compilation | A focused thread-safe module and typed public API report availability, status, real filenames, diagnostics, target profiles, and ownership in successful and failing installed-consumer paths. |
| Runtime shaderc packaging | Official installed packages guarantee the provider on Linux, macOS, and Windows; disabled source builds retain precompiled-SPIR-V rendering with an honest unavailable-capability result. |
| Borrowed frame commands | Tutorial code records without destroying, resetting, submitting, transitioning, or retaining the frame command buffer. |
| Dynamic viewport and scissor | Resizing preserves correct output and requires no unexplained raw/wrapper state duplication. |
| Resolved Canvas frame format | Pipelines use the actual Canvas render-target format before first drawing; requested, resolved, and per-frame formats are distinguished; later format changes are detected without assuming RGBA/BGRA ordering or invalidating in-flight resources. |
| Optional depth attachment | Format, borrowed handles, resize recreation, generation, layout, clear, and lifetime contracts are public and tested. |
| OBJ UV support | `v`, `vt`, `vn`, independent and negative indices, triangulation, malformed input, cleanup, and generated geometry arrays are tested. |
| Image upload clarity | Texture upload, layouts, views, samplers, descriptors, and barriers remain explicit while repeated accidental ceremony is reduced where justified. |
| Direct arcball use | A low-level arcball connects to the Canvas input router and supplies resize-aware model-view-projection state without the retained scene layer. |

Optional depth and OBJ UV support must land in RC3 even though their teaching chapters are in RC4, because RC4 is the consumer and freeze candidate rather than the first public API exposure.

## RC4 Checkpoint 1: 3D Foundation

Implement chapters four and five:

4. Per-frame state and transforms.
5. Indexed 3D geometry and depth.

Begin with a comprehensible small mesh before Suzanne. Teach model-view-projection state, perspective, index buffers, indexed drawing, depth attachment ownership, depth testing, culling, coordinate conventions, and resize recreation through visible experiments. Do not combine texture or lighting into the first 3D checkpoint.

Validate repeated frames, resizing, minimized-window recovery, frame-slot identity, resource-set refresh, depth recreation, index bounds, cleanup order, and Vulkan attachment-layout correctness.

## RC4 Checkpoint 2: Texture And Suzanne

Implement chapter six with a small triangulated, UV-unwrapped, smooth-normal Suzanne OBJ and deterministic Datoviz-owned UV diagnostic texture.

Before committing assets:

1. record the Blender version and export, triangulation, unwrap, smoothing, coordinate, and scaling recipe;
2. verify redistribution and choose explicit asset licenses;
3. record vertex, normal, texture-coordinate, face, triangle, and index counts plus deterministic hashes;
4. include the texture-generation source or script and deterministic output record;
5. obtain exact approval for the generated PNG or other binary asset;
6. define install paths and runtime discovery for source-tree and installed-package consumers;
7. keep the assets out of the `data` submodule.

Teach image allocation, upload, layout transitions, image views, samplers, descriptor binding, UV coordinates, filtering, and sRGB albedo sampling. Readers must be able to replace the supplied texture without changing the rendering architecture.

## RC4 Checkpoint 3: Interaction And Lighting

Implement:

7. Mouse-driven arcball.
8. Normals and basic lighting.

Use the public CPU-side arcball and Canvas input router rather than deriving trackball mathematics. Teach resize-aware model-view-projection updates and the distinction between CPU controller state and GPU uniform or push-constant state.

The lighting chapter must show normal visualization before lighting, then explain normalization, interpolation, the dot product, object and world space, the inverse-transpose normal matrix, Lambert diffuse lighting, ambient fill, and linear-space lighting of an sRGB texture. A short Blinn-Phong specular extension may be optional. Do not broaden the course into general PBR, materials, or BRDF theory.

Keep the directional light fixed in world or view space while Suzanne rotates so incorrect coordinate spaces and normal transforms are visibly diagnosable.

## RC4 Checkpoint 4: Full-Course Freeze

Complete cross-chapter editing, navigation, diagrams, screenshots, exercises, source synchronization, compatibility labels, API links, ownership tables, and troubleshooting. Validate every chapter from exact RC4 source archives and installed wheels rather than only the development tree.

Required RC4 proof:

- all tutorial CMake consumers build from clean directories against installed Datoviz;
- external shaders and assets resolve without source-tree assumptions;
- runtime shaderc succeeds from exact packaged artifacts;
- deterministic offscreen captures are current and inspected;
- bounded GLFW interaction covers resize, depth recreation, arcball, repeated frames, and shutdown;
- Vulkan validation is clean or every limitation is explicitly recorded;
- Linux, macOS, and Windows hosted lanes pass, and unavailable physical hardware remains an exclusion rather than a pass;
- every asset has known provenance and license;
- public tutorial APIs and the release-pinned tutorial profile are frozen except for blockers;
- strict documentation, example, package, source-archive, license, known-issue, and release-note gates pass.

RC4 feedback must ask readers to complete the tutorial from the installed package and report platform, GPU, driver, time-to-first-result, chapter completion, compiler diagnostics, asset discovery, interaction, conceptual confusion, and any ownership or cleanup mistakes induced by the text.

## Final v0.4.0

Resolve or explicitly record RC4 feedback. Regenerate final screenshots and any short tutorial clip from the exact final source and assets. Publish the release-pinned compatibility statement, known limitations, checksums, provenance, licenses, release notes, and documentation.

Do not redesign tutorial-facing public API, replace the tutorial runtime boundary, add a second shader language track, introduce custom course tooling, or expand into PBR, model loading formats beyond the required OBJ slice, raw Vulkan bootstrap, or full WebGPU parity during the final gate.

## Deferred

- Live shader hot-reload unless the completed course demonstrates a small, safe addition.
- A standalone tutorial repository or commit-by-commit progression tool.
- A complete raw Vulkan implementation track.
- Full WebGPU lesson parity.
- General asset pipelines, glTF, PBR, multiple lights, shadows, and advanced materials.
- Comprehensive Vulkan synchronization, presentation, and compute coverage before the textured-lit-Suzanne course is complete; later chapters may follow after the final-v0.4 core tutorial gate if they do not destabilize the release.
