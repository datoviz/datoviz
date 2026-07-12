# Diagnose WebGPU Support

Check whether a Datoviz browser route should run, why it failed, and what fallback to use when the
current browser or adapter cannot run it.

The WebGPU path in v0.4 is experimental and example-scoped. A native Datoviz scene can be correct
while its browser route is planned, deferred, native-only, or unsupported on the selected adapter.
Debug the route status and browser environment before changing scene code.

For serving and deployment, use [Deploy Datoviz scenes to the web](deploy-to-web.md). For build,
import, or native device failures, use
[Diagnose build and platform issues](diagnose-platform.md).

## Diagnostic order

Use this order:

```text
example browser status
-> standalone live route
-> HTTP/HTTPS serving
-> browser WebGPU availability
-> console diagnostics
-> adapter and required feature limits
-> native baseline
-> WebGPU smoke or fixture output
```

Start with the standalone route. An iframe embedded in a gallery page adds page layout, permissions,
loading, and caching variables that are not useful for the first diagnosis.

## Check the example status

Look at the generated example page or the [WebGPU matrix](../examples/webgpu-matrix.md). The route
status determines what you should expect:

| Status | What to do |
| --- | --- |
| `webgpu-live` | A public browser route exists. Debug browser support, route loading, and runtime diagnostics. |
| `webgpu-planned` | Do not treat the missing live route as a runtime failure. Use the native example or static media. |
| `webgpu-deferred` | Browser support is intentionally postponed. Use the native path or fallback documentation. |
| `native-only` | The example depends on native behavior. Do not advertise a live WebGPU route. |

Only `webgpu-live` examples should be opened as public browser routes.

## Open the standalone route

Serve the docs site over HTTP, then open the route directly:

```sh
just wasm-scene-build
just serve
```

Open:

```text
http://localhost:8000/examples/webgpu/live.html?id=features_basic_scene
```

Replace `features_basic_scene` with the example id from the gallery page or matrix. Do not use
`file://`; WebGPU and module loading require a proper browser security context.

If the route id is wrong, the page should report a route error. Fix the route id before inspecting
shader, adapter, or scene behavior.

## Verify browser WebGPU availability

Use a current WebGPU-capable browser. Browser support depends on operating system, GPU, driver,
adapter choice, user settings, and the route's required features.

Check these in order:

| Check | What failure means |
| --- | --- |
| `navigator.gpu` exists. | The browser does not expose WebGPU in this context. |
| Page is served from `http://localhost`, HTTP, or HTTPS. | Direct filesystem loading or blocked origin policy can prevent WebGPU. |
| The browser console shows an adapter/device. | Browser could not select a usable GPU adapter or create a device. |
| Required assets load with 200 responses. | WASM, JavaScript, JSON, shader, or data assets are missing or blocked. |
| The canvas initializes before scene packets replay. | The route loaded, but WebGPU runtime setup may have failed. |

Record the browser version, OS, GPU/adapter, and first console error. Later errors are often
secondary.

## Read console diagnostics

The browser console is the primary diagnostic surface for live routes. Common messages usually map
to one of these layers:

| Console symptom | Likely layer | First action |
| --- | --- | --- |
| `navigator.gpu` missing. | Browser support or security context. | Try a WebGPU-capable browser and serve over HTTP/HTTPS. |
| Route id not found. | Example registration. | Check the id in the WebGPU matrix and generated route registry. |
| WASM module failed to load. | Build or static serving. | Run `just wasm-scene-build`, then serve the site again. |
| Adapter/device request failed. | Browser/GPU/driver capability. | Record adapter diagnostics and try another browser or GPU. |
| Unsupported command or visual. | WebGPU subset gap. | Check whether the example should be `webgpu-live`; compare native baseline. |
| Shader compilation error. | WGSL emission or backend path. | Save the console message and route id; compare WebGPU fixture output. |
| Canvas lost/reconfigured. | Browser runtime, resize, or device loss. | Reproduce on standalone route and record resize/device-loss messages. |
| Scene loads but frame is blank. | Scene data, packet replay, or rendering state. | Use [Debug rendering output](debug-rendering.md) with the native baseline. |

Do not summarize console output as "WebGPU failed" when filing a bug. The first specific adapter,
device, shader, route, or unsupported-feature message is the useful part.

## Compare against native output

For a `webgpu-live` route, compare the browser result with the canonical native example or generated
screenshot:

```sh
just example-c features/basic_scene
./build/examples/c/features/basic_scene
```

Use the source path listed on the gallery page. If the native example fails, fix native rendering or
platform support first. If native works and the WebGPU route fails, keep the investigation in the
browser route, WebGPU subset, WASM scene host, or adapter.

Native Vulkan success does not prove browser WebGPU success. WebGPU has a separate adapter, feature
set, shader path, presentation model, and asynchronous readback behavior.

## Run maintainer checks

For repository work, use the narrowest relevant validation:

```sh
just webgpu-fixture-preflight
just wasm-scene-smoke
just webgpu-browser-smoke
```

Use `just wasm-scene-smoke` when the question is whether the WASM scene host emits the expected
packets. Use `just webgpu-browser-smoke` when the question is whether live browser routes initialize
and render in the local browser environment.

Browser smoke can be limited by the local machine, CI runner, browser, or GPU. A browser skip or
adapter failure should be recorded as environment evidence, not silently converted into example
support.

## Browser versus native differences

Expect these differences in v0.4:

- browser routes use the promoted WebGPU subset, not full native Vulkan parity;
- browser request/query/readback is asynchronous;
- browser input, focus, resize, and timing differ from GLFW/native windows;
- the browser does not expose Qt, native video export, native GUI, direct Vulkan, or low-level
  vklite/canvas APIs;
- WebGPU JavaScript is host glue for registered routes, not a stable public Datoviz JS API.

Do not port a native example by rewriting scene behavior in JavaScript. A public live route should
reuse the canonical C example or portable C scenario.

## Choose a fallback

When a browser route does not run:

| Situation | Public fallback |
| --- | --- |
| Example is `webgpu-planned` or `webgpu-deferred`. | Link the native example and show static screenshot/video. |
| Browser lacks WebGPU. | Show static media and native instructions. |
| Current adapter lacks a required feature. | Record adapter diagnostics and provide native/static fallback. |
| Route is `webgpu-live` but fails with an unsupported command. | Treat as a route/subset bug or status mismatch. |
| Native example fails too. | Diagnose native rendering or platform support first. |

Do not relabel an example as `native-only` just because one browser/adapter failed. Change status
only at the source metadata after deciding the feature is outside the promoted subset.

## Report template

```text
Example id:
Gallery page:
Route URL:
Browser/version:
OS:
GPU/adapter:
Served from HTTP/HTTPS:
Route status from matrix:
Native example result:
First browser console error:
WASM build/smoke command result:
Screenshot or video:
```

The route URL, browser version, adapter, and first console error are usually enough to distinguish a
browser support problem from a Datoviz scene or renderer issue.

## Common mistakes

- Treating `webgpu-planned` as a failing live route.
- Debugging an iframe before trying `examples/webgpu/live.html?id=<example-id>` directly.
- Opening the route through `file://`.
- Assuming GLFW/native input behavior applies to browser routes.
- Assuming native Vulkan success implies browser WebGPU success.
- Reimplementing native scene behavior in JavaScript to work around an unsupported route.
- Dropping the first console error and keeping only later secondary failures.

## Next steps

- [Deploy Datoviz scenes to the web](deploy-to-web.md)
- [Debug rendering output](debug-rendering.md)
- [Diagnose build and platform issues](diagnose-platform.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [WebGPU matrix](../examples/webgpu-matrix.md)

??? example "Related examples"

    - [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - Gallery: [WebGPU Matrix](../examples/webgpu-matrix.md)
    - Manifest: `examples/c/MANIFEST.yaml`
