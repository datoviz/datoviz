# Map Scalar Values with Colormaps

Convert scalar data into colors and expose the scale to readers.

![Scalar values mapped through a colormap with a matching color scale](../assets/gallery/v0.4/features/features_colormap_scale.webp)

!!! info "At a glance"

    **Status:** Supported continuous and categorical scale objects · **Languages:** Python and C
    **Prerequisites:** Scalar values with an explicit finite domain, or category IDs with labels
    **Result:** A reproducible color encoding that can be shared with colorbars and readouts

## Task workflow

Choose the scalar range, choose a colormap, upload the mapped colors or sampled-field data, then add
a colorbar when the visual result needs interpretation.

Choose the color path by what the data represents:

| Data | Use | Add |
| --- | --- | --- |
| Scalar values on point or pixel items. | Set the `"color"` attribute format to `DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32`, bind a continuous `DvzScale`, then upload float values to `"color"`. | Colorbar when readers need the numeric range. |
| Regular 2D or 3D scalar arrays. | Keep the data as a `DvzSampledField`, bind the same continuous scale to the image or volume visual. | Colorbar and probe/readout that use the same range. |
| Already-computed RGBA colors. | Upload RGBA directly to `"color"`. | No colorbar unless there is a real scalar scale behind those colors. |
| Category or label ids. | Use a categorical scale and legend. | Legend, not a continuous colorbar. |

## Minimal call sequence

Prerequisite: create `scene`, `panel`, a point `visual`, and C-contiguous position and scalar arrays.
The result maps the scalar `"color"` attribute through Viridis over `[0, 1]`. The snippets are setup
fragments; see the complete
[Scalar Color Scale example](../examples/gallery/features/features_colormap_scale.md).

### Python

```python
import ctypes
import numpy as np
import datoviz as dvz

positions = np.asarray(positions, dtype=np.float32, order="C")
values = np.asarray(values, dtype=np.float32, order="C")
colormap = dvz.dvz_colormap_builtin(scene, dvz.DVZ_BUILTIN_COLORMAP_VIRIDIS)

desc = dvz.dvz_scale_desc()
desc.kind = dvz.DVZ_SCALE_CONTINUOUS
desc.label = b"scalar value"
scale = dvz.dvz_scale(scene, ctypes.byref(desc))
if not colormap or not scale:
    raise RuntimeError("colormap or scale creation failed")
dvz.dvz_scale_set_domain(scale, 0.0, 1.0)
dvz.dvz_scale_set_colormap(scale, colormap)

dvz.dvz_visual_set_attr_format(visual, b"color", dvz.DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32)
dvz.dvz_visual_set_scale(visual, b"color", scale)
dvz.dvz_visual_set_data(visual, "position", positions)
dvz.dvz_visual_set_data(visual, "color", values)
dvz.dvz_panel_add_visual(panel, visual, None)
```

### C

```c
float values[N] = {0};

DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
DvzScale* scale = dvz_scale(
    scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
               .kind = DVZ_SCALE_CONTINUOUS,
               .label = "scalar value",
           });
dvz_scale_set_domain(scale, 0.0, 1.0);
dvz_scale_set_colormap(scale, colormap);

dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
dvz_visual_set_scale(visual, "color", scale);
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", values, n);
dvz_panel_add_visual(panel, visual, NULL);
```

If you already have RGBA colors, upload them directly to `"color"` and skip the scale and colormap.
For sampled fields, use the image or volume path shown in the field examples instead of manually
expanding every scalar to geometry.

Values at the low and high ends of the domain use the ends of Viridis; intermediate values are
interpolated. Add a colorbar bound to this same `scale` when readers need to recover numeric values.


## Important details

Keep the scalar domain explicit. The colorbar should match the normalization used for the visual,
not just the colormap name.

The `DvzScale` is the contract between scalar data, color mapping, colorbar labels, and readouts.
Create one continuous scale, set its domain and colormap, bind that scale to the visual, and pass
the same scale to the colorbar. Do not rebuild a separate color ramp for the colorbar.

For point and pixel visuals with scalar colors, the required sequence is:

1. Call `dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32)`.
2. Call `dvz_visual_set_scale(visual, "color", scale)`.
3. Upload float values to the `"color"` attribute.

For image and volume data, preserve the sampled-field shape instead of expanding the grid into
individual points. The field semantic, format, dimensions, scale domain, colorbar range, and probe
readout should all describe the same scalar values.

When scalar values or normalization change, update the retained scalar data or scale state. Panning
or zooming does not require recomputing colors because the scalar mapping is independent of the
visible panel range.

Categorical labels are different from scalar values. Even if category ids are numeric, use a
categorical scale and legend when the numbers name classes rather than ordered magnitudes.

## Common mistakes

- Remapping colors after every pan or zoom instead of only when scalar values or normalization
  change.
- Creating a separate colorbar scale that does not match the visual's scale.
- Uploading direct RGBA colors and then adding a colorbar that implies a scalar mapping.
- Showing a colorbar for categorical colors that have no scalar order.
- Using a continuous colormap for label ids or segmented regions.
- Letting probe/readout values use a different range or unit format than the colorbar.
- Mixing premultiplied and straight alpha expectations in transparent colormaps.

## See also

- [Use sampled fields and textures](use-sampled-fields.md)
- [Add colorbars, scale bars, and legends](adornments.md)
- [Probe image or field values](probe-fields.md)
- [Update visual data](update-visual-data.md)
- [Control depth, blending, and transparency](depth-blending.md)

??? example "Complete and related examples"

    - Canonical complete example: [Scalar Color Scale](../examples/gallery/features/features_colormap_scale.md) - Source: `examples/c/features/colormap_scale.c`
    - [Colorbar](../examples/gallery/features/features_colorbar.md) - Source: `examples/c/features/colorbar.c`
    - [Sampled Field Update](../examples/gallery/features/features_sampled_field_update.md) - Source: `examples/c/features/sampled_field_update.c`
