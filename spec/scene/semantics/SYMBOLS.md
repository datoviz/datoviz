# Symbol Resources

This document defines the v0.4 parity target for reusable centered graphical symbols. The complete
object model is not installed yet; it is now release scope because v0.3 marker parity requires
built-in symbols, texture-backed marker symbols, and SVG/MSDF import capability.

Symbols are intentionally separate from the `marker` visual family. A marker visual is one consumer
that places symbols at data, screen, or world positions. Legends, annotations, vector heads, and
dashboard cursors may also consume symbols later.


## v0.4 Parity Status

The v0.4 release should restore the visible v0.3 marker-symbol capability while improving the public
boundary:

| Capability | v0.4 target status | Notes |
|---|---|---|
| Built-in code-SDF symbols | supported | Includes the v0.3 shape vocabulary plus `target`. |
| `shape` marker attribute | supported | Compatibility spelling for built-in symbol ids. |
| `DvzSymbolSet` and `symbol` attribute | supported for built-ins and bitmap markers | Texture-backed SDF/MSDF sources are registered on the same boundary. |
| Bitmap symbols | supported for homogeneous bitmap marker visuals | RGBA8 payload is copied into scene-owned symbol atlas pages and lowered to an atlas texture plus per-item UV rectangles. |
| SDF symbols | API/atlas storage installed, render pending | Single-channel distance-field source metadata retained. |
| MSDF symbols | API/atlas storage installed, render pending | RGB distance-field source metadata retained; shader lowering pending. |
| SVG path import | supported or advanced/unstable | Prefer a convenience import API over exposing parser internals. |
| `mtsdf` | advanced/unstable or deferred | Preserve only if the implementation cost is small after MSDF lands. |
| Exact symbol-mask picking | supported for code-SDF, target for texture-backed sources | Bitmap markers currently render through alpha discard, but exact query semantics still need explicit coverage. |


## Purpose

A symbol is a centered graphical mark with stable identity and rendering metadata.

Typical examples:

1. built-in scientific marks such as disc, ring, cross, arrow, target, pin, and bars;
2. user-provided SVG path icons converted to SDF/MSDF;
3. user-provided bitmap icons;
4. precomputed SDF/MSDF symbol textures imported from external tools;
5. annotation decorations such as callout heads, threshold markers, and probe reticles.

The public model should ask users which symbol they want, not which shader encoding should render
it. `code`, `bitmap`, `sdf`, and `msdf` are encodings/backends, not the primary user-facing
abstraction.


## Public Object Model

Target public names:

```c
DvzSymbolSet* symbols = dvz_symbol_set(scene, 0);

DvzSymbolId target = dvz_symbol_builtin(symbols, DVZ_SYMBOL_TARGET);
DvzSymbolId star = dvz_symbol_svg_path(symbols, "star", svg_path, NULL);
DvzSymbolId logo = dvz_symbol_bitmap(symbols, "logo", rgba, width, height, NULL);

dvz_marker_set_symbols(marker, symbols);
dvz_visual_set_data(marker, "symbol", symbol_ids, count);
```

Concepts:

| Concept | Meaning |
|---|---|
| `DvzSymbolSet` | Scene-owned collection of reusable symbols. |
| `DvzSymbolId` | Stable id within one symbol set. |
| `DvzSymbolSource` | Built-in code shape, SVG path, bitmap, SDF, MSDF, or imported atlas entry. |
| `symbol` attribute | Per-item id selecting a symbol from the bound set. |
| `DvzSymbolAtlas` | Lower-level backing storage. Keep internal until a real public need appears. |

Short-form APIs may exist for simple marker use:

```c
dvz_marker_set_symbol(marker, DVZ_SYMBOL_TARGET);
```

That convenience must lower to the same symbol-set machinery rather than creating a parallel marker
shape system.


## Relationship To Marker

The marker visual owns placement and styling:

1. `position`;
2. `diameter`;
3. `angle`;
4. `color`;
5. `symbol`;
6. fill/stroke/outline style;
7. picking behavior and item identity.

The symbol set owns graphical symbol identity and source data:

1. built-in shape enum;
2. SVG path strings or parsed vector outlines;
3. bitmap pixels;
4. SDF/MSDF textures and distance-field metadata;
5. atlas packing metadata and UV rectangles;
6. fallback metadata.

This split keeps `marker` as a point-like visual family while letting symbols evolve into a reusable
resource layer.


## Relationship To Glyphs

Symbols and glyphs should share implementation substrate where useful, but they are not the same
semantic object.

Shared implementation candidates:

1. atlas packing;
2. texture page allocation and upload;
3. UV rectangle bookkeeping;
4. SDF/MSDF generation and decode helpers;
5. distance-field range and scale metadata;
6. dirty-region tracking and atlas rebuild/patch policy;
7. cache import/export for prebuilt atlases.

Semantic differences:

| Area | Glyph/Text | Symbol |
|---|---|---|
| Identity | font face plus glyph id/codepoint | user or built-in symbol id |
| Layout | baseline, advance, kerning, shaping, fallback fonts | centered mark box |
| Item model | text runs lowered into glyph quads | one symbol per marker/annotation item |
| Styling | text color, font size, text placement | fill, stroke, outline, diameter, angle |
| Fallback | missing glyph/font fallback | missing symbol fallback shape or diagnostic |
| Picking | text/glyph readback semantics | symbol/marker item hit semantics |

Glyph atlas pages, symbol atlas pages, and MSDF helpers may share lower-level code, but public APIs
must preserve these different semantics.


## Source Types And Encodings

The symbol API should expose sources:

| Source | User-facing meaning | Preferred encoding |
|---|---|---|
| built-in | Built-in named symbol such as `target` or `arrow`. | Code-SDF or prepacked atlas. |
| SVG path | Vector path string or parsed path object. | MSDF, SDF fallback. |
| bitmap | RGBA or alpha raster icon. | Bitmap atlas. |
| SDF | User-provided single-channel distance field. | SDF atlas. |
| MSDF | User-provided multi-channel distance field. | MSDF atlas. |

Encoding policy:

1. code-SDF, bitmap, SDF, and MSDF remain valid backend encodings;
2. users should normally select symbol source, not marker render mode;
3. advanced APIs may set preferred encoding for reproducibility and diagnostics;
4. the runtime may choose a fallback encoding when the preferred one is unsupported;
5. diagnostics must report any fallback that changes visible quality, picking exactness, or symbol
   availability.


## Built-In Symbol Vocabulary

The built-in vocabulary should restore useful v0.3 marker shapes and add probe-oriented symbols:

| Symbol | Purpose |
|---|---|
| `disc`, `ring`, `square`, `diamond`, `triangle`, `cross` | Core scatter/categorical marks. |
| `target` or `crosshair` | Cursor probes, measurement reticles, linked-panel inspectors. |
| `arrow` | Vector heads, direction fields, flow indicators. |
| `ellipse`, `hbar`, `vbar` | Statistical and anisotropic marks. |
| `pin`, `tag`, `rounded_rect` | Map/annotation marks. |
| `asterisk`, `chevron`, `clover`, `club`, `spade`, `heart`, `infinity` | v0.3 parity and categorical symbol sets. |

`target` must be one native symbol item, not multiple overlaid sprites. It should support
screen-stable diameter when used by `marker`.


## Marker Upgrade Path

The marker visual should evolve in stages:

1. add native `target`/`crosshair` built-in shape to unblock probe workflows;
2. expand the current code-SDF built-in shape set toward the v0.3 vocabulary;
3. introduce `DvzSymbolSet` and a per-item `symbol` attribute;
4. keep `shape` as compatibility vocabulary or as an alias for built-in symbol ids;
5. implement symbol sources for bitmap, SDF, MSDF, and SVG path import;
6. move texture-backed marker logic behind symbol-set binding instead of exposing marker render
   modes as the primary API; bitmap atlas-backed lowering is the first installed texture-backed
   path;
7. add exact symbol-mask picking for marker items;
8. add diagnostics and fallback policy for unsupported symbol sources or encodings.


## v0.3 Capability To Preserve

v0.3 marker exposed these capabilities directly on the marker visual:

1. render modes: `code`, `bitmap`, `sdf`, `msdf`, `mtsdf`;
2. built-in code shapes: `disc`, `asterisk`, `chevron`, `clover`, `club`, `cross`, `diamond`,
   `arrow`, `ellipse`, `hbar`, `heart`, `infinity`, `pin`, `ring`, `spade`, `square`, `tag`,
   `triangle`, `vbar`, and `rounded_rect`;
3. texture binding and `tex_scale` for bitmap/SDF/MSDF markers;
4. `dvz_msdf_from_svg()` for SVG-path custom marker symbols.

v0.4 should preserve the capability but improve the API boundary:

1. sources belong to `DvzSymbolSet`;
2. encodings belong to symbol backing storage and runtime lowering;
3. marker items reference symbols by id;
4. marker style remains separate from symbol identity;
5. glyph/text and symbols share atlas/MSDF internals without sharing public text layout semantics.


## Open Decisions

1. Whether `DvzSymbolSet` should be mutable after first frame or require explicit freeze/rebuild.
2. Whether symbol ids are dense `uint32_t` values scoped to one set or stable scene-global handles.
3. Whether atlas paging is public or fully internal.
4. Whether per-symbol nominal bounds are normalized to a centered unit square or source-specific
   bounds with an anchor.
5. How much of the SVG parser/path object should be public versus a convenience import function.
6. Whether `mtsdf` needs to return as a distinct encoding after MSDF support lands.
