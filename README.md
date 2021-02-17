# Datoviz: GPU interactive scientific data visualization with Vulkan

**Datoviz** is an open-source **high-performance interactive scientific data visualization library** leveraging the graphics processing unit (**GPU**) for speed, visual quality, and scalability. It supports both 2D and 3D rendering, as well as minimal graphical user interfaces (using the [Dear ImGUI library](https://github.com/ocornut/imgui)).

**Written in C**, Datoviz has been designed from the ground up for **performance**. It provides native **Python bindings** (based on Cython). Bindings to other languages could be developed thanks to community efforts (Julia, R, MATLAB, Rust, C#, and so on). Datoviz uses the [**Vulkan graphics API**](https://www.khronos.org/vulkan/) created by the Khronos consortium, successor of OpenGL. Supporting other modern graphics API, such as WebGPU, would constitute interesting developments.

Datoviz is currently being developed mostly by [Cyrille Rossant](https://cyrille.rossant.net) at the [International Brain Laboratory](http://internationalbrainlab.org/), a consortium of neuroscience research labs around the world.

Datoviz is at an early stage of development. The library is quite usable but evolves quickly. Datoviz has been tested on Linux, macOS (Intel), and to a lesser extent, Windows. It should work on most computers, with or without a discrete GPU (but with up-to-date graphics drivers). Many more features will come later and the documentation will be improved. Contributions are highly welcome!

## Screenshots

![](https://raw.githubusercontent.com/datoviz/data/master/screenshots/datoviz.jpg)
*Credits: mouse brain volume: [Allen SDK](https://alleninstitute.github.io/AllenSDK/). France: [Natural Earth](https://www.naturalearthdata.com/). Molecule: [Crystal structure of S. pyogenes Cas9 from PDB](https://www.rcsb.org/structure/4cmp) (thanks to Eric for conversion to OBJ mesh). Earth: [Pixabay](https://pixabay.com/fr/illustrations/terre-planet-monde-globe-espace-1617121/). Raster plot: IBL. 3D human brain: [Anneke Alkemade et al. 2020](https://www.frontiersin.org/articles/10.3389/fnana.2020.536838/full), thanks to Pierre-Louis Bazin and Julia Huntenburg.*


## Documentation

The documentation is divided into:

* **[Installation](https://datoviz.org/tutorials/install/)**: install guide (no precompiled packages yet, coming soon),
* **[Tutorials](https://datoviz.org/tutorials/quickstart)**: quickstart Python tutorial,
* **[Examples](https://datoviz.org/examples/)**: gallery of screenshots and associated Python code,
* **[How to guides](https://datoviz.org/howto/)**: advanced topics for expert users, explaining how to use C and Vulkan for making custom visuals and applications,
* **[Reference](https://datoviz.org/reference/)**: comprehensive list of included colormaps, visuals, and graphics,
* **[C API reference](https://datoviz.org/api/)**: list of all publicly available C functions,
* **[Discussions](https://datoviz.org/discussions/)**: explanations, Vulkan crash course, and notes.

<!-- NOTE: we use absolute URLs so that the links work both on the GitHub README and on the website -->


## Preliminary performance results

* scatter plot with 10M points: **250 FPS** (`point` visual)
* high-resolution 3D mesh with 10M triangles and 5M vertices: **400 FPS**
* 1000 signals with 30K points each (30M vertices): **200 FPS**

*GPU: 2019 NVIDIA GeForce RTX 2070 SUPER. Window size: 1024x768.*


## Features

* **High-quality antialiased 2D visuals**: markers, paths, lines, text, polygons, and more (visuals originally implemented in [Glumpy](https://glumpy.github.io/))
* **3D visuals**: meshes, surfaces, volumes
* **Mixing 2D and 3D** plots seamlessly in the same window
* **~150 colormaps** included (from matplotlib, colorcet, MATLAB)
* **High-level interactivity**: pan & zoom, mouse arcball, first-person cameras
* **Axes**: ticks, grids, labels
* **Subplots** organized in a grid layout
* **DPI-aware**: partial support for high-resolution monitors
* **GUIs** integrated via the **Dear ImGUI** C++ library (Qt or other backends not required)
* **Custom visuals**, with custom shaders and/or custom data transformations
* Mouse picking
* Initial support for multiple canvases
* Initial builtin support for screencasts and video recording with ffmpeg (optional dependency)
* Initial support for offscreen rendering and CPU emulation via swiftshader

Upcoming features:

* More visuals: arrows, triangulations, planar straight-line graphs (PSLG), histograms, areas, graphs...
* Further data transformations: logarithmic, polar, basic Earth coordinate systems for geographical data
* Colorbars
* 3D axes
* Better support of multiple GPUs
* Qt integration
* Continuous integration, more robust testing

Long-term future (or shorter if there are community contributions):

* Support for other languages (Julia, R, MATLAB, Rust...)
* Jupyter notebook integration
* Web integration via WebGPU?
* Remote desktop integration?


## Credits and related projects

Datoviz is developed primarily by [Cyrille Rossant](https://cyrille.rossant.net/) at the [International Brain Laboratory](https://www.internationalbrainlab.com/). The logo was graciously created by [Chiara Zini](https://www.linkedin.com/in/czini/).

Datoviz borrows heavily ideas and code from other projects.

### VisPy

[**VisPy**](https://vispy.org/) is a Python scientific visualization library created in 2013 by Luke Campagnola (developer of [**pyqtgraph**](http://www.pyqtgraph.org/)), Almar Klein (developer of [**visvis**](https://github.com/almarklein/visvis)), Nicolas Rougier (developer of [**glumpy**](https://glumpy.github.io/)), and myself (Cyrille Rossant, developer of **galry**). We joined forces to create a single library unifying all of our approaches, which proved to be challenging. There is today a community of users and projects based on VisPy ([napari](https://napari.org/)), and the library is currently being maintained by David Hoese, Eric Larson, and others. VisPy recently received [funding from the **Chan Zuckerberg Initiative**](https://chanzuckerberg.com/eoss/proposals/rebuilding-the-community-behind-vispys-fast-interactive-visualizations/) to improve the documentation and knowledge base.

VisPy is written entirely in Python and it is based on OpenGL, an almost 30 year old technology. Vulkan was first released in 2016 by the Khronos consortium and it can be seen, to some extent, as a successor to OpenGL. However, Vulkan is a lower-level library and it is harder to use. This is the price to pay to reach better GPU performance.

Datoviz may be seen as a ground-up reincarnation of VisPy, with two fundamental differences: it is written in **C** rather than Python, and it uses **Vulkan** rather than OpenGL.

* The main advantages of C compared to Python are: ability to bind to any other language beyond Python; performance; possibility to use the Vulkan C API directly rather than via a wrapper.
* The main advantages of Vulkan compared to OpenGL are: modern API, more adapted to today's hardware; performance. However, it is more complex and less user-friendly. Datoviz abstracts a lot of that complexity away.


### Glumpy

Glumpy, developed by Nicolas Rougier, provides [efficient implementations of high-quality 2D visuals on the GPU](https://www.labri.fr/perso/nrougier/python-opengl/), using algorithms from the antigrain geometry library. The GPU code of most 2D visuals in Datoviz comes directly from Glumpy.


### Dependencies and algorithms

* [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) **(mandatory)**
* [GLFW](https://www.glfw.org/) **(mandatory)**
* [ffmpeg](https://ffmpeg.org/) (optional), for making live screencasts
* [libpng](http://www.libpng.org/pub/png/libpng.html) (optional), for making PNG screenshots
* [glslang](https://github.com/KhronosGroup/glslang) (optional), for compiling GLSL shaders to SPIR-V on the fly
* [earcut](https://github.com/mapbox/earcut) (included), developed by Mapbox, for polygon triangulations
* [triangle](https://www.cs.cmu.edu/~quake/triangle.html) (included), developed by Jonathan Richard Shewchuk, for Delaunay triangulations
* [extended Wilkinson algorithm](http://vis.stanford.edu/papers/tick-labels) (included) for tick placement
* [Dear ImGUI](https://github.com/ocornut/imgui) (included)
* [antigrain geometry](https://en.wikipedia.org/wiki/Anti-Grain_Geometry) (GLSL implementation included)
* [msdfgen](https://github.com/Chlumsky/msdfgen): multi-channel signed distance field (to do: bundle as submodule?)



An upcoming version will also have the following dependencies:

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

Datoviz is being developed at the International Brain Laboratory, with funding from the Simons Foundation, the Flatiron Institute, the Wellcome Trust, INCF.

![](docs/images/simons.png)
![](docs/images/flatiron.png)
![](docs/images/wellcome.jpg)
![](docs/images/incf.jpg)


Glossary:

*[Vulkan]: Low-level graphics API created by Khronos, and successor of OpenGL
*[shaders]: code written in GLSL and executed on the GPU to customize the graphics pipeline
*[GLSL]: OpenGL shading language, the C-like language used to write shaders
