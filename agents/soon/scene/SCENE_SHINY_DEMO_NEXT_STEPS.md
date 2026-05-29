# Scene Shiny Demo Next Steps

> **Execution Status**
> - **Status:** `RECOMMENDED FOLLOW-UP DISPATCH`
> - **Updated on:** `2026-05-29`
> - **Purpose:** record the current recommendation for high-payoff v0.4 RC1 or early v0.4+
>   showcase work after text, overlays, legends, colorbars, labels, and scale bars landed.
> - **Audience:** agents choosing the next moderately complicated feature or gallery-polish lane.

Use this file when asked what to build next for shiny examples beyond the required WebGPU/WASM and
raw `ctypes` release lanes. It is an execution recommendation, not a durable visual contract.
Durable visual-family behavior belongs under `spec/scene/visuals/`.


## Current Read

Several earlier-looking gaps are now mostly implemented:

1. **Labels and label volumes:** first-class integer labels render, bind categorical scales,
   integrate with legends, support selected/hidden/boundary state, and have a polished showcase.
   Raw 2D label-id probing and sparse signed/unsigned label-volume lookup are active. The remaining
   high-value label gap is larger-field, transform, and request-path pressure testing.
2. **Explanatory layout:** axes, colorbars, legends, scale bars, and panel reserve plumbing exist.
   The remaining gap is a composed proof example, not core layout infrastructure.
3. **Textured mesh:** the retained UV/textured-mesh slice is implemented, including mesh texture
   field binding, texture color mode, material integration, shaders, focused scene tests, and the
   `examples/c/visuals/textured_mesh.c` proof. The remaining work is gallery/fixture promotion and
   capture validation for the terrain or planet story.
4. **Showcases:** protein, LiDAR, brain, labels, and textured mesh examples mostly exist. The
   remaining work is default tuning, screenshots/captures, smoke validation, and gallery polish.


## Recommended Order

| Priority | Lane | Why it is next | First useful slice |
| ---: | --- | --- | --- |
| 1 | Gallery proof pass | Converts existing showcase code into release confidence. | Run/polish protein, LiDAR, brain, labels, textured mesh or terrain/planet, colorbar/legend, and capture paths; tune defaults and record validation. |
| 2 | Vector visual | Best new semantic visual for scientific demos; unlocks wind, CFD, displacement, normals, trajectories, and napari-style vectors. | Pressure the first-class vector visual with a wind-field showcase and polish head styling. |
| 3 | Label probe hardening | Smaller than a new visual family and directly improves the napari-style labels demo. | Stress raw label-id probing under transforms, larger fields, request churn, and gallery-style hover/click readout. |
| 4 | Explanatory layout proof | Low-risk RC proof for already-landed adornment layout pieces. | Add or polish one example combining axes, colorbar, legend, scale bar, and panel reserves without collisions. |
| 5 | Splat visual | Flashy and self-contained; now acceptable as a v0.4 experimental showcase target if the new visual lands cleanly. | Implement a retained point-cloud splat/Gaussian-like visual, add fixture/capture proof, and keep full Gaussian-splat pipelines explicitly deferred. |


## Lane Notes

### Retained Textured Mesh

The implementation record is
[`../../done/SCENE_TEXTURED_MESH_IMPLEMENTATION.md`](../../done/SCENE_TEXTURED_MESH_IMPLEMENTATION.md).

The first slice is active: a mesh resource with `texcoords`, one 2D RGBA sampled field bound as
mesh material input, `color_mode = texture`, sampler defaults, and composition with the current
lighting/material path.

The remaining pressure work is a deterministic terrain or planet-surface C example or fixture with
one screenshot capture. Defer cubemaps, skyboxes, multi-texture materials, normal maps, PBR,
terrain LOD, asset download/cache automation, and mesh face picking unless the release proof
exposes a concrete need.

### Vector Visual

Public `dvz_vector()` and `DVZ_VISUAL_TYPE_VECTOR` are installed. The remaining useful work is
gallery pressure and styling polish, not a second arrow API.

Use [`SCENE_VECTOR_VISUALS_PLAN.md`](SCENE_VECTOR_VISUALS_PLAN.md) as the execution starting point.
Keep follow-up narrow: positions, vectors, color, length/scale, optional vector-head style, and
panzoom/arcball-compatible transforms. Defer tensor glyphs, streamtube generation, vector-field LOD,
and advanced per-item metadata unless the showcase needs them.

### Label Probe Hardening

The labels showcase now uses a GPU-backed `dvz_labels()` segment probe for hover and click readout.
Sparse signed/unsigned label-volume lookup and high-id query edge cases are also covered. The
remaining work is hardening: mapped data coordinates, panzoom/keep-aspect coverage, large-field
pressure, and optimizing the request path beyond full-texture copies.

Use [`SCENE_NAPARI_IMAGE_LABELS_PLAN.md`](SCENE_NAPARI_IMAGE_LABELS_PLAN.md) and
[`../../../spec/scene/integration/napari/NAPARI.md`](../../../spec/scene/integration/napari/NAPARI.md)
for semantics. Add pressure tests for transformed panels, larger fields, and request churn.

### Explanatory Layout Proof

Panel reserve aggregation is already implemented for axes, colorbars, legends, and related
adornments. The next useful work is an example and focused validation that compose them in one real
figure.

The proof should combine at least axes, a continuous colorbar, a categorical legend, and a scale bar
in one or more panels. Treat failures as concrete layout bugs rather than a reason to design a broad
new layout engine.

### Gallery Proof Pass

Protein, LiDAR, brain, and labels already have most of the intended runtime features. A future
agent should first run them, capture screenshots or offscreen frames where possible, tune defaults,
and fix concrete rough edges exposed by the run.

Optional local moodboard, when present:
[`../../../_showcase_references/main_showcase_reference.png`](../../../_showcase_references/main_showcase_reference.png).
This ignored reference image is visual direction only; do not treat it as committed data or an
expected-output artifact.

Prefer this lane when the goal is RC1 confidence. Prefer vector or splat when the goal is a
new capability that makes the gallery visibly richer.

### Splat Visual

No public `dvz_splat()` or `DVZ_VISUAL_TYPE_SPLAT` is installed. Existing splatting documents are
roadmap material, not active implementation. If the visual is implemented soon, the v0.4 target is
an experimental showcase, not a feature-freeze blocker.

Use [`../../later/SPLATTING_TIERED_PLAN.md`](../../later/SPLATTING_TIERED_PLAN.md) and
[`../../../spec/scene/proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](../../../spec/scene/proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md)
before implementation. Keep the first slice deliberately narrow: retained splat items with center,
radius, color, opacity, a documented depth/blending policy, one deterministic fixture, one
synthetic or LiDAR-like gallery capture, and explicit deferral of full differentiable/3D
Gaussian-splatting ambitions, trained asset formats, out-of-core scenes, and advanced LOD.


## Practical Choice

For required v0.4 gallery scope, start with the **gallery proof pass**, especially textured mesh or
terrain/planet capture. For an additional shiny feature, start with **vector visual**, then
immediately pressure it with a wind-field showcase. For RC1 rigor after those, use **raw label-id
GPU probing** and the **explanatory layout proof**. For maximum visual novelty after the release
proof is stable or the visual lands cleanly, add the **splat visual** as a v0.4 experimental
showcase.
