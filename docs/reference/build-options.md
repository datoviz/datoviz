# Build Options

This page lists the CMake and `just` build options most relevant to source builds, packaging, and
release validation.

## Core CMake Options

| Option | Default | Meaning |
| --- | --- | --- |
| `DVZ_BUILD_CORE` | `ON` | Build common, data-structure, file I/O, math, and thread layers. |
| `DVZ_BUILD_CONTROLLER` | `ON` | Build scene-independent camera/controller primitives. |
| `DVZ_BUILD_VK` | `ON` | Build Vulkan and vklite layers. |
| `DVZ_BUILD_CANVAS` | `ON` | Build input, window, stream, video, and canvas layers. |
| `DVZ_BUILD_DRP2` | `ON` | Build the DRP2 command-stream layer. |
| `DVZ_BUILD_WEBGPU` | `OFF` | Build the native WebGPU layer where available. Browser WebGPU routes use the WASM/browser tooling. |
| `DVZ_BUILD_SCENE` | `ON` | Build retained scene, visuals, controllers, frame planning, and scene DRP2 emission. |
| `DVZ_BUILD_APP` | `ON` | Build app presentation helpers. |
| `DVZ_BUILD_GUI` | `ON` | Build the Dear ImGui app overlay layer. |
| `DVZ_BUILD_TESTING` | `PROJECT_IS_TOP_LEVEL` | Build Datoviz test executables. |
| `DVZ_BUILD_EXAMPLES` | `PROJECT_IS_TOP_LEVEL` | Build Datoviz example executables. |
| `DVZ_INSTALL` | `PROJECT_IS_TOP_LEVEL` | Add install, CMake package export, pkg-config, and header install rules. |

## Optional Feature Options

| Option | Default | Meaning |
| --- | --- | --- |
| `DVZ_ENABLE_CUDA` | platform-dependent | Enable CUDA interop hooks and tests when the toolkit is available. |
| `DVZ_ENABLE_KVAZAAR` | `ON` | Enable the optional Kvazaar software HEVC backend. |
| `DVZ_WITH_GLFW` | `ON` | Enable the GLFW window backend. |
| `DVZ_WITH_ZLIB` | `ON` | Enable zlib support for gzip-compressed data files. |
| `DVZ_WITH_FREETYPE` | `ON` | Enable FreeType-backed scene text atlases when available. |
| `DVZ_WITH_MSDF_ATLAS` | `ON` | Enable msdf-atlas-gen scene text atlases when available. |
| `DVZ_WITH_MSDF_SVG` | `ON` | Enable SVG path import through msdfgen when tinyxml2 is available. |
| `DVZ_ENABLE_QT_BRIDGE` | `AUTO` | Build the optional Qt bridge provider when Qt6 Gui development files are available. |
| `DVZ_ENABLE_SHADERC` | `AUTO` | Enable runtime GLSL compilation through lazy-loaded shaderc when headers/library are available. |
| `DVZ_ENABLE_ASAN_IN_DEBUG` | `OFF` | Enable sanitizer instrumentation in Debug builds. |
| `DVZ_ENABLE_COVERAGE` | `OFF` | Enable code coverage instrumentation for supported compilers. |
| `DVZ_ENABLE_GPROF` | `OFF` | Enable gprof instrumentation on supported compilers. |
| `DVZ_SANITIZER` | empty | Optional sanitizer override for Debug builds: `asan`, `msan`, `tsan`, or `off`. |
| `DVZ_USE_MIMALLOC_RELEASE_DEFAULT` | `ON` | Use mimalloc as the default allocator in Release builds when available. |

## Dependency Source

| Option | Default | Meaning |
| --- | --- | --- |
| `DVZ_VENDORED_DEPS` | `ON` | Prefer bundled third-party source trees when source mode is `AUTO`. Set to `OFF` to prefer supported dependencies from the host package manager. |
| `DVZ_WITH_GLFW` | `ON` | Enable the GLFW window backend. With `DVZ_VENDORED_DEPS=OFF`, CMake looks for a system `glfw3` package. If absent, non-GUI builds fall back to the headless backend; GUI builds fail because ImGui needs GLFW. |
| `DVZ_MIMALLOC_SOURCE` | `AUTO` | Select mimalloc source: `AUTO`, `SYSTEM`, `VENDORED`, or `OFF`. `AUTO` follows `DVZ_VENDORED_DEPS`, falling back to the other source when needed. |
| `DVZ_CGLM_SOURCE` | `AUTO` | Select cglm source: `AUTO`, `SYSTEM`, `VENDORED`, or `OFF`. cglm is required by the active math stack, so `OFF` is currently rejected. |
| `DVZ_KVAZAAR_SOURCE` | `AUTO` | Select Kvazaar source: `AUTO`, `SYSTEM`, `VENDORED`, or `OFF`. `OFF` disables the optional software HEVC backend. |
| `DVZ_BUILD_TESTING` | `PROJECT_IS_TOP_LEVEL` | Build Datoviz test executables. Package/install smoke presets and FetchContent consumers normally disable this. |
| `DVZ_BUILD_EXAMPLES` | `PROJECT_IS_TOP_LEVEL` | Build Datoviz example executables. Package/install smoke presets and FetchContent consumers normally disable this. |
| `DVZ_INSTALL` | `PROJECT_IS_TOP_LEVEL` | Add install, CMake package export, pkg-config, and header install rules. FetchContent consumers normally leave this disabled. |

Explicit `SYSTEM` or `VENDORED` source modes fail configuration if the requested source is not
available. Only `AUTO` may fall back to the other source.

`msdf-atlas-gen` remains source/vendored-only. It is too niche to rely on as a package-manager
dependency across supported distributions.

## Package Smoke Presets

| Preset | Purpose |
| --- | --- |
| `package-smoke-vendored` | Narrow Release build using the default vendored dependency preference. |
| `package-install-vendored` | Install-tree smoke using the default vendored dependency preference. |
| `package-smoke-system-auto` | Narrow Release build with `DVZ_VENDORED_DEPS=OFF`, preferring system packages while allowing `AUTO` fallback. |
| `package-install-system-auto` | Install-tree smoke with `DVZ_VENDORED_DEPS=OFF`, preferring system packages while allowing `AUTO` fallback. |
| `package-smoke-system-required` | Package CI preset requiring system cglm, Kvazaar, mimalloc, and GLFW. Use only in an environment that installs those development packages first. |
| `package-install-system-required` | Install-tree smoke requiring system cglm, Kvazaar, mimalloc, and GLFW. Use only in an environment that installs those development packages first. |

## Package CI Matrix

| Platform | System dependencies to install | Expected preset lane | Dependency policy |
| --- | --- | --- | --- |
| Ubuntu 24.04 | `libglfw3-dev libcglm-dev libmimalloc-dev` | `package-smoke-system-auto`, then `package-install-system-auto` | Prefer system GLFW, cglm, and mimalloc. Leave Kvazaar as `AUTO` or set `DVZ_KVAZAAR_SOURCE=VENDORED` unless the builder provides `libkvazaar-dev`. |
| Fedora | `glfw-devel mimalloc-devel` plus any available cglm/Kvazaar development packages | `package-smoke-system-auto`, then `package-install-system-auto` | Prefer system packages that exist in the target Fedora/EPEL release. Keep cglm and Kvazaar as `AUTO` unless the packaging environment explicitly provides them. |
| macOS / Homebrew | `glfw cglm kvazaar mimalloc` | `package-smoke-system-required`, then `package-install-system-required` | Homebrew has all four package-manager candidates, so this is the strict system-dependency lane. |

`msdf-atlas-gen` is intentionally absent from the matrix. Treat it as source/vendored-only for
v0.4 packaging; do not add it as a distro package requirement.

Useful local checks:

```sh
cmake --preset package-smoke-vendored
cmake --build --preset package-smoke-vendored

cmake --preset package-smoke-system-auto
cmake --build --preset package-smoke-system-auto

cmake --build --preset package-install-system-auto
```

Out-of-tree CMake consumer checks:

```sh
just c-integration-smoke
```

## FetchContent

Datoviz can be embedded as a CMake subproject. Tests, examples, and install/package export rules
default to disabled when Datoviz is not the top-level project:

```cmake
include(FetchContent)

FetchContent_Declare(datoviz
    GIT_REPOSITORY https://github.com/datoviz/datoviz.git
    GIT_TAG v0.4.0
)

FetchContent_MakeAvailable(datoviz)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

FetchContent builds Datoviz from source and still needs the native toolchain and SDK dependencies.
For most installed-user workflows, prefer `find_package(datoviz)` from a wheel or package-manager
install.
