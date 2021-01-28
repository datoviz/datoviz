

## History and motivations

There is a large number of visualization libraries in many languages. Why the need for a new one?

In our experience, most existing libraries do not scale well to large datasets, and performance is frequently an afterthought. Usage of the graphics processor for visualization is typically restricted to 3D, whereas 2D visualization would highly benefit from GPU when displaying millions of points. Additionally, many libraries target a single programming language. We felt there was a lack of a high-performance, GPU-accelerated, language-agnostic scientific visualization library.


### VisPy

Visky borrows ideas and code from several existing projects, the main one being **VisPy**.

This Python scientific visualization library was created in 2013 by Luke Campagnola (developer of **pyqtgraph**), Almar Klein (developer of **visvis**), Nicolas Rougier (developer of **glumpy**), and myself (Cyrille Rossant, developer of **galry**). We had decided to join forces to create a single library borrowing ideas from the four projects. There is today a small community of developers and users around VisPy, although the four initial developers are no longer actively involved in the project. VisPy has recently received [funding from the **Chan Zuckerberg Initiative**](https://chanzuckerberg.com/eoss/proposals/rebuilding-the-community-behind-vispys-fast-interactive-visualizations/), thanks to the efforts of David Hoese, the new maintainer of the library. VisPy is also extensively used by the [napari image viewer software](https://napari.org/).

VisPy is written in pure Python and leverages OpenGL. Visky owes a lot to VisPy, beyond its very name. Many concepts have matured in VisPy and made it to Visky.

However, VisPy had several limitations:

* **Performance**: VisPy's performance is limited by (1) the fact that it is written in pure Python, (2) the fact that it is based on OpenGL, an API that shows its age, and (3) the fact that it accesses OpenGL via Python. With OpenGL, multiple graphics calls need to be made at every event loop iteration, and the Python overheads add up quickly. Making performance acceptable despite these limitations led to a high level of complexity, both in the implementation and in the programming interfaces provided by VisPy.

* **Python**: although Python is a leading language in data visualization and scientific computing, other platform may benefit from a high-performance visualization library, including MATLAB, Julia, R, C++, Rust, Java, and others. VisPy is de facto limited to Python applications.

* **OpenGL**: OpenGL was created in 1992, almost 30 years ago. It has been hugely popular, in particular thanks to a relatively accessible API. However, more modern, low-level graphics APIs such as Vulkan are much more powerful and allow for more efficient usage of modern graphics hardware.
