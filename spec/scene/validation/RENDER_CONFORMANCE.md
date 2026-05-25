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


## Prioritized Fixture Plan

The first fixtures should maximize diagnostic value before breadth. Start with tests that prove the
runner and artifact comparison behavior, then add small scene fixtures that isolate one layer of the
scene -> DRP2 -> runtime path at a time.

### Wave 0: Harness and Diagnostics

These tests should land before the fixture corpus grows. Every later failure depends on their
reporting being trustworthy.

| Priority | Test | Coverage |
|----------|------|----------|
| P0 | `conformance_runner_finds_fixture` | fixture discovery and deterministic ordering |
| P0 | `conformance_ref_update_requires_flag` | normal runs never rewrite references |
| P0 | `conformance_snapshot_diff_reports_lines` | readable normalized DRP2 snapshot diffs |
| P0 | `conformance_image_diff_reports_metrics` | dimensions, max delta, changed pixels, actual/diff paths |
| P0 | `drp2_diagnostics_matches_app_trace` | shared normalizer preserves current app trace behavior |

### Wave 1: First Scene Fixtures

These are the first real scene fixtures to write. Require normalized DRP2 snapshots and vklite image
references for each fixture. Canonical stream references may start with payload byte counts and
stable hashes so the references remain reviewable.

| Priority | Fixture | Coverage |
|----------|---------|----------|
| P1 | `point_basic_2d` | retained visual, vertex upload, point pipeline, viewport, draw |
| P1 | `image_colormap_basic` | texture upload, sampler, scale/colormap binding, fragment sampling |
| P1 | `mesh_depth_basic` | indexed draw, depth attachment, depth compare/write, camera MVP |
| P1 | `multi_panel_scissor_basic` | panel layout, per-panel viewport/scissor, no cross-panel bleed |
| P1 | `retained_second_frame_no_churn` | stable resource reuse and no unnecessary semantic stream churn |
| P1 | `partial_update_region` | buffer or texture subrange update in both stream and rendered output |

### Wave 2: Visual Family Coverage

Add these after the first wave proves the artifact format and failure reports. Keep each fixture
small, deterministic, and focused on one visual family or visual-specific shader path.

| Priority | Fixture | Coverage |
|----------|---------|----------|
| P2 | `pixel_basic` | square pixel sizing and pixel shader path |
| P2 | `marker_basic_shapes` | marker shape selection, size, and color |
| P2 | `primitive_basic` | triangle or quad primitive lowering |
| P2 | `segment_basic` | segment width and endpoint behavior |
| P2 | `path_join_modes` | miter, bevel, and round join behavior |
| P2 | `sphere_impostor_basic` | impostor shader, depth, and lighting cue |
| P2 | `volume_slice_basic` | 3D texture upload and slice sampling |
| P2 | `volume_mip_basic` | deterministic bright-slice projection |
| P2 | `text_label_basic` | text output once atlas and tolerances are stable |
| P2 | `colorbar_continuous_basic` | ramp, ticks, title, and colorbar bookkeeping |

### Wave 3: Cameras, Layout, and Techniques

These fixtures should catch regressions in frame planning, graph-backed passes, and state carried
between passes.

| Priority | Fixture | Coverage |
|----------|---------|----------|
| P3 | `panzoom_transform_fixed` | deterministic 2D transform |
| P3 | `arcball_mesh_fixed` | deterministic 3D camera matrix |
| P3 | `dpi_scale_viewport` | framebuffer size versus logical size |
| P3 | `clear_color_alpha` | render-target clear behavior |
| P3 | `msaa_resolve_basic` | multisample target plus resolve target |
| P3 | `edl_points_basic` | graph-backed postprocess input/output wiring |
| P3 | `ssao_mesh_basic` | depth texture sampling and SSAO pass ordering |
| P3 | `wboit_two_layers` | order-independent transparency output |
| P3 | `source_over_depth_blend` | alpha blending plus depth interaction |
| P3 | `depth_peel_two_layers` | multi-pass transparency and depth-peeling resources |

### Wave 4: Requests and Interaction Outputs

Request fixtures should be tiny and deterministic. Prefer one-pixel or one-item targets so failures
can report exact expected and observed payloads.

| Priority | Fixture | Coverage |
|----------|---------|----------|
| P4 | `point_pick_center_hit` | pick render/readback path and item-id payload |
| P4 | `point_pick_miss` | no-hit status and stable payload |
| P4 | `image_probe_center_pixel` | probe readback returns expected texel/value |
| P4 | `image_probe_oob` | out-of-bounds status behavior |
| P4 | `linked_panel_pick_route` | request targets the intended panel |
| P4 | `selection_highlight_point` | pick result drives a visible retained update |

### Wave 5: Backend Parity and Stress

Add these once native vklite references are stable. The WebGPU fixtures should use the same scene
semantics and only relax backend-specific image tolerances.

| Priority | Fixture | Coverage |
|----------|---------|----------|
| P5 | `webgpu_point_basic` | DRP2 point subset parity |
| P5 | `webgpu_image_colormap_basic` | texture, sampler, and WGSL parity |
| P5 | `webgpu_mesh_depth_basic` | depth-state parity |
| P5 | `webgpu_marker_segment_basic` | marker and segment shader parity |
| P5 | `resize_recreate_mesh_image` | descriptor refresh after target/resource recreation |
| P5 | `many_panels_scissor_matrix` | layout and scissor pressure |
| P5 | `large_payload_hash_only` | reviewable stream refs for large uploads |
| P5 | `saved_stream_replay_point_image_mesh` | backend regression isolated from scene emission |


## Open Decisions

1. final artifact directory names and whether refs live under `spec/scene/refs/` or `testing/`,
2. exact payload policy for canonical DRP2 refs: full payloads, hashes, or both,
3. platform key format for image refs,
4. image tolerance defaults per backend,
5. whether the shared normalization API remains private to tests/app or becomes a public diagnostic
   API.
