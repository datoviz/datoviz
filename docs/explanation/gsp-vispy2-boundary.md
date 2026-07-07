# Datoviz, GSP, and VisPy2

Datoviz v0.4 owns the rendering engine. GSP/VisPy2 owns high-level scientific plotting.

## Boundary

This boundary is intentional. Datoviz should provide a predictable C engine, retained scene model,
visual families, runtime execution, offscreen capture, embedding hooks, generated Python binding,
DRP2 streams, fixtures, and portability experiments. It should not grow a parallel
high-level Python plotting API inside the v0.4 docs.

## When to Use Datoviz

Use Datoviz directly when you need engine control: C applications, native embedding, explicit
visual data, retained resources, low-level Python smoke tests, backend validation, WebGPU/WASM
experiments, or contributor work on rendering internals.

## When to Use GSP or VisPy2

Use GSP/VisPy2 when you want plotting objects, automatic scales, declarative chart-like workflows,
notebook-first ergonomics, rich Python object models, or migration from old high-level Python
plotting patterns.

## Documentation Rule

This does not make Datoviz less user-facing. It means the user-facing Datoviz surface is the engine
surface: examples, visual families, scenes, panels, controllers, capture, diagnostics, and exact
status labels. Higher-level libraries can build friendlier plotting APIs on top without forcing the
engine documentation to promise behavior it does not own.

When writing docs or examples, make the layer explicit. If a page describes C scene/app use,
Datoviz owns it. If a page describes `datoviz.raw`, it is documenting the exact pointer/count call
form of the same Python binding. If a page describes high-level plotting convenience, it belongs
outside the Datoviz v0.4 public docs unless it is explicitly marked as GSP/VisPy2 scope.

See also:

- [Why Datoviz?](why-datoviz.md)
- [Choose your layer](../start/choose-your-layer.md)
- [Feature status](../reference/feature-status.md)
