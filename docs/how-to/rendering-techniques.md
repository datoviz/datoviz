# Control Depth, Blending, and Transparency

Choose the rendering state for overlapping geometry, transparent visuals, and depth-based visual
cues.

## Use This When

- 3D objects should occlude each other correctly.
- A visual has meaningful alpha values and must blend with the scene.
- Dense or translucent geometry looks wrong because draw order matters.
- Depth variation is hard to read and needs depth cueing, SSAO, or another depth technique.
- A WebGPU target needs a status check before using a native rendering technique.

## Minimal Sequence

Start with an opaque scene. Enable depth testing on 3D visuals that should participate in scene
occlusion.

```c
dvz_visual_set_depth_test(visual, true);
```

Only then add transparency to the visuals that actually need it.

```c
dvz_visual_set_data(visual, "color", rgba, n);
dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED);
```

Use alpha blending only when the color data carries useful alpha. Keep technique state on the
visual that needs it instead of changing unrelated scene state.

## Technique Choice

| Goal | Use | Notes |
| --- | --- | --- |
| Ordinary opaque 3D geometry | `dvz_visual_set_depth_test(visual, true)` with the default opaque alpha mode | Best default for mesh, primitive, sphere, point, and pixel visuals in 3D. |
| Screen-like overlays or diagnostics | `dvz_visual_set_depth_test(visual, false)` | Useful for overlays; avoid using it to hide an ordering bug in 3D data. |
| Simple translucent visual | `DVZ_ALPHA_BLENDED` | Source-over blending. Result can depend on draw order. |
| Many overlapping translucent items | `DVZ_ALPHA_WBOIT` | Weighted blended order-independent transparency. More stable than source-over, but approximate. |
| Layered transparent surfaces | `DVZ_ALPHA_DEPTH_PEEL` | More accurate order-independent transparency for layered surfaces; native path, higher cost. |
| Depth perception in dense 3D scenes | `dvz_visual_set_depth_cue()` | Fades or darkens by depth without changing geometry. |
| Jagged edges | MSAA example and panel/runtime sample settings | Antialiasing is separate from alpha correctness. |

## Transparency Rules

Transparent rendering is not just a color-alpha setting. Depth testing, alpha mode, and draw order
interact:

- Opaque visuals should usually render first with depth testing enabled.
- Transparent visuals should usually keep depth testing enabled so opaque geometry can occlude them.
- `DVZ_ALPHA_BLENDED` does not sort your data for you; overlapping transparent objects may still
  need a different technique.
- `DVZ_ALPHA_WBOIT` and `DVZ_ALPHA_DEPTH_PEEL` ask the scene planner for specialized transparency
  passes. Check backend support before relying on them in browser examples.
- Text, labels, images, and some annotation helpers may configure alpha blending internally because
  their pixels naturally include transparent coverage.

## Depth Cue

Depth cueing is a visual perception aid, not a camera or clipping change. It is supported on point,
pixel, primitive, mesh, and sphere visuals.

```c
DvzDepthCueDesc cue = dvz_depth_cue_desc();
cue.near_depth = 0.10f;
cue.far_depth = 0.90f;
dvz_visual_set_depth_cue(visual, &cue);
```

`near_depth` and `far_depth` use the descriptor metric, where lower values are closer. The default
metric is normalized clip depth after the scene transform. Pass `NULL` to
`dvz_visual_set_depth_cue()` to disable depth cueing.

## Backend Status

The native Vulkan path is the reference path for the advanced transparency techniques. In the
current WebGPU/WASM slice, depth testing and basic alpha blending have live coverage, while depth
cueing, WBOIT, and depth peeling are marked deferred in the example matrix. Treat deferred WebGPU
techniques as native-only until the corresponding gallery page advertises a live route.

## Canonical Examples

![Depth test toggle](../assets/gallery/v0.4/features/technique_depth_test.webp)

- [Depth Test Toggle](../examples/gallery/features/technique_depth_test.md) - compare overlapping
  3D points with depth testing on and off. Source:
  `examples/c/features/technique_depth_test.c`.
- [Alpha Blending](../examples/gallery/features/alpha_blending.md) - ordinary source-over alpha
  blending. Source: `examples/c/features/alpha_blending.c`.
- [Transparency Order](../examples/gallery/features/technique_transparency.md) - compare
  source-over, WBOIT, and depth-peel transparency on overlapping cubes. Source:
  `examples/c/features/technique_transparency.c`.
- [Depth Cue](../examples/gallery/features/technique_depth_cue.md) - depth-dependent fading on a
  3D sphere lattice. Source: `examples/c/features/technique_depth_cue.c`.
- [Multisample Antialiasing](../examples/gallery/features/technique_msaa.md) - use MSAA for edge
  quality. Source: `examples/c/features/technique_msaa.c`.


## Important Details

- Depth testing is per visual in the retained scene API. Do not solve one visual's ordering issue by
  disabling depth everywhere.
- Transparent visuals can still be hidden by opaque depth. That is usually correct.
- Picking and probing follow rendered visibility rules. A visual hidden by depth state may also be
  absent from the expected query result.
- Volume, SSAO, EDL, and other depth-based techniques have their own examples and status. Do not
  assume a technique is portable just because the visual family is portable.

## Common Mistakes

- Disabling depth globally to fix one transparent visual.
- Assuming alpha values change draw order automatically.
- Using `DVZ_ALPHA_BLENDED` for many interpenetrating translucent objects and expecting exact
  order-independent results.
- Turning off depth testing for labels or overlays without checking whether they should be occluded
  by 3D geometry.
- Copying deferred or native-only technique code into a WebGPU target without checking status.

## See Also

- [Use lighting and materials](lighting-and-materials.md)
- [Debug rendering output](debug-rendering.md)
- [Profile rendering performance](profile-performance.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [Visual API reference](../reference/c-api/visuals.md)
