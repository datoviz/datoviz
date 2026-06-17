# Control Depth, Blending, and Transparency

Tune rendering behavior when overlapping geometry matters.

## Task Workflow

First make the opaque scene correct with depth testing. Add alpha blending only for visuals that
need it. Use specialized techniques such as MSAA, depth cue, SSAO, or volume occlusion when they
solve a specific visual problem.

## Minimal Call Sequence

```c
dvz_visual_set_data(visual, "color", rgba, n);
dvz_visual_set_depth_test(visual, true);
dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED);
```

Use alpha blending only for visuals with meaningful alpha. Keep technique state close to the visual
or pass that actually needs it.


## Important Details

Transparent rendering is order-sensitive. Depth testing, blending mode, and draw order must be
considered together.

## Common Mistakes

- Disabling depth globally to fix one transparent visual.
- Assuming alpha values change draw order automatically.
- Copying deferred or native-only technique code into a WebGPU target without checking status.

## See Also

- [Use lighting and materials](lighting-and-materials.md)
- [Debug rendering output](debug-rendering.md)
- [Profile rendering performance](profile-performance.md)

??? example "Related examples"

    - Gallery: [Depth Test Toggle](../examples/gallery/features/technique_depth_test.md)
    - Source: `examples/c/features/technique_depth_test.c`
    - Gallery: [Alpha Blending](../examples/gallery/features/alpha_blending.md)
    - Source: `examples/c/features/alpha_blending.c`
    - Gallery: [Transparency Order](../examples/gallery/features/technique_transparency.md)
    - Source: `examples/c/features/technique_transparency.c`
    - Gallery: [Multisample Antialiasing](../examples/gallery/features/technique_msaa.md)
    - Source: `examples/c/features/technique_msaa.c`
