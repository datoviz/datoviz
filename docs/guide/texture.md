# Textures

Textures in Datoviz are GPU-side image data used by many visuals, such as `image`, `marker`, `mesh`, and `volume`. They can be 1D, 2D, or 3D, single-channel or multi-channel, and support different interpolation and wrapping modes.

This page explains how to create, configure, and use textures in your application.

---

## Creating a Texture

To create a texture from a NumPy array, use the `app.texture_ND()` methods (with `N=1`, `2`, or `3`):

```python
texture = app.texture_2D(image)
```

You can also create an empty texture with a specified shape and upload data to it later.

### Texture Dimensions

* `texture_1D(data)`: for 1D colormap textures or lookup tables
* `texture_2D(image)`: for standard 2D images
* `texture_3D(volume)`: for volumetric data

---

## Texture Parameters

When creating a texture, you can specify either an image, or the texture parameters:

| Parameter       | Description                                                       |
| --------------- | ------------------------------------------------------------------|
| `shape`         | Shape of the texture (optional if `image` is given).              |
| `n_channels`    | Number of channels (e.g., 1 for grayscale, 3 for RGB, 4 for RGBA).|
| `dtype`         | Data type of the texture. Common types: `np.uint8`, `np.float32`. |

In all cases, you can also specify interpolation and address wrapping mode:

| Parameter       | Description                                                       |
| --------------- | ------------------------------------------------------------------|
| `interpolation` | Interpolation mode                                                |
| `address_mode`  | Address wrapping mode                                             |

---

## Texture Formats

When using an `image`, the format is automatically inferred from its shape and dtype:

* **1-channel**: grayscale or scalar field
* **3-channel**: RGB color
* **4-channel**: RGBA color

Datoviz supports integer and float types such as `uint8`, `int16` and `float32` arrays.

---

## Interpolation Modes

Interpolation determines how texels are sampled when scaling or filtering.

| Value                 | Description                                |
| --------------------- | ------------------------------------------ |
| `nearest` (default)   | No interpolation; use nearest texel        |
| `linear`              | Smooth linear interpolation between texels |

---

## Address Modes

Address mode defines how out-of-bound texture coordinates are handled.

| Value                         | Description                            |
| ----------------------------- | -------------------------------------- |
| `clamp_to_border` (default)   | Clamp to a transparent or fixed border |
| `clamp_to_edge`               | Clamp to the edge texel                |
| `repeat`                      | Wrap around to the other side          |
| `mirrored_repeat`             | Mirror the texture across boundaries   |
| `mirror_clamp_to_edge`        | Mirror once and clamp at the edge      |


---

## Uploading data later

You can create an empty texture and upload data later:

```python
texture = app.texture_2D(shape=(512, 512), n_channels=4, dtype=np.uint8)
texture.data(my_image)
```

You can also upload to a sub-region using an offset:

```python
texture.data(my_tile, offset=(128, 128, 0))
```

---

## Using a Texture in a Visual

Once created, textures can be assigned to visuals using:

```python
visual.set_texture(texture)
```

### Example: 2D Image

```python
visual = app.image(...)
texture = app.texture_2D(image, interpolation='linear')
visual.set_texture(texture)
```

### Example: 3D Volume

```python
visual = app.volume(mode='rgba')
texture = app.texture_3D(volume_data, shape=(W, H, D))
visual.set_texture(texture)
```

---

## See Also

* [**Image** visual](../visuals/image.md)
* [**Volume** visual](../visuals/volume.md)
* [**Mesh** visual](../visuals/mesh.md)
* [**Marker** visual](../visuals/marker.md)
* [**Glyph** visual](../visuals/glyph.md)
