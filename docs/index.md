# Datoviz

**Datoviz** is a high-performance, cross-platform scientific visualization library built for modern GPUs. It provides a minimal, low-overhead API in both **C** and **Python**, enabling fast and interactive rendering of large-scale 2D and 3D data.

Whether you're building research tools, real-time visual interfaces, or custom scientific apps, Datoviz gives you the control and speed you need — without sacrificing interactivity or clarity.

!!! warning

    Datoviz is a young library. The API is stabilizing, but breaking changes may still occur as the project evolves with broader usage.

![Datoviz screenshot](https://raw.githubusercontent.com/datoviz/data/main/screenshots/hero.png)


## Features

- 🔬 Designed for **scientific data**: high-dimensional, large-scale, precise
- 🚀 **GPU-accelerated** rendering with Vulkan (and WebGPU support in progress)
- 🖥️ **Integrated GUIs** with Dear ImGui
- 🧩 Support for multiple **visual primitives**: points, lines, images, meshes, volumes, and more
- 🎯 Minimal core API, no unnecessary dependencies
- 🔁 Built-in interactivity: pan & zoom, arcball, keyboard & mouse input, event hooks
- 🧪 C/C++ and Python bindings


## What Datoviz is — and isn't

**Datoviz is a relatively low-level visualization library.** It focuses on rendering visual primitives like points, lines, images, and meshes — efficiently and interactively.

Unlike libraries such as **Matplotlib**, Datoviz does **not** provide high-level plotting functions like `plt.plot()`, `plt.scatter()`, or `plt.imshow()`. Its goal is **not** to replace plotting libraries, but to serve as a powerful rendering backend for scientific graphics.

A higher-level plotting API is being developed as part of **VisPy 2.0**, which will use Datoviz as a rendering backend. An intermediate layer called **GSP** (Graphics Specification Protocol) will provide a backend-agnostic API.



## Who is it for?

Datoviz is built for people who need performance, control, and precision:

* **Researchers and analysts** exploring large, complex datasets in real time
* **Scientists and engineers** creating custom visual interfaces for experiments and simulations
* **Developers** who want a lightweight, portable alternative to bloated visualization frameworks

Whether you're prototyping, building a GUI, or integrating visualization into a larger system, Datoviz gives you low-level power with high-level usability.


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
