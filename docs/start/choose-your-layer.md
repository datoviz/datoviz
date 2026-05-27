# Choose Your Layer

Datoviz v0.4 deliberately separates engine, command-stream, binding, and plotting responsibilities.

| Need | Use | Status |
| --- | --- | --- |
| Native rendering engine, windows, offscreen views, capture | Datoviz C `scene` and `app` APIs | supported/experimental by feature |
| Backend-agnostic render stream or replay | DRP2/DVZR | advanced/unstable |
| Low-level Python loading of the C library | Raw generated `ctypes` | experimental |
| High-level scientific plotting | VisPy2/GSP | external/GSP |
| Old Datoviz v0.3 Pythonic plotting API | Not part of v0.4 Datoviz docs | deferred/external |


## Datoviz C

Choose the C API when your program owns rendering decisions and needs explicit control over scenes,
visual data, runtime resources, windows, offscreen targets, or capture.


## Raw `ctypes`

Choose raw `ctypes` only when you need direct access to generated C bindings from Python. Treat it
as a low-level integration and smoke-testing path, not as a high-level plotting API.


## DRP2/DVZR

Choose DRP2 when you are working on command streams, render fixture replay, WebGPU portability, or
backend adapters. DRP2 is not the first interface most application users should learn.


## VisPy2/GSP

Choose VisPy2/GSP when you want high-level scientific plotting. Datoviz v0.4 documentation does not
provide a migration guide for the old Pythonic Datoviz API.
