# Datoviz: high-performance GPU scientific data visualization C/C++ library


[**[User guide]**](docs/userguide.md) &nbsp;
[**[Examples]**](docs/examples.md) &nbsp;
[**[API reference]**](docs/api.md) &nbsp;


<!-- INTRODUCTION -->

**⚡️ Datoviz** is a cross-platform, open-source, high-performance GPU scientific data visualization library written in **C/C++** on top of the [**Khronos Vulkan**](https://www.vulkan.org/) graphics API and the [**glfw**](https://www.glfw.org/) window library. It provides raw ctypes bindings in **Python 🐍**. In the long term, Datoviz will mostly be used as a **VisPy 2.0 backend**.

Designed for speed, visual quality, and scalability to datasets comprising up to $10^6-10^8$ points, it supports 2D/3D interactive rendering and minimal GUIs via [Dear ImGui](https://github.com/ocornut/imgui/).

**⚠️ Warning:** Datoviz is still in its **early stages** and would greatly benefit from increased **community feedback**, especially regarding binaries, packaging, and hardware compatibility. The API is still in flux, so expect regular (but hopefully minimal) **breaking changes** for now. The current working version is **v0.2** and documentation is only available on GitHub for now. The `datoviz.org` documentation website is still about the **deprecated v0.1**. The current version only provides common 2D/3D visuals and basic interactivity for now, no axes yet (coming soon, 90% done).


<!-- SCREENSHOTS -->

## Screenshots from the v0.1 version

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*



<!-- FEATURES -->

## Features

* **📊 High-quality antialiased 2D visuals**: markers, lines, paths, glyphs
* **🌐 3D visuals**: meshes, volumes, volume slices
<!-- * **📊🌐 Mixing 2D and 3D** plots seamlessly in the same window -->
* **🌈 150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **🖱️ High-level interactivity**: pan & zoom for 2D, arcball for 3D (more later)
* **🎥 Manual control of cameras**: custom interactivity
* **𓈈 Subplots** organized in a grid layout
* **🖥️ Minimal GUIs** using [Dear ImGui](https://github.com/ocornut/imgui/)

Work in progress (currently planned for **v0.3**):

* **➕ Axes**: ticks, grids, labels
* **🎨 Color bars**

Future work (planned for **v0.4+**):

* **📐 More visuals**: arrows, polygons, planar straight-line graphs (PSLG), histograms, areas, graphs
* **🎯 Picking**
* **📈 Nonlinear transforms**
* **🖌️ Dynamic shaders**
* **🧮 Compute shaders**
* **🖥️ Qt integration**
* **🐍 Jupyter integration**


## History and current status

In **2012**, developers of various GPU scientific visualization libraries (Galry, Glumpy, pyqtgraph, visvis) collaborated to create [**VisPy**](https://vispy.org/), an OpenGL-based scientific visualization library for Python.

In **2015**, [**Vulkan**](https://www.khronos.org/vulkan/), the successor to OpenGL, was announced by Khronos, [sparking the idea of a future Vulkan-based visualization library]((https://cyrille.rossant.net/compiler-data-visualization/)).

In **2019**, [Cyrille Rossant](https://cyrille.rossant.net/), one of the original VisPy developers, began experimenting with Vulkan.

In **2021**, the [first experimental version of Datoviz **v0.1** was released](https://cyrille.rossant.net/datoviz/). This initial release laid the groundwork for further development.

Over the next three years, the technology matured, aided by a [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) awarded to VisPy in **2021**.

In **2024**, a second [CZI grant](https://chanzuckerberg.com/eoss/proposals/) facilitated the release of Datoviz **v0.2**. This version was redesigned from the ground up to enhance modularity and stability, ensuring it can keep pace with the continuous advancements in GPU hardware and graphics rendering APIs. It features a modular architecture that will allow the porting of Datoviz technology to non-Vulkan environments, such as WebGPU-enabled web browsers.

Datoviz is closely related to **VisPy**, as it is being developed by one of the VisPy cofounders. VisPy 2.0 will offer a high-level scientific API that supports a Datoviz renderer, as well as a Matplotlib renderer and potentially others.

The long-term vision is for high-performance GPU-based 2D/3D scientific visualization to be uniformly available across multiple platforms, environments (desktop, web, cloud-based remote visualization), and programming languages (C/C++, Python, Julia, Rust, etc.).



<!-- INSTALLATION -->

## Installation instructions

### Ubuntu 24.04

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
    ```

### macOS (arm64)

TODO.

### Windows

TODO.


<!-- DOCUMENTATION -->

## Documentation

* [**📖 User guide**](docs/userguide.md)
* [**🐍 Examples**](docs/examples.md)
* [**📚 API** reference](docs/api.md)
* [**🏛️ Architecture** overview](ARCHITECTURE.md)
* [**🏗️ Build** instructions](BUILD.md)
* [**👥 Contributors** instructions](CONTRIBUTING.md)
* [**🛠️ Maintainers** instructions](MAINTAINERS.md)



## Application developer instructions

This section provides general instructions for C/C++ developers who want to use Datoviz in their library or application.

### Ubuntu

TODO.

### macOS (arm64)

Looking at the [justfile](justfile) (`pkg` and `testpkg` commands) may be helpful.
To build an application using Datoviz:

1. You need to link your application to `libdatoviz.dylib`, that you can build yourself or find in the provided `.pkg` installation file.
2. You also need to link to the non-system dependencies of Datoviz, for now they are `libvulkan`, `libMoltenVK` ("emulating" Vulkan on top of Apple Metal), `libpng` and `freetype`. You can see the dependencies with `just deps` (which uses `otool` on `libdatoviz.dylib`). You'll find these dependencies in [`libs/vulkan/macos`](libs/vulkan/macos) in the GitHub repository.
3. You should bundle these `dylib` dependencies alongside your application, and that will depend on how your application is built and distributed.
4. Note that the `just pkg` script modifies the rpath of `libdatoviz.dylib` with [`install_name_tool`](https://www.unix.com/man-page/osx/1/install_name_tool/) before building the `.pkg` package to declare that its dependencies are to be found in the same directory.
5. Another thing to keep in mind is that, for now, the `VK_DRIVER_FILES` environment variable needs to be set to the absolute path to [`MoltenVK_icd.json`](libs/vulkan/macos/MoltenVK_icd.json) (available in this GitHub repository). The `.pkg` package installs it to `/usr/local/lib/datoviz/MoltenVK_icd.json`. Right now, [`datoviz.h`](include/datoviz.h) automatically sets this environment variable if it's included in the source file implementing your `main()` entry-point. These complications are necessary to avoid requiring the end-users to install the Vulkan SDK manually.

### Windows

TODO.

### Technical notes for C/C++ developers

* **🧠 Memory management.** Datoviz uses opaque pointers and manages its own memory. While we have taken great care in proper memory allocation, C is inherently unsafe. Porting the relatively light high-level code of Datoviz (scene API) to a more modern and safer language may be considered in the future.
* **💻 C/C++ usage.** Datoviz employs a restricted and straightforward usage of C, with very limited C++ functionality (mostly common dynamic data structures, in ~10% of the code).
* **📂 Data copies.** When passing data to visuals, data is copied by default to Datoviz for memory safety reasons. This might impact performance and memory usage when handling large datasets (tens of millions of points). We will soon document how to avoid these extra copies and prevent crashes related to Datoviz accessing deallocated memory.
* **🏗️ Modular architecture.** Datoviz v0.2+ features a modular architecture where the low-level Vulkan-specific rendering engine is decoupled from the higher-level visual and interactive logic. A private asynchronous message-based protocol is used internally, enabling a future Javascript/WebAssembly/WebGPU port of Datoviz, which we plan to work on in the coming years.
* **👥 Contributing.** This modular architecture allows C/C++ contributors without GPU knowledge to propose improvements and new functionality in the higher-level parts.
* **🔗 Bindings.** While we provide raw ctypes bindings in Python to the Datoviz C API, our goal is to implement as much functionality in C/C++ to offer the same functionality to other languages that may provide Datoviz bindings in the future (Julia, Rust, R, MATLAB...).


<!-- FUNDING -->

## Funding

Datoviz is developed by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

It is partly funded by [Chan Zuckerberg Initiative](https://chanzuckerberg.com/)'s [Essential Open Source Software for Science program](https://chanzuckerberg.com/eoss/).
