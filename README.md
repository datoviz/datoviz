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

### Current Status

In 2021, the [first experimental version of Datoviz **v0.1x** was released](https://cyrille.rossant.net/datoviz/).

In 2024, version **v0.2x** was released. This version has been redesigned from the ground up to enhance modularity and stability, ensuring it can keep pace with the continuous advancements in GPU hardware and graphics rendering APIs.

### History and Status

In **2012**, developers of various GPU scientific visualization libraries (Galry, Glumpy, pyqtgraph, visvis) collaborated to create [**VisPy**](https://vispy.org/), an OpenGL-based scientific visualization library for Python.

In **2015**, [**Vulkan**](https://www.khronos.org/vulkan/), the successor to OpenGL, was announced by Khronos, [inspiring the idea of a future Vulkan-based visualization library](https://cyrille.rossant.net/compiler-data-visualization/).

In **2019**, [Cyrille Rossant](https://cyrille.rossant.net/), one of the original VisPy developers, began experimenting with Vulkan.

Following an experimental release in **2021**, the technology matured over the next three years, thanks in part to a generous [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) awarded to VisPy in 2021.

<!-- NEW VERSION -->

### New v0.2x Version and Long-term Vision

In **2024**, thanks to a second [Chan Zuckerberg Initiative (CZI) grant](https://chanzuckerberg.com/eoss/proposals/) awarded to the VisPy project in 2024, a redesigned **v0.2x** version was released for testing and community feedback. It features a redesigned architecture that will allow us to port Datoviz technology to non-Vulkan environments, such as WebGPU-enabled web browsers.

This will help us achieve a long-term vision where **high-performance GPU-based 2D/3D scientific visualization** is uniformly available across multiple platforms, environments (desktop, web, cloud-based remote visualization), and programming languages (C/C++, Python, Julia, Rust, etc.).



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

Work in progress.

* [Architecture overview](ARCHITECTURE.md)



<!-- BUILDING -->

## Building instructions

If packages are not available on your system, you can build Datoviz yourself.

### Ubuntu 24.04

```bash
# Install the build and system dependencies.
sudo apt install build-essential cmake gcc ccache ninja-build xorg-dev clang-format libtinyxml2-dev libfreetype-dev

# Install just, see https://github.com/casey/just
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash

# Clone the Datoviz repo and build.
git clone https://github.com/datoviz/datoviz.git@v0.2x --recursive
cd datoviz

# This call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Compile and run an example.
just example scatter
```


### macOS (arm64)

```bash
# Install brew if you don't have it already.
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
(echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> /Users/cyrille/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"

# Install dependencies.
brew install just cmake ccache ninja freetype

# Clone the Datoviz repo.
git clone https://github.com/datoviz/datoviz.git@v0.2x --recursive
cd datoviz

# This call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Compile and run an example.
just example scatter
```

### Windows

TODO.


<!-- PACKAGING -->

## Packaging instructions

This section provides instructions for maintainers who need to create binary packages.

### Ubuntu 24.04

```bash
sudo apt-get install dpkg-dev fakeroot

# Generate a .deb package in packaging/
just deb

# Test .deb installation in a Docker container
just testdeb
```

### macOS (arm64)

```bash
# Generate a .pkg package in packaging/
just pkg

# Test .pkg installation in an UTM virtual machine
just testpkg
```

Note: in addition to `libvulkan.dylib`, the package should also save both `libs/vulkan/macos/MoltenVK_icd.json` and `libMoltenVK.dylib` to a [standard system location](https://vulkan.lunarg.com/doc/view/1.3.243.0/mac/LoaderDriverInterface.html#user-content-example-macos-driver-search-path).


### Windows

TODO.


<!-- FUNDING -->

## Funding

Datoviz is developed by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

It is partly funded by [Chan Zuckerberg Initiative](https://chanzuckerberg.com/)'s [Essential Open Source Software for Science program](https://chanzuckerberg.com/eoss/).
