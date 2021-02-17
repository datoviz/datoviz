# Enumerations

## Scene

### `DvzControllerType`



## Visuals

### `DvzMarkerType`

### `DvzJoinType`

### `DvzCapType`

### `DvzPathTopology`



## Miscellaneous

### `DvzDataType`


## Flags

!!! note
    Flags specification for the canvas, scene, visuals, controllers are still being improved and may change at any time.

### Panel flags

```
0x00XX: transform
0xXX00: controller-specific flags
```


### Visual flags

```
0x000X: visual-specific flags
0x00X0: POS prop transformation flags
0x0X00: graphics depth test
0xX000: interact axes
```

### Axes

Axes visual flags:

```
0x000X`, X=0 (x axis) or  X=1 (y axis).
```

When using an axes controller, the controller-specific flags in `0x0X00` (to hide minor/grid level) are passed to the axes visual flags as `0x000X` (bit shift). Note that the first bit must be reserved to the axis coordinate (0/1), so we use higher bits for the axes flags (4 and 8).
