# Visual Families

Each public visual family has a concise reference page with status, backend support, data model,
attributes, picking/probing notes, and links to the canonical example.

Use the [Examples gallery](../../examples/visuals.md) for complete executable source. Use
[Choose a visual family](../../how-to/choose-a-visual-family.md) when deciding between neighboring
families.

Visual families are batching units. Prefer one visual with many items over many visuals with one
item each. Use per-item attributes for position, color, size, radius, state, and related styling
whenever the visual family supports them.

| Preview | Family | Status | Primary use | Example |
| --- | --- | --- | --- | --- |
| [![Point](../../assets/gallery/v0.4/visuals/visuals_point.webp)](../../examples/gallery/visuals/visuals_point.md) | [Point](point.md) | supported | Dense circular point sprites for scatter plots and point clouds | [Example](../../examples/gallery/visuals/visuals_point.md) |
| [![Pixel](../../assets/gallery/v0.4/visuals/visuals_pixel.webp)](../../examples/gallery/visuals/visuals_pixel.md) | [Pixel](pixel.md) | supported | Screen-aligned square cells or dense sparse rasters | [Example](../../examples/gallery/visuals/visuals_pixel.md) |
| [![Marker](../../assets/gallery/v0.4/visuals/visuals_marker.webp)](../../examples/gallery/visuals/visuals_marker.md) | [Marker](marker.md) | supported | Symbolic point marks with configurable shapes | [Example](../../examples/gallery/visuals/visuals_marker.md) |
| [![Splat](../../assets/gallery/v0.4/visuals/visuals_splat.webp)](../../examples/gallery/visuals/visuals_splat.md) | [Splat](splat.md) | experimental | Experimental Gaussian splats | [Example](../../examples/gallery/visuals/visuals_splat.md) |
| [![Segment](../../assets/gallery/v0.4/visuals/visuals_segment.webp)](../../examples/gallery/visuals/visuals_segment.md) | [Segment](segment.md) | supported | Independent line segments with per-segment styling | [Example](../../examples/gallery/visuals/visuals_segment.md) |
| [![Path](../../assets/gallery/v0.4/visuals/visuals_path.webp)](../../examples/gallery/visuals/visuals_path.md) | [Path](path.md) | supported | Connected polylines, curves, and trajectories | [Example](../../examples/gallery/visuals/visuals_path.md) |
| [![Vector](../../assets/gallery/v0.4/visuals/visuals_vector.webp)](../../examples/gallery/visuals/visuals_vector.md) | [Vector](vector.md) | supported | Arrow or vector-field glyphs | [Example](../../examples/gallery/visuals/visuals_vector.md) |
| [![Primitive](../../assets/gallery/v0.4/visuals/visuals_primitive.webp)](../../examples/gallery/visuals/visuals_primitive.md) | [Primitive](primitive.md) | supported | Simple triangle or line primitive batches | [Example](../../examples/gallery/visuals/visuals_primitive.md) |
| [![Image](../../assets/gallery/v0.4/visuals/visuals_image.webp)](../../examples/gallery/visuals/visuals_image.md) | [Image](image.md) | supported | 2D sampled fields, textures, and image quads | [Example](../../examples/gallery/visuals/visuals_image.md) |
| [![Text](../../assets/gallery/v0.4/visuals/visuals_text.webp)](../../examples/gallery/visuals/visuals_text.md) | [Text](text.md) | supported | Semantic text annotations and labels | [Example](../../examples/gallery/visuals/visuals_text.md) |
| [![Glyph](../../assets/gallery/v0.4/visuals/visuals_glyph.webp)](../../examples/gallery/visuals/visuals_glyph.md) | [Glyph](glyph.md) | experimental | Low-level atlas glyph quads | [Example](../../examples/gallery/visuals/visuals_glyph.md) |
| [![Labels](../../assets/gallery/v0.4/visuals/visuals_labels.webp)](../../examples/gallery/visuals/visuals_labels.md) | [Labels](labels.md) | supported | Categorical integer sampled fields and segmentation masks | [Example](../../examples/gallery/visuals/visuals_labels.md) |
| [![Mesh](../../assets/gallery/v0.4/visuals/visuals_mesh.webp)](../../examples/gallery/visuals/visuals_mesh.md) | [Mesh](mesh.md) | supported | Triangle meshes, surfaces, and textured geometry | [Example](../../examples/gallery/visuals/visuals_mesh.md) |
| [![Sphere](../../assets/gallery/v0.4/visuals/visuals_sphere.webp)](../../examples/gallery/visuals/visuals_sphere.md) | [Sphere](sphere.md) | supported | 3D sphere impostors with radius and lighting | [Example](../../examples/gallery/visuals/visuals_sphere.md) |
| [![Volume](../../assets/gallery/v0.4/visuals/visuals_volume.webp)](../../examples/gallery/visuals/visuals_volume.md) | [Volume](volume.md) | supported | 3D sampled scalar fields | [Example](../../examples/gallery/visuals/visuals_volume.md) |

Reference pages intentionally avoid long standalone programs. The canonical source files under
`examples/c/visuals/` and generated gallery pages are the executable source of truth.
