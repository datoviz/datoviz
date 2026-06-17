# Mesh with Arcball Navigation

This composed workflow now points to the task guides for mesh rendering and 3D control.

## Task Workflow

Load or generate mesh data, upload mesh attributes, attach a 3D controller, enable depth and
lighting as needed, then run the window or offscreen path.

## Minimal Call Sequence

```text
mesh visual -> normals/materials -> 3D controller -> depth/lighting -> render
```

## Canonical Examples

- Gallery: [Mesh](../examples/gallery/visuals/visual_mesh.md)
- Source: `examples/c/visuals/mesh.c`
- Gallery: [Arcball Controller](../examples/gallery/features/feature_controller_arcball.md)
- Source: `examples/c/features/controller_arcball.c`
- Gallery: [Protein](../examples/gallery/showcases/protein_arcball_viewer.md)
- Source: `examples/c/showcases/protein.c`

## Important Details

Use the visual and controller examples as the source of truth. The protein viewer is a composed
showcase, not the minimal mesh or arcball recipe.

## Common Mistakes

- Debugging arcball before checking mesh scale, normals, and depth state.
- Treating the showcase's data loading as required for all mesh viewers.
- Using 2D panzoom with 3D mesh navigation.

## See Also

- [Use 3D controllers](3d-navigation.md)
- [Use lighting and materials](lighting-and-materials.md)
- [Configure cameras](configure-cameras.md)
