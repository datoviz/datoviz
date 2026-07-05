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
5. [HIDPI_LIVE_WINDOW_IMPLEMENTATION.md](HIDPI_LIVE_WINDOW_IMPLEMENTATION.md): live GLFW high-DPI
   sizing, input normalization, and text-anchor implementation plan.
6. [OPTIONAL_PROVIDERS.md](OPTIONAL_PROVIDERS.md): optional dependency, bridge, recipe, custom
   visual, and technique extension boundaries.
7. [CUSTOM_VISUALS.md](CUSTOM_VISUALS.md): registration and integration of user-defined visuals.
8. [HOSTED_BACKENDS.md](HOSTED_BACKENDS.md): Qt, Python console, IPython, Jupyter, SDL, Tk,
   and other host-owned event-loop integrations.
9. [CUPY_CUDA_INTEROP.md](CUPY_CUDA_INTEROP.md): zero-copy CUDA/CuPy memory sharing design for
   real-time Datoviz visualization.
10. [NVDEC_VIDEO_SOURCE_PLAN.md](NVDEC_VIDEO_SOURCE_PLAN.md): future low-level video source and
   NVIDIA hardware-decode plan for image/sampled-field workflows.
11. [TIMED_MEDIA_SYNC.md](TIMED_MEDIA_SYNC.md): future synchronized video, audio, events, and
    signal timing model for scientific playback and analysis.
12. [ENTRY_POINTS.md](ENTRY_POINTS.md): low-level and hybrid public integration lanes for canvas,
   vklite, DRP2, scene export, and full scene/app usage.
13. [WEBGPU_WASM.md](WEBGPU_WASM.md): experimental browser WebGPU and WASM scene integration
    contract.
14. [WASM_WEBGPU_PARITY_PLAN.md](WASM_WEBGPU_PARITY_PLAN.md): implementation plan for broad
    native Vulkan and WASM/WebGPU scene parity.
15. [GSP_TEXTURE2D_MESH_PLAN.md](GSP_TEXTURE2D_MESH_PLAN.md): Datoviz-side plan for generic
    field-slot sampling needed by GSP strict Texture2D meshes.
16. [future](future/README.md): Android, iOS, and touch pressure notes.
17. [napari](napari/README.md): informative napari adapter guidance and historical PoC notes.


## Active Proposal Inputs

1. [../proposals/promoted/UI_BACKEND_INTEGRATION.md](../proposals/promoted/UI_BACKEND_INTEGRATION.md)
2. [../proposals/active/ASYNC_CALLBACKS.md](../proposals/active/ASYNC_CALLBACKS.md)
