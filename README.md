# Datoviz: high-performance GPU scientific data visualization C/C++ library

<!-- INTRODUCTION -->

**Datoviz** is an open-source **high-performance GPU scientific data visualization C/C++ library**. Designed for speed, visual quality, and scalability for interactive exploration of large datasets, it supports 2D/3D rendering and GUIs.

Although its inception and development occurred over a long history spanning more than a decade, Datoviz is still an **early stage library** that would now benefit from increased community feedback.

The experimental [**v0.1x version**](https://datoviz.org/) was released in 2021. The **v0.2x version** is now available (in a separate git branch) to audacious users for early testing and community feedback.

The medium-term plan is to **rebuild VisPy 2.0 on top of Datoviz** (and possibly other renderers via a shared intermediate-level graphics protocol).



<!-- SCREENSHOTS -->

## Screenshots from the v0.1x version

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*



<!-- FEATURES -->

## Features

* **High-quality antialiased 2D visuals**: markers, lines, paths, glyphs
* **3D visuals**: meshes, volumes, volume slices
* **Mixing 2D and 3D** plots seamlessly in the same window
* **150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **High-level interactivity**: pan & zoom, mouse arcball
* **Subplots** organized in a grid layout

Work in progress:

* **Axes**: ticks, grids, labels
* **GUIs**
* **Custom visuals**, with custom shaders and/or custom data transformations

Future work:

* **More visuals**: arrows, polygons, planar straight-line graphs (PSLG), histograms, areas, graphs...
* **Picking**
* **IPython/Jupyter integration**
* **Nonlinear transforms**
* **Compute shaders**
* **Qt integration**


<!-- HISTORY AND STATUS -->

## Overview

### History and status

In **2012**, several developers of GPU scientific visualization libraries (Galry, Glumpy, pyqtgraph, visvis) joined forces and created [**VisPy**](https://vispy.org/), an OpenGL-based scientific visualization library in Python.

In **2015**, [**Vulkan**]((https://www.khronos.org/vulkan/)), successor of OpenGL, was announced by Khronos, [spurring ideas on a future, Vulkan-based visualization library.](https://cyrille.rossant.net/compiler-data-visualization/)

In **2019**, [Cyrille Rossant](https://cyrille.rossant.net/), one of the original VisPy developers, started experiments with Vulkan.

In **2021**, [a first experimental version of Datoviz **v0.1x** was released](https://cyrille.rossant.net/datoviz/). The underlying technology continued to mature for three years, notably thanks to a generous [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) attributed to VisPy in 2021.

In **2024**, a redesigned **v0.2x** version is available for testing and community feedback.


<!-- NEW VERSION -->

### New v0.2x version and long-term vision

The v0.2x version has been redesigned from the ground-up for increased modularity and independence with respect to the underlying GPU technology and constant innovations on hardware technology and graphics rendering APIs.

Thanks to a generous [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) attributed to the VisPy project in 2024, this redesigned architecture will let us port the Datoviz technology to non-Vulkan environments, such as WebGPU-enabled web browsers.

This will help us achieve a long-term vision where **high-performance GPU-based 2D/3D scientific visualization** is uniformly available on multiple platforms, multiple environments (desktop, web, cloud-based remote visualization), and multiple programming languages (C/C++, Python, Julia, Rust...).


<!-- STAGED RELEASE PROCESS -->

### Staged release process for gathering community testing and feedback

We plan a slow, staged release process that aims at **improving the quality and stability** of the rendering engine, improving **hardware support**, and testing the user API via **community feedback**.

We plan to release the v0.2x in the following steps:

1. [Summer 2024] **C shared library** on **Linux**
2. [TBD] **C shared library** on the **three main platforms** (Linux, macOS, Windows)
3. [TBD] **Python wrapper** on **Linux**
4. [TBD] **Python wrapper** on the **three main platforms**

In the meantime, experienced users can try to build the library themselves on their preferred platform and contribute to the project via issues or pull requests.


<!-- FUNDING -->

### Funding

Datoviz is developed at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

It is funded notably by [Chan Zuckerberg Initiative](https://chanzuckerberg.com/)'s [Essential Open Source Software for Science program](https://chanzuckerberg.com/eoss/).



<!-- QUICK START -->

## Getting started

Work in progress.

### Ubuntu 24.04

* Download the .deb package.
* Install the .deb package to your system:

    sudo dpkg -i libdatoviz*.deb

* Test the library with the built-in demo:

    # The .deb package should have installed the shared library libdatoviz.so into /usr/local/lib
    # This line loads this shared library and calls the exposed dvz_demo() C function from Python.
    python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"




<!-- DOCUMENTATION -->

## Documentation

Work in progress.

* [Architecture overview](ARCHITECTURE.md)



<!-- BUILDING -->

## Building instructions

This section provides instructions for building the library, if packages are not yet available.

### Ubuntu 24.04

```bash
# Install the build and system dependencies.
sudo apt install build-essential cmake gcc ninja-build xorg-dev clang-format libtinyxml2-dev libfreetype-dev

# Install just, see https://github.com/casey/just
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash

# Clone the Datoviz repo and build.
git clone https://github.com/datoviz/datoviz.git@v0.2x
cd datoviz
git submodule update --init --recursive

# This call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should suceed.
just build

# Compile and run an example.
just example scatter
```




<!-- PACKAGING -->

## Packaging instructions

This section provides instructions for maintainers who need to create binary packages.

### Debian

```bash
sudo apt-get install dpkg-dev fakeroot

# Generate a .deb package in packaging/
just deb

# Test .deb installation in a Docker container
just testdeb
```
