# Choose Your Layer

Datoviz v0.4 deliberately separates engine, command-stream, binding, and plotting responsibilities.

When using a coding agent, make the layer explicit. For normal Datoviz C visualization, ask for the
`scene` and `app` APIs and start from a current minimal example.

| Need | Use | Status |
| --- | --- | --- |
| Native rendering engine, windows, offscreen views, capture | Datoviz C `scene` and `app` APIs | supported/experimental by feature |
| Python scene and visual calls with NumPy array adaptation | `import datoviz as dvz` | supported for policy-declared calls |
| Backend-agnostic render stream or replay | DRP2/DVZR | advanced/unstable |
| Low-level Python loading of the C library | Python raw `ctypes` binding | supported |
| High-level scientific plotting | VisPy2/GSP | external/GSP |
| Old Datoviz v0.3 Pythonic plotting API | Not part of v0.4 Datoviz docs | deferred/external |


## Datoviz C

Choose the C API when your program owns rendering decisions and needs explicit control over scenes,
visual data, runtime resources, windows, offscreen targets, or capture.

This is the default Datoviz layer for generated examples that create figures, panels, visuals,
controllers, captures, or pick/probe requests.


## Python

Choose the main Python package for normal Python use with the v0.4 engine:

```python
import datoviz as dvz
```

It uses the same `dvz_*` function names as the C examples and accepts NumPy arrays for supported
calls. It is not a high-level plotting wrapper and should not be presented as a replacement for
GSP/VisPy2.


## Python Raw `ctypes`

Choose Python raw `ctypes` only when you need direct access to generated C bindings from Python.
Treat it as a low-level integration and smoke-testing path, not as a high-level plotting API.

Ask agents to preserve raw C names such as `dvz_scene()` and to follow C ownership rules. Do not ask
for Pythonic helpers unless they are documented as part of the current package.


## DRP2/DVZR

Choose DRP2 when you are working on command streams, render fixture replay, WebGPU portability, or
backend adapters. DRP2 is not the first interface most application users should learn.


## VisPy2/GSP

Choose VisPy2/GSP when you want high-level scientific plotting. Datoviz v0.4 documentation does not
provide a migration guide for the old Pythonic Datoviz API.
