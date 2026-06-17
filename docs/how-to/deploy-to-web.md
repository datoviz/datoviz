# Deploy WebGPU Examples to the Browser

Use generated WebGPU live routes for portable browser examples.

## Task Workflow

Start from an example marked `webgpu-live` in `examples/c/MANIFEST.yaml`. Build the docs/site
artifacts, then open the generated route `examples/webgpu/live.html?id=<example-id>`.

## Minimal Call Sequence

```text
examples/webgpu/live.html?id=feature_basic_scene
```

The route hosts the WebGPU runtime, WASM bridge, canvas, and scenario data. Do not inline that
runtime inside ordinary documentation pages.


## Important Details

Only examples marked `webgpu-live` have browser routes. `webgpu-planned`, `webgpu-deferred`, and
`native-only` examples need fallback links or native validation.

## Common Mistakes

- Assuming every native example has a WebGPU live route.
- Copying GLFW input code into browser examples.
- Moving browser runtime JavaScript into handwritten How-To pages.

## See Also

- [Diagnose WebGPU support](debug-webgpu.md)
- [Record and replay frame streams](replay-dvzr.md)
- [Debug rendering output](debug-rendering.md)

??? example "Related examples"

    - Gallery: [Basic Scene](../examples/gallery/features/feature_basic_scene.md)
    - Source: `examples/c/features/basic_scene.c`
    - Gallery: [Linked Panels](../examples/gallery/features/feature_panel_linked.md)
    - Source: `examples/c/features/panel_linked.c`
    - Gallery: [Point](../examples/gallery/visuals/point_2d.md)
    - Source: `examples/c/visuals/point.c`
