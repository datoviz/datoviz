> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve picking rationale, precision backlog, and result-shape notes after
>   promotion into interaction, visual-family, frame-plan, and API specs.

# Picking Design

This is a promoted proposal record. It must not duplicate the active picking contract.


## Decision Addressed

Picking is a scene-level identity system. Backends may render encoded ids and read them back, but
public results should resolve to scene-visible identities, payload kinds, and interaction policy.

The original proposal covered object, item, group, mesh-face, image-pixel, and instanced-mesh
precision. That precision direction is now canonicalized by family in
[`../../interaction/PICKING.md`](../../interaction/PICKING.md).


## Canonical Specs

Active rules moved to:

1. [`../../interaction/PICKING.md`](../../interaction/PICKING.md) for identity levels, family
   expectations, request timing, `FramePlan` participation, diagnostics, and C API sync/hover
   behavior.
2. [`../../interaction/SELECTION.md`](../../interaction/SELECTION.md) for selection and highlight
   state.
3. [`../../api/API_SURFACE.md`](../../api/API_SURFACE.md) and
   [`../../api/API_DESIGN.md`](../../api/API_DESIGN.md) for public result/request shape.
4. [`../../pipeline/FRAME_PLAN.md`](../../pipeline/FRAME_PLAN.md) and
   [`../../pipeline/INVALIDATION_AND_CACHING.md`](../../pipeline/INVALIDATION_AND_CACHING.md) for
   planning and invalidation.
5. The relevant `../../visuals/*.md` specs for family-specific hit identity.


## Precision Details To Keep

These target conventions remain useful as backlog notes where family specs do not yet spell out the
same detail:

| Family | Preferred target detail |
|---|---|
| `pixel`, `point`, `marker`, `sphere` | `ITEM` identity |
| `segment` | `SEGMENT` identity |
| `path` | parent `STRIP`/subpath when available, plus `SEGMENT` or `VERTEX` payload |
| `image` | `ITEM` for image placements; `PIXEL`/`SAMPLE` for texel identity requests |
| `mesh` | `FACE`/`TRIANGLE`; fill `instance_id` for instanced visuals |
| `text` | object/string identity first; glyph-level picking deferred |

Mesh face picking remains the first high-resolution mesh mode. Mesh vertex picking, sub-character
text picking, and richer barycentric detail remain deferred unless a concrete workflow needs them.


## Result-Shape Notes

The canonical API keeps raw and resolved identities visible. This proposal preserves the older
status vocabulary for the public enum discussion:

1. `HIT`
2. `MISS`
3. `OUTSIDE_PANEL`
4. `UNSUPPORTED_TARGET`
5. `NO_CAPABLE_VISUAL`
6. `GPU_EXEC_FAILED`
7. `READBACK_FAILED`
8. `STALE_DROPPED`
9. `INVALID_RESULT`

Useful payload kinds to reserve:

1. `object`
2. `mesh_face`
3. `item_vertex`
4. `image_pixel`
5. `primitive_item`
6. `strip_group`
7. `line_segment`
8. `triangle_item`

`hit = false` should not be the only failure channel; true misses, unsupported precision, stale
results, GPU failure, and readback failure have different UI and diagnostic meaning.


## Proposal-Owned Backlog

1. Define an explicit hit-selection policy for transparent and mixed-overlay scenes. Candidate
   policies: `frontmost`, `opaque_preferred`, and `all_hits_sorted`.
2. Preserve room for multi-hit results. A default pick may return one resolved hit, but richer
   workflows should be able to request sorted candidates with raw and resolved identities.
3. Keep transparent picking separate from WBOIT/color compositing. Identity rendering should not
   reuse transparency accumulation outputs.
4. Keep grouped primitive raw-hit identity available even when interaction policy resolves to a
   parent group for hover or selection.
5. Ensure partial resource updates preserve ordering assumptions used by pick-resolution tables.
