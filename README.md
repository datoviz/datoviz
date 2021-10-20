# Datoviz: GPU interactive scientific data visualization with Vulkan

**Datoviz** is an open-source **high-performance interactive scientific data visualization library** leveraging the graphics processing unit (**GPU**) for speed, visual quality, and scalability. It supports both 2D and 3D rendering, as well as minimal graphical user interfaces (using the [Dear ImGUI library](https://github.com/ocornut/imgui)).

**Written in C/C++**, Datoviz has been designed from the ground up for **performance**. It provides native **Python bindings** (based on Cython). Bindings to other languages could be developed thanks to community efforts (Julia, R, MATLAB, Rust, C#, and so on). Datoviz uses the [**Vulkan graphics API**](https://www.khronos.org/vulkan/) created by the Khronos consortium, successor of OpenGL. Supporting other modern graphics API, such as WebGPU, would constitute interesting developments.

Datoviz is currently being developed mostly by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

!!! note
    Datoviz is at an early stage of development and the API is not yet stabilized. Use at your own risks, but feel free to share your feedback, suggestions, use-cases, feature requests on GitHub.


## Screenshots

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*


## Code example

```python
import numpy as np
from datoviz import canvas, run, colormap

panel = canvas(show_fps=True).scene().panel()
visual = panel.visual('marker')

N = 10_000
pos = np.random.randn(N, 3)
ms = np.random.uniform(low=2, high=35, size=N)
color = colormap(np.random.rand(N), cmap='viridis')

visual.data('pos', pos)
visual.data('ms', ms)
visual.data('color', color)

run()
```


## Documentation

The documentation is divided into:

* **[Installation](https://datoviz.org/tutorials/install/)**: install guide,
* **[Quick start tutorial](https://datoviz.org/tutorials/quickstart)** with Python,
* **[Examples](https://datoviz.org/examples/)**: gallery of Python examples,
* **[How to guides](https://datoviz.org/howto/)**: advanced topics explaining how to use the C API, how to create custom visuals...
* **[Reference](https://datoviz.org/reference/)**: comprehensive list of included colormaps, visuals, and graphics pipelines,
* **[Discussions](https://datoviz.org/discussions/)**: high-level discussions, Vulkan crash course...
* **[C API reference](https://datoviz.org/api/)**,

<!-- NOTE: we use absolute URLs so that the links work on both the GitHub README and the website -->


## Preliminary performance results

* scatter plot with 10M points: **250 FPS** (`point` visual)
* high-resolution 3D mesh with 10M triangles and 5M vertices: **400 FPS**
* 1000 signals with 30K points each (30M vertices): **200 FPS**

*GPU: 2019 NVIDIA GeForce RTX 2070 SUPER. Window size: 1024x768.*


## Features

### Current features

* **High-quality antialiased 2D visuals**: markers, paths, lines (contributions by Nicolas P. Rougier, code from [Glumpy](https://glumpy.github.io/))
* **3D visuals**: meshes
* **Mixing 2D and 3D** plots seamlessly in the same window
* **~150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **High-level interactivity**: pan & zoom, mouse arcball, first-person cameras
* **Axes**: ticks, grids, labels
* **Subplots** organized in a grid layout
* **GUIs** integrated via the **Dear ImGUI** C++ library (Qt or other backends not required)
* **Custom visuals**, with custom shaders and/or custom data transformations

### Experimental/work-in-progress features

* Multiple canvases
* Builtin creencasts and video recording with ffmpeg (optional dependency)
* Offscreen rendering and CPU emulation via swiftshader
* Mouse picking
* IPython event-loop integration
* DPI-aware canvases
* Continuous integration and continuous building
* Panel linking, shared axes

### Longer-term features

* More visuals: arrows, triangulations, planar straight-line graphs (PSLG), histograms, areas, graphs, fake 3D spheres...
* Further data transformations: logarithmic, polar, basic Earth coordinate systems for geographical data
* Fixed aspect ratio
* Colorbars
* 3D axes
* Deep zooming
* CUDA-Vulkan interoperability example
* Better support of multiple GPUs
* Qt integration
* Bindings in other languages (Julia, R, MATLAB, Rust...)
* Automated benchmarkes
* Distributed architecture (integration in the web browser, Jupyter...)


## History and roadmap

* late 2019: first experiments with using Vulkan for scientific visualization
* 2020: multiple cycles of prototyping and refactoring
* **17 Feb 2021**: first public experimental release (manual compilation required)
* **31 May 2021**: first experimental release `v0.1.0-alpha.0` with precompiled pip wheels for Linux, Windows, macOS
* **20 Oct 2021?**: new experimental release `v0.1.0-alpha.1` with bug fixes and minor improvements
* early 2022? `v0.1.0` release
* late 2022?: redesigned internal architecture for multithreading and distributed environments (still a work-in-progress)



## Credits and related projects

Datoviz borrows heavily ideas and code from other projects.


### VisPy

[**VisPy**](https://vispy.org/) is a Python scientific visualization library created in 2013 by Luke Campagnola (developer of [**pyqtgraph**](http://www.pyqtgraph.org/)), Almar Klein (developer of [**visvis**](https://github.com/almarklein/visvis)), Nicolas Rougier (developer of [**glumpy**](https://glumpy.github.io/)), and myself (Cyrille Rossant, developer of **galry**). We joined forces to create a single library unifying all of our approaches. There is today a community of users and projects based on VisPy ([napari](https://napari.org/)). David Hoese and some of the original VisPy developers are currently maintaining the library. The current version of VisPy suffers from the limitations of OpenGL, a 30-year-old technology.

In 2020, VisPy received a 1-year [funding from the **Chan Zuckerberg Initiative (CZI)**](https://chanzuckerberg.com/eoss/proposals/rebuilding-the-community-behind-vispys-fast-interactive-visualizations/) to improve the documentation and knowledge base.

In 2021, VisPy received another [CZI grant](https://chanzuckerberg.com/eoss/proposals/vispy-2-0-next-generation-interactive-scientific-visualization-in-python/) for building VisPy 2.0, a completely redesigned library leveraging newer GPU technology such as Vulkan and WebGPU. The current strategy is to rebuild VisPy on top of Datoviz, pygfx (a WebGPU-based library developed by Almar Klein), and other backends.


### Glumpy

Glumpy, developed by Nicolas Rougier, provides [efficient implementations of high-quality 2D visuals on the GPU](https://www.labri.fr/perso/nrougier/python-opengl/), using algorithms from the antigrain geometry library. The GPU code of most 2D visuals in Datoviz comes directly from Glumpy.


### Dependencies and algorithms

* [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) **(mandatory)**
* [GLFW](https://www.glfw.org/) **(mandatory)** (support for alternative window backends will be considered)
* [ffmpeg](https://ffmpeg.org/) (optional), for making live screencasts
* [libpng](http://www.libpng.org/pub/png/libpng.html) (optional), for making PNG screenshots
* [glslang](https://github.com/KhronosGroup/glslang) (optional), for compiling GLSL shaders to SPIR-V on the fly
* [earcut](https://github.com/mapbox/earcut) (included), developed by Mapbox, for polygon triangulations
* [triangle](https://www.cs.cmu.edu/~quake/triangle.html) (included), developed by Jonathan Richard Shewchuk, for Delaunay triangulations
* [extended Wilkinson algorithm](http://vis.stanford.edu/papers/tick-labels) (included) for tick placement
* [Dear ImGUI](https://github.com/ocornut/imgui) (included)
* [antigrain geometry](https://en.wikipedia.org/wiki/Anti-Grain_Geometry) (GLSL implementation included)
* [msdfgen](https://github.com/Chlumsky/msdfgen): multi-channel signed distance field (to do: bundle as submodule?)
* [freetype](https://www.freetype.org/) (optional)


### Related projects

* [mayavi](https://docs.enthought.com/mayavi/mayavi/)
* [VTK](https://vtk.org/)
* [napari](https://napari.org/)
* [vedo](https://github.com/marcomusy/vedo)
* [ipygany](https://ipygany.readthedocs.io/en/latest/)
* [ipyvolume](https://github.com/maartenbreddels/ipyvolume)
* [Makie.jl](http://makie.juliaplots.org/stable/)


## Funding

Datoviz is being developed by [Cyrille Rossant](https://cyrille.rossant.net/) at the [International Brain Laboratory](https://www.internationalbrainlab.com/), with funding from the Simons Foundation, the Flatiron Institute, the Wellcome Trust, INCF. The logo was graciously created by [Chiara Zini](https://www.linkedin.com/in/czini/).

![](https://raw.githubusercontent.com/datoviz/datoviz/main/docs/images/simons.png)
![](https://raw.githubusercontent.com/datoviz/datoviz/main/docs/images/flatiron.png)
![](https://raw.githubusercontent.com/datoviz/datoviz/main/docs/images/wellcome.jpg)
![](https://raw.githubusercontent.com/datoviz/datoviz/main/docs/images/incf.jpg)


Glossary:

*[Vulkan]: Low-level graphics API created by Khronos, and successor of OpenGL
*[shaders]: code written in GLSL and executed on the GPU to customize the graphics pipeline
*[GLSL]: OpenGL shading language, the C-like language used to write shaders
