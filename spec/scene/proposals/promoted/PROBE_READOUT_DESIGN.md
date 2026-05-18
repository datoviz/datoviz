> **Execution Status**
> - **Status:** `PARTIALLY PROMOTED`
> - **Updated on:** `2026-05-16`
> - **Purpose:** preserve cross-family probe/readout rationale while active rules move into
>   picking, annotation, visual-family, and API specs.

# Probe and Readout Design

This promoted note preserves the rationale for turning picks and queries into meaningful semantic
readouts. Current routing, annotation, visual-family, and API rules now live in specialized specs.


## Authority Note

Probe/readout no longer owns the pick identity model. Active picking and readback routing belong in
[`../../interaction/PICKING.md`](../../interaction/PICKING.md); probe annotations and pinned readouts
belong in [`../../semantics/ANNOTATIONS.md`](../../semantics/ANNOTATIONS.md); sampled image and volume
payload rules belong in [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md) and
[`../../visuals/VOLUME.md`](../../visuals/VOLUME.md); public API shape belongs in
[`../../api/API_SURFACE.md`](../../api/API_SURFACE.md).

This proposal remains the cross-family rationale for semantic values, units, formatting, and
probe-only versus probe-plus-selection policy until those details are fully split into specialized
specs.
The detailed sections below are historical design material unless the owning specialized specs
explicitly cite or absorb them.


## Historical Objective

Support coherent probe/readout behavior for:

1. image pixel queries,
2. volume slice and sample queries,
3. mesh face or point queries,
4. axes/domain-aware coordinate display,
5. annotation and external-UI inspectors.


## Existing Grounding In The Repo

Useful current context:

1. promoted picking rationale:
   [PICKING_DESIGN.md](PICKING_DESIGN.md)
2. active volume note:
   [VOLUME_DESIGN.md](../active/VOLUME_DESIGN.md)
3. active scientific-coordinate note:
   [SCIENTIFIC_COORDINATE_NORMALIZATION.md](../active/SCIENTIFIC_COORDINATE_NORMALIZATION.md)
4. active axes/domain note:
   [AXES_DOMAIN_DESIGN.md](../active/AXES_DOMAIN_DESIGN.md)
5. active colorbar/colormap note:
   [COLORBAR_COLORMAP_DESIGN.md](../active/COLORBAR_COLORMAP_DESIGN.md)

This note preserves the cross-family rationale and remaining backlog for semantic readouts.


## Core Recommendation

Picking identifies what was hit. Probing/readout explains what that hit means.

Recommended split:

1. picking returns logical identity and minimal hit structure,
2. probe/readout resolves semantic coordinates, values, labels, and units from that identity,
3. annotations, tooltips, and inspectors consume the probe payload rather than re-deriving it.


## Why A Separate Note Is Needed

Several scene proposals already depend on this contract:

1. image picking wants pixel-level semantics,
2. volume picking wants sampled values and scientific coordinates,
3. axes/domain work wants unit-aware cursor and coordinate display,
4. colorbars want scale-aware value interpretation,
5. selection/highlight should stay distinct from value readout.


## Historical Probe Result Shape

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

1. coordinate formatting should reuse panel/domain formatting policy by default,
2. sampled scalar interpretation may reference the shared scale object,
3. colorbar/legend UI can reflect the same value domain seen by probes,
4. panel-level probe-format defaults may exist on top of the shared formatting machinery,
5. per-readout local overrides may also exist when one readout needs custom presentation,
6. local overrides should layer on top of the shared formatting machinery rather than replacing it
   wholesale.


## Selection Relationship

Selection and probing should stay separate but interoperable.

Recommended rule:

1. a click may both select and produce a probe result,
2. persistent selection state should not be required for one-off readout,
3. images and slices should support both probe-only behavior and optional persistent pixel/sample
   selection through explicit API policy,
4. a selected item may drive a pinned readout annotation later,
5. policy may optionally create pinned readouts automatically from persistent sample selection, but
   this should remain disabled by default.

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

Recommended state split:

1. transient hover readout is separate from pinned readout state,
2. pinned readout should be retained scene-owned state,
3. do not model pinned readout as only “the last hover result frozen in place”,
4. the first slice should allow several pinned readouts rather than only one,
5. when available, a pinned readout should retain both the raw originating hit and the resolved
   semantic payload that was stored from it.

Recommended interaction behavior:

1. pinned readouts should behave as passive views by default,
2. policy may optionally let clicking or focusing a pinned readout reselect or reprobe its target,
3. do not hard-wire pinned readouts into the active selection path.


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


## Remaining Scope Notes

The narrowest useful remaining implementation target is:

1. image pixel readout,
2. volume slice readout with sampled value and scientific coordinate,
3. point or repeated-item identity-to-coordinate readout,
4. common formatting hooks for units and domain-aware display.


## Explicit Non-Goals For The First Slice

1. designing every future analysis/probe tool now,
2. full multi-sample line or area probes,
3. forcing every visual family to produce the same rich payload immediately,
4. conflating annotation ownership with probe state ownership.
