> **Execution Status**
> - **Status:** `PARTIALLY PROMOTED`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve cross-family probe/readout rationale while active rules move into
>   picking, annotation, visual-family, and API specs.

# Probe and Readout Design

This is a promoted proposal record. It preserves the distinction between picking an identity and
explaining the semantic value behind that identity.


## Decision Addressed

Picking identifies what was hit. Probe/readout resolves what that hit means: coordinates, sampled
values, labels, units, scale interpretation, and display strings for overlays or external UI.


## Canonical Specs

Active rules moved to:

1. [`../../interaction/PICKING.md`](../../interaction/PICKING.md) for identity and readback routing.
2. [`../../api/API_SURFACE.md`](../../api/API_SURFACE.md) for `DvzProbeResult` shape.
3. [`../../semantics/ANNOTATIONS.md`](../../semantics/ANNOTATIONS.md) for probe annotations,
   pinned readouts, overlays, and invalidation.
4. [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md) and
   [`../../visuals/VOLUME.md`](../../visuals/VOLUME.md) for image and volume payload rules.
5. [`../active/SCIENTIFIC_COORDINATE_NORMALIZATION.md`](../active/SCIENTIFIC_COORDINATE_NORMALIZATION.md),
   [`AXES_DOMAIN_DESIGN.md`](AXES_DOMAIN_DESIGN.md), and
   [`COLORBAR_COLORMAP_DESIGN.md`](COLORBAR_COLORMAP_DESIGN.md) for remaining
   active coordinate, domain, and scale/colorbar design.


## Probe Payload Details To Preserve

The baseline probe payload should reserve room for:

1. panel and visual identity,
2. payload kind and payload-local id,
3. logical, world, or scientific coordinate,
4. scalar, vector, categorical, or missing-value payload,
5. units,
6. formatted display text,
7. scale or colormap reference when value interpretation is mapped,
8. source pick result id when derived from a pick.

Do not make callers reconstruct semantic values from raw pick ids alone.


## Family Expectations To Keep As Backlog

1. `image`: pixel/cell coordinate plus sampled scalar or vector value when available.
2. `volume` slice: slice coordinate, world/scientific coordinate, and sampled value.
3. `point` and repeated-item families: item identity plus retained semantic coordinates or
   per-item values when available.
4. `mesh`: face identity first, with derived coordinate or attached metadata later.


## Policy Notes Not Fully Canonical Yet

1. Probe-only behavior and probe-plus-selection behavior should remain separate policy choices.
2. A click may both select and produce a probe, but persistent selection should not be required for
   one-off readout.
3. Persistent image/slice selection should store stable pixel or cell identity; formatted
   coordinates and values should be derived by probe resolution.
4. Transient hover readout and pinned readout state should remain separate.
5. Pinned readouts should be retained scene-owned annotations, not just “last hover frozen in
   place.”
6. Empty clicks or misses should clear transient probe state by default, with policy-controlled
   exceptions for pinned or inspector workflows.


## Remaining Backlog

1. Image pixel readout with sampled value.
2. Volume slice readout with sampled value and scientific coordinate.
3. Point/repeated-item identity-to-coordinate readout.
4. Shared formatting hooks for units and domain-aware display.
5. Explicit capability reporting for identity-only picking versus richer sampled-value readout.


## Non-Goals Still Valid

1. Designing every future analysis/probe tool now.
2. Full multi-sample line or area probes.
3. Forcing every visual family to produce the same rich payload immediately.
4. Conflating annotation ownership with probe state ownership.
