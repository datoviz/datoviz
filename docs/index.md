# Datoviz v0.4

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


## Documentation Shape

The v0.4 docs are organized around runnable C examples, concise task pages, exact reference tables,
and contributor guidance. Legacy v0.3 Pythonic plotting material is not the current Datoviz v0.4
surface.

This documentation tree is being rebuilt in the v0.4 development branch. Pages marked as draft
stubs identify intended content and stable source files before the final release documentation pass.
