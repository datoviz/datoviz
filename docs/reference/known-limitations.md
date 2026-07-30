# Known Limitations

This page records limits of the current v0.4 release-candidate line. Release notes remain authoritative for limitations of a specific published package; [Feature status](feature-status.md) classifies the broader public surface, and [Platform support](platform-support.md) records runtime and provider requirements.

## Release And Compatibility

- v0.4 is a release candidate, so public APIs may still change before the final release.
- v0.4 does not preserve v0.3 source or ABI compatibility. The old object-oriented Python plotting API is not part of this release line; high-level plotting belongs to GSP/VisPy2.
- A successful source build, hosted CI run, or gallery route does not prove that every published package, physical GPU, driver, or browser/adapter combination works. Check the validation evidence and release notes for the exact artifact.

## Native Runtime And Optional Providers

- Native interactive and offscreen rendering require a working Vulkan loader, driver/ICD, and compatible graphics device. Headless rendering is not a CPU-only path unless the specific command says so.
- Qt and PyQt hosting are implemented but remain source-build-only in the published RC2 packages because those wheels do not include the native `datoviz_qtbridge` provider.
- Shader compilation, video encoding, GUI integration, and CUDA interoperability depend on optional build/runtime providers and must report unavailable configurations explicitly.
- CUDA external-memory interoperability is experimental and limited to Linux/NVIDIA evidence. Direct shared Vulkan images and the broader framework matrix remain deferred or unproven as listed in [GPU array interoperability](gpu-array-interop.md).

## Scene And Rendering Scope

- Scene-managed nonlinear or geographic transforms are deferred. Pre-project data on the CPU and upload ordinary Cartesian positions.
- Custom replacement shaders for built-in visuals and built-in shader hot reload are deferred because the built-in shader ABI is internal.
- Glyph and splat visual families are experimental. Other visual families are release-facing only for the documented attributes, queries, and backend routes.
- Scene compute is a narrow experimental compute-to-render path, not a general-purpose compute framework.
- Raster capture and video export use the documented app-level readback path; encoder availability and platform-specific zero-copy paths are separate concerns.

## Browser Scope

- WebGPU/WASM is experimental and is not feature- or pixel-parity with native Vulkan.
- A `webgpu-live` label means a public route exists for the manifest-declared subset. It does not prove every browser, adapter, driver, or rendering effect.
- Native windows, Qt hosting, native GUI widgets, video export, low-level desktop runtime modules, custom shader replacement, and several advanced rendering/query paths are outside the current browser subset.

## Where To Check Exact Scope

- [Feature status](feature-status.md) for supported, experimental, advanced/unstable, deferred, and external/GSP classifications.
- [Platform support](platform-support.md) for operating-system, runtime, package, and optional-provider requirements.
- [WebGPU subset](webgpu-subset.md) and the generated [WebGPU matrix](../examples/webgpu-matrix.md) for browser coverage.
- [v0.3 visible parity](v03-visible-parity.md) for the disposition of v0.3-era capabilities.
- The release notes for known issues tied to an exact published candidate.
