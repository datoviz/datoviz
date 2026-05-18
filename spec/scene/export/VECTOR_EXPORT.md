# Vector Export Scope

This document records the current v0.4 scope decision for vector export.


## Decision

Datoviz v0.4 does not implement a native SVG/PDF/vector exporter.

Datoviz is the interactive GPU backend and raster-output backend for the GSP protocol. Publication
oriented PDF/SVG/vector output should be produced outside Datoviz by the GSP layer, with Matplotlib
as the intended vector/export backend.


## Rationale

Arbitrary Datoviz output is produced by GPU rasterization. Converting that output back into
faithful vector geometry is not a reliable backend-level operation, especially for shaders,
volumes, transparency, postprocess effects, images, text atlases, and future WebGPU execution.

The correct source for vector export is the semantic scene description above the renderer. GSP can
lower that semantic description to different backends:

1. Datoviz for interactive GPU rendering and raster capture,
2. Matplotlib for publication-oriented static PDF/SVG/vector export.

This keeps Datoviz from growing a second, partial CPU renderer for vector output and keeps the
publication path aligned with the backend-agnostic GSP contract.


## Datoviz v0.4 Scope

In scope for Datoviz v0.4:

1. offscreen image capture,
2. screenshot/gallery capture,
3. video or frame-sequence capture where supported by the active app/video path,
4. DVZR-style recording/replay for renderer/runtime debugging and reproducibility,
5. diagnostics that make it clear when a caller asks Datoviz for unsupported vector export.

Out of scope for Datoviz v0.4:

1. `dvz_figure_export_svg()` or equivalent native Datoviz SVG export,
2. native PDF export,
3. structural SVG export of axes, labels, legends, or colorbars,
4. CPU redraw paths for `path`, `segment`, or other visual families,
5. SVG preservation of text, glyphs, or annotation objects.


## Relationship To Scene Semantics

Scene semantics should still preserve enough information for GSP or another higher layer to export
the same intended figure through a vector backend. Axes, labels, legends, colorbars, units,
annotations, and visual mappings should remain semantic scene objects rather than backend-only draw
commands.

Datoviz does not need to expose or implement vector export to keep those semantics clean.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `export/IMAGE_EXPORT.md` | Datoviz-native raster image capture remains in scope. |
| `semantics/AXES.md` | Axes remain semantic objects for rendering and for GSP export. |
| `semantics/ANNOTATIONS.md` | Annotations remain semantic objects for rendering and for GSP export. |
| `semantics/LEGENDS_AND_COLORBARS.md` | Legends and colorbars remain semantic objects for rendering and for GSP export. |
| `integration/CUSTOM_VISUALS.md` | Custom Datoviz visuals are renderer/backend features, not vector export promises. |
