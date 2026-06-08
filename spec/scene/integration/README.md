# Scene Integration Specs

This directory contains boundaries with host applications, external UI systems, threading, custom
visuals, and display integration.

Use these files when scene behavior crosses into application-owned state, UI frameworks, background
threads, high-DPI windows, or custom user-provided visual families.


## Files

1. [EXTERNAL_UI.md](EXTERNAL_UI.md): boundary with UI frameworks such as ImGui.
2. [THREAD_SAFETY.md](THREAD_SAFETY.md): threading model and async data handoff.
3. [HIGH_DPI.md](HIGH_DPI.md): logical pixels, device pixel ratio, and DPI changes.
4. [DPI_SCALE_IMPLEMENTATION_PLAN.md](DPI_SCALE_IMPLEMENTATION_PLAN.md): implementation roadmap
   for high-DPI consistency and dynamic user scale.
5. [OPTIONAL_PROVIDERS.md](OPTIONAL_PROVIDERS.md): optional dependency, bridge, recipe, custom
   visual, and technique extension boundaries.
6. [CUSTOM_VISUALS.md](CUSTOM_VISUALS.md): registration and integration of user-defined visuals.
7. [HOSTED_BACKENDS.md](HOSTED_BACKENDS.md): Qt, Python console, IPython, Jupyter, SDL, Tk,
   and other host-owned event-loop integrations.
8. [CUPY_CUDA_INTEROP.md](CUPY_CUDA_INTEROP.md): zero-copy CUDA/CuPy memory sharing design for
   real-time Datoviz visualization.
9. [ENTRY_POINTS.md](ENTRY_POINTS.md): low-level and hybrid public integration lanes for canvas,
   vklite, DRP2, scene export, and full scene/app usage.
10. [WEBGPU_WASM.md](WEBGPU_WASM.md): experimental browser WebGPU and WASM scene integration
    contract.
11. [WASM_WEBGPU_PARITY_PLAN.md](WASM_WEBGPU_PARITY_PLAN.md): implementation plan for broad
    native Vulkan and WASM/WebGPU scene parity.
12. [future](future/README.md): Android, iOS, and touch pressure notes.
13. [napari](napari/README.md): informative napari adapter guidance and historical PoC notes.


## Active Proposal Inputs

1. [../proposals/promoted/UI_BACKEND_INTEGRATION.md](../proposals/promoted/UI_BACKEND_INTEGRATION.md)
2. [../proposals/active/ASYNC_CALLBACKS.md](../proposals/active/ASYNC_CALLBACKS.md)
