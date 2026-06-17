# Use Sampled Fields and Textures

Render regular 2D or 3D scalar data as image, texture, or volume content.

## Task Workflow

Keep regular grids in sampled-field form when possible. Use image visuals for 2D arrays, volume
visuals for 3D arrays, and textured mesh only when the texture is attached to surface geometry.

## Minimal Call Sequence

```c
DvzVisual* image = dvz_image(scene, 0);
dvz_visual_set_data(image, "position", pos, 4);
dvz_visual_set_data(image, "texcoords", uv, 4);

DvzSampledField* field = dvz_sampled_field(
    scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
               .dim = DVZ_FIELD_DIM_2D,
               .format = DVZ_FIELD_FORMAT_R32_FLOAT,
               .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
               .width = width,
               .height = height,
               .depth = 1});
dvz_sampled_field_set_data(
    field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
               .data = values,
               .bytes_per_row = width * sizeof(float),
               .rows_per_image = height});
dvz_visual_set_field(image, "field", field);
dvz_panel_add_visual(panel, image, NULL);
```

Use `dvz_visual_set_field()` for image and volume sampled fields. The older texture convenience
wrappers still exist, but new examples should keep dimensions, format, semantic role, and row pitch
explicit in the sampled-field descriptor and data view.


## Important Details

Images and volumes are not just dense point clouds. Preserve grid dimensions, value range, and
texture format so filtering, colormapping, and probing remain meaningful.

## Common Mistakes

- Expanding large fields into millions of independent primitives.
- Forgetting that 3D texture and volume support has different native and WebGPU status.
- Reusing image probe logic for a transformed mesh texture without accounting for UV mapping.

## See Also

- [Probe image or field values](probe-fields.md)
- [Map scalar values with colormaps](use-colormaps.md)
- [Use lighting and materials](lighting-and-materials.md)

??? example "Related examples"

    - Gallery: [2D Sampled Field](../examples/gallery/features/feature_sampled_field_2d.md)
    - Source: `examples/c/features/sampled_field_2d.c`
    - Gallery: [3D Sampled Field](../examples/gallery/features/feature_sampled_field_3d.md)
    - Source: `examples/c/features/sampled_field_3d.c`
    - Gallery: [Textured Mesh](../examples/gallery/features/feature_mesh_texture.md)
    - Source: `examples/c/features/mesh_texture.c`
