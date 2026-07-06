# WebGPU Subset

Status: experimental v0.4 browser subset.

Datoviz v0.4 includes a browser path for selected examples. It lets you open supported scenes in a
WebGPU-capable browser without installing the native desktop runtime.

This browser path is useful for demos, documentation, portability checks, and early testing. It is
not full native Vulkan parity. Each public example says whether browser support is live, planned,
deferred, or native-only.


## How To Find Browser Examples

Start from the [Examples](../examples/index.md) section. On each generated example page, check the
example details:

```text
Browser support: Live in browser
```

Live examples use this route shape:

```text
examples/webgpu/live.html?id=<example-id>
```

For the generated public gallery:

| Browser status | Count | Meaning |
| --- | ---: | --- |
| Live in browser | 66 | A public live route exists for the promoted browser subset. |
| Planned | 15 | Browser support is intended, but the route is not promoted yet. |
| Deferred | 10 | Browser support is intentionally postponed. |
| Native only | 14 | The example depends on native runtime behavior and should use screenshots, videos, or native instructions instead. |

See the generated [WebGPU matrix](../examples/webgpu-matrix.md) for the current example-by-example
list.


## What Is Supported

The promoted browser subset covers the visual families and features needed by the live gallery
routes. Current coverage includes:

- point, pixel, marker, segment, path, primitive, image, labels, glyph, text, mesh, sphere, and
  vector examples;
- common 2D layout and annotation features, including panels, linked panels, axes, guides,
  colorbars, scale bars, legends, and readouts;
- panzoom for 2D scenes and selected 3D controller examples such as arcball, fly, and turntable;
- selected picking, probing, selection, animation, update, compute, and showcase routes.

Browser support is narrower than native support. When a feature matters to your application, check
the specific example page and the [Feature status](feature-status.md) page rather than assuming
native and browser behavior are identical.


## What Is Not Supported

The browser subset intentionally does not include:

- native windows, GLFW, Qt hosting, Vulkan, video, GUI, or low-level native runtime modules;
- full native app lifecycle parity;
- stable public JavaScript or TypeScript bindings for building arbitrary Datoviz scenes directly in
  browser code;
- custom shader replacement;
- broad volume, advanced technique, rich-label, query/readback, and zero-copy transport parity.

These limits may change after v0.4, but they should not be described as current browser support
until the example matrix and validation checks prove them.


## What `webgpu-live` Means

`webgpu-live` means a public route is registered and included in the promoted browser subset. It
does not mean every browser, GPU adapter, driver, and CI runner has produced visual evidence for
that route.

Keep these facts separate:

| Fact | Where to check |
| --- | --- |
| Public route exists | The generated example page and [WebGPU matrix](../examples/webgpu-matrix.md). |
| Browser smoke coverage passes | Maintainer validation output from `just webgpu-browser-smoke`. |
| A specific adapter renders correctly | Manual or CI evidence for that browser/GPU/driver combination. |

If a browser route fails, record the adapter, browser version, operating system, and diagnostic
message. Do not silently relabel an example as native-only unless the public browser-support status
is changed at the example source.


## Validation

Maintainers use these checks before promoting or publishing browser routes:

```sh
just webgpu-fixture-preflight
just webgpu-runner-smoke
just wasm-scene-smoke
just webgpu-browser-smoke
```

For local manual testing, serve the site over HTTP and open a live route:

```text
http://localhost:8000/examples/webgpu/live.html?id=feature_basic_scene
```

Do not open WebGPU routes through `file://`; browser WebGPU APIs require a proper browser security
context.


## See Also

- [WebGPU matrix](../examples/webgpu-matrix.md)
- [Deploy WebGPU examples to the browser](../how-to/deploy-to-web.md)
- [Diagnose WebGPU support](../how-to/debug-webgpu.md)
- [Feature status](feature-status.md)
