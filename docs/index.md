# Datoviz

Datoviz is a GPU rendering engine for scientific visualization, written in C/C++ and built on
Vulkan. It renders large datasets — millions of points, images, meshes — at interactive frame rates,
and supports both windowed and offscreen modes.

It is a low-level engine intended for developers who need direct control: embedding in a native
application, building a higher-level plotting library on top, or generating replayable render
streams. High-level scientific plotting belongs in VisPy2/GSP, which uses Datoviz as one of its
backends.


## Get Started

- [What is Datoviz?](start/what-is-datoviz.md) — scope, goals, and what it is not.
- [Choose your layer](start/choose-your-layer.md) — when to use the C API, DRP2, raw `ctypes`, or VisPy2/GSP.
- [Install](start/install.md) and [build from source](start/build-from-source.md).
- [First C program](start/first-c-program.md) — the recommended starting point.
- [Project status](start/project-status.md) — what is supported, experimental, or deferred in v0.4.


## Main Sections

<div class="grid cards" markdown="1">

-   **Tutorials**

    Step-by-step walkthroughs: first scene, interactive window, offscreen capture.

    [:octicons-arrow-right-24: Tutorials](tutorials/index.md)

-   **Examples**

    Runnable C examples covering the main visual types and scene configurations.

    [:octicons-arrow-right-24: Examples](examples/index.md)

-   **Reference**

    C API, feature status, platform support, DRP2, and raw bindings.

    [:octicons-arrow-right-24: Reference](reference/index.md)

-   **Explanation**

    Scene model, frame lifecycle, runtime boundary, and backend architecture.

    [:octicons-arrow-right-24: Explanation](explanation/architecture.md)

</div>
