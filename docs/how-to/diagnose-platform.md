# Diagnose Build and Platform Issues

Separate install, build, runtime, and backend failures.

## Task Workflow

Check the platform support reference, build a minimal example, run an offscreen smoke test when
possible, then test interactive or WebGPU paths separately.

## Minimal Checklist

```sh
just build
just test
git diff --check
```

For documentation-only changes, `git diff --check` is the required final hygiene check.


## Important Details

Native window, offscreen rendering, video export, and browser WebGPU exercise different platform
layers. Verify the narrowest failing layer before changing code.

## Common Mistakes

- Debugging a browser adapter issue through native Vulkan logs.
- Treating generated runtime binaries or downloaded SDK payloads as source changes.
- Ignoring dirty submodules or generated files when preparing commits.

## See Also

- [Debug rendering output](debug-rendering.md)
- [Diagnose WebGPU support](debug-webgpu.md)
- [Use from C or C++](c-integration.md)

??? example "Related examples"

    - Start page: [Install](../start/install.md)
    - Reference: [Platform support](../reference/platform-support.md)
    - Reference: [Build options](../reference/build-options.md)
    - [Offscreen Capture](../examples/gallery/runtime/feature_offscreen_capture.md) - Source: `examples/c/runtime/offscreen_capture.c`
