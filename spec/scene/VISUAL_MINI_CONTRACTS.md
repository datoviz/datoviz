# Scene Visual Family Constraints

This document defines the shared contract template and cross-family boundary rules for the v0.4
visual families.

For the full per-family attribute schemas, parameter listings, and worked examples, see the
individual files in `spec/scene/visuals/`.


## Purpose

These constraints make family boundaries explicit so that:

1. implementation does not drift toward backend-shaped design,
2. fallback and variant decisions stay within the correct family,
3. cross-family confusion (e.g., volume-slice in `image`) is caught at spec level.


## Shared Template

Each family spec in `visuals/` must address:

1. semantic purpose,
2. scene-facing resource classes,
3. parameter schema,
4. transform model,
5. stage participation,
6. picking model,
7. variant axes,
8. fallback notes.


## Family Boundary Rules

### `primitive`

`primitive` is intentionally constrained. If a use case needs richer semantic behavior, the
preferred direction is another family — not expanding `primitive`.


### `pixel`

`pixel` must not grow a shape system. Shape, rotation, and edge treatment belong in `marker`.


### `point`

If a feature request introduces shape, edge, or rotation semantics, that is pressure toward
`marker`, not toward expanding `point`.


### `marker`

If a runtime cannot support a richer marker path, the preferred fallback is a simpler marker
variant or a downgrade toward `point`, provided the semantic loss is reported clearly.


### `segment`

If joins, topology, or grouped ordering become central, that is pressure toward `path`, not
toward expanding `segment`.


### `path`

Wiggle-like behavior is path-scoped unless it later proves to require a materially different
family contract. See `VISUAL_FAMILIES.md` for the current direction on `wiggle`.

`monoglyph` must not return as a separate family; any useful simplification is a `glyph`
variant or implementation path.


### `glyph`

`monoglyph` must not return as a separate family. Any useful simplification should be a `glyph`
variant or implementation path.


### `image`

Volume slice display belongs to the `volume` family (`render_mode = slice`), not to `image`.
`image` handles flat 2D rasters only. If image placement or colormap mode is unavailable, the
preferred fallback is a display simplification — not collapsing into another family.


### `mesh`

If a richer mesh variant is unavailable, fallback should stay within the mesh family whenever
the core geometry semantics remain intact. When one mesh visual represents several stable logical
parts, picking should support semantic group or region identity, not only primitive identity.


### `sphere`

The default conceptual path is impostor-first. If a runtime cannot support a preferred sphere
path, fallback may remain inside the sphere family by switching variants rather than collapsing
into `point` or `mesh` semantically.


### `volume`

If a runtime cannot support a richer volume path, fallback should remain semantic and explicit
rather than silently degrading into an unrelated family. Volume slice display has its own fallback
path within the `volume` family.
