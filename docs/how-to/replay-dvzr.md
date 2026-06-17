# Record and Replay Frame Streams

Use DVZR frame streams for reproducible rendering and portability checks.

## Task Workflow

Record the command stream from a scene or runtime path, then replay it with the matching tool or
example. Use this when debugging the runtime boundary rather than ordinary scene authoring.

## Minimal Call Sequence

```c
/* Use the record/replay calls from examples/c/features/record_replay.c. */
/* Keep the recorded stream with the exact runtime and asset assumptions it needs. */
```


## Important Details

DVZR is a frame-stream/runtime concern. Use ordinary scene examples for application code unless the
task is replay, validation, or backend debugging.

## Common Mistakes

- Treating a replay stream as editable scene source.
- Recording with hidden local asset paths.
- Debugging high-level visual behavior through DRP2 before checking the scene example.

## See Also

- [Debug rendering output](debug-rendering.md)
- [Deploy WebGPU examples to the browser](deploy-to-web.md)
- [Profile rendering performance](profile-performance.md)

??? example "Related examples"

    - Gallery: [Record Replay](../examples/gallery/features/feature_record_replay.md)
    - Source: `examples/c/features/record_replay.c`
    - Advanced gallery: [Raw Triangle DRP2](../examples/gallery/advanced/advanced_raw_triangle_drp2.md)
    - Source: `examples/c/advanced/raw_triangle_drp2.c`
