# Animate a Scene

Update visual attributes over time.

## Task Workflow

Create the visual once, then update only the changing attributes on each tick. Use CPU-side updates
for ordinary animations and compute-buffer examples for GPU-driven animation.

## Minimal Call Sequence

```c
static void on_timer(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)t;
    (void)dt;
    DvzVisual* visual = user_data;
    update_positions(pos);
    dvz_visual_set_data(visual, "position", pos, n);
}

DvzAnimation* timer = dvz_anim_timer(scene, 0.0, on_timer, visual);
dvz_anim_start(timer, 0.0);
```

Run the app normally after registering the scene animation.


## Important Details

Prefer stable visual and panel objects. Animate attributes, transforms, visibility, or controller
state. Use deterministic seeds for screenshots and tests.

## Common Mistakes

- Recreating GPU objects every frame.
- Updating arrays from another thread without synchronizing with the render loop.
- Assuming browser and native animation paths expose the same callbacks.

## See Also

- [Update visual data](update-visual-data.md)
- [Export videos](video-export.md)
- [Profile rendering performance](profile-performance.md)

??? example "Related examples"

    - Gallery: [Timer Animation](../examples/gallery/features/feature_timer_animation.md)
    - Source: `examples/c/features/timer_animation.c`
    - Gallery: [Animation Tracks](../examples/gallery/features/feature_animation_tracks.md)
    - Source: `examples/c/features/animation_tracks.c`
    - Gallery: [Compute Buffer Animation](../examples/gallery/features/feature_compute_buffer_animation.md)
    - Source: `examples/c/features/compute_buffer_animation.c`
