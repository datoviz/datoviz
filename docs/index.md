# Datoviz

**Datoviz** is a high-performance, cross-platform scientific visualization library built for modern GPUs. It provides a minimal, low-overhead API in both **C** and **Python**, enabling fast and interactive rendering of large-scale 2D and 3D data.

Whether you're building research tools, real-time visual interfaces, or custom scientific apps, Datoviz gives you the control and speed you need — without sacrificing interactivity or clarity.

> ⚠️ **Note:** Datoviz is a young library. The API is stabilizing, but breaking changes may still occur as the project evolves with broader usage.

![Datoviz screenshot](https://raw.githubusercontent.com/datoviz/data/main/screenshots/hero.png)


## Features

- 🔬 Designed for **scientific data**: high-dimensional, large-scale, precise
- 🚀 **GPU-accelerated** rendering with Vulkan (and WebGPU support in progress)
- 🎯 Minimal core API, no unnecessary dependencies
- 🧩 Support for multiple **visual primitives**: points, lines, images, meshes, volumes, and more
- 🔁 Built-in interactivity: pan, zoom, arcball, user input, event hooks
- 🧪 C and Python bindings, ideal for integrating with existing workflows


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
