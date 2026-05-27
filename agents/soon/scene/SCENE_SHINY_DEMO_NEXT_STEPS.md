# Scene Shiny Demo Next Steps

> **Execution Status**
> - **Status:** `RECOMMENDED FOLLOW-UP DISPATCH`
> - **Updated on:** `2026-05-27`
> - **Purpose:** record the current recommendation for high-payoff v0.4 RC1 or early v0.4+
>   showcase work after text, overlays, legends, colorbars, labels, and scale bars landed.
> - **Audience:** agents choosing the next moderately complicated feature or gallery-polish lane.

Use this file when asked what to build next for shiny examples beyond the required WebGPU/WASM and
raw `ctypes` release lanes. It is an execution recommendation, not a durable visual contract.
Durable visual-family behavior belongs under `spec/scene/visuals/`.


## Current Read

Several earlier-looking gaps are now mostly implemented:

1. **Labels:** first-class integer labels render, bind categorical scales, integrate with legends,
   support selected/hidden/boundary state, and have a polished showcase. The remaining high-value
   gap is raw label-id GPU probing plus larger sparse-id pressure tests.
2. **Explanatory layout:** axes, colorbars, legends, scale bars, and panel reserve plumbing exist.
   The remaining gap is a composed proof example, not core layout infrastructure.
3. **Showcases:** protein, LiDAR, brain, and labels examples mostly exist. The remaining work is
   default tuning, screenshots/captures, smoke validation, and gallery polish.
4. **Textured mesh:** the gallery strategy now requires a true retained textured-mesh terrain or
   planet-surface proof for v0.4. Baked vertex colors are not an acceptable substitute for this
   lane.


## Recommended Order

| Priority | Lane | Why it is next | First useful slice |
| ---: | --- | --- | --- |
| 1 | Retained textured mesh | Required for the v0.4 terrain/planet showcase and useful for many mesh examples; current retained mesh path does not yet bind textures as material input. | Add UV upload, mesh texture binding, `color_mode = texture`, sampler defaults, lighting/material integration, fixture coverage, and a terrain/planet capture. |
| 2 | Vector/arrow visual | Best new semantic visual for scientific demos; unlocks wind, CFD, displacement, normals, trajectories, and napari-style vectors. | Add a first-class arrow/vector visual or tightly scoped arrow helper, then use it in the wind-field showcase. |
| 3 | Raw label-id GPU probe | Smaller than a new visual family and directly improves the napari-style labels demo. | Return integer label ids from `dvz_labels()` fields through a labels-specific scene request/readback path; add sparse/high-id stress tests. |
| 4 | Explanatory layout proof | Low-risk RC proof for already-landed adornment layout pieces. | Add or polish one example combining axes, colorbar, legend, scale bar, and panel reserves without collisions. |
| 5 | Gallery proof pass | Converts existing showcase code into release confidence. | Run/polish protein, LiDAR, brain, labels, textured terrain/planet, colorbar/legend, and capture paths; tune defaults and record validation. |
| 6 | Splat visual | Flashy and self-contained; now acceptable as a v0.4 experimental showcase target if the new visual lands cleanly. | Implement a retained point-cloud splat/Gaussian-like visual, add fixture/capture proof, and keep full Gaussian-splat pipelines explicitly deferred. |


## Lane Notes

### Retained Textured Mesh

The first slice should stay deliberately narrow: a mesh resource with `texcoords`, one 2D RGBA
sampled texture or field bound as mesh material input, `color_mode = texture`, linear/nearest
sampler defaults, and composition with the current lighting/material path. It should prove
replacement or layer switching without recreating unrelated mesh buffers.

The pressure example is a deterministic terrain or planet-surface C example with one screenshot
capture. Defer cubemaps, skyboxes, multi-texture materials, normal maps, PBR, terrain LOD, asset
download/cache automation, and mesh face picking unless the first showcase exposes a concrete need.

### Vector/Arrow Visual

No public `dvz_vector()` or `DVZ_VISUAL_TYPE_VECTOR` is installed. Wind-field-style examples can
temporarily use primitives, but that should not become the permanent semantic model.

Use [`SCENE_VECTOR_VISUALS_PLAN.md`](SCENE_VECTOR_VISUALS_PLAN.md) as the execution starting point.
Keep the first slice narrow: positions, vectors, color, length/scale, optional arrowhead style, and
panzoom/arcball-compatible transforms. Defer tensor glyphs, streamtube generation, vector-field LOD,
and advanced per-item metadata unless the first showcase needs them.

### Raw Label-Id GPU Probe

The labels showcase now uses a GPU-backed `dvz_labels()` segment probe for hover and click readout.
The remaining work is hardening: mapped data coordinates, panzoom/keep-aspect coverage, sparse-id
pressure, and optimizing the request path beyond full-texture copies.

Use [`SCENE_NAPARI_IMAGE_LABELS_PLAN.md`](SCENE_NAPARI_IMAGE_LABELS_PLAN.md) and
[`../../../spec/scene/integration/napari/NAPARI.md`](../../../spec/scene/integration/napari/NAPARI.md)
for semantics. Add pressure tests for sparse ids and ids that exceed ordinary small categorical
indices.

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

Prefer this lane when the goal is RC1 confidence. Prefer vector/arrow or splat when the goal is a
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

For required v0.4 gallery scope, start with **retained textured mesh** and pressure it with a
terrain or planet-surface capture. For an additional shiny feature, start with **vector/arrow
visual**, then immediately pressure it with a wind-field showcase. For RC1 rigor after those, use
**raw label-id GPU probing** and the **explanatory layout proof**. For maximum visual novelty after
the release proof is stable or the visual lands cleanly, add the **splat visual** as a v0.4
experimental showcase.
