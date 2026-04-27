# Vector And SVG Export

This document defines the scene-layer contract for vector and SVG export.


## Scope And Philosophy

True GPU-to-vector export is not feasible for arbitrary rendered content: GPU fragment
output is rasterized by definition and cannot be back-converted to vector geometry.

The scene supports **structural SVG export**: scene elements that have an inherently
vector nature are emitted as real SVG elements; GPU-rendered visual content is embedded
as a raster image inside the SVG document.

The result is a hybrid SVG that:
1. is scalable for structural elements (axes, tick labels, annotations, colorbars),
2. contains a raster embed for visual content (points, lines, meshes, volumes),
3. can be post-processed in Inkscape, Illustrator, or similar tools.


## What Is Emitted As True Vector

The following scene elements are emitted as native SVG elements:

| Scene element | SVG representation |
|---|---|
| Axis lines | `<line>` |
| Tick marks | `<line>` |
| Tick labels | `<text>` |
| Axis titles | `<text>` |
| Colorbar gradient | `<linearGradient>` + `<rect>` |
| Colorbar tick labels | `<text>` |
| Annotation text labels | `<text>` |
| Guide lines (horizontal, vertical, diagonal) | `<line>` |
| Panel borders | `<rect>` |
| Figure background | `<rect>` |

SVG text uses the same font family and size as the scene font declarations.
Exact font rendering may differ from the GPU path when fonts are not embedded in the SVG.


## What Is Emitted As Vector (Visual Families)

`path` and `segment` visuals are emitted as native SVG elements via a CPU re-draw path:

| Visual family | SVG representation |
|---|---|
| `path` | `<polyline>` per path |
| `segment` | `<line>` per segment |

This covers the most common line-based scientific visualization output (signal traces,
contours, error bars) at true vector quality.

PDF export is not supported in v0.4. SVG output can be converted to PDF externally
(Inkscape, CairoSVG).


## What Is Embedded As Raster

Visual content rendered by GPU shaders is captured as a high-resolution raster image
and embedded in the SVG as a `<image>` element with `preserveAspectRatio="none"`.

This includes all other visual families: markers, pixels, points, glyphs, images, spheres,
volumes, meshes, etc. `path` and `segment` are excepted (see above).

The raster resolution for the embed is controlled by the render scale
(see `IMAGE_EXPORT.md`): a higher render scale produces a sharper embed at the cost
of a larger SVG file.


## Export API

```text
dvz_figure_export_svg(figure, "output.svg", &opts)
```

Options (`DvzSVGExportOptions`):

| Field | Type | Default | Description |
|---|---|---|---|
| `render_scale` | `float32` | `2.0` | raster resolution multiplier for visual embeds |
| `embed_fonts` | `bool` | `true` | embed font data as base64 data URIs (fully self-contained SVG); set to `false` to use `@font-face` references for a smaller file |
| `dpi` | `float32` | `96.0` | nominal DPI for `px`-to-`pt` conversion in SVG units |

`dvz_figure_export_svg` drives one offline frame at the requested render scale,
captures the raster output, and assembles the SVG document on the CPU.
The call is synchronous from the application's perspective.


## Coordinate Mapping

SVG uses a top-left origin with Y pointing down.
The scene uses a bottom-left origin with Y pointing up internally.
The exporter handles the Y-flip transparently.

All SVG coordinates are in `pt` units at the declared `dpi`.
The logical panel size in the scene maps to the SVG `viewBox`.


## Limitations

1. **No per-item vector output** — individual data points, lines, or polygons from GPU
   visuals are not emitted as SVG path elements.
   This is a fundamental limitation of the raster-embed approach.
2. **Annotation shapes** — annotation anchors and callout lines may be emitted as vector
   in a future extension; deferred.
3. **Multi-panel figures** — all panels in the figure are exported into a single SVG
   document, maintaining their relative positions.
4. **No WebGL/WebGPU path** — SVG export requires a local offline render; it is not
   available in browser-embedded Datoviz without a server-side render step.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `IMAGE_EXPORT.md` | render scale for the raster embed; offline frame driving |
| `AXES.md` | axes emit vector SVG elements |
| `ANNOTATIONS.md` | text annotations emit vector SVG elements |
| `LEGENDS_AND_COLORBARS.md` | colorbar emits gradient and tick SVG elements |
| `HIGH_DPI.md` | DPI scale affects raster embed resolution |
