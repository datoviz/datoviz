# Platform Support

Datoviz v0.4 targets native Vulkan-first rendering with an experimental WebGPU/WASM browser path.
Support status is feature-specific; see [Feature status](feature-status.md) for the release-surface
classification.

## Native Runtime

| Platform | Status | Notes |
| --- | --- | --- |
| Linux | Supported target | Requires a working Vulkan loader/ICD for native rendering. GLFW is the normal native window backend when available. |
| macOS | Supported target through Vulkan compatibility stack | Release wheels target macOS 15 on arm64 and Intel. Requires the packaged or system Vulkan/MoltenVK runtime path used by the build/install. |
| Windows | Supported native wheel and source-build target | Validated wheel lanes cover AMD64 and ARM64. Native source development uses MSVC, Vulkan SDK, and vcpkg; WSL is a separate Linux environment. |
| Headless/offscreen | Supported feature | Still requires a graphics-capable Vulkan runtime unless the specific test/example is CPU-only. |

## Browser/WebGPU

The browser path is experimental. It runs selected gallery examples in WebGPU-capable browsers
through public live routes.

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

Default native source builds need a C/C++ toolchain, CMake/build tools, Vulkan headers and loader,
cglm, and the enabled module dependencies. A usable Vulkan driver/ICD is also required at runtime.
Optional or configuration-dependent dependencies include GLFW, mimalloc, Kvazaar, zlib, FreeType,
msdf-atlas-gen, Qt6, shaderc, CUDA, and video encoders.

The default source build prefers vendored dependency submodules. Distribution builders may use
`DVZ_VENDORED_DEPS=OFF` to prefer supported system packages with `AUTO` fallback. `glslc` is the
recommended build-time compiler for embedded SPIR-V; shaderc is the runtime-GLSL fallback and is
required by release-wheel configuration. The active default canvas build invokes
`glslangValidator`, so full source builds require that tool as well.

Use [Build options](build-options.md) for the CMake switches that enable or disable these paths.

Wheel/installed-package support and source-build support are different claims. An installed wheel
should carry its declared native runtime payload; a source build must satisfy the dependencies
selected by its own CMake configuration.

## Known Limitations

- Scene-managed nonlinear/geographic transforms are deferred; pre-project data on the CPU.
- WebGPU is not native Vulkan parity.
- GUI/native input examples do not automatically apply to browser routes.
- Optional providers must fail with clear diagnostics without breaking core imports/builds.
- Generated runtime binaries and SDK payloads are not part of the public source documentation.
- Passing hosted wheel CI does not mean an RC or final package has been published; release notes are
  authoritative for installable versions and artifact URLs.

## See Also

- [Build options](build-options.md)
- [WebGPU subset](webgpu-subset.md)
- [Compute and graphics](compute-graphics.md)
- [Diagnose platform issues](../how-to/diagnose-platform.md)
