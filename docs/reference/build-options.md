# Build Options

Draft reference stub.

This page should document CMake and `just` build options, including core, Vulkan, canvas, DRP2,
scene, app, WebGPU, validation, and optional dependency flags.

## Dependency Source

| Option | Default | Meaning |
| --- | --- | --- |
| `DVZ_VENDORED_DEPS` | `ON` | Prefer bundled third-party source trees when source mode is `AUTO`. Set to `OFF` to prefer supported dependencies from the host package manager. |
| `DVZ_WITH_GLFW` | `ON` | Enable the GLFW window backend. With `DVZ_VENDORED_DEPS=OFF`, CMake looks for a system `glfw3` package. If absent, non-GUI builds fall back to the headless backend; GUI builds fail because ImGui needs GLFW. |
| `DVZ_MIMALLOC_SOURCE` | `AUTO` | Select mimalloc source: `AUTO`, `SYSTEM`, `VENDORED`, or `OFF`. `AUTO` follows `DVZ_VENDORED_DEPS`, falling back to the other source when needed. |
| `DVZ_CGLM_SOURCE` | `AUTO` | Select cglm source: `AUTO`, `SYSTEM`, `VENDORED`, or `OFF`. cglm is required by the active math stack, so `OFF` is currently rejected. |
| `DVZ_KVAZAAR_SOURCE` | `AUTO` | Select Kvazaar source: `AUTO`, `SYSTEM`, `VENDORED`, or `OFF`. `OFF` disables the optional software HEVC backend. |

Explicit `SYSTEM` or `VENDORED` source modes fail configuration if the requested source is not
available. Only `AUTO` may fall back to the other source.
