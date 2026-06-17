# Why Datoviz?

Datoviz v0.4 is a C-first rendering engine for scientific visualization. It is not a plotting
library, a notebook widget toolkit, or a compatibility layer for the old Datoviz v0.3 Python API.
The purpose of the v0.4 rewrite is to make the rendering core explicit enough to validate,
embed, replay, port, and harden.

## Engine First

The engine is designed around retained scene state. Users create figures, panels, visuals,
controllers, adornments, and resources. Datoviz keeps that state, validates it, and lowers it into
frame plans and backend-agnostic DRP2 command streams. The runtime then executes those streams
through the native Vulkan-backed path or, for the experimental browser subset, through WebGPU.

This separation gives Datoviz three jobs:

- provide a predictable native C API for scenes, app views, offscreen rendering, capture, and
  embedding;
- keep GPU work inspectable through frame plans, DRP2 streams, fixtures, and replay-oriented
  validation;
- act as a rendering backend that higher-level scientific plotting layers can target.

## Deliberate Boundaries

Datoviz v0.4 deliberately does less than a full plotting environment. It does not own high-level
`plot()`, `scatter()`, or `imshow()` APIs, declarative chart grammars, notebook layout systems,
publication-oriented vector export, or Python-first object models. Those responsibilities belong to
GSP/VisPy2 or other libraries above Datoviz.

## Documentation Consequence

That boundary matters for the documentation. Datoviz pages should teach the engine, the retained
scene model, visual data contracts, runtime ownership, status labels, and low-level binding scope.
They should not promise migration compatibility with the v0.3 Python plotting API.

See also:

- [GSP and VisPy2 boundary](gsp-vispy2-boundary.md)
- [Project status](../reference/project-status.md)
- [Choose your layer](../start/choose-your-layer.md)
