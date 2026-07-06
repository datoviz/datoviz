# Deploy WebGPU Examples to the Browser

Use the website's live WebGPU routes to run supported Datoviz examples in a browser.

This page is for examples that are already marked as browser-supported. If you are writing a new
visualization, start with the Python or native example first, then check whether the same example
has a live browser route.

## Task Workflow

Start from an example whose gallery page says `Browser support: Live in browser`. Serve the docs
site over HTTP or HTTPS, then open the route shown on that page.

## Route Shape

```text
examples/webgpu/live.html?id=feature_basic_scene
```

The `id` selects one supported example. If the `id` is unknown, the page shows a route error instead
of silently choosing another example.


## Run Locally

Build the WebGPU assets, start a local server, then open the route from that server:

```sh
just wasm-scene-build
just serve
```

Then open the route from the served site, not from `file://`:

```text
http://localhost:8000/examples/webgpu/live.html?id=feature_basic_scene
```

The exact port depends on the local `just serve` invocation. Do not open WebGPU pages through
`file://`; browsers restrict GPU access and resource loading from direct filesystem URLs.


## Check Browser Support

| Check | Why it matters |
| --- | --- |
| Browser supports WebGPU | The live route needs the browser WebGPU API. |
| Page is served over HTTP or HTTPS | Direct `file://` loading is not reliable for WebGPU routes. |
| Example status is `Live in browser` | Only promoted examples have browser routes. |
| Required data is available | Real-data examples may need prepared web assets. |
| Screenshot fallback exists | Users should still see a useful static preview if WebGPU is unavailable. |

Use [Diagnose WebGPU support](debug-webgpu.md) when the route fails to initialize.


## Status Values

| Status | Meaning |
| --- | --- |
| `webgpu-live` | Public live route exists for the promoted browser subset. |
| `webgpu-planned` | Intended browser route, but not promoted yet. |
| `webgpu-deferred` | Browser support is intentionally postponed. |
| `native-only` | Native/runtime feature should link to native validation or static media instead. |

Only `webgpu-live` examples should be linked as live browser routes. Other statuses need fallback
links, screenshots, videos, or native instructions.

A live route is not proof on every browser and GPU. Before publishing a route as supported, run the
browser smoke test or record a manual browser result on the target platform:

```sh
just wasm-scene-smoke
just webgpu-browser-smoke
```


## Deployment Notes

Serve WebGPU pages over HTTP or HTTPS. Browser WebGPU APIs do not work reliably from direct
filesystem URLs, and browser support varies by platform, GPU, driver, and user settings.

Data-backed live examples need prepared web bundles with redistribution and provenance handled
explicitly. Do not silently synthesize missing data in browser glue, and do not grow the base WASM
module by preloading unrelated datasets.


## Important Details

Only examples marked `webgpu-live` have browser routes. `webgpu-planned`, `webgpu-deferred`, and
`native-only` examples need fallback links or native validation.

Do not inline the WebGPU runtime inside handwritten How-To pages. Embed or link the standalone live
route so it owns its own document, scripts, canvas, query parameters, permissions, and diagnostics.

If you are promoting a new browser route, use the contributor workflow rather than copying an
existing route by hand. Browser examples should reuse the canonical C example or portable C
scenario; JavaScript remains host glue.

## Common Mistakes

- Assuming every native example has a WebGPU live route.
- Copying GLFW input code into browser examples.
- Moving browser runtime JavaScript into handwritten How-To pages.
- Opening live routes through `file://` instead of a local or deployed HTTP server.
- Treating browser support as full native feature parity.

## See Also

- [Diagnose WebGPU support](debug-webgpu.md)
- [Record and replay frame streams](record-replay.md)
- [Debug rendering output](debug-rendering.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [Adding examples](../contributors/adding-examples.md)

??? example "Related examples"

    - [Basic Scene](../examples/gallery/features/feature_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Linked Panels](../examples/gallery/features/feature_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
    - [Point](../examples/gallery/visuals/point_2d.md) - Source: `examples/c/visuals/point.c`
