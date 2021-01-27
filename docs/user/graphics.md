# Graphics library


!!! note
    If not specified, the default vertex structure is `VklVertex`:

    | Field | Type | Description |
    | ---- | ---- | ---- |
    | `pos` | `vec3` | position |
    | `color` | `cvec4` | color RGBA (four bytes) |

<!-- The code snippets are handled by a mkdocs hook -->

## 2D graphics

### Marker

![](../images/graphics/marker.png)

```c
Marker
```

### Segment

![](../images/graphics/segment.png)

```c
Segment
```

### Text

![](../images/graphics/text.png)

```c
Text
```


### Image

![](../images/graphics/image.png)

```c
Image
```


## 3D graphics

### Mesh

![](../images/graphics/mesh.png)

```c
Mesh
```

The mesh graphics supports the following features:

- Blinn-Phong shading
- Up to four textures
- Customizable texture blending coefficients
- Transparency (but does not play well with depth test)
- Support for arbitrary RGB values (via cvec3 packing into vec2)
- Customizable plane clipping

Plane clipping: when the clip vector is non-zero, the fragment shader implements the following test. If the dot product of the clip vector with the vertex position (in scene coordinates) is negative, the fragment is discarded. This feature allows to cut the mesh along any arbitrary affine plane.


### Volume slice

![](../images/graphics/volume_slice.png)

```c
VolumeSlice
```


### Volume

![](../images/graphics/volume.png)

```c
Volume
```


## Basic graphics

### Points

![](../images/graphics/points.png)

```c
Point
```


### Lines

![](../images/graphics/lines.png)

```c
Lines
```


### Line strip

![](../images/graphics/line_strip.png)


### Triangles

![](../images/graphics/triangles.png)


### Triangle strip

![](../images/graphics/triangle_strip.png)


### Triangle fan

![](../images/graphics/triangle_fan.png)

!!! warning
    Triangle fan graphics is not supported on macOS and should therefore be avoided if macOS compatibility is desirable.
