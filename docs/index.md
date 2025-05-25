# Datoviz: high-performance rendering for scientific data visualization

<!-- INTRODUCTION -->

**⚡️ Datoviz** is an open-source, cross-platform, **high-performance rendering library for scientific data visualization**.

It delivers fast, **high-quality GPU rendering** of 2D and 3D graphical primitives—markers, paths, images, text, meshes, volumes, and more—that scale to millions of elements. Datoviz also supports **graphical user interfaces (GUIs)** for interactive visualization.

![](https://raw.githubusercontent.com/datoviz/data/master/hero.jpg)

Built from the ground up with performance in mind, Datoviz is written primarily in **C** and **C++**, leveraging the [**Khronos Vulkan graphics API**](https://www.vulkan.org/). It offers a C API, low-level Python bindings via `ctypes`, and a higher-level, idiomatic **Python API 🐍**.

Written by one of the original creators of [VisPy](https://vispy.org), a GPU-based Python scientific visualization library, Datoviz aims to serve as the default backend for the upcoming **VisPy 2.0**.

The library is lightweight with minimal dependencies: mostly Vulkan, [**GLFW**](https://www.glfw.org/) for windowing, and [Dear ImGui](https://github.com/ocornut/imgui/) for GUIs.

!!! warning

    Datoviz is a young library. The API is stabilizing, but breaking changes may still occur as the project evolves with broader usage.


<!-- FEATURES -->

## Current features

* **📊 2D visuals**: antialiased points, markers, line segments, paths, text, images
* **📈 2D axes**
* **🌐 3D visuals**: meshes, volumes, volume slices
* **🌈 150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **🖱️ High-level interactivity**: pan & zoom for 2D, arcball for 3D (more later)
* **🎥 Manual control of cameras**: custom interactivity
* **𓈈 Figure subplots** (aka "panels")
* **🖥️ GUIs** using [Dear ImGui](https://github.com/ocornut/imgui/)


## What Datoviz is — and isn't

**Datoviz is a relatively low-level visualization library.** It focuses on rendering visual primitives like points, lines, images, and meshes — efficiently and interactively.

Unlike libraries such as **Matplotlib**, Datoviz does **not** provide high-level plotting functions like `plt.plot()`, `plt.scatter()`, or `plt.imshow()`. Its goal is **not** to replace plotting libraries, but to serve as a powerful rendering backend for scientific graphics.

A higher-level plotting API is being developed as part of **VisPy 2.0**, which will use Datoviz as a rendering backend. An intermediate layer called **GSP** (Graphics Specification Protocol) will provide a backend-agnostic API.



## Who is it for?

Datoviz is built for people who need performance, control, and precision:

* **Researchers and analysts** exploring large, complex datasets in real time
* **Scientists and engineers** creating custom visual interfaces for experiments and simulations
* **Developers** who want a lightweight, portable alternative to bloated visualization frameworks


## Get started

- 👉 **[Quickstart guide](quickstart.md)** — create your first scatter plot in a few lines of code
- 📚 **[Learn](guide/index.md)** — deep dive into visuals, layout, interactivity, and more
- 🖼️ **[Gallery](gallery/index.md)** — curated examples of what Datoviz can render
- 🧩 **[API Reference](reference/api_py.md)** — full Python and C documentation


## Installation

Datoviz runs out of the box on all major platforms:

* ✅ **Windows**, **macOS** (Intel and Apple Silicon), and **Linux**
* ✅ Prebuilt wheels for 64-bit architectures (x86\_64 and arm64)
* ✅ No system dependencies — just install and run

Install the Python package via pip:

```bash
pip install datoviz
```

To use the C library directly, see the [build instructions](discussions/BUILD.md).


## License

Datoviz is open source and licensed under the [MIT License](discussions/LICENSE.md).
