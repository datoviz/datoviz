# Diagnose Build and Platform Issues

Separate install, build, native rendering, offscreen rendering, optional providers, and browser
WebGPU failures.

Datoviz uses different platform layers for source builds, Python imports, native Vulkan rendering,
visible windows, offscreen capture, Qt hosting, video export, and browser WebGPU. A failure in one
layer is not proof that the others are broken.

!!! info "At a glance"

    - **Status:** Supported triage workflow covering source, installed, native, browser, and optional-provider paths.
    - **Languages:** C/C++, Python, shell, and browser diagnostics as relevant to the failing layer.
    - **Prerequisites:** Exact install type, first failing command/error, OS/architecture, and GPU/driver details.
    - **Result:** One failing layer is isolated with a minimal reproducer and actionable platform report.

## Diagnostic order

Use this order:

```text
identify install type
-> check build/import
-> run minimal native example
-> run offscreen capture
-> test visible window/input
-> test optional provider or WebGPU separately
-> collect platform report
```

Always reduce to the smallest path that should work on the machine. Do not start with a complex
showcase, real data bundle, video export, Qt embed, or browser iframe.

## Identify the failing layer

| Layer | Minimal proof |
| --- | --- |
| Source checkout builds. | `just build` |
| C example build works. | `just example-c features/basic_scene` |
| Native runtime renders. | `./build/examples/c/features/basic_scene` |
| Offscreen rendering works. | `./build/examples/c/runtime/offscreen_capture` |
| Python package imports. | `python -c "import datoviz as dvz; print('datoviz import ok')"` |
| Browser WebGPU route works. | `http://localhost:8294/examples/webgpu/live.html?id=features_basic_scene` (or the address printed by `just serve`) |
| Qt/PyQt hosting works. | Qt bridge build and PyQt smoke for the optional provider. |

Stop at the first failing layer and debug that layer. For example, do not inspect WebGPU adapter
messages when `just build` has not completed, and do not inspect Vulkan logs when only a browser
route fails.

## Check a source checkout

From the repository root:

```sh
just build
```

If the build fails, read the first CMake or compiler error before later cascading errors. Common
build-layer issues include:

| Symptom | First check |
| --- | --- |
| CMake cannot find a dependency. | Confirm the dependency is installed or disable the optional path with the matching CMake option. |
| GLFW is missing. | Native visible windows may be unavailable; offscreen/headless paths may still build. |
| Vulkan headers or loader are missing. | Native rendering paths cannot build or run correctly. |
| Optional Qt bridge fails. | Disable `DVZ_ENABLE_QT_BRIDGE` or install matching Qt development packages. |
| Shaderc is missing. | Use precompiled shaders or configure `DVZ_ENABLE_SHADERC` according to the build option. |
| WASM build fails. | Run `just wasm-env-check` before debugging browser route code. |

CMake prints detected options near the end of configuration, including `DVZ_BUILD_*`,
`DVZ_WITH_*`, `DVZ_HAS_*`, and optional provider status. Use those lines to distinguish "feature
disabled by configuration" from "feature enabled but broken at runtime."

## Check the native runtime

Build and run a tiny native example:

```sh
just example-c features/basic_scene
./build/examples/c/features/basic_scene
```

If it fails before a window appears, suspect platform graphics setup: Vulkan loader, installed GPU
driver, MoltenVK/Vulkan compatibility stack on macOS, window-system availability, or dynamic library
loading.

If it opens a window but the frame is blank or wrong, switch to [Debug rendering output](debug-rendering.md).

Enable DRP2 tracing only after the basic scene/run split is clear:

```sh
DVZ_DRP2_TRACE=normal ./build/examples/c/features/basic_scene
```

Trace output helps inspect command flow; it does not fix missing drivers, missing window-system
support, or malformed scene data.

## Check offscreen rendering

Use offscreen rendering to remove visible-window and input variables:

```sh
just example-c runtime/offscreen_capture
./build/examples/c/runtime/offscreen_capture
```

Interpret the result:

| Result | Meaning |
| --- | --- |
| Offscreen works, visible window fails. | Focus on window creation, swapchain/presentation, GLFW, resize, or event loop. |
| Offscreen and visible window both fail. | Focus on graphics runtime, scene emission, command execution, or platform setup. |
| Offscreen works, screenshot file is wrong. | Check capture timing, framebuffer size, alpha, color management, and output path. |
| Offscreen fails on a headless machine. | Confirm a graphics-capable runtime is available; offscreen is not CPU-only. |

Offscreen rendering still needs a usable native graphics runtime. It avoids opening a visible
window, but it does not remove GPU/driver requirements.

## Check the Python package

For an installed package or local Python environment:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

If exact Python binding calls are involved, also check `datoviz.raw`:

```sh
python -c "import datoviz.raw as raw; print('raw import ok')"
```

Import success only proves that Python can load the package and shared library. It does not prove
that native rendering, the Vulkan runtime, optional Qt hosting, screenshots, or browser WebGPU will
work.

When source changes affect public headers, exported API, binding policy, or binding generation,
maintainers refresh and validate bindings with:

```sh
just ctypes
just ctypes-check
```

For ordinary platform diagnosis of an installed package, start with import and one minimal example
before running broader binding checks.

## Check browser WebGPU

Browser WebGPU is a separate experimental path. First serve the site:

```sh
just wasm-scene-build
just serve
```

Open:

```text
http://localhost:8294/examples/webgpu/live.html?id=features_basic_scene
```

If that port is busy, `just serve` selects and prints another port. Use the printed address rather
than assuming a fixed URL.

If native examples work but this route fails, use [Diagnose WebGPU support](debug-webgpu.md). Do not
debug browser adapter issues through native Vulkan logs.

Remember:

- only examples marked `webgpu-live` should have public live routes;
- routes must be served over HTTP or HTTPS, not `file://`;
- native success does not imply WebGPU success on the current browser/adapter;
- browser readback, input, timing, and resize behavior differ from native windows.

## Check optional providers

Optional providers should fail clearly without breaking the base library.

| Provider | Diagnostic split |
| --- | --- |
| Qt/PyQt hosting | Check base native rendering first, then Qt bridge build, then PyQt/PySide binding support, then runtime Qt version compatibility. |
| Video export | Check native rendering first, then encoder build options and installed encoder libraries. |
| CUDA/CuPy interop | Check native rendering first, then CUDA SDK availability and advanced/unstable interop status. |
| Shaderc runtime compilation | Call `dvz_shader_compiler_status()` before GPU initialization, inspect the typed status, and check the relevant provider override or packaged runtime directory. |

Do not add Qt, CUDA, video, or shaderc requirements to a base-platform bug unless the failing path
actually uses that provider.

## Platform-specific notes

| Platform | First checks |
| --- | --- |
| Linux | Vulkan loader/ICD, GPU driver, display server or headless graphics setup, GLFW availability. |
| macOS | Vulkan compatibility stack/MoltenVK availability, packaged versus system runtime path, app/window permissions, architecture match. |
| Windows | Vulkan runtime/driver, MSVC or MinGW build environment, DLL search path, wheel architecture. |
| Headless/CI | Graphics-capable runtime, offscreen example, no assumption that offscreen is CPU-only. |
| Browser | WebGPU browser support, adapter selection, HTTPS/HTTP serving, route status, console diagnostics. |

For packaging or wheel issues, record the package filename, Python version, architecture, and how
the package was installed.

## Triage common symptoms

| Symptom | Likely layer | First action |
| --- | --- | --- |
| `just build` fails. | Build configuration or dependency. | Read the first CMake/compiler error and detected `DVZ_*` options. |
| Python import fails. | Package install, shared library load, or ABI. | Check package path, architecture, and `datoviz.raw` import. |
| Native example starts but frame is blank. | Scene/rendering state. | Use [Debug rendering output](debug-rendering.md). |
| Native visible window fails, offscreen works. | Window system or presentation. | Check GLFW/window backend and resize/presentation diagnostics. |
| Offscreen fails too. | Graphics runtime or command execution. | Check Vulkan runtime/driver and DRP2/runtime diagnostics. |
| `no Vulkan loader could be loaded`. | Vulkan loader discovery. | See [No Vulkan loader found](#no-vulkan-loader-found) below. |
| Browser route fails, native works. | WebGPU route/browser/adapter. | Use [Diagnose WebGPU support](debug-webgpu.md). |
| Qt embed fails, native works. | Optional Qt provider. | Check bridge library, Qt runtime, and binding support. |
| Video export fails, screenshots work. | Encoder/provider path. | Check encoder build option and runtime libraries. |
| Real-data showcase fails, synthetic example works. | Data bundle or path. | Check prepared data availability and example manifest instructions. |

## No Vulkan loader found

Datoviz reaches the GPU through the Vulkan loader, and it reports `no Vulkan loader could be loaded`
when it cannot find one. On macOS it looks in this order, taking the first that works:

1. `DVZ_VULKAN_LOADER_LIBRARY`, a full path to a loader library. Use this to pin an exact loader.
2. Any directory listed in `DVZ_WHEEL_RUNTIME_DIRS`. Installed packages set this themselves.
3. Next to the Datoviz library itself, which is where official packages ship one.
4. `$VULKAN_SDK/lib`, which covers a source build, since the SDK exports `VULKAN_SDK`.
5. The platform default search path.

So a package install needs nothing from you, and a source build needs only the Vulkan SDK present in
the environment. If the message still appears, check that `VULKAN_SDK` is exported, or point
`DVZ_VULKAN_LOADER_LIBRARY` straight at the loader.

On Linux the loader is a system library, `libvulkan.so.1`, supplied by your distribution's Vulkan
runtime package; install that rather than setting variables.

### Conflicting macOS Vulkan environments

When a source checkout inherits driver selectors or loader paths from another Vulkan installation, opt into a clean SDK environment before reproducing the failure:

```sh
export DVZ_VULKAN_SANITIZE_ENV=1
source tools/vulkan-env.sh
```

This mode removes inherited Vulkan driver selection, `VK_ROOT`, and direct `DYLD_LIBRARY_PATH` overrides, selects the active SDK's MoltenVK manifest, and restricts `DYLD_FALLBACK_LIBRARY_PATH` to that SDK. Directly sourcing the script without this opt-in preserves the existing environment-merging behavior.

## Prepare a platform report

```text
Install type: source checkout / wheel / other
Datoviz commit or package version:
OS and version:
CPU architecture:
GPU/driver:
Python version, if relevant:
Build command and first error:
Minimal native example result:
Offscreen capture result:
Browser and route URL, if relevant:
Qt/video/CUDA provider details, if relevant:
Relevant console/log output:
```

Include the first failing command and its first specific error. Later errors are often cascades from
the first missing dependency, failed device creation, or missing asset.

Do not include credentials, private paths, proprietary datasets, or full environment dumps in a
public report. Reduce logs to the commands, versions, detected options, and first relevant errors.

## Common mistakes

- Debugging a browser adapter issue through native Vulkan logs.
- Assuming that offscreen rendering is CPU-only.
- Assuming that a working native window means browser WebGPU will also work.
- Testing a complex showcase before confirming that a minimal point or basic-scene example renders.
- Treating optional Qt, video, CUDA, or shaderc failures as base-library failures.
- Ignoring the first CMake/compiler/browser-console error and reporting only later secondary output.
- Mixing source checkout diagnostics with installed wheel diagnostics without recording which one
  was used.

## Next steps

- [Debug rendering output](debug-rendering.md)
- [Diagnose WebGPU support](debug-webgpu.md)
- [Render offscreen](render-offscreen.md)
- [Use from C or C++](c-integration.md)
- [Platform support](../reference/platform-support.md)
- [Build options](../reference/build-options.md)
- [Errors and logging](../reference/errors-and-logging.md)

??? example "Complete and related examples"

    - Canonical complete example: [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - Start page: [Install](../start/install.md)
    - Reference: [Platform support](../reference/platform-support.md)
    - Reference: [Build options](../reference/build-options.md)
    - [Offscreen Capture](../examples/gallery/runtime/runtime_offscreen_capture.md) - Source: `examples/c/runtime/offscreen_capture.c`
