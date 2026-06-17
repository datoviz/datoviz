# Queries

Queries connect rendered output back to retained scene state. Public pick/probe entry points have
been replaced by a unified panel query model.

Use "picking" for identity questions and "probing" for sampled-value questions, but route both
through query APIs.

## Model

| Term | Meaning |
| --- | --- |
| Query | One panel-local question about the rendered scene at a coordinate or region. |
| Picking | Query use case that resolves visual/item/group/instance identity. |
| Probing | Query use case that resolves sampled field, image, label, or volume values. |
| Readback | Runtime operation that returns GPU-produced query payloads to scene/application code. |

The core question is: what rendered scene contribution is under this panel coordinate?

## Current API Shape

The current v0.4 direction uses:

```c
int dvz_panel_query(DvzPanel* panel, double x, double y, const DvzQueryRequest* request);
bool dvz_scene_poll_query(DvzScene* scene, DvzQueryResult* out_result);
uint32_t dvz_figure_process_queries(
    DvzFigure* figure, DvzDrp2Runtime* runtime, const DvzCapabilitySnapshot* caps);
int dvz_panel_query_now(
    DvzPanel* panel, DvzDrp2Runtime* runtime, double x, double y,
    const DvzQueryRequest* request, DvzQueryResult* out_result);
```

Check the generated [scene C API](c-api/scene.md) for exact signatures in the current tree.

## Result Semantics

| Result field class | Examples |
| --- | --- |
| Request identity | request id, freshness serial, panel id. |
| Status | hit, miss, outside panel, stale/dropped, unsupported target/family/profile, GPU/readback/decode failure. |
| Scene identity | visual id, visual family, item/group/instance/primitive/face/voxel/texel ids where available. |
| Coordinates | framebuffer position, panel/data coordinate, visual-local coordinate, UV/UVW or texel coordinate. |
| Values | scalar, vector, category, text, displayed RGBA, or opaque family payload. |

A miss is not a capability failure. A capability failure is not a background hit.

## GPU Authority

Rendered visual queries are GPU-authoritative. CPU code may queue requests, track freshness, decode
GPU payloads, map ids to scene objects, format readouts, and mutate hover/selection state. CPU code
must not decide rendered hits or sample retained visual data as a fallback for unsupported GPU
queries.

This keeps queries aligned with the same transforms, depth, clipping, visibility, shader discard,
and viewport rules that produced the rendered frame.

## Freshness And Delivery

| Use case | Delivery rule |
| --- | --- |
| Hover | Latest request wins; stale results are discarded. |
| Click selection | Result must map to the issuing request or report miss/error. |
| Tool/query helper | Synchronous helper may be used for tests and tools when a runtime is available. |
| Browser/WebGPU | Only the promoted subset is live; unsupported cases must produce explicit diagnostics. |

## Current Support Shape

Support is visual-family and backend specific. The native query implementation is GPU-backed for the
active rendered-query families and exposes unsupported status for gaps. WebGPU live routes currently
cover a promoted subset such as point/marker picking, point hover/selection, sphere and mesh
selection, and image probing.

Do not assume every visual family supports every target kind, value kind, or result payload.

## See Also

- [Pick items](../how-to/pick-and-probe.md)
- [Probe fields](../how-to/probe-fields.md)
- [WebGPU subset](webgpu-subset.md)
- [C scene API](c-api/scene.md)
