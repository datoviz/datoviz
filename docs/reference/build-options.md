# Build Options

Draft reference stub.

This page should document CMake and `just` build options, including core, Vulkan, canvas, DRP2,
scene, app, WebGPU, validation, and optional dependency flags.

## Dependency Source

| Option | Default | Meaning |
| --- | --- | --- |
| `DVZ_VENDORED_DEPS` | `ON` | Use bundled third-party source trees when available. Set to `OFF` to resolve supported dependencies from the host package manager. |
| `DVZ_WITH_GLFW` | `ON` | Enable the GLFW window backend. With `DVZ_VENDORED_DEPS=OFF`, CMake looks for a system `glfw3` package. If absent, non-GUI builds fall back to the headless backend; GUI builds fail because ImGui needs GLFW. |
