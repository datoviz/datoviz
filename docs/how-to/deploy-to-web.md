# Deploy WebGPU Examples to the Browser

Use generated WebGPU live routes for portable browser examples.

The browser route is an experimental deployment surface for examples that already run through the
shared scene -> DRP2 -> WASM/WebGPU path. It is not a separate JavaScript renderer.

## Task Workflow

Start from an example marked `webgpu-live` in `examples/c/MANIFEST.yaml`. Build the docs/site
artifacts, then open the generated route `examples/webgpu/live.html?id=<example-id>`.

## Minimal Route

```text
examples/webgpu/live.html?id=feature_basic_scene
```

The route hosts the WebGPU runtime, WASM bridge, canvas, and scenario data. If the `id` is unknown,
the live route fails through `examples/webgpu/live_examples.js`.


## Build And Smoke

Use the narrow WebGPU validation loop before treating a route as deployable:

```sh
just wasm-scene-build
just wasm-scene-smoke
just webgpu-browser-smoke
just serve
```

Then open the route from the served site, not from `file://`:

```text
http://localhost:8000/examples/webgpu/live.html?id=feature_basic_scene
```

The exact port depends on the local `just serve` invocation.


## Source Of Truth

| File | Role |
| --- | --- |
| `examples/c/MANIFEST.yaml` | Declares public example metadata and WebGPU status. |
| `examples/webgpu/live_examples.js` | Registers live route ids, labels, scenario ids, and animation flags. |
| canonical C example or portable C scenario | Owns scene behavior and data semantics. |
| `tools/wasm_scene_smoke.mjs` | Checks WASM scene packet and scenario coverage. |
| `tools/webgpu_browser_smoke.mjs` | Exercises selected browser routes. |

Browser glue should mount and run the canonical scenario. Do not reimplement scene behavior,
animation, picking, selection, query/probe, or data semantics in JavaScript.


## Status Values

| Status | Meaning |
| --- | --- |
| `webgpu-live` | Public live route exists and should work in supported WebGPU browsers. |
| `webgpu-planned` | Intended browser route, but not promoted yet. |
| `webgpu-deferred` | Browser support is intentionally postponed. |
| `native-only` | Native/runtime feature should link to native validation or static media instead. |

Only `webgpu-live` examples should be linked as live browser routes. Other statuses need fallback
links, screenshots, videos, or native instructions.


## Deployment Notes

Serve WebGPU pages over HTTP or HTTPS. Browser WebGPU APIs do not work reliably from direct
filesystem URLs, and browser support varies by platform, GPU, driver, and user settings.

Data-backed live examples need prepared web bundles with redistribution and provenance handled
explicitly. Do not silently synthesize missing data in browser glue, and do not grow the base WASM
module by preloading unrelated datasets.


## Important Details

Only examples marked `webgpu-live` have browser routes. `webgpu-planned`, `webgpu-deferred`, and
`native-only` examples need fallback links or native validation.

Do not inline the WebGPU runtime inside handwritten how-to pages. Embed or link the standalone live
route so it owns its own document, scripts, canvas, query parameters, permissions, and diagnostics.

## Common Mistakes

- Assuming every native example has a WebGPU live route.
- Copying GLFW input code into browser examples.
- Moving browser runtime JavaScript into handwritten How-To pages.
- Opening live routes through `file://` instead of a local or deployed HTTP server.
- Registering a live route without updating the manifest status and smoke coverage.
- Treating WebGPU parity as full native Vulkan parity rather than the documented experimental
  subset.

## See Also

- [Diagnose WebGPU support](debug-webgpu.md)
- [Record and replay frame streams](replay-dvzr.md)
- [Debug rendering output](debug-rendering.md)
- [WebGPU subset](../reference/webgpu-subset.md)

??? example "Related examples"

    - [Basic Scene](../examples/gallery/features/feature_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Linked Panels](../examples/gallery/features/feature_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
    - [Point](../examples/gallery/visuals/point_2d.md) - Source: `examples/c/visuals/point.c`
