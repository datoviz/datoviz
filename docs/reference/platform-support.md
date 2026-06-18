# Platform Support

Datoviz v0.4 targets native Vulkan-first rendering with an experimental WebGPU/WASM browser path.
Support status is feature-specific; see [Feature status](feature-status.md) for the release-surface
classification.

## Native Runtime

| Platform | Status | Notes |
| --- | --- | --- |
| Linux | Supported target | Requires a working Vulkan loader/ICD for native rendering. GLFW is the normal native window backend when available. |
| macOS | Supported target through Vulkan compatibility stack | Requires the packaged or system Vulkan/MoltenVK runtime path used by the build/install. Native windowing and offscreen paths should be validated locally. |
| Windows | Supported target in packaging/build lanes | Requires Vulkan runtime/driver support. Build recipes include MSVC and MinGW-oriented paths. |
| Headless/offscreen | Supported feature | Still requires a graphics-capable Vulkan runtime unless the specific test/example is CPU-only. |

## Browser/WebGPU

The browser path is experimental. It runs promoted live routes through:

```text
C/WASM scene state -> scene frame artifact -> DRP2 packets -> browser WebGPU runtime
```

Requirements:

- browser with WebGPU enabled and available on the selected adapter;
- supported canvas format and WebGPU features for the route;
- route marked `webgpu-live` in the example metadata;
- diagnostics accepted for routes classified as planned, deferred, native-only, or unsupported on
  the current adapter.

Native Vulkan success does not imply WebGPU success on a particular browser/adapter. A published
`webgpu-live` route is a routing and subset claim; browser/adapter visual proof is recorded
separately in smoke output or compatibility notes.

## Optional Providers

| Provider | Status | Requirement |
| --- | --- | --- |
| Qt/PyQt hosting | Supported, optional provider | Needs the optional `datoviz_qtbridge`, compatible Qt runtime, PyQt6 Vulkan binding surface, and platform WSI support. |
| PySide6 hosting | Not a v0.4 target by default | Only viable if a binding exposes the same required Vulkan surface and pointer-unwrapping support. |
| Shaderc runtime compilation | Optional | Enabled when headers/library are found or required by `DVZ_ENABLE_SHADERC=ON`; otherwise precompiled shaders are required. |
| CUDA/CuPy interop | Advanced/unstable | Native/provider-style work only; not portable WebGPU support. |
| Video encoders | Optional/backend-dependent | Software or hardware encoders depend on build options and installed libraries. |

The base `libdatoviz` build and base Python wheel do not link Qt. PyQt hosting needs a
`datoviz_qtbridge` shared library built with `DVZ_ENABLE_QT_BRIDGE=ON` or supplied separately, plus
a compatible Qt/PyQt runtime at load time.

## Build Dependencies

Core source builds need a C/C++ toolchain, CMake/Ninja-compatible build tools, and the enabled
module dependencies. Common optional dependencies include Vulkan SDK/runtime, GLFW, cglm, mimalloc,
Kvazaar, zlib, FreeType, msdf-atlas-gen, Qt6, and shaderc.

Use [Build options](build-options.md) for the CMake switches that enable or disable these paths.

## Known Limitations

- Scene-managed nonlinear/geographic transforms are deferred; pre-project data on the CPU.
- WebGPU is not native Vulkan parity.
- GUI/native input examples do not automatically apply to browser routes.
- Optional providers must fail with clear diagnostics without breaking core imports/builds.
- Generated runtime binaries and SDK payloads are not source files and should not be committed.

## See Also

- [Build options](build-options.md)
- [WebGPU subset](webgpu-subset.md)
- [Compute and graphics](compute-graphics.md)
- [Diagnose platform issues](../how-to/diagnose-platform.md)
