# Choose a Visual Family

Pick the visual whose primitive and data layout match your task.

## Task Workflow

Start from the geometry you have, not from the plot name. Use point or marker for independent 2D
samples, path or segment for lines, image or volume for sampled fields, mesh or sphere for 3D
surfaces and objects, and text or labels for glyph output.

Choose the coarsest visual family that lets you batch similar elements together. The preferred
Datoviz layout is few visuals, many items per visual. Split elements into separate visuals only
when they truly need different visual families, materials, transforms, panels, or update schedules.

## Minimal Decision Table

| Task | Use | Canonical gallery |
| --- | --- | --- |
| Scatter samples | Point, marker | [Point](../examples/gallery/visuals/point_2d.md) |
| Polylines or curves | Path | [Path](../examples/gallery/visuals/visual_path.md) |
| Independent line segments | Segment, vector | [Segment](../examples/gallery/visuals/visual_segment.md) |
| 2D sampled field | Image | [Image](../examples/gallery/visuals/visual_image.md) |
| 3D scalar field | Volume | [Volume](../examples/gallery/visuals/volume.md) |
| Triangulated surface | Mesh | [Mesh](../examples/gallery/visuals/visual_mesh.md) |
| Many copies of the same 3D object | Mesh with `instance_transform` | [Mesh Instance Selection](../examples/gallery/features/feature_selection_mesh_instances.md) |
| 3D balls, atoms, or particles with world-space radius | Sphere | [Sphere](../examples/gallery/visuals/sphere_impostor.md) |
| Text annotations | Text, labels | [Labels](../examples/gallery/visuals/visual_labels.md) |


## Important Details

Visual families are lower-level than plotting functions. A composed chart may use several visuals
plus adornments. Keep those compositions in examples and use How-To pages for the reusable workflow.

Attributes are per-visual arrays. Put as many homogeneous items as possible into those arrays so a
single visual can drive a large GPU batch.

For many similar 3D meshes, such as a field of cubes or repeated glyph-like solids, use one mesh
visual with shared geometry and upload one `"instance_transform"` matrix per copy. Do not create one
mesh visual per cube unless each cube needs a distinct material, lifetime, or update schedule.

## Common Mistakes

- Using mesh for point clouds; use point, marker, sphere, or splat depending on visual weight.
- Using image for sparse cells; use pixel or primitive when every cell is independent geometry.
- Creating one visual per data item instead of one visual containing many items.
- Expecting labels and text to behave like data-space mesh geometry.

## See Also

- [Add visuals to a panel](add-a-visual.md)
- [Use sampled fields and textures](use-sampled-fields.md)
- [Add text, labels, and annotations](add-annotations.md)

??? example "Related examples"

    - Source directory: `examples/c/visuals/`
    - Gallery index: [Visual examples](../examples/visuals.md)
    - Manifest: `examples/c/MANIFEST.yaml`
