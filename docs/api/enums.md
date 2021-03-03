# Enumerations

## Scene

### `DvzControllerType`



## Visuals

### `DvzMarkerType`

### `DvzJoinType`

### `DvzCapType`

### `DvzPathTopology`




## Colormaps

The following colormaps, in addition to being stored in the colormap texture, are also implemented directly in GLSL. They may thereby be used directly from shaders without using the colormap texture. Avoiding texture lookup is sometimes faster.

This is implemented in the function `vec4 colormap(int cmap, float x)` in `colormaps.glsl`.

| Colormap |
| ---- |
| `DVZ_CMAP_HSV` |
| `DVZ_CMAP_CIVIDIS` |
| `DVZ_CMAP_INFERNO` |
| `DVZ_CMAP_MAGMA` |
| `DVZ_CMAP_PLASMA` |
| `DVZ_CMAP_VIRIDIS` |
| `DVZ_CMAP_AUTUMN` |
| `DVZ_CMAP_BONE` |
| `DVZ_CMAP_COOL` |
| `DVZ_CMAP_COPPER` |
| `DVZ_CMAP_HOT` |
| `DVZ_CMAP_SPRING` |
| `DVZ_CMAP_SUMMER` |
| `DVZ_CMAP_WINTER` |
| `DVZ_CMAP_JET` |




## Miscellaneous

### `DvzDataType`






## Flags

!!! note
    Flags specification for the canvas, scene, visuals, controllers are still being improved and may change at any time.

### Canvas flags

```
0x0001: ImGUI
0x0002: FPS
0x0003: FPS+FPS GUI
0x0004: enable picking
0xD000: DPI scaling (D=1..4 for 50%, 100%, 150%, 200%)
```



### Panel flags

```
0x00XX: transform
0xXX00: controller-specific flags
```


### Visual flags

```
0x000X: visual-specific flags
0x00X0: POS prop transformation flags
0x0X00: graphics flags (0x0100: enable depth test)
0xX000: interact axes
```

### Axes

Axes visual flags:

```
0x000X`, X=0 (x axis) or  X=1 (y axis).
```

When using an axes controller, the controller-specific flags in `0x0X00` (to hide minor/grid level) are passed to the axes visual flags as `0x000X` (bit shift). Note that the first bit must be reserved to the axis coordinate (0/1), so we use higher bits for the axes flags (4 and 8).
