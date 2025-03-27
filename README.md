# Datoviz: high-performance GPU scientific data visualization C/C++ library

[**[Installation]**](#%EF%B8%8F-installation-instructions) &nbsp;
[**[Usage]**](#-usage) &nbsp;
[**[User guide]**](docs/userguide.md) &nbsp;
[**[Examples]**](docs/examples.md) &nbsp;
[**[API reference]**](docs/api.md) &nbsp;

<!-- INTRODUCTION -->

**⚡️ Datoviz** is a cross-platform, open-source, high-performance GPU scientific data visualization library written in **C/C++** on top of the [**Khronos Vulkan**](https://www.vulkan.org/) graphics API and the [**glfw**](https://www.glfw.org/) window library. It provides raw ctypes bindings in **Python 🐍**. In the long term, Datoviz will mostly be used as a **VisPy 2.0 backend**.

Designed for speed, visual quality, and scalability to datasets comprising millions of points, it supports 2D/3D interactive rendering and minimal GUIs via [Dear ImGui](https://github.com/ocornut/imgui/).

**⚠️ Warning:** Although Datoviz has been years in the making, it is still in its **early stages** and would greatly benefit from increased **community feedback**, particularly concerning package and hardware compatibility. The API is still evolving, so expect regular (though hopefully minimal) **breaking changes** for now.

<!-- ROADMAP -->

## 🕐 Current status and roadmap [March 2025]

**The current version is v0.2.3**. Next expected milestones are:

* v0.3 (mid 2025?): 2D axes
* After v0.3 is released, we will focus on improving many internal aspects of Datoviz in order to support more features in the near future:

    * Improved 3D rendering of meshes and volumes (correct transparency)
    * Multisample Anti-Aliasing
    * Picking
    * CUDA compatibility
    * Vulkan compute shaders (similar to CUDA kernels)
    * Mixing GPGPU compute and graphics
    * Sharing GPU data across visuals
    * IPython integration
    * Qt backend
    * WebGPU backend


<!-- SCREENSHOTS -->

## 🖼️ Screenshots from the v0.1 version

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*



<!-- FEATURES -->

## ✨ Features

* **📊 High-quality antialiased 2D visuals**: markers, lines, paths, glyphs
* **🌐 3D visuals**: meshes, volumes, volume slices
* **🌈 150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **🖱️ High-level interactivity**: pan & zoom for 2D, arcball for 3D (more later)
* **🎥 Manual control of cameras**: custom interactivity
* **𓈈 Figure subplots** (aka "panels")
* **🖥️ Minimal GUIs** using [Dear ImGui](https://github.com/ocornut/imgui/)

### List of visuals

![List of visuals](https://raw.githubusercontent.com/datoviz/data/main/screenshots/visuals.png)

### Work in progress

These features are currently planned for **v0.3**:

* **➕ Axes**: ticks, grids, labels
* **🎨 Colorbars**
* **🖱️ More interactivity patterns**
* **📖 More documentation**

### Future work

These features are currently planned for **v0.4 and later**:

* **📐 More visuals**: arrows, polygons, planar straight-line graphs (PSLG), histograms, areas, graphs
* **🎯 Picking**
* **📈 Nonlinear transforms**
* **🖌️ Dynamic shaders**
* **🌐 WebGPU/WebAssembly compatibility**
* **🧮 Compute shaders**
* **🐍 IPython, Jupyter, Qt integration**


<!-- INSTALLATION -->

## 🛠️ Installation instructions

Requirements:

- A supported OS (Linux, macOS 12+, Windows 10+)
- A Vulkan-capable graphics chipset (either integrated or dedicated graphics process unit)
- Python and NumPy

_Note_: You no longer need to install the Vulkan SDK or to manually build the library. Precompiled wheels for Linux, Windows, and macOS have been uploaded to PyPI.


```bash
pip install datoviz
```

<!-- ### Ubuntu 24.04

1. Download the .deb package.
2. Install the .deb package on your system:

    ```bash
    sudo dpkg -i libdatoviz*.deb
    ```

3. Try to run the built-in demo:

    ```bash
    # The .deb package should have installed the shared library libdatoviz.so into /usr/local/lib
    # This line loads this shared library and calls the exposed dvz_demo() C function from Python.
    python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"
    ``` -->



<!-- DOCUMENTATION -->


## 🚀 Usage

Simple scatter plot example (points with random positions, colors, and sizes) in Python, which closely follow the C API.

```python
import numpy as np
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

figure = dvz.figure(scene, 800, 600, 0)
panel = dvz.panel_default(figure)
dvz.panel_panzoom(panel)
visual = dvz.point(batch, 0)

n = 100_000
dvz.point_alloc(visual, n)

pos = np.random.normal(size=(n, 3), scale=.25).astype(np.float32)
dvz.point_position(visual, 0, n, pos, 0)

color = np.random.uniform(size=(n, 4), low=50, high=240).astype(np.uint8)
dvz.point_color(visual, 0, n, color, 0)

size = np.random.uniform(size=(n,), low=10, high=30).astype(np.float32)
dvz.point_size(visual, 0, n, size, 0)

dvz.panel_visual(panel, visual, 0)
dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)

```

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/scatter.png)

Check out the [examples documentation](docs/examples.md) for more usage examples.


## 📚 Documentation

* [**📖 User guide**](docs/userguide.md)
* [**🐍 Examples**](docs/examples.md)
* [**📚 API** reference](docs/api.md)
* [**🏛️ Architecture** overview](ARCHITECTURE.md)
* [**🏗️ Build** instructions](BUILD.md)
* [**👥 Contributors** instructions](CONTRIBUTING.md)
* [**🛠️ Maintainers** instructions](MAINTAINERS.md)


## 🕰️ History

In **2012**, developers of various GPU scientific visualization libraries (Galry, Glumpy, pyqtgraph, visvis) collaborated to create [**VisPy**](https://vispy.org/), an OpenGL-based scientific visualization library for Python.

In **2015**, [**Vulkan**](https://www.khronos.org/vulkan/), the successor to OpenGL, was announced by Khronos, [sparking the idea of a future Vulkan-based visualization library](https://cyrille.rossant.net/compiler-data-visualization/).

In **2019**, [Cyrille Rossant](https://cyrille.rossant.net/), one of the original VisPy developers, began experimenting with Vulkan.

In **2021**, the [first experimental version of Datoviz **v0.1** was released](https://cyrille.rossant.net/datoviz/). This initial release laid the groundwork for further development.

Over the next three years, the technology matured, aided by a [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) awarded to VisPy in **2021**.

In **2024**, Datoviz **v0.2** is released. This version is redesigned from the ground up to enhance modularity and stability, ensuring it can keep pace with the continuous advancements in GPU hardware and graphics rendering APIs.
It features a modular architecture that will allow the porting of Datoviz technology to non-Vulkan environments, such as WebGPU-enabled web browsers (thanks to a second [CZI grant](https://chanzuckerberg.com/eoss/proposals/)).

Datoviz is closely related to **VisPy**, as it is being developed by one of the VisPy cofounders. VisPy 2.0, initiated by Cyrille Rossant and Nicolas Rougier, will offer a high-level scientific API on top of Datoviz, matplotlib, and other renderers via a common medium-level visualization layer called "graphics server protocol (GSP)".

The long-term vision is for high-performance GPU-based 2D/3D scientific visualization to be uniformly available across multiple platforms, environments (desktop, web, cloud-based remote visualization), and programming languages (C/C++, Python, Julia, Rust, etc.).


## 🤝 Contributing

See the [contributing notes](CONTRIBUTING.md).


## 📄 License

See the [MIT license](LICENSE).


## 🙏 Credits

Datoviz is developed by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

It is funded notably by [Chan Zuckerberg Initiative](https://chanzuckerberg.com/)'s [Essential Open Source Software for Science program](https://chanzuckerberg.com/eoss/).
