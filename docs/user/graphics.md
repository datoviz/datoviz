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
