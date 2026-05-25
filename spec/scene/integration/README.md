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
5. [CUSTOM_VISUALS.md](CUSTOM_VISUALS.md): registration and integration of user-defined visuals.
6. [HOSTED_BACKENDS.md](HOSTED_BACKENDS.md): Qt, Python console, IPython, Jupyter, SDL, Tk,
   and other host-owned event-loop integrations.
7. [CUPY_CUDA_INTEROP.md](CUPY_CUDA_INTEROP.md): zero-copy CUDA/CuPy memory sharing design for
   real-time Datoviz visualization.
8. [ANDROID_SUPPORT.md](ANDROID_SUPPORT.md): Android Vulkan hosted-surface build and runtime plan.
9. [IOS_SUPPORT.md](IOS_SUPPORT.md): iOS MoltenVK hosted-surface build and runtime plan.
10. [TOUCH_SUPPORT.md](TOUCH_SUPPORT.md): touch contact, gesture, controller, and validation plan.
11. [WEBGPU_WASM.md](WEBGPU_WASM.md): experimental browser WebGPU and WASM scene integration
    contract.
12. [napari](napari/README.md): informative napari adapter guidance and historical PoC notes.


## Active Proposal Inputs

1. [../proposals/promoted/UI_BACKEND_INTEGRATION.md](../proposals/promoted/UI_BACKEND_INTEGRATION.md)
2. [../proposals/active/ASYNC_CALLBACKS.md](../proposals/active/ASYNC_CALLBACKS.md)
