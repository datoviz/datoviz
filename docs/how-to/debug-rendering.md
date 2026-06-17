# Debug Rendering Output

Narrow down blank frames, wrong colors, missing geometry, and unexpected depth behavior.

## Task Workflow

Reduce to the smallest canonical example, verify the scene hierarchy, verify visual attributes and
counts, render offscreen, then add controllers, transforms, and techniques back one at a time.

## Minimal Checklist

```text
scene -> figure -> panel -> visual -> attributes -> panel add -> app/view -> frame
```

If any step is missing, fix that before investigating backend behavior.

## Canonical Examples

- Gallery: [Basic Scene](../examples/gallery/features/feature_basic_scene.md)
- Source: `examples/c/features/basic_scene.c`
- Gallery: [Depth Test Toggle](../examples/gallery/features/technique_depth_test.md)
- Source: `examples/c/features/technique_depth_test.c`
- Gallery: [Offscreen Capture](../examples/gallery/features/feature_offscreen_capture.md)
- Source: `examples/c/features/offscreen_capture.c`

## Important Details

Blank output is usually caused by no attached visual, wrong attribute count, wrong coordinate range,
camera/domain mismatch, or a technique state such as depth or blending hiding geometry.

## Common Mistakes

- Debugging Vulkan before checking whether `dvz_panel_add_visual()` was called.
- Testing with transparent colors and no known opaque baseline.
- Comparing screenshots with random data or non-deterministic camera state.

## See Also

- [Render offscreen and capture](render-offscreen.md)
- [Control depth, blending, and transparency](rendering-techniques.md)
- [Diagnose build and platform issues](diagnose-platform.md)
