> **Execution Status**
> - **Status:** `ACTIVE PROBE / READOUT DESIGN NOTE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 contract for semantic probe results and readout payloads
>   across image, volume, mesh, and annotation workflows.

# Probe and Readout Design

This note records the active v0.4 direction for turning picks and queries into meaningful semantic
readouts rather than leaving each visual family to invent its own payload shape.


## Objective

Support coherent probe/readout behavior for:

1. image pixel queries,
2. volume slice and sample queries,
3. mesh face or point queries,
4. axes/domain-aware coordinate display,
5. annotation and external-UI inspectors.


## Existing Grounding In The Repo

Useful current context:

1. active picking note:
   [agents/now/PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/PICKING_DESIGN.md)
2. active volume note:
   [agents/now/VOLUME_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VOLUME_DESIGN.md)
3. active scientific-coordinate note:
   [agents/now/SCIENTIFIC_COORDINATE_NORMALIZATION.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCIENTIFIC_COORDINATE_NORMALIZATION.md)
4. active axes/domain note:
   [agents/now/AXES_DOMAIN_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/AXES_DOMAIN_DESIGN.md)
5. active colorbar/colormap note:
   [agents/now/COLORBAR_COLORMAP_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/COLORBAR_COLORMAP_DESIGN.md)

This note defines the active cross-family contract.


## Core Recommendation

Picking identifies what was hit. Probing/readout explains what that hit means.

Recommended split:

1. picking returns logical identity and minimal hit structure,
2. probe/readout resolves semantic coordinates, values, labels, and units from that identity,
3. annotations, tooltips, and inspectors consume the probe payload rather than re-deriving it.


## Why A Separate Note Is Needed

Several active notes already depend on this contract:

1. image picking wants pixel-level semantics,
2. volume picking wants sampled values and scientific coordinates,
3. axes/domain work wants unit-aware cursor and coordinate display,
4. colorbars want scale-aware value interpretation,
5. selection/highlight should stay distinct from value readout.


## Probe Result Shape

Reserve a structured probe payload now.

Recommended baseline fields:

1. panel id,
2. visual id,
3. payload kind,
4. payload-local id when relevant,
5. logical/world/scientific coordinate,
6. one or more sampled or derived values,
7. optional units,
8. optional formatted display string or label text.

Do not make callers reconstruct semantic meaning from raw pick ids alone.


## Family Expectations

Recommended first expectations by family:

1. `image`
   - pixel or cell coordinate
   - sampled scalar or vector value when available
2. `volume` slice
   - slice coordinate
   - world/scientific coordinate
   - sampled value
3. `point` / repeated item families
   - item identity
   - original semantic coordinate or retained per-item value fields when available
4. `mesh`
   - face identity first
   - optional derived coordinate or attached metadata later


## Coordinate Semantics

Probe payloads should report semantic coordinates, not only normalized render coordinates.

Recommended rule:

1. internal normalization/downcast is allowed for rendering,
2. probe/readout converts back to the meaningful scene or domain coordinate frame,
3. units remain attached when known.


## Value Semantics

Values are not always one scalar.

Reserve room now for:

1. scalar values,
2. vector values,
3. categorical labels or region ids,
4. derived values such as intensity windowed through a scale view,
5. missing-value or invalid-sample state.


## Scale And Domain Relationship

Probe results should integrate with scales and axes rather than bypassing them.

Recommended behavior:

1. coordinate formatting may use panel/domain formatting policy,
2. sampled scalar interpretation may reference the shared scale object,
3. colorbar/legend UI can reflect the same value domain seen by probes.


## Selection Relationship

Selection and probing should stay separate but interoperable.

Recommended rule:

1. a click may both select and produce a probe result,
2. persistent selection state should not be required for one-off readout,
3. images and slices should support both probe-only behavior and optional persistent pixel/sample
   selection through explicit API policy,
4. a selected item may drive a pinned readout annotation later.

Recommended identity rule for persistent image/slice selection:

1. store the stable selected identity as pixel or cell index,
2. derive semantic coordinates and formatted readout from probe resolution rather than storing them
   as the primary selection identity.


## Annotation And UI Relationship

Probe payloads are a natural source for overlay and inspector content.

Recommended consumers:

1. hover tooltip,
2. pinned probe label,
3. property inspector,
4. status-bar coordinate readout,
5. linked colorbar/scale indicator later.


## Request Model

Probe/readout should remain explicit and compatible with the picking request model.

Recommended direction:

1. simple probe requests can ride on the same pick/readback path,
2. richer sampled-value queries may require additional readback or semantic resolution work,
3. the public API should expose one coherent query model rather than separate unrelated picking and
   probing systems,
4. interaction policy should be able to decide whether a probe request also mutates persistent
   selection state,
5. empty clicks or misses should clear the current transient probe by default,
6. probe-retention versus clear-on-miss behavior should still be policy-controlled for pinned or
   inspector-driven workflows.


## Capability And Failure Model

Probe richness must be capability-aware and family-aware.

Recommended rule:

1. identity-only picking may be available before rich sampled-value readout,
2. each visual family declares which probe payload fields it can supply,
3. unavailable fields should be absent with explicit meaning, not silently fabricated.


## Public API Direction

The exact names can still move, but the conceptual API should support:

1. scene or panel probe request,
2. structured `DvzProbeResult` or equivalent payload,
3. family-specific semantic resolvers behind one common result shape,
4. formatted text helpers layered on top of the structured payload,
5. explicit policy for probe-only versus probe-plus-persistent-selection behavior where that
   distinction matters.


## Immediate Scope Recommendation

The narrowest useful active implementation target is:

1. image pixel readout,
2. volume slice readout with sampled value and scientific coordinate,
3. point or repeated-item identity-to-coordinate readout,
4. common formatting hooks for units and domain-aware display.


## Explicit Non-Goals For The First Slice

1. designing every future analysis/probe tool now,
2. full multi-sample line or area probes,
3. forcing every visual family to produce the same rich payload immediately,
4. conflating annotation ownership with probe state ownership.
