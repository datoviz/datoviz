# Use Sampled Fields and Textures

Render regular 2D or 3D scalar data as image, texture, or volume content.

![Sampled Field Update](../assets/gallery/v0.4/features/feature_sampled_field_update.webp)

## Task Workflow

Keep regular grids in sampled-field form when possible. Use image visuals for 2D arrays, volume
visuals for 3D arrays, labels visuals for integer categorical fields, and textured mesh only when
the texture is attached to surface geometry.

Choose the field descriptor before choosing the visual:

| Data | Field descriptor | Visual binding |
| --- | --- | --- |
| 2D scalar array | `DVZ_FIELD_DIM_2D`, scalar format, `DVZ_FIELD_SEMANTIC_SCALAR` | `dvz_image()` slot `"field"` |
| 2D RGBA texture | `DVZ_FIELD_DIM_2D`, `DVZ_FIELD_FORMAT_RGBA8_UNORM`, `DVZ_FIELD_SEMANTIC_COLOR` | `dvz_image()` slot `"field"` or mesh slot `"texture"` |
| Integer segmentation or label mask | 2D integer format, `DVZ_FIELD_SEMANTIC_LABEL` | `dvz_labels()` slot `"field"` plus categorical scale |
| 3D scalar array | `DVZ_FIELD_DIM_3D`, scalar format, `DVZ_FIELD_SEMANTIC_SCALAR` | `dvz_volume()` slot `"field"` |

## 2D Scalar Image

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

Bind a color scale to the image when the scalar values should be colormapped:

```c
dvz_visual_set_scale(image, "color", scale);
```

Use `dvz_visual_set_field()` for image, labels, mesh texture, and volume sampled fields. Public
examples should keep dimensions, format, semantic role, and row pitch explicit in the sampled-field
descriptor and data view.

## Categorical Labels

Use labels visuals for integer sampled fields such as segmentation masks. Labels need a categorical
scale; ordinary floating-point scalar fields belong on an image visual instead.

```c
DvzVisual* labels = dvz_labels(scene, 0);
dvz_visual_set_data(labels, "position", position, 1);
dvz_visual_set_data(labels, "extent", extent, 1);

DvzSampledField* field = dvz_sampled_field(
    scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
               .dim = DVZ_FIELD_DIM_2D,
               .format = DVZ_FIELD_FORMAT_R32_SINT,
               .semantic = DVZ_FIELD_SEMANTIC_LABEL,
               .width = width,
               .height = height,
               .depth = 1});
dvz_sampled_field_set_data(
    field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
               .data = label_ids,
               .bytes_per_row = width * sizeof(int32_t),
               .rows_per_image = height});

dvz_visual_set_field(labels, "field", field);
dvz_visual_set_scale(labels, "labels", categorical_scale);
dvz_labels_set_background(labels, 0);
dvz_panel_add_visual(panel, labels, NULL);
```

## 3D Volumes

For 3D fields, set `depth` to the number of slices and bind the field to a volume visual.
`bytes_per_row` is the byte stride between adjacent rows in one slice, and `rows_per_image` is the
number of rows per slice.

```c
DvzSampledField* field = dvz_sampled_field(
    scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
               .dim = DVZ_FIELD_DIM_3D,
               .format = DVZ_FIELD_FORMAT_R8_UNORM,
               .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
               .width = size,
               .height = size,
               .depth = size});
dvz_sampled_field_set_data(
    field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
               .data = voxels,
               .bytes_per_row = size,
               .rows_per_image = size});

DvzVisual* volume = dvz_volume(scene, 0);
dvz_visual_set_field(volume, "field", field);
dvz_panel_add_visual(panel, volume, NULL);
```

## Textured Meshes

Use a sampled field as a mesh texture only when the texture belongs to surface geometry. The mesh
still needs geometry attributes such as position, normal, and texture coordinates.

```c
DvzSampledField* texture = dvz_sampled_field(
    scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
               .dim = DVZ_FIELD_DIM_2D,
               .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
               .semantic = DVZ_FIELD_SEMANTIC_COLOR,
               .width = texture_width,
               .height = texture_height,
               .depth = 1});
dvz_sampled_field_set_data(
    texture, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                .data = rgba,
                .bytes_per_row = texture_width * 4u,
                .rows_per_image = texture_height});

dvz_visual_set_field(mesh, "texture", texture);
```


## Important Details

Images and volumes are not just dense point clouds. Preserve grid dimensions, value range, and
texture format so filtering, colormapping, and probing remain meaningful.

Keep the sampled-field pointer for updates, and bind it to visuals with
`dvz_visual_set_field()`. If the values change without changing dimensions, call
`dvz_sampled_field_set_data()` again or use `dvz_sampled_field_update_region()` for a subregion.
If the dimensions change, call `dvz_sampled_field_resize()` so bound visuals can reallocate the
texture on the next frame.

Use `DvzFieldGeometry` when the array has physical origin, spacing, axis order, flips, or units
that matter to probing or measurement. The visual placement still controls where the field appears
in the panel; geometry metadata records what the samples mean.

For color textures, set the semantic and color role intentionally. Scientific scalar fields should
use scalar semantics and a scale. Ordinary RGBA textures should use color semantics so color-space
handling is explicit.

## Common Mistakes

- Expanding large fields into millions of independent primitives.
- Forgetting `bytes_per_row` and `rows_per_image` when uploading padded or 3D data.
- Binding integer label fields to `dvz_image()` instead of `dvz_labels()`.
- Binding a texture to a mesh without uploading texture coordinates.
- Recreating the whole visual when only a sampled field region changed.
- Forgetting that 3D texture and volume support has different native and WebGPU status.
- Reusing image probe logic for a transformed mesh texture without accounting for UV mapping.

## See Also

- [Probe image or field values](probe-fields.md)
- [Map scalar values with colormaps](use-colormaps.md)
- [Use lighting and materials](lighting-and-materials.md)

??? example "Related examples"

    - [Sampled Field Update](../examples/gallery/features/feature_sampled_field_update.md) - Source: `examples/c/features/sampled_field_update.c`
    - [Volume](../examples/gallery/visuals/volume.md) - Source: `examples/c/visuals/volume.c`
    - [Textured Mesh](../examples/gallery/features/feature_mesh_texture.md) - Source: `examples/c/features/mesh_texture.c`
