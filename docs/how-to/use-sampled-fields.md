# Use Sampled Fields and Textures

Render regular 2D or 3D scalar data as image, texture, or volume content.

![A scalar sampled field rendered as an image after its values are updated](../assets/gallery/v0.4/features/features_sampled_field_update.webp)

!!! info "At a glance"

    **Status:** Supported native sampled-field workflow; backend coverage varies by field kind
    **Languages:** C-first; Python exposes the exact pointer-shaped API without an ndarray field adapter
    **Prerequisites:** Explicit dimensions, format, semantic role, row pitch, and CPU storage
    **Result:** A regular 2D/3D field retained once and bound to an image, labels, mesh, or volume

## Task workflow

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

The four code blocks below are C function-body excerpts, not standalone programs. The current
top-level Python facade adapts dense visual arrays but does not yet infer sampled-field dimensions,
strides, or lifetimes from a NumPy array; use `datoviz.raw` only if you intentionally want the exact
`DvzFieldDataView` pointer contract.

## 2D scalar image

The snippets below are function-body excerpts. They assume valid scene/panel objects, dimensions,
CPU arrays, placement attributes, and scales where shown; their `return false` paths belong to an
enclosing setup function. For complete lifecycle and error handling, start from the linked gallery
sources.

```c
DvzVisual* image = dvz_image(scene, 0);
if (image == NULL || dvz_visual_set_data(image, "position", pos, 4) != 0 ||
    dvz_visual_set_data(image, "texcoords", uv, 4) != 0)
    return false;

DvzSampledFieldDesc desc = dvz_sampled_field_desc();
desc.dim = DVZ_FIELD_DIM_2D;
desc.format = DVZ_FIELD_FORMAT_R32_FLOAT;
desc.semantic = DVZ_FIELD_SEMANTIC_SCALAR;
desc.width = width;
desc.height = height;
desc.depth = 1;
DvzSampledField* field = dvz_sampled_field(scene, &desc);
if (field == NULL)
    return false;

DvzFieldDataView data = dvz_field_data_view();
data.data = values;
data.bytes_per_row = width * sizeof(float);
data.rows_per_image = height;
if (dvz_sampled_field_set_data(field, &data) != 0)
    return false;
if (dvz_visual_set_field(image, "field", field) != 0 ||
    dvz_panel_add_visual(panel, image, NULL) != 0)
    return false;
```

Bind a color scale to the image when the scalar values should be colormapped:

```c
if (dvz_visual_set_scale(image, "color", scale) != 0)
    return false;
```

Use `dvz_visual_set_field()` for image, labels, mesh texture, and volume sampled fields. Public
examples should keep dimensions, format, semantic role, and row pitch explicit in the sampled-field
descriptor and data view.

## Categorical labels

Use labels visuals for integer sampled fields such as segmentation masks. Labels need a categorical
scale; ordinary floating-point scalar fields belong on an image visual instead.

```c
DvzVisual* labels = dvz_labels(scene, 0);
if (labels == NULL || dvz_visual_set_data(labels, "position", position, 1) != 0 ||
    dvz_visual_set_data(labels, "extent", extent, 1) != 0)
    return false;

DvzSampledFieldDesc desc = dvz_sampled_field_desc();
desc.dim = DVZ_FIELD_DIM_2D;
desc.format = DVZ_FIELD_FORMAT_R32_SINT;
desc.semantic = DVZ_FIELD_SEMANTIC_LABEL;
desc.width = width;
desc.height = height;
desc.depth = 1;
DvzSampledField* field = dvz_sampled_field(scene, &desc);

DvzFieldDataView data = dvz_field_data_view();
data.data = label_ids;
data.bytes_per_row = width * sizeof(int32_t);
data.rows_per_image = height;
if (field == NULL || dvz_sampled_field_set_data(field, &data) != 0)
    return false;

if (dvz_visual_set_field(labels, "field", field) != 0 ||
    dvz_visual_set_scale(labels, "labels", categorical_scale) != 0 ||
    dvz_labels_set_background(labels, 0) != 0 ||
    dvz_panel_add_visual(panel, labels, NULL) != 0)
    return false;
```

## 3D volumes

For 3D fields, set `depth` to the number of slices and bind the field to a volume visual.
`bytes_per_row` is the byte stride between adjacent rows in one slice, and `rows_per_image` is the
number of rows per slice.

```c
DvzSampledFieldDesc desc = dvz_sampled_field_desc();
desc.dim = DVZ_FIELD_DIM_3D;
desc.format = DVZ_FIELD_FORMAT_R8_UNORM;
desc.semantic = DVZ_FIELD_SEMANTIC_SCALAR;
desc.width = size;
desc.height = size;
desc.depth = size;
DvzSampledField* field = dvz_sampled_field(scene, &desc);

DvzFieldDataView data = dvz_field_data_view();
data.data = voxels;
data.bytes_per_row = size;
data.rows_per_image = size;
if (field == NULL || dvz_sampled_field_set_data(field, &data) != 0)
    return false;

DvzVisual* volume = dvz_volume(scene, 0);
if (volume == NULL || dvz_visual_set_field(volume, "field", field) != 0 ||
    dvz_panel_add_visual(panel, volume, NULL) != 0)
    return false;
```

The result is a retained 3D texture sampled by the volume visual. Camera, transfer, and ray-march
settings determine its final appearance; the field descriptor alone does not choose those policies.

## Textured meshes

Use a sampled field as a mesh texture only when the texture belongs to surface geometry. The mesh
still needs geometry attributes such as position, normal, and texture coordinates.

```c
DvzSampledFieldDesc desc = dvz_sampled_field_desc();
desc.dim = DVZ_FIELD_DIM_2D;
desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
desc.width = texture_width;
desc.height = texture_height;
desc.depth = 1;
DvzSampledField* texture = dvz_sampled_field(scene, &desc);

DvzFieldDataView data = dvz_field_data_view();
data.data = rgba;
data.bytes_per_row = texture_width * 4u;
data.rows_per_image = texture_height;
if (mesh == NULL || texture == NULL || dvz_sampled_field_set_data(texture, &data) != 0)
    return false;

if (dvz_visual_set_field(mesh, "texture", texture) != 0)
    return false;
```


## Important details

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

`DvzFieldDataView.data` is borrowed only for the upload call; Datoviz copies the supplied payload.
The descriptor dimensions and data-view strides must nevertheless describe every uploaded byte
correctly. For subregion updates, offsets and extents are expressed in field sample coordinates.

For color textures, set the semantic and color role intentionally. Scientific scalar fields should
use scalar semantics and a scale. Ordinary RGBA textures should use color semantics so color-space
handling is explicit.

## Common mistakes

- Expanding large fields into millions of independent primitives.
- Forgetting `bytes_per_row` and `rows_per_image` when uploading padded or 3D data.
- Binding integer label fields to `dvz_image()` instead of `dvz_labels()`.
- Binding a texture to a mesh without uploading texture coordinates.
- Recreating the whole visual when only a sampled field region changed.
- Forgetting that 3D texture and volume support has different native and WebGPU status.
- Reusing image probe logic for a transformed mesh texture without accounting for UV mapping.

## See also

- [Probe image or field values](probe-fields.md)
- [Map scalar values with colormaps](use-colormaps.md)
- [Use lighting and materials](lighting-and-materials.md)

??? example "Complete and related examples"

    - Canonical complete example: [Sampled Field Update](../examples/gallery/features/features_sampled_field_update.md) - Source: `examples/c/features/sampled_field_update.c`
    - [Volume](../examples/gallery/visuals/visuals_volume.md) - Source: `examples/c/visuals/volume.c`
    - [Textured Mesh](../examples/gallery/features/features_mesh_texture.md) - Source: `examples/c/features/mesh_texture.c`
