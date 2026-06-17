# Visual Families

Each public visual family has a concise reference page with status, backend support, data model,
attributes, picking/probing notes, and links to the canonical example.

Use the [Examples gallery](../../examples/visuals.md) for screenshots and complete executable
source. Use [Choose a visual family](../../how-to/choose-a-visual-family.md) when deciding between
neighboring families.

| Family | Primary use |
| --- | --- |
| [Point](point.md) | Dense circular point sprites for scatter plots and point clouds |
| [Pixel](pixel.md) | Screen-aligned square cells or dense sparse rasters |
| [Marker](marker.md) | Symbolic point marks with configurable shapes |
| [Primitive](primitive.md) | Simple triangle or line primitive batches |
| [Segment](segment.md) | Independent line segments with per-segment styling |
| [Path](path.md) | Connected polylines, curves, and trajectories |
| [Vector](vector.md) | Arrow or vector-field glyphs |
| [Image](image.md) | 2D sampled fields, textures, and image quads |
| [Labels](labels.md) | Categorical integer sampled fields and segmentation masks |
| [Mesh](mesh.md) | Triangle meshes, surfaces, and textured geometry |
| [Sphere](sphere.md) | 3D sphere impostors with radius and lighting |
| [Volume](volume.md) | 3D sampled scalar fields |
| [Splat](splat.md) | Experimental Gaussian splats |
| [Text](text.md) | Semantic text annotations and labels |
| [Glyph](glyph.md) | Low-level atlas glyph quads |

Reference pages intentionally avoid long standalone programs. The canonical source files under
`examples/c/visuals/` and generated gallery pages are the executable source of truth.
