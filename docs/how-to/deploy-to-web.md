# Deploy Datoviz Scenes to the Web

Publish a Datoviz scene as an experimental browser WebGPU page.

The v0.4 browser path is meant for Datoviz scene code, not for handwritten JavaScript ports of
native examples. You build the same scene semantics into a WebAssembly module, the C/WASM scene
host emits Datoviz frame packets, and the browser runtime replays those packets through WebGPU.

!!! info "At a glance"

    - **Status:** Experimental repository integration, not a stable export command or public JS API.
    - **Languages:** Portable C scene/scenario code plus repository-owned JavaScript host glue.
    - **Prerequisites:** A native-valid scene inside the promoted WebGPU subset and a WebGPU-capable secure browser context.
    - **Result:** A registered HTTP-served route renders the same scene semantics on a browser canvas.

## Deployment workflow

Start with a working Datoviz scene on the native path. Keep it within the portable scene subset,
build it into the WASM scene host, serve the generated browser assets over HTTP or HTTPS, then open
the live route for that scene.

```text
native Datoviz scene -> WASM scene host -> DRP2 packets -> browser WebGPU runtime -> canvas
```

For code already in the Datoviz repository, the public gallery routes are the deployed form of this
workflow. For your own application code, treat the current path as an experimental integration
route: there is no stable `datoviz export-to-web` command in v0.4.


## Check what you can deploy

The browser path works best for scene-layer code that uses the promoted WebGPU subset: retained
visuals, ordinary data buffers, supported controllers, selected annotations/layout features, and
selected query/readback or compute examples.

Avoid native-only surfaces in the code you want to publish to the browser:

- GLFW windows, Qt hosting, native app callbacks, native video export, and GUI panels;
- direct Vulkan, `vklite`, `canvas`, or `stream` control;
- custom native resource ownership that the WASM scene host cannot recreate;
- assumptions that browser input, timing, filesystem access, or readback are synchronous.

Python `ctypes` code is a native API surface in v0.4. It is useful for developing and validating
the scene, but it is not uploaded unchanged into the browser. To publish that visualization today,
move the scene construction into a portable C scene/scenario or use a Datoviz-provided live route
that already does this.


## Prepare the scene

1. Validate the scene natively first.
2. Check that every required feature appears in the [WebGPU subset](../reference/webgpu-subset.md)
   or in a `webgpu-live` gallery example.
3. Keep data loading explicit. Browser examples with real data need prepared web bundles with
   redistribution and provenance handled before publication.
4. Add a portable scenario entry point when the scene needs browser input, animation, frame
   callbacks, queries, or retained updates.

The browser JavaScript should remain host glue. Do not reimplement scene behavior, picking,
animation, or data transforms in JavaScript just to make the page work.


## Build and serve

Inside the Datoviz checkout, build the WASM scene assets and start a local server:

```sh
just wasm-scene-build
just serve
```

### Route shape

```text
examples/webgpu/live.html?id=<scene-id>
```

The `id` selects one registered WASM scene route. If the `id` is unknown, the page shows a route
error instead of silently choosing another scene.


Open the route from the served site, not from `file://`:

```text
http://localhost:8294/examples/webgpu/live.html?id=features_basic_scene
```

Port 8294 is the default; `just serve` selects the next available port when it is busy and prints
the actual address. Do not open WebGPU pages through `file://`; browsers restrict GPU access and
resource loading from direct filesystem URLs.


## Use existing live routes

Existing gallery examples are useful as compatibility references. Start from an example whose
gallery page says `Browser support: Live in browser`, serve the docs site, then open the route shown
on that page.

The embedded route below is the canonical Basic Scene example running through the standalone
WebGPU host. It uses the same portable C scene as native validation; the iframe only hosts the
browser runtime, canvas, and diagnostics.

<div class="dvz-webgpu-live" markdown="1">
  <iframe src="../../examples/webgpu/live.html?id=features_basic_scene"
          title="Basic Scene WebGPU live example" loading="lazy"
          allow="fullscreen; webgpu"></iframe>
</div>

<a href="../../examples/webgpu/live.html?id=features_basic_scene">Open the Basic Scene live
example</a>.

Only examples marked `webgpu-live` have public browser routes. `webgpu-planned`,
`webgpu-deferred`, and `native-only` examples need fallback links, screenshots, videos, or native
instructions.


## Check browser support

| Check | Why it matters |
| --- | --- |
| Browser supports WebGPU | The live route needs the browser WebGPU API. |
| Page is served over HTTP or HTTPS | Direct `file://` loading is not reliable for WebGPU routes. |
| Scene route is registered | Only registered WASM scene routes can be opened by `live.html`. |
| Browser status is `Live in browser` | Only promoted examples should be advertised as public browser routes. |
| Required data is available | Real-data examples may need prepared web assets. |
| Screenshot fallback exists | Users should still see a useful static preview if WebGPU is unavailable. |

Use [Diagnose WebGPU support](debug-webgpu.md) when the route fails to initialize.


## Read status values

The authoritative [WebGPU example matrix](../examples/webgpu-matrix.md) defines `webgpu-live`,
`webgpu-planned`, `webgpu-deferred`, and `native-only`. Publish a live link only for a
`webgpu-live` row; use static media or native instructions for every other status.

A live route is not proof on every browser and GPU. Before publishing a route as supported, run the
browser smoke test or record a manual browser result on the target platform:

```sh
just wasm-scene-smoke
just webgpu-browser-smoke
```


## Deployment notes

Serve WebGPU pages over HTTP or HTTPS. Browser WebGPU APIs do not work reliably from direct
filesystem URLs, and browser support varies by platform, GPU, driver, and user settings.

Keep the generated WASM module, JavaScript runtime files, route registry, screenshots, and prepared
data bundles together. A static host is enough when it serves the files with normal HTTP semantics.

Generated WASM/JavaScript outputs are deployment artifacts. Rebuild them from the matching Datoviz
source and route registry; do not mix assets from different commits or cache versions.

Data-backed live examples need prepared web bundles with redistribution and provenance handled
explicitly. Do not silently synthesize missing data in browser glue, and do not grow the base WASM
module by preloading unrelated datasets.


## Caveats

The WebGPU path is experimental in v0.4. It is a portability route for a promoted scene subset, not
full native Vulkan parity and not a stable public JavaScript or TypeScript API.

Browser request/query/readback is asynchronous. Timing, adapter selection, canvas resize, browser
security rules, and GPU capability limits can change behavior even when the native scene is valid.

Do not inline the WebGPU runtime inside handwritten How-To pages. Embed or link the standalone live
route so it owns its own document, scripts, canvas, query parameters, permissions, and diagnostics.

If you are promoting a new public browser route in this repository, use the contributor workflow
rather than copying an existing route by hand. Browser examples should reuse the canonical C example
or portable C scenario.

## Common mistakes

- Assuming every native example has a WebGPU live route.
- Expecting a Python `ctypes` script to run unchanged in the browser.
- Copying GLFW input code into browser examples.
- Reimplementing scene behavior in JavaScript instead of using the WASM scene host.
- Moving browser runtime JavaScript into handwritten How-To pages.
- Opening live routes through `file://` instead of a local or deployed HTTP server.
- Treating browser support as full native feature parity.

## Next steps

- [Diagnose WebGPU support](debug-webgpu.md)
- [Record and replay frame streams](record-replay.md)
- [Debug rendering output](debug-rendering.md)
- [WebGPU subset](../reference/webgpu-subset.md)

??? example "Complete and related examples"

    - Canonical complete example: [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Linked Panels](../examples/gallery/features/features_panel_linked.md) - Source: `examples/c/features/panel_linked.c`
    - [Point](../examples/gallery/visuals/visuals_point.md) - Source: `examples/c/visuals/point.c`
