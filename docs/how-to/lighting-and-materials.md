# Use Lighting and Materials

Make 3D surfaces readable with normals, material attributes, and lights.

## Task Workflow

Use mesh or sphere visuals for lit geometry, upload normals or material attributes required by the
visual, enable the lighting path shown in the feature example, and verify depth testing.

## Minimal Call Sequence

```c
DvzVisual* mesh = dvz_mesh(scene, 0);
dvz_visual_set_data(mesh, "position", pos, vertex_count);
dvz_visual_set_data(mesh, "normal", normal, vertex_count);
dvz_visual_set_data(mesh, "color", color, vertex_count);

DvzMaterialDesc material = dvz_standard_material_desc();
material.light_direction[0] = -0.45f;
material.light_direction[1] = +0.35f;
material.light_direction[2] = +0.82f;
material.standard.roughness = 0.62f;
dvz_visual_set_material(mesh, &material);
dvz_visual_set_depth_test(mesh, true);

dvz_panel_add_visual(panel, mesh, NULL);
```

Use the material-specific calls from the canonical source for metallic, emissive, texture, or
advanced lighting attributes.


## Important Details

Lighting only works when the visual has the attributes the shader needs. Imported meshes often need
normals, orientation, and scale cleanup before lighting looks correct.

## Common Mistakes

- Debugging lighting before verifying face winding and normals.
- Using transparency and expecting opaque material behavior.
- Forgetting backend status differences for advanced material features.

## See Also

- [Configure cameras](configure-cameras.md)
- [Control depth, blending, and transparency](rendering-techniques.md)
- [Use sampled fields and textures](use-sampled-fields.md)

??? example "Related examples"

    - Gallery: [Lighting](../examples/gallery/features/feature_lighting.md)
    - Source: `examples/c/features/lighting.c`
    - Gallery: [Mesh Materials](../examples/gallery/features/feature_material_mesh.md)
    - Source: `examples/c/features/material_mesh.c`
    - Gallery: [Textured Mesh](../examples/gallery/features/feature_mesh_texture.md)
    - Source: `examples/c/features/mesh_texture.c`
