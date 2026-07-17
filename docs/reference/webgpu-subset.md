# WebGPU Subset

Status: experimental v0.4 browser subset.

Datoviz v0.4 can run selected examples in a browser. This uses WebGPU, a browser feature for drawing
with the computer's graphics hardware.

The browser path is useful for demos, documentation, and early testing. It is smaller than the
desktop path. Check each example's browser status before assuming it works online.


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

The example manifest changes as routes are promoted. Use the generated
[WebGPU matrix](../examples/webgpu-matrix.md) for the current example-by-example list and status;
this authored page deliberately does not copy counts from that generated source.


## What Is Supported

The browser subset covers the visuals and features needed by the live gallery routes. Current
coverage includes:

- point, pixel, marker, segment, path, primitive, image, labels, glyph, text, mesh, sphere, and
  vector examples;
- common 2D layout and annotation features, including panels, linked panels, axes, guides,
  colorbars, scale bars, legends, and readouts;
- panzoom for 2D scenes and selected 3D controller examples such as arcball, fly, and turntable;
- selected picking, probing, selection, animation, update, compute, and showcase routes.

Browser support is narrower than desktop support. When a feature matters to your application, check
the specific example page and the [Feature status](feature-status.md) page.


## What Is Not Supported

The browser subset intentionally does not include:

- native windows, Qt hosting, video export, GUI widgets, or low-level desktop runtime modules;
- every desktop application feature;
- stable public JavaScript or TypeScript bindings for building arbitrary Datoviz scenes directly in
  browser code;
- custom shader replacement;
- broad volume, advanced technique, rich-label, query/readback, and zero-copy transport support.

These limits may change after v0.4, but they should not be described as current browser support
until the example matrix and validation checks prove them.


## What `webgpu-live` Means

`webgpu-live` means a public browser route is registered for the example. It does not mean every
browser, graphics card, driver, and CI runner has rendered that route successfully.

When a live route omits or reduces a rendering effect used by its desktop counterpart, the live
page and gallery preview show a **WebGPU rendering differences** warning. Treat that warning as
part of the example's support status; `webgpu-live` alone does not imply pixel-level parity.

Keep these facts separate:

| Fact | Where to check |
| --- | --- |
| Public route exists | The generated example page and [WebGPU matrix](../examples/webgpu-matrix.md). |
| Browser smoke coverage passes | Maintainer validation output from `just webgpu-browser-smoke`. |
| A specific machine renders correctly | Manual or CI evidence for that browser, operating system, and graphics card. |

If a browser route fails, record the browser version, operating system, graphics card, and diagnostic
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
http://localhost:8294/examples/webgpu/live.html?id=features_basic_scene
```

Port 8294 is the default. `just serve` chooses the next free port when necessary and prints the
actual URL. Do not open WebGPU routes through `file://`; browser WebGPU APIs require a proper
browser security context.


## See Also

- [WebGPU matrix](../examples/webgpu-matrix.md)
- [Deploy Datoviz scenes to the web](../how-to/deploy-to-web.md)
- [Diagnose WebGPU support](../how-to/debug-webgpu.md)
- [Feature status](feature-status.md)
