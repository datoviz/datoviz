# Transform Visual Data

Apply object transforms or user scales without rewriting source data.

## Task Workflow

Use panel domains for the view's data range. Use visual transforms when one visual needs a local
translation, scale, or rotation relative to the panel. Use user scales when an example explicitly
models non-default coordinate scaling.

Choose the mechanism by the question you are answering:

| Need | Use | Changes |
| --- | --- | --- |
| Show a different data interval | Panel domain and controller | Visible range, not source data. |
| Interpret positions as data, view, or panel coordinates | `DvzVisualAttachDesc.coord_space` | How one panel attachment maps visual positions. |
| Move, rotate, shear, or scale one visual as an object | `dvz_visual_set_transform()` | One visual's retained local model transform. |
| Enlarge screen-space markers, strokes, axes, or UI-like elements | `dvz_view_set_user_scale()` | Presentation size, not data units. |
| Map scalar or categorical values to colors/labels | `DvzScale` and `dvz_visual_set_scale()` | Semantic color/label scale, not geometry. |

## Visual Transform Call Sequence

```c
mat4 transform = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.2f, 0.1f, 0.0f, 1.0f},
};
dvz_visual_set_transform(visual, transform);
dvz_panel_add_visual(panel, visual, NULL);
```

`dvz_visual_set_transform()` stores a visual-local affine transform on the visual. It applies to
every panel attachment of that visual before panel controller and view transforms. Use
`dvz_visual_has_transform()`, `dvz_visual_get_transform()`, and `dvz_visual_clear_transform()` when
an example needs to inspect or reset retained transform state.

The transform is useful when the same local geometry needs object placement:

```c
mat4 transform = {
    {1.20f, 0.00f, 0.00f, 0.00f},
    {0.00f, 0.80f, 0.00f, 0.00f},
    {0.00f, 0.00f, 1.00f, 0.00f},
    {0.25f, 0.10f, 0.00f, 1.00f},
};
dvz_visual_set_transform(visual, transform);
```

Use the matrix setup pattern from `examples/c/features/visual_transform.c` for rotations, shears,
non-uniform scales, or combined affine transforms.

## Domains and Attachment Space

Do not use a visual transform to compensate for a wrong data domain. If positions are data values,
set the panel domain and attach the visual in data coordinate space:

```c
dvz_panel_set_domain(panel, DVZ_DIM_X, xmin, xmax);
dvz_panel_set_domain(panel, DVZ_DIM_Y, ymin, ymax);

DvzVisualAttachDesc attach = dvz_visual_attach_desc();
attach.coord_space = DVZ_COORD_DATA;
dvz_panel_add_visual(panel, visual, &attach);
```

Use `DVZ_COORD_VIEW` only when positions are already normalized view coordinates, typically around
`[-1, +1]`. Use `DVZ_COORD_PANEL` for panel-fixed overlays. See
[Use coordinate systems](coordinate-systems.md) for the full coordinate-space distinction.

## User Scale

User scale is presentation scale on a view. It is appropriate for screen-space sizes such as marker
diameter, stroke width, axes, labels, and GUI-adjusted readability. It should not be used to change
data units or world geometry.

```c
float scale = dvz_view_user_scale(view);
dvz_view_set_user_scale(view, scale * 1.25f);
```

The user-scale example drives this value from a GUI slider. It changes visual readability without
rewriting source positions or changing the panel domain.

## Semantic Scales

`DvzScale` is a semantic mapping object, not a geometry transform. Use it for scalar colormaps,
categorical labels, colorbars, legends, and query/probe metadata.

```c
DvzScale* scale = dvz_scale(scene, NULL);
dvz_scale_set_domain(scale, 0.0, 1.0);
dvz_scale_set_colormap(scale, colormap);
dvz_visual_set_scale(visual, "color", scale);
```

Use [Use colormaps](use-colormaps.md) and [Use sampled fields](use-sampled-fields.md) for scale
bindings on scalar images, volumes, labels, points, or pixels.


## Important Details

Transforms are retained scene objects. Keep them alive with the scene and update them through the
scene API rather than baking every camera or scale change into raw positions.

Visual transforms belong to the visual, not to one panel attachment. If the same visual is attached
to multiple panels, the retained transform affects all of those attachments. Create a separate
visual when two panels need different object transforms.

The future descriptor path, `dvz_visual_set_transform_desc()`, accepts only `NULL` or
`DVZ_VISUAL_TRANSFORM_NONE` in v0.4. Use `dvz_visual_set_transform()` for the supported affine
visual-local transform.

For animated or frequently changing placement, update the retained transform instead of uploading
new positions every frame when the vertex data itself is unchanged.

## Common Mistakes

- Using transforms to compensate for a wrong panel domain.
- Uploading data coordinates with the default `DVZ_COORD_VIEW` attachment, then adding a transform
  to make the result look right.
- Applying both data scaling and visual scaling without documenting the final units.
- Expecting `dvz_view_set_user_scale()` to change data coordinates or camera distance.
- Expecting `DvzScale` color/label mappings to scale geometry.
- Sharing one transformed visual between panels that need different local transforms.
- Expecting WebGPU parity for every native transform example; check the manifest status.

## See Also

- [Use coordinate systems](coordinate-systems.md)
- [Add visuals to a panel](add-a-visual.md)
- [Configure cameras](configure-cameras.md)
- [Use colormaps](use-colormaps.md)
- [Use sampled fields](use-sampled-fields.md)
- [Profile rendering performance](profile-performance.md)

??? example "Related examples"

    - [Visual Transform](../examples/gallery/features/feature_visual_transform.md) - Source: `examples/c/features/visual_transform.c`
    - [User Scale](../examples/gallery/features/feature_user_scale.md) - Source: `examples/c/features/user_scale.c`
    - [Reference Grid](../examples/gallery/features/feature_reference_grid.md) - Source: `examples/c/features/reference_grid.c`
