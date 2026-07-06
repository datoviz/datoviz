# Deploy WebGPU Examples to the Browser

Use the website's live WebGPU routes to run supported Datoviz examples in a browser.

This page is for testing or deploying examples that are already marked as browser-supported. It is
not the starting point for writing a new visualization; start with the native or Python example
first, then check whether the same example has browser support.

## Task Workflow

Start from an example whose gallery page says `Browser support: Live in browser`. Serve the docs
site over HTTP, then open the generated route:

## Minimal Route

```text
examples/webgpu/live.html?id=feature_basic_scene
```

If the `id` is unknown, the page shows a route error instead of silently choosing another example.


## Build And Check Locally

Use the WebGPU validation loop before treating a route as deployable:

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


## Where Browser Support Comes From

| File | Role |
| --- | --- |
| `examples/c/MANIFEST.yaml` | Declares public example metadata and WebGPU status. |
| `examples/webgpu/live_examples.js` | Registers live route ids, labels, scenario ids, and animation flags. |
| canonical C example or portable C scenario | Owns scene behavior and data semantics. |
| `tools/wasm_scene_smoke.mjs` | Checks WASM scene packet and scenario coverage. |
| `tools/webgpu_browser_smoke.mjs` | Exercises selected browser routes. |

The browser page should run the same supported example behavior. Do not reimplement animation,
picking, selection, query/probe, or data semantics in page-specific JavaScript.


## Status Values

| Status | Meaning |
| --- | --- |
| `webgpu-live` | Public live route exists for the promoted browser subset. |
| `webgpu-planned` | Intended browser route, but not promoted yet. |
| `webgpu-deferred` | Browser support is intentionally postponed. |
| `native-only` | Native/runtime feature should link to native validation or static media instead. |

Only `webgpu-live` examples should be linked as live browser routes. Other statuses need fallback
links, screenshots, videos, or native instructions.

A live route is not the same as proof on every browser and GPU. For release evidence, pair the
manifest status with `just webgpu-browser-smoke` output or a recorded manual browser result.


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
- Registering a live route without updating the manifest status and browser smoke coverage.
- Treating browser support as full native feature parity.

## See Also

- [Diagnose WebGPU support](debug-webgpu.md)
- [Record and replay frame streams](record-replay.md)
- [Debug rendering output](debug-rendering.md)
- [WebGPU subset](../reference/webgpu-subset.md)

??? example "Related examples"

    - [Basic Scene](../examples/gallery/features/feature_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Linked Panels](../examples/gallery/features/feature_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
    - [Point](../examples/gallery/visuals/point_2d.md) - Source: `examples/c/visuals/point.c`
