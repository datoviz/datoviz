# Automated Testing Strategy

This note records the broad automated-testing strategy for the active scene architecture. It is a
companion to the narrower render-conformance plan and is meant to keep visual-first validation
layered, diagnosable, and useful during aggressive v0.4 refactors.

The core idea is that screenshots should not carry the whole burden. A visual regression should be
caught at the cheapest layer that can prove the contract: scene semantics first, DRP2 contract next,
backend rendering after that, and human review only for the parts that truly require visual judgment.


## Goals

1. catch regressions before they become ambiguous visual failures,
2. keep manual visual review reproducible and artifact-backed,
3. separate scene, DRP2, backend, shader, and platform causes,
4. make long-running resource churn and lifecycle bugs observable,
5. keep fast local validation useful while preserving deeper CI and release lanes.


## Test Layers

### Scene Semantic Tests

Scene semantic tests validate user-facing scene behavior without requiring a GPU. They should cover:

1. object lifecycle for figures, panels, visuals, controllers, scales, annotations, and requests,
2. retained updates, repeated setters, partial updates, visibility changes, and zero-count visuals,
3. invalid input handling for mismatched counts, bad dimensions, null pointers, and unsupported
   formats,
4. deterministic frame-plan output for canonical scenes,
5. owned versus borrowed resource behavior,
6. multi-panel semantics such as linked controllers, clipping, z-order, and independent state.

These tests should be the default place for API and ownership regressions.


### DRP2 Contract Tests

DRP2 contract tests validate the main architecture boundary between scene planning and runtime
execution. They should cover:

1. stable scene-emitted DRP2 snapshots for canonical scenes,
2. command-ordering invariants such as create-before-use, write-before-draw, and pass begin/end
   pairing,
3. resource lifetime, including destroy-after-use and no use-after-destroy,
4. bind-group layout order, dynamic offsets, and descriptor refresh behavior,
5. visual-family pipeline descriptors and vertex attribute formats,
6. graph-backed pass structure for MSAA, transparency, postprocess, and readback,
7. saved-stream replay independent of the current scene emitter.

When this layer fails, the output should be a readable protocol or semantic diff rather than only an
image difference.


### Render Conformance Tests

Render conformance tests compare backend-rendered output for small deterministic fixtures. The
detailed fixture plan lives in [RENDER_CONFORMANCE.md](RENDER_CONFORMANCE.md).

These tests should use:

1. small fixture sizes such as 64x64 or 128x128 whenever possible,
2. backend-specific image references and tolerances,
3. one visual behavior per fixture,
4. deterministic cameras, clear colors, DPI, and viewports,
5. failure reports with dimensions, max channel delta, changed-pixel count or percentage, and
   actual/diff artifact paths.

Image assertions can be broader than whole-image equality. Useful checks include nonblank pixel
count, expected sampled pixels, changed regions after partial updates, panel-region isolation, depth
ordering at known pixels, and approximate transparent composition.


### Metamorphic Visual Tests

Metamorphic tests assert relationships between renders instead of comparing against one exact
reference. Useful cases include:

1. rendering the same scene twice without mutation produces the same snapshot and near-identical
   image,
2. translating all data and the camera together preserves the screen-space image,
3. reordering opaque depth-tested visuals does not change selected pixels,
4. changing only clear color affects only background pixels,
5. resizing and downsampling preserves coarse structure,
6. updating a subregion does not affect unrelated image regions,
7. WBOIT transparent layers are less order-sensitive than source-over layers.

These tests are useful when exact golden images would be too brittle.


### Property and Fuzz Tests

Property and fuzz tests should use pinned seeds and report the seed on failure. Good targets are:

1. random scene construction with bounded panels, visuals, data sizes, updates, and destroy order,
2. random resize/render/update loops,
3. random valid and invalid DRP2 command streams within the active grammar,
4. dimensions and counts near zero, upper limits, and overflow-prone sizes,
5. repeated create/destroy cycles,
6. randomized partial update ranges.

The expected result can be simple: no crash, no validation-layer error, deterministic final stream,
bounded live-resource counts, and clean error codes for invalid inputs.


### Long-Run Churn Tests

Long-run churn tests should find bugs that only appear across many frames or repeated resource
changes. Useful loops include:

1. render a static scene for hundreds of frames and assert no semantic DRP2 churn,
2. resize every few frames,
3. alternate visual visibility,
4. update one buffer or texture repeatedly,
5. recreate image and mesh resources repeatedly,
6. enqueue pick and probe requests while rendering,
7. run with `DVZ_DRP2_TRACE=normal` and fail on unexpected stream changes.

Whenever practical, these tests should record live buffers, textures, pipelines, bind groups,
transient objects, submitted command counts, and descriptor refresh counts.


### Shader ABI Tests

Shader ABI tests should fail before visual output becomes mysteriously wrong. They should cover:

1. C struct layout versus GLSL/WGSL uniform and storage layout,
2. vertex attribute offsets and formats,
3. bind-group layout order,
4. material and light parameter packing,
5. shader variant compilation,
6. GLSL and WGSL resource parity for portable fixtures.


### Backend Capability and Parity Tests

Backend tests should make support explicit instead of assuming every backend can render every
feature. They should cover:

1. executable capability checks for depth, MSAA, readback, storage buffers, float textures,
   blending, and 3D textures,
2. fixture skip or expected-failure behavior based on declared capabilities,
3. clean failures for unsupported features,
4. shared DRP2 streams rendered through vklite and WebGPU where supported,
5. semantic parity checks such as dimensions, nonblank masks, readback values, and loose image
   comparisons.


### Example and Gallery Tests

Examples are user-facing integration tests. A gallery harness should:

1. build examples that are expected to remain active,
2. run selected examples offscreen for one frame,
3. capture thumbnails,
4. check dimensions and nonblank output,
5. optionally compare against loose references,
6. produce a contact sheet for human review.

Large showcase examples should not replace small conformance fixtures, but they are valuable release
confidence signals.


### Failure-Injection Tests

Failure-injection tests should prove cleanup and recovery paths. Useful injected failures include:

1. allocation failure,
2. shader compile failure,
3. texture or buffer creation failure,
4. runtime unavailable,
5. readback unavailable,
6. swapchain or offscreen-target resize failure,
7. unsupported backend capability,
8. partial stream failure after resources were created.

After a failure, tests should assert that no half-live objects remain reachable and that a later
valid frame can still run when recovery is expected.


### Architecture Lint Tests

Some architecture rules can be checked directly with lightweight scripted tests:

1. scene code does not call Vulkan APIs directly,
2. app owns presentation and scene does not,
3. inactive modules are not linked by default,
4. public headers do not include private backend headers,
5. WebGPU code does not fork scene semantics,
6. examples use public API rather than scene internals,
7. C source avoids direct `malloc`, `calloc`, `free`, `memcpy`, `memset`, `fprintf`, and
   `vfprintf` where shared Datoviz wrappers are required.


## Observability and Diagnostics

Automated tests need stable inspection hooks. Useful diagnostics include:

1. frame-plan dumps as structured text or JSON,
2. DRP2 stream snapshots and fingerprints,
3. live runtime resource-table summaries,
4. per-frame counters for resources, descriptors, passes, draws, writes, and readbacks,
5. last-frame timing and command-count summaries,
6. explicit invalidation reasons explaining why a frame was dirty.

These hooks should be instance-scoped and should avoid file-scope mutable state.


## Record and Replay Strategy

Record/replay should be a first-class test tool for visual architecture. Important examples and
fixtures should be able to produce a saved DRP2 stream that can be rendered independently from the
current scene emitter.

This separates failures cleanly:

1. scene changed when the stream changes,
2. backend changed when the same stream produces different output,
3. player or recording changed when replay no longer matches the saved stream contract.


## Golden Reference Governance

Golden references are useful only if updates are deliberate and reviewable.

Rules:

1. normal test runs must not rewrite references,
2. reference update flags must be explicit,
3. image references should be backend-specific,
4. tiny focused fixtures should be preferred over large screenshots,
5. each complex reference should have a short note explaining what it proves,
6. actual and diff images should be saved as failure artifacts, not silently committed,
7. intentional visual changes should update semantic, DRP2, and image references according to the
   layer that changed.


## Human Review Workflow

Human visual review remains necessary for aesthetics, readability, and perceptual quality. It should
be made reproducible by generating:

1. a gallery contact sheet,
2. before/after thumbnails when comparing branches,
3. changed-pixel percentages for render fixtures,
4. saved DRP2 streams for representative scenes,
5. exact commands needed to regenerate each artifact.

Manual approval should be tied to artifacts rather than only an interactive observation.


## CI Tiers

The test strategy should map to tiers so local development remains fast:

1. `fast`: CPU scene tests, DRP2 validation, shader ABI checks, and architecture lint,
2. `gpu-smoke`: small offscreen render fixtures with nonblank and sampled-pixel checks,
3. `gpu-conformance`: backend image references and DRP2 snapshot references,
4. `stress`: long-run churn, resize, update, and fuzz loops,
5. `gallery`: example captures, thumbnails, and human-review artifacts,
6. `nightly`: validation layers, sanitizers, broader backend matrix, and longer stress runs.


## Regression Policy

Every fixed visual regression should become at least one reusable artifact:

1. a scene semantic test,
2. a DRP2 snapshot fixture,
3. a render conformance fixture,
4. a saved-stream replay,
5. an example or gallery capture.

Manual-only checks should be reserved for subjective aesthetic review or temporary investigations.


## Initial Implementation Order

1. Extract shared DRP2 diagnostics from the app trace path.
2. Add the conformance runner harness and failure reporting.
3. Add the first P1 render fixtures from [RENDER_CONFORMANCE.md](RENDER_CONFORMANCE.md).
4. Add resource-counter diagnostics for churn tests.
5. Add a small architecture-lint script for the clearest boundary rules.
6. Add one saved-stream replay lane for point, image, and mesh fixtures.
7. Add gallery thumbnail generation for a short active-example subset.
