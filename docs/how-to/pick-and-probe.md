# Pick Items

Map a pointer location to a rendered item or scene value.

## Task Workflow

Use picking when the target is a rendered item, instance, or primitive. Use probing when the target
is a sampled field value at a data coordinate.

## Minimal Call Sequence

```c
/* Convert pointer input through the panel/view path. */
/* Call the picking helper shown in examples/c/features/picking.c. */
/* Update selection or readout visuals from the returned item id. */
```

Keep the visual's item order stable if the pick result is used as an index into application data.


## Important Details

Picking is tied to what is rendered. Hidden, clipped, transparent, or depth-tested items may not
behave like a CPU-side nearest-neighbor search.

## Common Mistakes

- Using picking to read image scalar values; use field probing.
- Reordering visual data without updating the application-side id mapping.
- Expecting identical results from native and WebGPU paths without checking feature status.

## See Also

- [Probe image or field values](probe-fields.md)
- [Select and highlight data](select-items.md)
- [Handle input events](input-events.md)

??? example "Related examples"

    - Gallery: [Picking](../examples/gallery/features/feature_picking.md)
    - Source: `examples/c/features/picking.c`
    - Gallery: [Pixel Selection](../examples/gallery/features/feature_selection_pixel.md)
    - Source: `examples/c/features/selection_pixel.c`
    - Gallery: [Label Probe](../examples/gallery/features/feature_probe_labels.md)
    - Source: `examples/c/features/probe_labels.c`
