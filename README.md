# Datoviz: high-performance scientific data visualization

[**[Installation]**](#%EF%B8%8F-installation-instructions) &nbsp;
[**[Usage]**](#-usage) &nbsp;
[**[User guide]**](docs/userguide.md) &nbsp;
[**[Examples]**](examples/) &nbsp;
[**[API reference]**](docs/api.md) &nbsp;

<!-- INTRODUCTION -->

**⚡️ Datoviz** is an open-source, cross-platform, high-performance library for **scientific data visualization**.

It delivers fast, **high-quality GPU rendering** of 2D and 3D graphical primitives—markers, paths, images, text, meshes, volumes, and more—that scale to millions of elements. Datoviz also supports **graphical user interfaces (GUIs)** for interactive visualization.

Built from the ground up with performance in mind, Datoviz is written primarily in **C** and **C++**, leveraging the [**Khronos Vulkan graphics API**](https://www.vulkan.org/). It offers a C API, low-level Python bindings via `ctypes`, and a higher-level, idiomatic **Python API 🐍**.

Written by one of the original creators of [VisPy](https://vispy.org), a GPU-based Python scientific visualization library, Datoviz aims to serve as the default backend for the upcoming **VisPy 2.0**.

The library is lightweight with minimal dependencies: mostly Vulkan, [**GLFW**](https://www.glfw.org/) for windowing, and [Dear ImGui](https://github.com/ocornut/imgui/) for GUIs.

> ⚠️ **Note:** Datoviz is a young library. The API is stabilizing, but breaking changes may still occur as the project evolves with broader usage.



<!-- SCREENSHOTS -->

## 🖼️ Screenshots from the v0.1 version

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*


<!-- CURRENT STATUS -->

## 🕐 Current status [May 2025]

**The current version is v0.3.**
This release introduces major updates over v0.2, including 2D axes, a new Pythonic API, numerous improvements, and bug fixes.


<!-- FEATURES -->

## ✨ Current features

* **📊 High-quality antialiased 2D visuals**: markers, lines, paths, glyphs
* **📈 2D axes**
* **🌐 3D visuals**: meshes, volumes, volume slices
* **🌈 150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **🖱️ High-level interactivity**: pan & zoom for 2D, arcball for 3D (more later)
* **🎥 Manual control of cameras**: custom interactivity
* **𓈈 Figure subplots** (aka "panels")
* **🖥️ GUIs** using [Dear ImGui](https://github.com/ocornut/imgui/)


### List of visuals

![List of visuals](https://raw.githubusercontent.com/datoviz/data/main/screenshots/visuals.png)



<!-- ROADMAP -->

## 🕐 Roadmap [May 2025]

Looking ahead, the upcoming v0.4 release (late 2025) will focus on foundational improvements to the low-level engine, paving the way for the following key features in future versions:

* 🧊 Correct transparency in 3D mesh and volume rendering
* ✨ Multisample anti-aliasing (MSAA)
* 🎯 Object picking
* 📈 Nonlinear coordinate transforms
* ⚡ CUDA interoperability
* 🧮 Vulkan compute shaders (similar to CUDA kernels)
* 🖌️ Dynamic and customizable shaders
* 🎛️ Combined GPGPU compute and graphics workflows
* 🔗 GPU memory sharing across visuals
* 🐍 IPython integration
* 🖥️ Qt backend support
* 🌐 WebGPU backend


<!-- INSTALLATION -->

## 🛠️ Installation instructions

### Requirements

- A supported operating system: Linux, macOS 12+, or Windows 10+
- A Vulkan-capable GPU (most integrated or dedicated GPUs from the past decade should work)
- Python 3 and NumPy

### Install with pip

```bash
pip install datoviz
```

This command installs a Python wheel that includes the C library, automatically precompiled for your system.


<!-- DOCUMENTATION -->

## 🚀 Usage

Here’s a simple 2D scatter plot example with axes in Python, displaying points with random positions, colors, and sizes.

```python
import numpy as np
import datoviz as dvz

n = 1000
x = np.random.normal(scale=0.2, size=n)
y = np.random.normal(scale=0.2, size=n)

color = np.random.randint(size=(n, 4), low=100, high=240, dtype=np.uint8)
color[:, 3] = 255

size = np.random.uniform(low=10, high=30, size=n)

app = dvz.App(background='white')
figure = app.figure(800, 600)
panel = figure.panel()

xmin, xmax = -1, +1
ymin, ymax = -1, +1
axes = panel.axes((xmin, xmax), (ymin, ymax))

visual = app.point(
    position=axes.normalize(x, y),
    color=color,
    size=size,
)
panel.add(visual)

app.run()
app.destroy()
```

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/quickstart.png)


## 📚 Documentation

* [**📖 User guide**](docs/userguide.md)
* [**🐍 Examples**](examples/)
* [**📚 API** reference](docs/api.md)
* [**🏛️ Architecture** overview](ARCHITECTURE.md)
* [**🏗️ Build** instructions](BUILD.md)
* [**👥 Contributors** instructions](CONTRIBUTING.md)
* [**🛠️ Maintainers** instructions](MAINTAINERS.md)


## 🕰️ History

Datoviz builds on more than a decade of open-source GPU-based scientific visualization work:

- **2012** — Developers of several GPU visualization libraries (Galry, Glumpy, pyqtgraph, visvis) joined forces to create [**VisPy**](https://vispy.org/), a high-performance OpenGL-based visualization library for Python.

- **2015** — The [**Vulkan API**](https://www.khronos.org/vulkan/) was introduced as a modern low-level successor to OpenGL, [inspiring early ideas](https://cyrille.rossant.net/compiler-data-visualization/) for a future visualization library built on Vulkan.

- **2019** — [Cyrille Rossant](https://cyrille.rossant.net/), a VisPy cofounder, began experimenting with Vulkan as the foundation for a new graphics engine tailored to scientific visualization.

- **2021** — The first experimental release of **Datoviz v0.1** laid the foundation for future development. That same year, a [Chan Zuckerberg Initiative (CZI)](https://chanzuckerberg.com/eoss/proposals/) grant to VisPy helped support the ecosystem.

- **2024** — Datoviz **v0.2** was released with a full redesign. It introduced a modular architecture focused on stability and extensibility, paving the way for cross-platform rendering (including WebGPU) with support from a second [CZI grant](https://chanzuckerberg.com/eoss/proposals/).

- **2025** — Datoviz **v0.3** added 2D axes, a more Pythonic API, and core improvements in usability and flexibility.

Datoviz remains closely tied to **VisPy** and is being developed by one of its original authors. As part of the **VisPy 2.0** initiative (led by Cyrille Rossant and Nicolas Rougier), Datoviz will act as a low-level backend beneath a unified scientific visualization layer called the **Graphics Server Protocol (GSP)**, enabling frontends to target multiple renderers (Datoviz, Matplotlib, etc.).

The long-term vision is to enable high-performance 2D/3D scientific visualization across platforms (desktop, web, cloud) and languages (C/C++, Python, Julia, Rust).


## 🤝 Contributing

See the [contributing notes](CONTRIBUTING.md).


## 📄 License

See the [MIT license](LICENSE).


## 🙏 Credits

**Datoviz** is developed by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a global consortium of neuroscience research labs.

### 💸 Funding

Datoviz is supported by:

- <img src="https://upload.wikimedia.org/wikipedia/commons/5/58/Wellcome_Trust_logo.svg" alt="Wellcome Trust" width="100"> [Wellcome Trust](https://wellcome.org/)
- <img src="https://upload.wikimedia.org/wikipedia/commons/c/c5/Simons_Foundation_logo.png" alt="Simons Foundation" width="100"> [Simons Foundation](https://www.simonsfoundation.org/)
- <img src="https://upload.wikimedia.org/wikipedia/fr/f/f0/Chan_Zuckerberg_Initiative_Logo.png" alt="Chan Zuckerberg Initiative" width="100"> [Chan Zuckerberg Initiative](https://chanzuckerberg.com/), through the [Essential Open Source Software for Science program](https://chanzuckerberg.com/eoss/)
