# Diagnose WebGPU Support

Check whether a browser route should run and what fallback to use when it does not.

## Task Workflow

Confirm the example is marked `webgpu-live`, open the standalone live route, inspect browser WebGPU
availability, then compare against the native example or generated screenshot.

## Minimal Checklist

```text
manifest status -> live route -> browser WebGPU support -> console errors -> native baseline
```

Use the standalone route before debugging an iframe embedded in a gallery page.

## Canonical Examples

- Gallery: [Basic Scene](../examples/gallery/features/feature_basic_scene.md)
- Source: `examples/c/features/basic_scene.c`
- Gallery: [WebGPU Matrix](../examples/webgpu-matrix.md)
- Manifest: `examples/c/MANIFEST.yaml`

## Important Details

WebGPU support depends on browser, adapter, operating system, and feature requirements. A native
example can be correct while the WebGPU route is planned, deferred, or unsupported on the current
adapter.

## Common Mistakes

- Treating `webgpu-planned` as a failing live route.
- Debugging an iframe before trying `examples/webgpu/live.html?id=<example-id>` directly.
- Assuming GLFW/native input behavior applies to browser routes.

## See Also

- [Deploy WebGPU examples to the browser](deploy-to-web.md)
- [Debug rendering output](debug-rendering.md)
- [Diagnose build and platform issues](diagnose-platform.md)
