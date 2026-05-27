# What Is Datoviz?

Datoviz v0.4 is a low-level rendering engine for scientific visualization. It is written primarily
in C/C++ and exposes a C API for native applications, examples, backend integration, and runtime
validation.

The active v0.4 stack is:

1. `scene`: retained figures, panels, visuals, controllers, annotations, scales, and frame plans;
2. `drp2`: backend-agnostic command streams and replayable render descriptions;
3. `vklite` and `canvas`: Vulkan resource and frame execution helpers;
4. `app`: a small presentation layer for offscreen and GLFW-backed views.

Datoviz does not aim to provide the old high-level Python plotting API in v0.4. High-level
scientific plotting belongs in VisPy2/GSP, with Datoviz as one rendering backend.


## Use Datoviz For

- native C rendering and embedding;
- retained visual resources with explicit ownership;
- offscreen rendering, capture, and validation;
- low-level backend or adapter work;
- DRP2 stream generation, replay, and portability experiments;
- raw generated `ctypes` access for low-level Python smoke tests.


## Use Another Layer For

- high-level `plot()`, `scatter()`, or `imshow()` style scientific plotting;
- publication-oriented static vector output;
- Python-first object-oriented plotting workflows.

Those workflows should target VisPy2/GSP or other plotting libraries.
