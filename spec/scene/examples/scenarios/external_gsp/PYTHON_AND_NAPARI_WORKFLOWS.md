# External GSP And Python Workflow Scenarios

> **Example status:** external/GSP primary
> **Target:** GSP/VisPy2/Matplotlib user-facing examples, with Datoviz C fixtures underneath
> **Data:** Python ecosystem loaders and prepared caches
> **Validation:** external smoke where practical, Datoviz renderer fixtures for low-level coverage

Datoviz v0.4 remains C-first for native renderer/runtime proof. Raw Python examples in this
repository should stay thin and close to generated bindings. Pythonic plotting, notebook,
dashboard, napari, and publication workflows belong primarily in GSP/VisPy2 or Matplotlib-backed
layers.


## Own In Datoviz

1. native C scene/app examples;
2. renderer and visual fixtures;
3. deterministic screenshot/readback examples;
4. DRP2, DVZR, WebGPU, and hosted-runtime fixtures;
5. minimal raw `ctypes` smoke examples.


## Own Above Datoviz

1. high-level scatter/line/image/volume plotting;
2. notebook and dashboard UX;
3. napari application integration;
4. rich data loaders and preprocessing galleries;
5. publication-quality static/vector output through GSP/Matplotlib.


## Mirror Only When The Layer Teaches Something Different

Do not duplicate a scenario just to show syntax. Mirror selected Datoviz C scenarios into GSP or
plot examples only when the higher-level layer teaches a genuinely different workflow or validates
a different contract.
