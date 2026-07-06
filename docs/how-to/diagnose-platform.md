# Diagnose Build and Platform Issues

Separate install, build, rendering, and browser-support failures.

## Task Workflow

Start with the smallest path that should work on your machine. Check that Datoviz imports or builds,
run a minimal native example, then test offscreen rendering, native windows, and WebGPU browser
routes separately.

## Minimal Checklist

For a source checkout, first make sure the project builds:

```sh
just build
```

Then run one small native example:

```sh
just example-c start/scatter
./build/examples/c/start/scatter --live
```

For a Python package, start with an import check:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```


## Important Details

Native window, offscreen rendering, video export, and browser WebGPU exercise different platform
layers. Verify the narrowest failing layer before changing code.

Use offscreen rendering when a machine has a GPU runtime but no visible desktop session. Use WebGPU
diagnostics when a native example works but a browser route does not.

## Common Mistakes

- Debugging a browser adapter issue through native Vulkan logs.
- Assuming that offscreen rendering is CPU-only.
- Assuming that a working native window means browser WebGPU will also work.
- Testing a complex showcase before confirming that a minimal point example renders.

## See Also

- [Debug rendering output](debug-rendering.md)
- [Diagnose WebGPU support](debug-webgpu.md)
- [Use from C or C++](c-integration.md)

??? example "Related examples"

    - Start page: [Install](../start/install.md)
    - Reference: [Platform support](../reference/platform-support.md)
    - Reference: [Build options](../reference/build-options.md)
    - [Offscreen Capture](../examples/gallery/runtime/feature_offscreen_capture.md) - Source: `examples/c/runtime/offscreen_capture.c`
