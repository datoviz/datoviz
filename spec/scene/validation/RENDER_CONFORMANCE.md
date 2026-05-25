# Scene Render Conformance

This note records the planned automated render-conformance lane for the active
scene -> DRP2 -> runtime path.

The goal is to cover small scene API surface elements with deterministic fixtures, save both
normalized DRP2 references and rendered image references, and use those artifacts to detect
regressions across native and browser backends.


## Goals

1. exercise each small scene feature and API surface element with focused fixtures,
2. test the scene -> DRP2 boundary with stable DRP2 reference artifacts,
3. test DRP2 -> backend -> image execution with backend-specific image references,
4. keep failures diagnosable by separating semantic stream changes from rendering-output changes,
5. reuse the existing live DRP2 trace normalization logic instead of growing a second canonicalizer.

This is not a replacement for unit tests. It is a rendering conformance layer that complements the
current DRP2 fixture corpus, scene unit tests, and manual smoke examples.


## Fixture Shape

Each fixture should build one small scene deterministically and then produce one or more artifacts:

1. a canonical DRP2 stream reference,
2. a normalized DRP2 snapshot reference,
3. one rendered image reference per backend/configuration.

Example fixture families:

1. retained point, pixel, marker, primitive, mesh, path/segment, image, volume, and sphere visuals,
2. color mapping, image scale binding, colormap updates, and colorbar bookkeeping,
3. camera and controller states such as panzoom, arcball, fly, and turntable,
4. panel layout, viewport/scissor, DPI, and multi-panel figures,
5. depth-enabled 2D/3D passes, MSAA, EDL, SSAO, transparency, and material variants,
6. GPU-backed point pick and image probe request paths where output can be made deterministic.

Fixtures should avoid animation, wall-clock timing, random seeds without explicit initialization,
and backend-dependent layout defaults. If a fixture needs randomness, it must pin the seed in the
fixture source.


## Artifact Layers

### Canonical DRP2 Stream Reference

The canonical stream reference should be the strictest scene -> DRP2 artifact.

It should preserve command order and stable protocol fields, including resource ids or stable
labels, resource descriptors, write ranges, pipeline descriptors, pass setup, bind-group layout
order, viewport/scissor state, draw payloads, queue submissions, and readback request shape.

Large payloads should not make review impossible. For large buffer or texture updates, the
reference may store payload metadata such as byte count and a stable hash. Replayable fixtures may
additionally store full payloads through the existing DRP2 recording/blob path.

This layer answers: did the scene emit the same protocol contract?


### Normalized DRP2 Snapshot Reference

The normalized snapshot should be a compact, readable semantic view of the stream. It should ignore
volatile mechanics that do not represent scene behavior, such as transient encoder ids, pass ids,
borrowed frame handles, command-buffer handles, submission ids, payload pointers, and other
frame-local runtime details.

This layer answers: did the emitted stream change in a way humans should inspect?

The current app trace code already implements this idea for live stream churn detection. Its
normalizer/fingerprint behavior should be reused by the conformance lane after refactoring it out of
the app-only layer.


### Backend Image Reference

Rendered image references should be backend-specific. Native Vulkan/vklite, WebGPU/browser,
operating systems, GPU vendors, MSAA resolve behavior, texture filtering, and presentation paths can
differ slightly while still satisfying the same scene semantics.

Example layout:

```text
spec/scene/fixtures/render/
  point_basic.c
  image_colormap.c
  mesh_depth.c

spec/scene/refs/drp2/
  point_basic.stream.json
  point_basic.snapshot.txt

spec/scene/refs/images/vklite/macos-arm64/
  point_basic.png

spec/scene/refs/images/webgpu/chrome-macos-arm64/
  point_basic.png
```

Image comparison should use explicit tolerances rather than exact byte equality for GPU output.
The runner should report at least image dimensions, max channel delta, changed-pixel count or
percentage, and paths to actual and diff images on failure.

This layer answers: did the backend still render the fixture as expected?


## Failure Interpretation

The layers are intended to make failures actionable:

| Canonical stream | Normalized snapshot | Image | Likely meaning |
|------------------|---------------------|-------|----------------|
| changed          | changed             | changed or same | scene emission or API semantics changed |
| changed          | same                | same or changed | low-level stream mechanics changed; inspect strict ref |
| same             | same                | changed | backend/runtime/shader/render-target regression |
| same             | changed             | any | normalizer bug or overly lossy/overly strict snapshot rule |

Intentional visual semantics changes may require updating all three artifact layers. Backend-only
changes should usually update only the affected backend image refs.


## Reusing Existing Trace Normalization

The existing live app trace lane should be the starting point:

1. `_dvz_app_trace_fingerprint()` computes a stable semantic fingerprint for a DRP2 stream,
2. `_dvz_app_trace_snapshot_build()` creates compact normalized stream lines,
3. existing app tests cover ignored transient ids, payload handling, scoped resources, and draw
   payload retention.

To avoid duplication, move the reusable stream normalization and fingerprinting logic from the app
module into a DRP2-owned diagnostics/canonicalization layer. The app trace code should call this
shared layer, and the render-conformance runner should call the same layer.

Possible placement:

```text
src/drp2/diagnostics.c
src/drp2/_diagnostics.h
```

or, if test tools and future clients need the API:

```text
include/datoviz/drp2/diagnostics.h
src/drp2/diagnostics.c
```

The refactor should preserve the app trace behavior and tests while adding fixture-oriented entry
points such as:

```c
bool dvz_drp2_stream_fingerprint(const DvzDrp2CommandStream* stream, uint64_t* out);
bool dvz_drp2_stream_snapshot_build(DvzDrp2Snapshot* snapshot, const DvzDrp2CommandStream* stream);
```

The exact public/private boundary can be decided during implementation. The important constraint is
that live trace suppression and automated conformance snapshots use one canonical implementation.


## Runner Workflow

The runner should support separate update paths:

```text
--update-drp2-refs
--update-image-refs
--update-all-refs
```

Reference updates must be explicit. A normal test run should never rewrite refs automatically.

Recommended validation lanes:

1. scene fixture -> canonical DRP2 stream ref,
2. scene fixture -> normalized DRP2 snapshot ref,
3. scene fixture -> vklite image ref,
4. scene fixture -> WebGPU/browser image ref,
5. optional saved DRP2 stream -> backend image ref, to isolate backend regressions while scene code
   is changing.

The optional saved-stream replay lane is useful because it tests DRP2 -> backend -> image without
depending on the current scene emitter.


## Initial Scope

Start with a small, stable corpus before expanding:

1. point basic,
2. image plus colormap,
3. mesh with depth,
4. multi-panel layout with viewport/scissor,
5. one transparency or postprocess fixture,
6. one pick/probe readback fixture if deterministic output is available.

Once the runner, artifact format, update workflow, and failure output are stable, grow the fixture
matrix toward the full active scene API surface.


## Open Decisions

1. final artifact directory names and whether refs live under `spec/scene/refs/` or `testing/`,
2. exact payload policy for canonical DRP2 refs: full payloads, hashes, or both,
3. platform key format for image refs,
4. image tolerance defaults per backend,
5. whether the shared normalization API remains private to tests/app or becomes a public diagnostic
   API.
