# Visky documentation

**Visky** is an open-source **high-performance interactive scientific data visualization library** leveraging the graphics processing unit (**GPU**) for speed, visual quality, and scalability. It supports both 2D and 3D rendering.

Visky is **written in C** and provides native **Python bindings** (based on Cython). It has been tested on Linux, macOS, and to a lesser extent, Windows. Bindings to other languages could be developed thanks to community efforts (Julia, R, MATLAB, Rust, C#, and so on). Visky uses the **Vulkan graphics API** created by the Khronos consortium, successor of OpenGL. Supporting other modern graphics API, such as WebGPU, would constitute interesting developments.

Visky is currently being developed mostly by [Cyrille Rossant](https://cyrille.rossant.net) at the **International Brain Laboratory**, a consortium of neuroscience research labs around the world.

Visky is at an early stage of development. The library is quite usable but evolves quickly. Many more features will come later and the documentation will be improved. Contributions are highly welcome!


## Screenshots

TODO

See also the [**gallery**](gallery.md).

## Features

* **High-quality antialiased 2D visuals**: markers, paths, lines, text, arrows, polygons, and more (the implementation originates from the glumpy project)
* **3D visuals**: meshes, surfaces, volumes
* **Mixing 2D and 3D** plots seamlessly in the same window
* **Colormaps** included (from matplotlib, colorcet, MATLAB)
* **High-level interactivity**: pan & zoom, mouse arcball, first-person cameras
* **Axes**: ticks, grids, labels
* **Subplots** organized in a grid layout
* **DPI-aware**: partial support for high-resolution monitors
* **GUIs** integrated via the **Dear ImGUI** C++ library (Qt or other backends not required)
* **Custom visuals**, with custom shaders and/or custom data transformations
* Initial support for multiple canvases
* Initial support for offscreen rendering and CPU emulation via swiftshader

Upcoming features:

* More visuals: triangulations, planar straight-line graphs (PSLG), histograms, areas, graphs...
* Further data transformations: logarithmic, polar, basic Earth coordinate systems for geographical data
* Colorbars
* 3D axes
* Mouse picking
* Screencast and video recording with ffmpeg (optional dependency)
* Better support of multiple GPUs
* Qt integration
* Continuous integration, more robust testing...

Long-term future (or shorter if there are community contributions):

* Support for other languages (Julia, R, MATLAB, Rust...)
* Jupyter notebook integration
* Web integration via WebGPU?
* Remote desktop integration?


## Documentation

The documentation is divided into:

* [**User manual**](user/index.md): for regular users with no GPU knowledge, with a focus on scientific 2D/3D plotting
* [**Expert manual**](expert/index.md): for advanced users, with a focus on writing custom visuals, understanding Vulkan and the architecture of the library, creating entirely custom applications, making GPU optimizations, and more
* [**C API reference**](api/index.md): for anyone who needs to use the C API


### Credits and related projects

Visky is developed primarily by Cyrille Rossant at the International Brain Laboratory.

Visky borrows heavily ideas and code from other projects.

#### VisPy

VisPy is a Python scientific visualization library created in 2013 by Luke Campagnola (developer of **pyqtgraph**), Almar Klein (developer of **visvis**), Nicolas Rougier (developer of **glumpy**), and myself (Cyrille Rossant, developer of **galry**). We joined forces to create a single library unifying all of our approaches, which proved to be challenging. There is today a community of users and projects based on VisPy ([napari](https://napari.org/)), and the library is currently being maintained by David Hoese, Eric Larson, and others. VisPy recently received [funding from the **Chan Zuckerberg Initiative**](https://chanzuckerberg.com/eoss/proposals/rebuilding-the-community-behind-vispys-fast-interactive-visualizations/) to improve the documentation and knowledge base.

VisPy is written entirely in Python and it is based on OpenGL, an almost 30 year old technology. Vulkan was first released in 2016 by the Khronos consortium and it can be seen, to some extent, as a successor to OpenGL. However, Vulkan is a lower-level library and it is harder to use. That is the price to pay to reach better GPU performance.

Visky may be seen as a ground-up reincarnation of VisPy, with two fundamental differences: it is written in **C** rather than Python, and it uses **Vulkan** rather than OpenGL.

* The main advantages of C compared to Python are: ability to bind to any other language beyond Python; performance; possibility to use the Vulkan C API directly rather than via a wrapper.
* The main advantages of Vulkan compared to OpenGL are: modern API, more adapted to today's hardware; performance. However, it is more complex and less user-friendly. Visky abstracts away a lot of that complexity.


#### Glumpy

Glumpy, developed by Nicolas Rougier, provides efficient implementations of high-quality 2D visuals on the GPU, using algorithms from the antigrain geometry library. The GPU code of most 2D visuals in Visky comes directly from Glumpy.


#### Dependencies and algorithms

* antigrain geometry
* extended Wilkinson algorithm for tick placement
* freetype
* triangle, for Delaunay triangulations
* earcut
* Dear ImGUI
* GLFW
* ffmpeg (optional)

#### Related projects

* mayavi
* VTK
* napari
* vedo



*[Vulkan]: Low-level graphics API created by Khronos, and successor of OpenGL
*[shaders]: code written in GLSL and executed on the GPU to customize the graphics pipeline
*[GLSL]: OpenGL shading language, the C-like language used to write shaders
