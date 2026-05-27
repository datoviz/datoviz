# GPU Query System

> **Execution Status**
> - **Status:** `ACTIVE DESIGN`
> - **Updated on:** `2026-05-27`
> - **Purpose:** define the long-term v0.4 scene query architecture: one GPU-only panel query
>   system replacing the separate public pick/probe request model.

This document is the durable design record for the pick/probe overhaul. Implementation pickup order
lives in
[`../../../agents/soon/scene/SCENE_GPU_QUERY_OVERHAUL.md`](../../../agents/soon/scene/SCENE_GPU_QUERY_OVERHAUL.md).

The v0.4 branch may break API and ABI to get this right. Do not preserve the current pick/probe
surface if it keeps the architecture ambiguous.


## Summary

The scene should expose one authoritative question:

> What rendered scene contribution is under this panel coordinate?

The answer is a query result. It may contain identity, visual-family payload, sampled value,
formatted readout, and selection/link metadata, but the hit itself must be determined by the GPU path
that matches rendering. Public `pick` and `probe` should become transitional implementation details
or be removed.

The query system must:

1. force GPU-backed picking/probing for rendered visuals,
2. forbid CPU-side visual hit testing and CPU-side sampled visual fallbacks,
3. keep visual-specific policy in visual-specific files,
4. keep generic query orchestration free of visual-family internals,
5. stay compatible with a future WebGPU backend where practical,
6. expose unsupported capability as an explicit result, never as a silent fallback.


## Non-Negotiable Invariants

1. Rendered visual queries are GPU-authoritative.
2. CPU code may queue requests, track freshness, map GPU-returned opaque IDs to scene handles or
   labels, format readout text, and mutate selection/link state after a valid result.
3. CPU code must not decide rendered hits, reconstruct visual geometry for query-time hit tests, or
   sample retained visual data as a fallback for a failed or unsupported GPU query.
4. Unsupported query precision returns an explicit unsupported/failure status.
5. A frontmost unsupported visual must not silently fall through to a background visual as if the
   background were the semantic hit.
6. Query capability is a visual-family contract, not a caller-side example convention.
7. Visual-specific query code lives under `src/scene/visuals/<family>/query.c` or a sibling
   family-specific file.
8. Generic query code under `src/scene/query/` must not contain visual type switches, direct
   `_attr_index()` calls, or direct access to `visual->field`, `visual->texture`, `visual->volume`,
   `visual->path`, `visual->mesh`, `visual->attrs`, or `visual->buffer`.


## Public API Direction

Replace the public pick/probe split with one panel query API. The exact C signatures may change
during implementation, but the intended shape is:

```c
int dvz_panel_query(DvzPanel* panel, double x, double y, const DvzQueryRequest* request);
bool dvz_scene_poll_query(DvzScene* scene, DvzQueryResult* out_result);
uint32_t dvz_figure_process_queries(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps);
```

An optional blocking helper may exist for tests/tools:

```c
int dvz_panel_query_now(
    DvzPanel* panel, DvzDrp2Runtime* runtime, double x, double y,
    const DvzQueryRequest* request, DvzQueryResult* out_result);
```

The old APIs:

```c
dvz_panel_pick(...)
dvz_panel_probe(...)
dvz_scene_poll_pick(...)
dvz_scene_poll_probe(...)
dvz_figure_process_requests(...)
```

should be removed from the public contract or kept only as private migration shims until all examples
and tests are converted. They should not remain as a parallel public model.


## Result Shape

`DvzQueryResult` should subsume current `DvzPickResult` and `DvzProbeResult` fields. Reserve space for:

1. request id and freshness serial,
2. status and hit/miss flag,
3. panel id and panel-local logical coordinate,
4. framebuffer pixel coordinate,
5. visual id and visual family,
6. raw GPU query profile and payload version,
7. raw target kind and raw id,
8. resolved target kind and resolved id,
9. item, group, auxiliary, instance, face, primitive, vertex, voxel, or texel ids as applicable,
10. link key and link channel when applicable,
11. visual-local coordinate,
12. data coordinate or UVW coordinate,
13. depth or front-to-back order metadata,
14. displayed RGBA when available,
15. value kind: none, scalar, vector, category, text, or opaque family payload,
16. scalar/vector/category fields,
17. label/unit/scale/readout metadata.

Statuses should distinguish at least:

1. hit,
2. miss,
3. outside panel,
4. stale or dropped request,
5. no capable visual,
6. unsupported target,
7. unsupported visual family,
8. unsupported query profile or GPU format,
9. GPU execution failure,
10. readback failure,
11. decode failure.

A miss is not a capability failure. A capability failure is not a background hit.


## Query Profiles And Formats

The query format ladder should be explicit and backend-neutral. Preferred profiles:

1. `DVZ_QUERY_PROFILE_U32_R32`: one `r32uint` identity attachment; required baseline if possible.
2. `DVZ_QUERY_PROFILE_U64_RG32`: one `rg32uint` identity attachment; preferred when supported.
3. `DVZ_QUERY_PROFILE_U64_2XR32`: two `r32uint` attachments; fallback when `rg32uint` is absent.
4. `DVZ_QUERY_PROFILE_PACKED_RGBA8`: packed `rgba8uint` or equivalent, only as a GPU-only compact
   fallback when it preserves the requested semantic precision.
5. `DVZ_QUERY_PROFILE_UNSUPPORTED`: explicit failure when no acceptable GPU path exists.

Do not use `rgb*` render-target formats for query payloads. Three-component render-target support is
not reliable enough and is awkward for WebGPU parity.

Identity and values should stay separate:

1. identity target: object/visual/item/family ids,
2. auxiliary target: face/primitive/voxel/texel/group ids,
3. value target: scalar/vector/category payload,
4. coordinate target: visual/data/UVW position when needed.

The scene should prefer one query pass per family when possible, but it is acceptable to use an
identity pass followed by a GPU value/readout pass for complex families.


## WebGPU Compatibility Constraints

The query design should avoid Vulkan-only assumptions:

1. use logical Datoviz/DRP2 texture formats, not raw `VkFormat` as the architectural contract,
2. prefer render attachments plus copy/readback over fragment shader storage-buffer writes,
3. avoid native 64-bit integer render-target assumptions,
4. avoid relying on backend primitive-id builtins as the only source of semantic ids,
5. keep WGSL shader variants in the validation path for query fixtures,
6. validate copy row-pitch and format constraints through DRP2 capability data.

Vulkan may still map the logical formats to `VkFormat` internally. The public DRP2/scene contract
should not require callers or scene plans to think in Vulkan enum values.


## Capability Model

`DvzCapabilitySnapshot` needs query-relevant facts, not only coarse render booleans. Add fields or a
nested query capability record for:

1. `supports_readback`,
2. `min_texture_copy_bytes_per_row_alignment`,
3. supported texture formats,
4. supported render-target formats,
5. supported query profiles,
6. max color attachments,
7. max readback size,
8. shader language support for GLSL and WGSL query variants,
9. storage texture support if any later query profile uses it,
10. capability diagnostics naming the missing requirement.

Scene planning should derive visual-family query support from these facts. For example, a 64-bit
item-id request on a runtime that only supports `r32uint` must either choose an explicitly valid
fallback profile or return unsupported.


## FramePlan And DRP2 Needs

The current scene readback path is too narrow. Query readback needs FramePlan nodes that describe:

1. source texture/resource id,
2. source attachment index where relevant,
3. source origin,
4. copy width, height, and depth,
5. logical format,
6. bytes per texel or block,
7. bytes per row,
8. rows per image,
9. destination buffer id,
10. destination offset,
11. total byte size,
12. request id/readback correlation id.

DRP2 already has texture-to-buffer copy and readback submission primitives. The missing work is mostly
scene planning, capability exposure, logical format cleanup, and tests for integer render targets and
multi-output query payloads.


## Source Layout

Target layout:

```text
src/scene/query/
  queue.c
  execute.c
  readback.c
  registry.c
  result.c
  internal.h

src/scene/visuals/point/query.c
src/scene/visuals/pixel/query.c
src/scene/visuals/marker/query.c
src/scene/visuals/sphere/query.c
src/scene/visuals/segment/query.c
src/scene/visuals/path/query.c
src/scene/visuals/primitive/query.c
src/scene/visuals/image/query.c
src/scene/visuals/labels/query.c
src/scene/visuals/mesh/query.c
src/scene/visuals/volume/query.c
src/scene/visuals/text/query.c
src/scene/visuals/glyph/query.c
```

Since the directory already carries the visual family, file names should be short: `query.c`,
`upload.c`, `pipeline.c`, `shader.c`, etc. Avoid redundant names such as `query_point.c` inside
`visuals/point/`.

Generic query files may traverse panel attachments and call registered family ops. They must not know
how image UVs, labels ids, mesh faces, path joins, or volume rays work.


## Internal Operations Contract

A visual-family query operation table should be the boundary between generic query orchestration and
family policy:

```c
typedef struct DvzSceneQueryFamilyOps
{
    DvzVisualType visual_type;
    DvzSceneVisualFamily family;
    uint32_t capabilities;
    bool (*eligible)(const DvzPanelAttach* attach, const DvzQueryRequest* request);
    bool (*build_plan)(DvzQueryBuildContext* ctx, DvzQueryPlan* out_plan);
    bool (*decode)(const DvzQueryDecodeContext* ctx, DvzQueryHit* out_hit);
    bool (*readout)(const DvzQueryReadoutContext* ctx, DvzQueryResult* result);
} DvzSceneQueryFamilyOps;
```

The exact names can change, but the separation must remain:

1. generic queue/freshness lives in query core,
2. generic readback/capability/profile selection lives in query core,
3. family-specific eligibility and payload construction lives in visual-family files,
4. family-specific decode/readout lives in visual-family files,
5. result finalization and public status mapping lives in query core.


## Visual Family Policies

### Points, Pixels, Markers, And Spheres

Initial payload:

1. visual id,
2. item id,
3. optional instance id,
4. optional link key,
5. displayed RGBA.

Markers should eventually use the marker shader's shape/discard semantics rather than broad proxy
bounds. If exact marker query is not implemented, report the exactness limitation explicitly in the
family operation.

### Segments And Paths

Initial payload:

1. item or segment id,
2. group/subpath id when available,
3. optional path distance,
4. displayed RGBA.

Do not build query-time CPU tessellation as the long-term strategy. Stroke/path query should reuse the
same GPU cache or GPU-side generated buffers used by rendering.

### Primitive And Mesh

Initial payload:

1. visual id,
2. instance id when available,
3. primitive or face id,
4. semantic region/group id when available,
5. optional barycentric coordinate,
6. optional visual/world hit position.

Do not rely solely on backend primitive-id builtins. WebGPU parity and semantic stability are better
served by explicit GPU query metadata buffers or flat attributes that carry face, region, source, and
instance ids.

### Images

Initial payload:

1. visual id,
2. image item id for multi-image visuals,
3. texel or pixel coordinate,
4. UV/data coordinate,
5. displayed RGBA,
6. source value when available through a GPU readout path.

Image query must use the GPU texture or equivalent GPU query pass. Example code should not flip
coordinates or combine separate point-pick/image-probe results.

### Labels

Initial payload:

1. visual id,
2. integer label id,
3. texel coordinate,
4. UV/data coordinate,
5. category label after CPU maps the GPU-returned id.

The GPU must return the raw integer label id. CPU category lookup and text formatting are allowed
after a valid GPU result. CPU sampling of retained labels data is not allowed.

### Volumes

Implementation order:

1. slice query first,
2. MIP query later,
3. DVR/composite query later.

Slice query payload:

1. visual id,
2. UVW coordinate,
3. voxel coordinate,
4. sample id,
5. scalar/vector/category value read by the GPU,
6. displayed RGBA when useful.

Recommended deferred policies:

1. MIP returns the sample that won the maximum-intensity comparison.
2. DVR/composite returns the first opacity-threshold crossing.
3. The default opacity threshold should be configurable; `0.5` is a reasonable first policy.
4. A max-contribution composite policy can be added later as a separate query mode.

Until these semantics are implemented, DVR/composite volume query should return unsupported rather
than a CPU ray or proxy-geometry approximation.

### Text, Glyphs, And Annotations

Initial payload:

1. object/string/annotation id,
2. optional glyph id later,
3. optional character cluster later.

Glyph-level precision can be deferred. Object-level GPU identity should be the first target.


## Transparency And Compositing

First implementation supports opaque and depth-tested frontmost query. Transparent query support is a
separate capability profile.

Do not let WBOIT, depth peeling, alpha blending, SSAO, EDL, bloom, or other postprocess effects
silently redefine query semantics. Each technique must either:

1. declare that ordinary depth/frontmost query semantics still apply,
2. provide a technique-specific query path,
3. return unsupported for the requested query mode.

For blended visuals, a future query mode may support full hit stack or contribution-weighted results.
That should be explicit API behavior, not accidental fallthrough.


## Enforcement

Add a source check that fails if generic query files contain visual-family internals. At minimum,
forbid these patterns in `src/scene/query/`:

1. `DVZ_VISUAL_TYPE_`,
2. `_attr_index(`,
3. `visual->attrs`,
4. `visual->field`,
5. `visual->texture`,
6. `visual->volume`,
7. `visual->path`,
8. `visual->mesh`,
9. `visual->buffer`,
10. `visual->scale` except in result formatting helpers explicitly owned by visual-family code.

Tests should also cover:

1. no CPU fallback when GPU readback fails,
2. unsupported result when the runtime lacks the required query profile,
3. frontmost unsupported visual does not fall through silently,
4. stale request rejection,
5. labels/volume do not read retained CPU data for query hits,
6. WebGPU/WGSL query fixture preflight where possible.


## Existing Code Audit Notes

Current code implications recorded on 2026-05-27:

1. `src/scene/request_execute.c` mixes generic request orchestration, visual-family policy, GPU
   readback, labels probing, volume probing, and result decode.
2. Current scene readback is hardcoded around a one-pixel, four-byte RGBA payload.
3. Current identity encoding is a 24-bit RGB item id, not a robust query payload format.
4. Labels probing currently computes UVs from retained CPU attrs and reads from retained field data
   through a temporary GPU copy path.
5. Volume slice probing currently uses CPU ray/box math and CPU sampled-field reads.
6. DRP2 already has explicit texture formats, color attachments, texture-to-buffer copies, and
   readback submissions.
7. DRP2 specs already mention supported render-target formats and readback capabilities, but the
   scene capability snapshot is still too coarse.
8. `r32uint` and `rg32uint` already exist in DRP2 schema/serialization, but scene query planning does
   not yet use them.


## Relationship To Other Specs

1. [`PANEL_QUERY.md`](PANEL_QUERY.md) records the high-level public interaction direction.
2. [`PICKING.md`](PICKING.md) records scene identity requirements that query results must preserve.
3. [`../integration/napari/NAPARI.md`](../integration/napari/NAPARI.md) records downstream payload
   pressure from points, image, labels, mesh/surface, and volume layers.
4. [`../../../spec/drp2/CAPABILITIES.md`](../../../spec/drp2/CAPABILITIES.md) records the DRP2
   capability model that query planning should extend.
