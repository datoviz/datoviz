# Reference

Reference pages provide exact facts: status labels, signatures, constraints, lifetimes, backend
support, and links to minimal examples.

## Project Metadata

| Page | Use it for |
| --- | --- |
| [Citation](citation.md) | Software citation guidance, v0.4 DOI placeholders, and scholarly citation status. |

## API

| Page | Use it for |
| --- | --- |
| [C API](c-api/index.md) | Generated public C reference, grouped by scene, visuals, app/runtime, and types. |
| [Python direct-engine facade](python-direct-engine.md) | C-shaped top-level Python calls with NumPy array adaptation and RGBA capture. |
| [Python raw ctypes](ctypes.md) | Low-level generated Python binding scope and usage. |
| [Visual families](visual-families/index.md) | Family status, attributes, backend support, and canonical examples. |

## Scene Contracts

| Page | Use it for |
| --- | --- |
| [Objects and lifetimes](objects-and-lifetimes.md) | Ownership, borrowed handles, frame artifacts, user data, and destroy order. |
| [Coordinate systems](coordinate-systems.md) | Data, panel, world/view, clip, framebuffer, and texture/sample spaces. |
| [Controllers](controllers.md) | Panzoom, arcball, fly, turntable, orbit, binding, linking, and invalidation. |
| [Callbacks](callbacks.md) | Callback lifetime, user data, threading assumptions, and mutation rules. |
| [Visual attributes](visual-attributes.md) | Attribute names, dense writes, sources, mutability, updates, and external buffers. |
| [Queries](queries.md) | Unified query model for picking, probing, readback, statuses, and freshness. |
| [Errors and logging](errors-and-logging.md) | Return/status behavior, diagnostic phases, DRP2 tracing, and common failure classes. |

## Status And Compatibility

| Page | Use it for |
| --- | --- |
| [Project status](project-status.md) | Meaning of status labels and broad release posture. |
| [Feature status](feature-status.md) | Feature-by-feature support, deferral, and ownership classification. |
| [v0.3 visible parity](v03-visible-parity.md) | Fixed, deferred, and external/GSP disposition for visible v0.3-era capabilities. |
| [Platform support](platform-support.md) | Native platforms, browser/WebGPU requirements, optional providers, and limitations. |
| [Build options](build-options.md) | CMake options, dependency-source policy, package smoke presets, and FetchContent. |

## Backends

| Page | Use it for |
| --- | --- |
| [DRP2](drp2/index.md) | Advanced command-stream, fixture, packet, capability, and conformance reference. |
| [WebGPU subset](webgpu-subset.md) | Experimental browser/WASM support and parity limits. |
| [Compute and graphics](compute-graphics.md) | Narrow experimental compute-to-render slice. |
