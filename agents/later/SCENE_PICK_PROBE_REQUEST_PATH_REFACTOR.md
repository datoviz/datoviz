# Scene Pick/Probe Request Path Refactor

> **Execution Status**
> - **Status:** `POST-V0.4 REFACTOR BACKLOG`
> - **Updated on:** `2026-05-26`
> - **Purpose:** record the current pick/probe findings after the old `scene.c` request-path split.

> **Superseded for new work:** for implementation planning, use
> [`../soon/scene/SCENE_GPU_QUERY_OVERHAUL.md`](../soon/scene/SCENE_GPU_QUERY_OVERHAUL.md). For
> durable semantics, use
> [`../../spec/scene/interaction/GPU_QUERY_SYSTEM.md`](../../spec/scene/interaction/GPU_QUERY_SYSTEM.md).
> This file remains useful as an audit snapshot of the pre-overhaul request path.

## Context

The original monolithic `scene.c` request path has already been split into files such as
`request_queue.c`, `hit_test.c`, `probe_plan.c`, and `request_execute.c`. That split removed a large
structural hotspot, but the current aggregation point is now `src/scene/request_execute.c`.

`request_execute.c` still mixes generic request orchestration with visual-family policy. It owns
auxiliary runtime preparation, readback execution, visual-family candidate checks, pick/probe plan
builders, and result decoding. This is workable for the current vertical slice, but it makes new pick
targets and richer probe payloads easy to add inconsistently.

## Current Findings

1. The C examples found during the 2026-05-26 audit use the public request API rather than direct
   CPU-side hit testing. Relevant examples are `examples/c/techniques/pick_hover.c`,
   `examples/c/techniques/image_probe.c`, and `examples/c/techniques/scheduler_lab.c`.
2. `scheduler_lab.c` still has two details worth revisiting: it flips image probe Y coordinates in
   example code, and it uses CPU-side `point_colors[]` as metadata after a GPU pick. The latter is
   not a CPU pick fallback, but it is example-specific post-pick lookup logic.
3. The current true CPU probe path found in the scene runtime is the retained volume-slice sample
   probe in `src/scene/request_execute.c`. No audited C example currently queues that
   `DVZ_SCENE_TARGET_SAMPLE` path.
4. GPU item picking currently covers point, pixel, marker, sphere, segment/stroke, path, primitive,
   mesh, image, and volume proxy visuals through resolver/planning paths.
5. Glyph, text, and labels have no implemented GPU picking path yet.
6. The public capability and target model is wider than the executor. Capability flags include object,
   item, vertex, face, pixel, sample, and group; the native pick executor currently gates on item
   picking and supports only none/item pick targets.
7. Payload fields are still narrower than the public structs imply. `instance_id`,
   `has_data_position`, and `data_position` are not populated by the current pick executor, and there
   is no mesh face/region, image texel, or volume ray-hit payload yet.
8. `DvzQueryHitPolicy` is carried by requests but is not materially applied during native execution.
   Current behavior is effectively one frontmost accepted result.
9. Some implemented paths are approximate or proxy-based: marker picking uses pixel/bounds-style
   acceptance rather than exact SDF/shape semantics; image picking resolves the quad/item rather than
   alpha-aware texels; volume picking resolves proxy geometry rather than a DVR/MIP ray hit; primitive
   and mesh item IDs are primitive-oriented rather than semantic face/region IDs.
10. Fixed-controller attachments are skipped as pick candidates today.
11. Visual-specific logic is not limited to the request path. Other broad files still carrying
    visual-family policy include `visual_shader_desc.c`, `visual_pass_caps.c`, `visual_uploads.c`,
    and `panel_render_emit.c`.

## Refactor Direction

Keep the already-split queue and result plumbing generic. In particular, `request_queue.c` should
remain responsible for request lifetime, freshness, and result queues rather than visual semantics.

For execution, either keep `request_execute.c` as the top-level orchestration file or split it
further, but move visual-family decisions behind a small internal table or operation layer. A
practical shape is an internal descriptor such as `DvzScenePickFamilyOps` with:

1. visual type/family and supported request targets,
2. supported pick/probe capability flags,
3. candidate predicate,
4. plan builder,
5. optional payload decoder/result mapper,
6. fixed-controller eligibility,
7. short exactness notes for approximate or proxy paths.

Use capability checks and operation tables instead of new open-coded family switches where possible.
This should make unsupported targets explicit, avoid silent fallbacks, and keep new visual families
from adding one-off request behavior in generic execution code.

Do not add CPU-side visual picking fallback paths. The scene picking spec requires GPU-render-path
based visual picking so visibility, transforms, panel scissor, depth, and visual-specific shaders stay
authoritative. The retained volume-slice sample probe is a separate sampled-field probe path and
should stay clearly documented as such.

## Feature Work To Keep Separate

The structure cleanup should preserve current behavior first. These feature gaps should be handled in
separate, focused changes unless a bug fix requires otherwise:

1. exact marker shape and rotation semantics,
2. mesh face/region and vertex target payloads,
3. image pixel/texel and alpha-aware picking,
4. glyph, text, and labels picking,
5. volume DVR/MIP ray-hit or voxel/sample payloads,
6. hit policy execution beyond the current frontmost-result behavior,
7. population of `instance_id` and `data_position` where the backing visual can provide them.

## Validation

For pure documentation updates, `git diff --check` is enough. For code changes in this area, use:

1. `just build`
2. `just test scene` or the narrowest available pick/probe filter
3. manual or automated smoke coverage for `pick_hover`, `image_probe`, and `scheduler_lab`
4. Vulkan validation-layer smoke testing when changing runtime readback, render targets, command
   buffers, or synchronization
