# GSP, VisPy2, and Datoviz Backend Strategy

Status: strategic boundary contract for v0.4+ planning.

This document records how Datoviz should relate to the future pure-Python VisPy2 stack and to GSP,
the Graphics Specification Protocol. It complements
[`PYTHON_GSP_SCOPE.md`](PYTHON_GSP_SCOPE.md), which remains the normative v0.4 Python-scope note.


## North Star

Use one semantic scientific-visualization description with multiple renderers:

```text
VisPy2
  Python user experience: plot APIs, figures, dashboards, notebooks, interactions

GSP
  Backend-independent semantic representation: visuals, data bindings, transforms, styles,
  cameras, axes, selections, annotations, and export intent

Backends
  Datoviz      fast interactive GPU rendering, advanced native rendering, raster/video output
  Matplotlib   publication-oriented static/vector output
  Future       WebGPU/browser, remote, headless, domain-specific, or experimental renderers
```

Datoviz should be a first-class GSP backend, not the owner of the Python object model. Matplotlib
should be a peer backend, not merely a weak fallback.


## Layer Ownership

### Datoviz

Datoviz owns the native rendering engine and direct low-level API:

- C API, retained scene/app objects, DRP2 emission, and runtime execution;
- Vulkan, WebGPU/WASM, headless, offscreen, capture, and native presentation paths;
- high-performance visual resources, partial updates, picking/probing, and telemetry;
- advanced renderer features that may not be portable to every GSP backend;
- raw generated Python bindings that mirror the C API for integration and smoke tests.

Direct Datoviz use remains appropriate when users need maximum performance, experimental renderer
features, custom command streams, native embedding, or precise control over GPU behavior.

### GSP

GSP owns the backend-independent representation:

- figures, panels, cameras, axes, transforms, scales, visual state, style, and layout intent;
- data-buffer references and update semantics that can be lowered by different renderers;
- portable picking/selection/probe requests and result vocabulary;
- capability queries and user-facing warnings for unsupported, approximate, slow, or rasterized
  features.

GSP should preserve scientific semantics rather than exposing Datoviz draw calls as its primary
model.

### VisPy2

VisPy2 owns the Python user experience above GSP:

- task-first APIs such as scatter, line, image, mesh, volume, and dashboard helpers;
- object-oriented Python figure, panel, visual, camera, interaction, and animation APIs;
- notebook, Qt, napari, data-science, and application integration;
- backend selection and fallback policy.

Python examples that teach rich data workflows, plotting ergonomics, or notebook UX should live in
VisPy2/GSP, not in the Datoviz repository.

### Matplotlib

The Matplotlib backend owns the publication-oriented path:

- PDF/SVG/vector export;
- publication typography, labels, axes, legends, colorbars, and layout;
- static 2D figures and best-effort static 3D scenes;
- mixed vector/raster output when a Datoviz-class visual cannot be represented structurally.

Datoviz should remain raster-first for screenshots, high-DPI output, video, and interactive capture.


## Core GSP Versus Backend Extensions

Do not force every Datoviz capability into the portable GSP core. Use three levels.

| Level | Meaning | Examples |
|---|---|---|
| GSP core | Expected to be representable by several backends | points, paths, images, meshes, axes, colormaps, labels, cameras |
| GSP capability | Portable semantic request with backend-specific quality/performance | volumes, 3D cameras, picking, transparency, large data updates |
| Backend extension | Explicitly backend-specific advanced feature | Datoviz PBR, ray tracing, EDL, SSAO, custom shaders, out-of-core residency |

Backend extensions should be discoverable and degrade cleanly. A scene should be able to ask whether
a backend supports a feature before using it.

Illustrative, non-binding Python shape:

```python
fig = vispy2.figure()
ax = fig.subplot(projection="3d")
mesh = ax.mesh(vertices, faces)

if fig.backend.supports("datoviz.material.pbr"):
    mesh.material = {"model": "pbr", "roughness": 0.35, "metallic": 0.0}
else:
    mesh.material = {"model": "matte"}

fig.show(backend="datoviz")
fig.savefig("figure.pdf", backend="matplotlib")
fig.savefig("figure.png", backend="datoviz", scale=4)
```


## Capability States

GSP should classify support per feature instead of with one global yes/no flag.

| State | Meaning |
|---|---|
| `supported` | The backend implements the feature directly. |
| `approximate` | The backend can render a useful but not identical representation. |
| `slow` | The backend can render it but users should expect poor performance. |
| `rasterized` | The feature can appear only as raster content in vector/static output. |
| `unsupported` | The backend cannot implement the feature. |

Useful capability groups include geometry, style/material, lighting, transparency, volume, large-data
updates, interactivity, picking/probing, layout, export, text, and color management.


## Direct Datoviz Escape Hatch

The GSP path should cover common scientific plotting and scene composition. The direct Datoviz path
should stay documented and supported for:

1. renderer capability proof and native examples;
2. advanced material, lighting, transparency, ray-tracing, and compute experiments;
3. high-throughput streaming, partial updates, and benchmark-sensitive loops;
4. C/C++ applications and language bindings that do not want the VisPy2 stack;
5. backend development, fixture generation, and DRP2/runtime validation.

This split avoids two bad outcomes: Datoviz becoming a large Python plotting framework, and GSP
becoming a lowest-common-denominator API that hides Datoviz's strongest features.


## Documentation Consequences

- Datoviz docs should be C-first and renderer-focused.
- Datoviz may include a small bridge page for using Datoviz as a GSP backend once the adapter exists.
- VisPy2/GSP should host Python plotting tutorials, notebooks, backend comparisons, and high-level
  interaction examples.
- Static/vector export examples should prefer the GSP/Matplotlib route.
- Advanced Datoviz-only examples should be explicit that they use renderer extensions.


## Open Questions

- Should GSP be packaged inside VisPy2 or as an independently versioned package?
- What is the first minimal Datoviz capability report required by GSP?
- Which advanced Datoviz features deserve named GSP extension namespaces?
- How much interaction state belongs in GSP versus VisPy2 tools that mutate GSP state?
- Should Datoviz expose standalone controller/camera components specifically for VisPy2 reuse?
