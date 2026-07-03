# Panel Query

> **Execution Status**
> - **Status:** `ACTIVE IMPLEMENTATION`
> - **Updated on:** `2026-06-27`
> - **Purpose:** replace separate public picking and probing concepts with one authoritative
>   "what is under this panel pixel?" query model.

For the long-term GPU-only architecture and implementation invariants, read
[`GPU_QUERY_SYSTEM.md`](GPU_QUERY_SYSTEM.md). This file records the high-level public interaction
direction; `GPU_QUERY_SYSTEM.md` is the more detailed source of truth for the overhaul.

This document records the aggressive v0.4 direction for scene interaction queries. The branch has
removed the older public pick/probe split in favor of one panel query model.


## Problem

The current model separates two related questions:

1. picking asks which scene identity was hit,
2. probing asks which value a visual can expose at a coordinate.

That split is too weak for composed scenes. A user-facing hover, click, tooltip, inspector, or
selection tool usually asks one question instead:

> What is under the cursor in the rendered panel?

When a point visual is drawn over an image visual, example code should not manually queue a point
pick and an image probe, flip coordinates by hand, and decide which result wins. That policy belongs
to the scene query system because it depends on the same visibility, ordering, transforms, clipping,
and alpha rules as rendering.


## Direction

The public interaction primitive should become a unified panel query API:

```c
int dvz_panel_query_px(DvzPanel* panel, double x, double y, const DvzQueryRequest* request);
int dvz_panel_query_data(DvzPanel* panel, double x, double y, const DvzQueryRequest* request);
bool dvz_panel_transform_point(
    DvzPanel* panel, DvzPanelCoordSpace from, DvzPanelCoordSpace to, double x, double y,
    double* out_x, double* out_y);
```

`dvz_panel_query_px()` takes `DVZ_PANEL_COORD_PANEL_PX` coordinates: logical pixels local to the
outer panel rectangle. `dvz_panel_query_data()` converts panel data coordinates through the same
public transform helper before queuing the query.

This replaces separate public pick and probe APIs rather than wrapping them indefinitely. Pick and
probe remain useful semantic words, but the public request/result mechanism is query.


## Result Model

A panel query result should be able to report:

1. request id,
2. status,
3. panel id,
4. framebuffer pixel coordinate,
5. outer-panel-local logical-pixel coordinate,
6. visual id,
7. visual family,
8. item id when applicable,
9. group id when applicable,
10. auxiliary family payload when applicable,
11. visual-local coordinate,
12. data or world coordinate when available,
13. displayed RGBA,
14. depth, z layer, or draw-order metadata,
15. optional sampled value or formatted semantic readout.

The status must distinguish at least:

1. hit,
2. miss,
3. outside panel,
4. unsupported visual or payload,
5. no capable visual,
6. GPU execution failure,
7. readback failure,
8. stale or dropped request.

A miss is not the same as an unsupported query or a failed readback.


## Query Authority

The authoritative implementation should be a dedicated query render/readback path.

The query pass should use the same scene composition rules as ordinary rendering:

1. panel viewport and scissor,
2. visual transforms,
3. controller modes such as fixed, panzoom, arcball, fly, and turntable,
4. visibility and hidden state,
5. z layer and draw order,
6. depth testing where the visual uses it,
7. clipping and discard rules,
8. alpha policy once defined,
9. image coordinate conventions,
10. per-visual query capability.

The preferred path is a tiny query target, ideally one pixel or a small tile, that encodes scene
identity and enough payload to recover the semantic result. The readback should be asynchronous and
integrated with the existing scene/app request machinery.

CPU-side shortcuts can be useful for tests or degenerate cases, but they should not define the
public semantics for composed rendered scenes.


## Full Hit Stack

The default query should return the frontmost hit. The API should still be designed so callers can
request a full hit stack later:

1. all capable hits under the panel pixel,
2. sorted by the same front-to-back policy as rendering,
3. with each hit carrying the same identity and readout fields as the frontmost result.

This matters for dense scatter over images, translucent overlays, annotations, selection cycling,
and inspectors that need to show several overlapping candidates.


## Visual Responsibilities

Each visual family should provide a query encoder or explicitly report that it is not queryable.

Initial expectations:

1. points, pixels, markers, segments, paths, spheres: item or group identity and displayed color,
2. images: texel or data-cell identity, sampled displayed color, and source value when available,
3. meshes and primitives: object, primitive, face, item, or group identity according to retained
   family data,
4. volumes and slices: voxel or sample coordinate and displayed value when available,
5. text and annotations: object or label identity first, with finer glyph payload deferred.

Unsupported visuals must not silently fall through in a way that lets a background visual appear to
be the frontmost semantic result.


## API Break Policy

Because v0.4 does not need to preserve the v0.3 API, the implementation plan should be willing to:

1. keep public pick/probe entry points removed now that panel query replaces them,
2. rename request/result types around `query` rather than `pick` or `probe`,
3. keep examples on one panel query instead of per-example composition of pick/probe requests,
4. centralize coordinate conversion in public panel transform helpers and the scene/query layer,
5. make query capability explicit in visual-family metadata,
6. treat image probing and point picking as query payload specializations.

Keeping both public models would make interaction behavior harder to reason about and would keep the
same class of bugs alive in examples and downstream tools.


## Migration Shape

The transition should be short and explicit:

1. define the query request/result structs and capability flags,
2. add a frame-plan/readback representation for query passes,
3. implement the authoritative path for the point-over-image case first,
4. convert `scheduler_lab` to issue one panel query instead of manual pick plus probe composition,
5. keep current tests on query result assertions,
6. keep obsolete public pick/probe APIs deleted,
7. extend visual-family encoders one family at a time.

During this migration, any temporary bridge should be internal and marked as such. It should not
become a compatibility promise.


## Relationship To Existing Notes

[`PICKING.md`](PICKING.md) remains useful for identity-level requirements. The query model absorbs
those identity requirements but broadens the contract from "what identity was picked?" to "what
rendered scene contribution is under this panel pixel?"

[`../../proposals/promoted/PROBE_READOUT_DESIGN.md`](../proposals/promoted/PROBE_READOUT_DESIGN.md)
remains useful for semantic readout rationale. The query model treats readout as payload attached to
the hit, not as a separate public system that callers manually reconcile with picking.
