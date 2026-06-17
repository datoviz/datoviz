# Choose a Visual Family

Pick the visual whose primitive and data layout match your task.

## Task Workflow

Start from the geometry you have, not from the plot name. Use point or marker for independent 2D
samples, path or segment for lines, image or volume for sampled fields, mesh or sphere for 3D
surfaces and objects, and text or labels for glyph output.

## Minimal Decision Table

| Task | Use | Canonical gallery |
| --- | --- | --- |
| Scatter samples | Point, marker | [Point](../examples/gallery/visuals/point_2d.md) |
| Polylines or curves | Path | [Path](../examples/gallery/visuals/visual_path.md) |
| Independent line segments | Segment, vector | [Segment](../examples/gallery/visuals/visual_segment.md) |
| 2D sampled field | Image | [Image](../examples/gallery/visuals/visual_image.md) |
| 3D scalar field | Volume | [Volume](../examples/gallery/visuals/volume.md) |
| Triangulated surface | Mesh | [Mesh](../examples/gallery/visuals/visual_mesh.md) |
| Text annotations | Text, labels | [Labels](../examples/gallery/visuals/visual_labels.md) |

## Canonical Examples

- Source directory: `examples/c/visuals/`
- Gallery index: [Visual examples](../examples/visuals.md)
- Manifest: `examples/c/MANIFEST.yaml`

## Important Details

Visual families are lower-level than plotting functions. A composed chart may use several visuals
plus adornments. Keep those compositions in examples and use How-To pages for the reusable workflow.

## Common Mistakes

- Using mesh for point clouds; use point, marker, sphere, or splat depending on visual weight.
- Using image for sparse cells; use pixel or primitive when every cell is independent geometry.
- Expecting labels and text to behave like data-space mesh geometry.

## See Also

- [Add visuals to a panel](add-a-visual.md)
- [Use sampled fields and textures](use-sampled-fields.md)
- [Add text, labels, and annotations](add-annotations.md)
