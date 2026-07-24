# Modern GPU Graphics Tutorial Execution Plan

Status: required RC3 API-and-pilot work followed by required RC4 course completion. Updated: 2026-07-24.

Use [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for the durable educational, abstraction, asset, versioning, and validation contract. This file owns execution order, checkpoints, and agent handoff from RC3 through final v0.4.0.

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

## RC3 Checkpoint 2: External Shader Contract

Move vertex and fragment GLSL into external files and compile them at application startup. Shader edits must require only an example restart, not a C rebuild.

Implement a generally useful public contract that:

- reads null-terminated text or compiles directly from a file;
- retains the actual source filename in shaderc diagnostics;
- reports compiler availability and actionable failure details;
- distinguishes build-time `glslc` and `glslangValidator` from runtime shaderc;
- returns owned SPIR-V under the `dvz_memory_free()` allocation contract;
- works from source builds and installed wheel/package layouts on Linux, macOS, and Windows;
- validates malformed stages, empty source, missing files, compilation errors, unavailable shaderc, and successful vertex, fragment, and compute compilation.

Audit existing `dvz_compile_glsl()` callers during this checkpoint. Correct allocator mismatches and misleading hardcoded diagnostic names without preserving an inferior v0.3-era contract.

## RC3 Checkpoint 3: Three-Chapter Pilot

Publish and validate:

1. First live triangle with shader-generated positions and colors.
2. External shaders and the graphics pipeline, ending with a visually rewarding shader experiment.
3. C vertex data and an explicit GPU vertex buffer.

Each chapter requires a preview, one principal concept, complete runnable source, external shaders, focused C guidance, a visible experiment, a deliberate failure or diagnostic exercise where appropriate, a checkpoint, an exercise, live GLFW execution, deterministic offscreen execution, and a validation command.

The public path must be a standalone CMake consumer using `find_package(datoviz CONFIG REQUIRED)`. Repository targets may provide contributor convenience but are not the reader prerequisite.

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
| External shader compilation | Real filenames appear in successful and failing installed-consumer shader paths; ownership and availability are testable. |
| Borrowed frame commands | Tutorial code records without destroying, resetting, submitting, transitioning, or retaining the frame command buffer. |
| Dynamic viewport and scissor | Resizing preserves correct output and requires no unexplained raw/wrapper state duplication. |
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

Validate repeated frames, resizing, minimized-window recovery, depth recreation, generation changes, index bounds, cleanup order, and Vulkan attachment-layout correctness.

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
