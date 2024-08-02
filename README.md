# Datoviz: high-performance GPU scientific data visualization C/C++ library

[**[User guide]**](docs/userguide.md) &nbsp;
[**[Examples]**](docs/examples.md) &nbsp;
[**[API reference]**](docs/api.md) &nbsp;

<!-- INTRODUCTION -->

**⚡️ Datoviz** is a cross-platform, open-source, high-performance GPU scientific data visualization library written in **C/C++** on top of the [**Khronos Vulkan**](https://www.vulkan.org/) graphics API and the [**glfw**](https://www.glfw.org/) window library. It provides raw ctypes bindings in **Python 🐍**. In the long term, Datoviz will mostly be used as a **VisPy 2.0 backend**.

Designed for speed, visual quality, and scalability to datasets comprising up to $10^6-10^8$ points, it supports 2D/3D interactive rendering and minimal GUIs via [Dear ImGui](https://github.com/ocornut/imgui/).

**⚠️ Warning:** Although Datoviz has been years in the making, it is still in its **early stages** and would greatly benefit from increased **community feedback**, particularly concerning binaries, packaging, and hardware compatibility. The API is still evolving, so expect regular (though hopefully minimal) **breaking changes** for now. The current version is **v0.2**, with documentation available only on GitHub. The `datoviz.org` website still reflects the **deprecated v0.1** documentation, it will be updated soon.

**🕐 Roadmap.** In the medium term: increasing OS and hardware compatibility, providing more visuals, interactivity patterns, and GUI controls. In the long term: picking, custom visuals and shaders, nonlinear transforms, WebGPU/WebAssembly compatibility, integration with IPython, Jupyter and Qt.


<!-- SCREENSHOTS -->

## Screenshots from the v0.1 version

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*



<!-- FEATURES -->

## Features

* **📊 High-quality antialiased 2D visuals**: markers, lines, paths, glyphs
* **🌐 3D visuals**: meshes, volumes, volume slices
* **🌈 150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **🖱️ High-level interactivity**: pan & zoom for 2D, arcball for 3D (more later)
* **🎥 Manual control of cameras**: custom interactivity
* **𓈈 Figure subplots** (aka "panels")
* **🖥️ Minimal GUIs** using [Dear ImGui](https://github.com/ocornut/imgui/)

Work in progress (currently planned for **v0.3**):

* **➕ Axes**: ticks, grids, labels
* **🖱️ More interactivity patterns**
* **🎨 Colorbars**

Future work (planned for **v0.4 and later**):

* **📐 More visuals**: arrows, polygons, planar straight-line graphs (PSLG), histograms, areas, graphs
* **🎯 Picking**
* **📈 Nonlinear transforms**
* **🖌️ Dynamic shaders**
* **🌐 WebGPU/WebAssembly compatibility**
* **🧮 Compute shaders**
* **🐍 IPython, Jupyter, Qt integration**


<!-- INSTALLATION -->

## Installation instructions

```bash
pip install git+https://github.com/datoviz/datoviz/tree/v0.2x
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


## Usage

Check that Datoviz works correctly by running the demo:

```python
import datoviz as dvz
dvz.demo()
```

Check out the [examples documentation](docs/examples.md) for more usage examples.


## Documentation

* [**📖 User guide**](docs/userguide.md)
* [**🐍 Examples**](docs/examples.md)
* [**📚 API** reference](docs/api.md)
* [**🏛️ Architecture** overview](ARCHITECTURE.md)
* [**🏗️ Build** instructions](BUILD.md)
* [**👥 Contributors** instructions](CONTRIBUTING.md)
* [**🛠️ Maintainers** instructions](MAINTAINERS.md)


## History and current status

In **2012**, developers of various GPU scientific visualization libraries (Galry, Glumpy, pyqtgraph, visvis) collaborated to create [**VisPy**](https://vispy.org/), an OpenGL-based scientific visualization library for Python.

In **2015**, [**Vulkan**](https://www.khronos.org/vulkan/), the successor to OpenGL, was announced by Khronos, [sparking the idea of a future Vulkan-based visualization library]((https://cyrille.rossant.net/compiler-data-visualization/)).

In **2019**, [Cyrille Rossant](https://cyrille.rossant.net/), one of the original VisPy developers, began experimenting with Vulkan.

In **2021**, the [first experimental version of Datoviz **v0.1** was released](https://cyrille.rossant.net/datoviz/). This initial release laid the groundwork for further development.

Over the next three years, the technology matured, aided by a [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) awarded to VisPy in **2021**.

In **2024**, a second [CZI grant](https://chanzuckerberg.com/eoss/proposals/) facilitated the release of Datoviz **v0.2**. This version was redesigned from the ground up to enhance modularity and stability, ensuring it can keep pace with the continuous advancements in GPU hardware and graphics rendering APIs. It features a modular architecture that will allow the porting of Datoviz technology to non-Vulkan environments, such as WebGPU-enabled web browsers.

Datoviz is closely related to **VisPy**, as it is being developed by one of the VisPy cofounders. VisPy 2.0 will offer a high-level scientific API on top of Datoviz, matplotlib, and other renderers via a common medium-level visualization layer called "graphics server protocol (GSP)".

The long-term vision is for high-performance GPU-based 2D/3D scientific visualization to be uniformly available across multiple platforms, environments (desktop, web, cloud-based remote visualization), and programming languages (C/C++, Python, Julia, Rust, etc.).


## Contributing

See the [contributing notes](CONTRIBUTING.md).


## License

See the [MIT license](LICENSE).


## Credits

Datoviz is developed by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

It is funded notably by [Chan Zuckerberg Initiative](https://chanzuckerberg.com/)'s [Essential Open Source Software for Science program](https://chanzuckerberg.com/eoss/).
