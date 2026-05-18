> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define what the current v0.4 scene design must preserve so a future ray-tracing
>   path can land without forcing a public scene-API rewrite.

# Ray Tracing Forward Compatibility

This note does not design a ray tracer. It records the active architectural implications of wanting
future ray-tracing support while the current implementation remains raster-first.


## Objective

Preserve a scene design that can support future ray-traced rendering for selected workflows without
making today’s mesh, volume, transparency, or lighting APIs rasterization-shaped.


## Existing Grounding In The Repo

There is already broad future-facing guidance here:

1. capability adaptation:
   [spec/scene/validation/ADAPTATION.md](../../validation/ADAPTATION.md)
2. lighting forward-compatibility:
   [spec/scene/semantics/LIGHTING.md](../../semantics/LIGHTING.md)
3. active material/light direction:
   [spec/scene/proposals/active/MATERIAL_LIGHTING_API.md](../active/MATERIAL_LIGHTING_API.md)

This note narrows the implications for the active v0.4 design set.


## Core Recommendation

Treat future ray tracing as a capability-driven runtime realization choice, not as a separate scene
API.

Recommended rule:

1. scene objects, materials, lights, transforms, picking semantics, and annotation semantics remain
   scene-level concepts,
2. runtime may realize a given frame plan through rasterization today and ray tracing later,
3. the public scene API should avoid assumptions that only make sense for raster passes.


## What Should Not Change Later

The following current design choices are good and should remain stable if ray tracing arrives:

1. scene owns semantic object identity,
2. materials and lights are scene-visible objects,
3. transforms and scientific normalization are scene concerns,
4. transparency and picking semantics are scene-visible policies,
5. capability adaptation is explicit rather than accidental.

These are exactly the decisions that keep a future ray path plausible.


## Current Implication For Mesh Design

The mesh API should continue to avoid raster-only assumptions.

Good current directions:

1. mesh resource identity is explicit,
2. material state is semantic rather than shader-layout-shaped,
3. instancing is semantic rather than “one draw call trick”,
4. picking results are logical identities rather than attachment ids.

This is the right foundation whether the future renderer emits triangles to raster or traces them.


## Current Implication For Volume Design

Volume is one of the strongest future ray-tracing pressure points.

Recommended current rule:

1. keep `volume` semantics about sampled fields, slice modes, transfer settings, and probes,
2. do not define the family purely in terms of one raster ray-march shader,
3. allow future runtime paths to realize those semantics differently.

This matters for future higher-quality DVR, shadows, and mixed scene effects.


## Transparency Implications

Transparency is one of the areas most likely to differ between raster and future ray paths.

Recommended current rule:

1. keep transparency as a scene-visible render/compositing policy,
2. do not define public semantics in terms of “WBOIT buffers” alone,
3. let WBOIT remain the active raster target while preserving higher-level transparency intent.

That makes it possible for a future ray path to honor the same semantic intent without exposing a
different public API.


## Picking Implications

Picking semantics should remain scene-semantic, not raster-surface-specific.

Recommended current rule:

1. pick results stay expressed as visual/instance/face/item/pixel identities,
2. hit-selection policy remains semantic,
3. future ray-based hit queries should be able to feed the same `DvzPickResult` shape.

This is one reason the picking note now emphasizes logical results and multi-hit policy.


## Material And Lighting Implications

Future ray tracing should not force a second material and lighting model.

Recommended current rule:

1. keep semantic materials and panel light sets,
2. reserve room for PBR in the material model,
3. avoid public APIs tied to one raster shader’s uniform packing.

This is already the direction of `MATERIAL_LIGHTING_API.md`.


## Resource And Runtime Boundary

Acceleration structures, BVHs, and related GPU-specific concerns belong below the scene boundary.

Recommended rule:

1. scene never exposes BVH or AS handles,
2. scene never requires users to build backend-specific acceleration structures,
3. runtime decides whether additional derived resources are needed for the chosen capability path.

This mirrors the current DRP2/runtime separation.


## Capability Policy

Ray tracing should be a standard capability/adaptation class, not a special escape hatch.

Recommended policy:

1. `off` by default,
2. `preferred` when applications want higher quality if available,
3. `required` only for workflows that truly need it.

Fallback should remain deterministic and scene-visible through diagnostics.


## What Current Specs Should Avoid

If we want future ray tracing, the current active specs should avoid:

1. public APIs phrased in terms of render passes or attachment topology,
2. transparency semantics defined only by raster implementation details,
3. picking semantics that assume one raster identity buffer is the only path,
4. volume semantics defined purely as one fragment-shader technique,
5. material APIs shaped around one specific shader packing convention.


## Immediate Scope Recommendation

The active implication is modest but important:

1. keep current APIs semantic,
2. keep capability adaptation explicit,
3. allow current raster-first implementation choices without freezing them into the public contract.

No active code path needs to implement ray tracing now for this note to be useful.


## Explicit Non-Goals For This Note

1. specifying a ray-tracing pipeline or BVH format,
2. adding ray-tracing implementation work to the current roadmap,
3. replacing the active raster-first Phase 1 goals,
4. inventing a second scene API just for future ray features.
