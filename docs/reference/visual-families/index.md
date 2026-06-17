# Visual Families

Each public visual family has a concise reference page with status, backend support, data model,
attributes, picking/probing notes, and links to the canonical example.

Use the [Examples gallery](../../examples/visuals.md) for complete executable source. Use
[Choose a visual family](../../how-to/choose-a-visual-family.md) when deciding between neighboring
families.

| Preview | Family | Status | Primary use | Example |
| --- | --- | --- | --- | --- |
| [![Point](../../assets/gallery/v0.4/visuals/point_2d.webp)](../../examples/gallery/visuals/point_2d.md) | [Point](point.md) | supported | Dense circular point sprites for scatter plots and point clouds | [Example](../../examples/gallery/visuals/point_2d.md) |
| [![Pixel](../../assets/gallery/v0.4/visuals/visual_pixel.webp)](../../examples/gallery/visuals/visual_pixel.md) | [Pixel](pixel.md) | supported | Screen-aligned square cells or dense sparse rasters | [Example](../../examples/gallery/visuals/visual_pixel.md) |
| [![Marker](../../assets/gallery/v0.4/visuals/visual_marker.webp)](../../examples/gallery/visuals/visual_marker.md) | [Marker](marker.md) | supported | Symbolic point marks with configurable shapes | [Example](../../examples/gallery/visuals/visual_marker.md) |
| [![Splat](../../assets/gallery/v0.4/visuals/visual_splat.webp)](../../examples/gallery/visuals/visual_splat.md) | [Splat](splat.md) | experimental | Experimental Gaussian splats | [Example](../../examples/gallery/visuals/visual_splat.md) |
| [![Segment](../../assets/gallery/v0.4/visuals/visual_segment.webp)](../../examples/gallery/visuals/visual_segment.md) | [Segment](segment.md) | supported | Independent line segments with per-segment styling | [Example](../../examples/gallery/visuals/visual_segment.md) |
| [![Path](../../assets/gallery/v0.4/visuals/visual_path.webp)](../../examples/gallery/visuals/visual_path.md) | [Path](path.md) | supported | Connected polylines, curves, and trajectories | [Example](../../examples/gallery/visuals/visual_path.md) |
| [![Vector](../../assets/gallery/v0.4/visuals/visual_vector.webp)](../../examples/gallery/visuals/visual_vector.md) | [Vector](vector.md) | supported | Arrow or vector-field glyphs | [Example](../../examples/gallery/visuals/visual_vector.md) |
| [![Primitive](../../assets/gallery/v0.4/visuals/visual_primitive.webp)](../../examples/gallery/visuals/visual_primitive.md) | [Primitive](primitive.md) | supported | Simple triangle or line primitive batches | [Example](../../examples/gallery/visuals/visual_primitive.md) |
| [![Image](../../assets/gallery/v0.4/visuals/visual_image.webp)](../../examples/gallery/visuals/visual_image.md) | [Image](image.md) | supported | 2D sampled fields, textures, and image quads | [Example](../../examples/gallery/visuals/visual_image.md) |
| [![Text](../../assets/gallery/v0.4/visuals/visual_text.webp)](../../examples/gallery/visuals/visual_text.md) | [Text](text.md) | supported | Semantic text annotations and labels | [Example](../../examples/gallery/visuals/visual_text.md) |
| [![Glyph](../../assets/gallery/v0.4/visuals/visual_glyph.webp)](../../examples/gallery/visuals/visual_glyph.md) | [Glyph](glyph.md) | experimental | Low-level atlas glyph quads | [Example](../../examples/gallery/visuals/visual_glyph.md) |
| [![Labels](../../assets/gallery/v0.4/visuals/visual_labels.webp)](../../examples/gallery/visuals/visual_labels.md) | [Labels](labels.md) | supported | Categorical integer sampled fields and segmentation masks | [Example](../../examples/gallery/visuals/visual_labels.md) |
| [![Mesh](../../assets/gallery/v0.4/visuals/visual_mesh.webp)](../../examples/gallery/visuals/visual_mesh.md) | [Mesh](mesh.md) | supported | Triangle meshes, surfaces, and textured geometry | [Example](../../examples/gallery/visuals/visual_mesh.md) |
| [![Sphere](../../assets/gallery/v0.4/visuals/sphere_impostor.webp)](../../examples/gallery/visuals/sphere_impostor.md) | [Sphere](sphere.md) | supported | 3D sphere impostors with radius and lighting | [Example](../../examples/gallery/visuals/sphere_impostor.md) |
| [![Volume](../../assets/gallery/v0.4/visuals/volume.webp)](../../examples/gallery/visuals/volume.md) | [Volume](volume.md) | supported | 3D sampled scalar fields | [Example](../../examples/gallery/visuals/volume.md) |

Reference pages intentionally avoid long standalone programs. The canonical source files under
`examples/c/visuals/` and generated gallery pages are the executable source of truth.
