# Datoviz v0.4 Documentation

Datoviz v0.4 is a C-first GPU rendering engine for scientific visualization. It provides a native
scene and app layer, a backend-facing DRP2 command stream, a Vulkan runtime, and raw generated
`ctypes` bindings for low-level Python integration.

Datoviz is the engine layer. Use Datoviz directly when you need a native C API, embedding,
offscreen rendering, replayable render streams, backend work, or exact control over retained visual
resources. Use VisPy2/GSP for high-level scientific plotting.


## Start Here

- [What is Datoviz?](start/what-is-datoviz.md) explains the v0.4 scope.
- [Choose your layer](start/choose-your-layer.md) separates Datoviz, raw `ctypes`, DRP2, GSP, and
  VisPy2.
- [Install](start/install.md) and [build from source](start/build-from-source.md) cover setup.
- [First C program](start/first-c-program.md) is the intended first runnable path.
- [Project status](start/project-status.md) lists supported, experimental, and deferred areas.


## Main Sections

<div class="grid cards" markdown="1">

-   **Tutorials**

    Follow the first scene, interactive window, offscreen capture, and composed workflow pages.

    [:octicons-arrow-right-24: Tutorials](tutorials/index.md)

-   **Examples**

    Browse executable release proof generated from the C example manifest.

    [:octicons-arrow-right-24: Examples](examples/index.md)

-   **Reference**

    Check feature status, platform support, C API, raw bindings, DRP2, and backend notes.

    [:octicons-arrow-right-24: Reference](reference/index.md)

-   **Explanation**

    Read the scene model, runtime boundary, frame lifecycle, and GSP/VisPy2 positioning.

    [:octicons-arrow-right-24: Explanation](explanation/architecture.md)

</div>


## Public Website Prototype

A local-only future root landing page prototype lives at [Landing](landing/index.md). It is not part
of the current public deployment plan and should stay local until v0.4 reaches a usable public
documentation state.
