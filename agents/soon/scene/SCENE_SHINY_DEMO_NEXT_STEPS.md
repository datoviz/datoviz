# Scene Shiny Demo Next Steps

> **Execution Status**
> - **Status:** `RECOMMENDED FOLLOW-UP DISPATCH`
> - **Updated on:** `2026-05-26`
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


## Recommended Order

| Priority | Lane | Why it is next | First useful slice |
| ---: | --- | --- | --- |
| 1 | Vector/arrow visual | Best new semantic visual for scientific demos; unlocks wind, CFD, displacement, normals, trajectories, and napari-style vectors. | Add a first-class arrow/vector visual or tightly scoped arrow helper, then use it in the wind-field showcase. |
| 2 | Raw label-id GPU probe | Smaller than a new visual family and directly improves the napari-style labels demo. | Return integer label ids from labels fields through the scene request/readback path; add sparse/high-id stress tests. |
| 3 | Explanatory layout proof | Low-risk RC proof for already-landed adornment layout pieces. | Add or polish one example combining axes, colorbar, legend, scale bar, and panel reserves without collisions. |
| 4 | Gallery proof pass | Converts existing showcase code into release confidence. | Run/polish protein, LiDAR, brain, labels, colorbar/legend, and capture paths; tune defaults and record validation. |
| 5 | Splat visual | Flashy and self-contained, but it is a new visual family and should follow the smaller proof lanes unless visual impact is the priority. | Implement a narrow point-cloud splat/Gaussian-like visual with a dense example and explicit deferred scope. |


## Lane Notes

### Vector/Arrow Visual

No public `dvz_vector()` or `DVZ_VISUAL_TYPE_VECTOR` is installed. Wind-field-style examples can
temporarily use primitives, but that should not become the permanent semantic model.

Use [`SCENE_VECTOR_VISUALS_PLAN.md`](SCENE_VECTOR_VISUALS_PLAN.md) as the execution starting point.
Keep the first slice narrow: positions, vectors, color, length/scale, optional arrowhead style, and
panzoom/arcball-compatible transforms. Defer tensor glyphs, streamtube generation, vector-field LOD,
and advanced per-item metadata unless the first showcase needs them.

### Raw Label-Id GPU Probe

The labels showcase currently demonstrates rendering, legend integration, and selection, but hover
and click readout should move off temporary CPU coordinate lookup. The target is a GPU-backed labels
probe that returns the integer label id and mapped data coordinates.

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
roadmap material, not active implementation.

Use [`../../later/SPLATTING_TIERED_PLAN.md`](../../later/SPLATTING_TIERED_PLAN.md) and
[`../../../spec/scene/proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](../../../spec/scene/proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md)
before implementation. Keep the first slice deliberately narrow: dense translucent point-cloud
splatting or Gaussian-like screen-space splats, one data-driven example, and explicit deferral of
full differentiable/3D Gaussian-splatting ambitions.


## Practical Choice

For a new shiny feature, start with **vector/arrow visual**, then immediately pressure it with a
wind-field showcase. For RC1 rigor, start with **raw label-id GPU probing** and the **explanatory
layout proof**. For maximum visual novelty after those, start the **splat visual** first slice.
