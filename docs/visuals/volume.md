# Volume Visual

The **volume** visual displays one 3D scalar or RGBA sampled field. In v0.4 it is a retained scene
visual backed by `DvzSampledField`, lowered through the scene -> DRP2 -> vklite runtime path.

<figure markdown="span">
![Volume visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/volume.png)
</figure>

---

## Current v0.4 Scope

- One volume visual renders one 3D field.
- The default mode is full-volume composite rendering.
- Slice and MIP modes are selected explicitly.
- Bounds place the volume proxy in object/scene space.
- Slice position is normalized in `[0, 1]`.
- Scalar fields use a value range plus a 256x1 RGBA transfer texture.
- RGBA8 fields render directly from voxel color.

---

## When to use

Use the volume visual when:

- You want to visualize 3D data like MRI, CT, simulations
- You need an interactive 3D volume preview
- You want a movable orthogonal slice through a 3D field
- You need volume, mesh, and slice visuals in one shared 3D scene

---

## Properties

### Public C API

```c
DvzVisual* volume = dvz_volume(scene, 0);
dvz_visual_set_field(volume, "field", field);
dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE);
dvz_volume_set_opacity(volume, 0.75f);
dvz_volume_set_step_count(volume, 128);
dvz_volume_set_value_range(volume, 0.0, 4096.0);
dvz_volume_set_alpha_stops(
    volume,
    (DvzVolumeAlphaStop[]){{0.0, 0.0f}, {0.2, 0.0f}, {1.0, 0.8f}},
    3);
```

Available render modes:

| Mode | Behavior |
|---|---|
| `DVZ_VOLUME_RENDER_COMPOSITE` | Full-volume front-to-back alpha compositing |
| `DVZ_VOLUME_RENDER_MIP` | Maximum intensity projection |
| `DVZ_VOLUME_RENDER_SLICE` | One orthogonal slice through the 3D field |

Current controls:

| Control | Description |
|---|---|
| `dvz_volume_set_bounds()` | Object-space proxy bounds |
| `dvz_volume_set_slice_axis()` | X/Y/Z orthogonal slice axis |
| `dvz_volume_set_slice_position()` | Normalized slice position in `[0, 1]` |
| `dvz_volume_set_sampling()` | Nearest or linear sampling |
| `dvz_volume_set_clipping_box()` | Normalized axis-aligned clipping box |
| `dvz_volume_set_clipping_plane()` | One arbitrary normalized clipping plane |
| `dvz_volume_set_axis_mapping()` | Texture axis order and flips without CPU swizzling |
| `dvz_volume_set_value_range()` | Raw scalar range mapped to transfer coordinate `[0, 1]` |
| `dvz_volume_set_alpha_stops()` | Up to 8 normalized opacity stops |
| `dvz_visual_set_scale(..., "colormap", scale)` | Scalar colormap/transfer binding |

---

## Creation

To create a volume visual:

```c
DvzSampledField* field = dvz_sampled_field(scene, &desc);
dvz_sampled_field_set_data(field, &view);

DvzVisual* volume = dvz_volume(scene, 0);
dvz_visual_set_field(volume, "field", field);
dvz_panel_add_visual(panel, volume, NULL);
```

## Remaining v0.4 Hardening

The active slice probe path resolves `dvz_panel_probe()` requests against retained slice volumes and
returns object coordinates, normalized UVW, and the nearest retained voxel value. MIP/DVR picking is
still deferred.

See also:

* [Slice](slice.md): for 2D views of 3D data
* [Mesh](mesh.md): for explicit 3D geometry
