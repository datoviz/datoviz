# API Shape Example: `SampledField`

This example pressure-tests the proposed shared `SampledField` API against three immediate scene
use cases:

1. scalar image with shared colormap scale,
2. direct RGBA image,
3. volumetric scalar field bound to a volume visual.

Normative inputs:

1. [../proposals/SAMPLED_FIELD_API_DESIGN.md](../proposals/SAMPLED_FIELD_API_DESIGN.md)
2. [../pipeline/RESOURCE_MODEL.md](../pipeline/RESOURCE_MODEL.md)
3. [../visuals/IMAGE.md](../visuals/IMAGE.md)
4. [../visuals/VOLUME.md](../visuals/VOLUME.md)
5. [../semantics/SCALES.md](../semantics/SCALES.md)


## Goals

The API sketch should show that:

1. one field object can describe both scalar and color data,
2. image and volume bind the same kind of scene-owned resource,
3. scale/colormap semantics stay separate from field storage,
4. future GPU-native execution can replace CPU fallback without changing the authored API.


## Sketch

```c
DvzScene* scene = dvz_scene();
DvzFigure* fig = dvz_figure(scene, 1200, 800, 0);
DvzPanel* panel = dvz_panel(fig, 0);

DvzScale* scale = dvz_scale_color(scene, &(DvzScaleDesc){
    .domain_min = 0.0,
    .domain_max = 1.0,
    .view_min = 0.2,
    .view_max = 0.9,
});

dvz_scale_set_label(scale, "Intensity");
dvz_scale_set_unit(scale, "a.u.");

float* scalar_pixels = ...;
rgba_u8* rgba_pixels = ...;
float* volume_values = ...;

DvzSampledField* field_scalar_2d = dvz_sampled_field(scene, &(DvzSampledFieldDesc){
    .dim = DVZ_FIELD_DIM_2D,
    .format = DVZ_FIELD_FORMAT_R32_FLOAT,
    .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
    .width = 512,
    .height = 512,
    .depth = 1,
});

dvz_sampled_field_set_geometry(field_scalar_2d, &(DvzFieldGeometry){
    .axis_order = {0, 1, 2},
    .axis_flip = {false, false, false},
    .origin = {0.0, 0.0, 0.0},
    .spacing = {1.0, 1.0, 1.0},
    .unit = "px",
});

dvz_sampled_field_set_data(field_scalar_2d, &(DvzFieldDataView){
    .data = scalar_pixels,
    .bytes_per_row = 512 * sizeof(float),
});

DvzVisual* image_scalar = dvz_image(scene, 0);
dvz_visual_set_field(image_scalar, "field", field_scalar_2d);
dvz_visual_set_scale(image_scalar, "colormap", scale);

DvzSampledField* field_rgba_2d = dvz_sampled_field(scene, &(DvzSampledFieldDesc){
    .dim = DVZ_FIELD_DIM_2D,
    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
    .width = 256,
    .height = 256,
    .depth = 1,
});

dvz_sampled_field_set_data(field_rgba_2d, &(DvzFieldDataView){
    .data = rgba_pixels,
    .bytes_per_row = 256 * 4,
});

DvzVisual* image_rgba = dvz_image(scene, 0);
dvz_visual_set_field(image_rgba, "field", field_rgba_2d);

DvzSampledField* field_scalar_3d = dvz_sampled_field(scene, &(DvzSampledFieldDesc){
    .dim = DVZ_FIELD_DIM_3D,
    .format = DVZ_FIELD_FORMAT_R32_FLOAT,
    .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
    .width = 256,
    .height = 256,
    .depth = 180,
});

dvz_sampled_field_set_geometry(field_scalar_3d, &(DvzFieldGeometry){
    .axis_order = {0, 1, 2},
    .axis_flip = {false, false, false},
    .origin = {0.0, 0.0, 0.0},
    .spacing = {0.5, 0.5, 1.0},
    .unit = "mm",
});

dvz_sampled_field_set_data(field_scalar_3d, &(DvzFieldDataView){
    .data = volume_values,
    .bytes_per_row = 256 * sizeof(float),
    .rows_per_image = 256,
});

DvzVisual* volume = dvz_volume(scene, 0);
dvz_visual_set_field(volume, "field", field_scalar_3d);
dvz_visual_set_scale(volume, "colormap", scale);

dvz_sampled_field_update_region(field_scalar_2d, (DvzFieldRegion){
    .x = 64,
    .y = 64,
    .z = 0,
    .width = 32,
    .height = 32,
    .depth = 1,
}, &(DvzFieldDataView){
    .data = patch_values,
    .bytes_per_row = 32 * sizeof(float),
});
```


## Pressure On The API

This sketch pressures the design in these places:

1. `dvz_sampled_field()` must not be image-only or volume-only,
2. `DvzFieldFormat` must cover scalar and color payloads without inventing separate resource
   families,
3. `dvz_visual_set_field()` must bind semantics, not raw backend texture handles,
4. update-region descriptors must use sample-space coordinates, not byte offsets,
5. physical metadata belongs to the field, not to every consumer visual separately.
