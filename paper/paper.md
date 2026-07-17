---
title: "Datoviz v0.4: a C-first GPU rendering engine for scientific visualization"
tags:
  - scientific visualization
  - GPU rendering
  - Vulkan
  - WebGPU
  - Python
  - C
authors:
  - name: Cyrille Rossant
    orcid: 0000-0003-2069-9093
    affiliation: "1"
affiliations:
  - index: 1
    name: International Brain Laboratory
date: 2 July 2026
bibliography: paper.bib
---

# Summary

Datoviz is an open-source rendering engine for interactive scientific visualization. It is designed
for applications and higher-level libraries that need to display large two- and three-dimensional
datasets with direct control over GPU resources, windows, panels, visual objects, interaction, and
image capture. Version 0.4 is a substantial rewrite that makes Datoviz a C-first engine rather than
a high-level Python plotting package. Its active path is a retained scene API that produces frame
plans, lowers them to DRP2 command streams, and executes them through a native Vulkan/vklite
runtime. The release also provides generated low-level Python `ctypes` bindings, an array-aware
Python facade for direct engine calls, and an experimental WebGPU/WASM backend for browser-hosted
examples. Datoviz is meant to be embedded in scientific tools, used by backend authors, and targeted
by higher-level systems such as the Graphics Server Protocol and future VisPy2-style plotting
layers.

The v0.4 surface includes retained visual families for points, pixels, markers, lines, paths,
images, text, labels, meshes, spheres, and volumes, together with panels, controllers, axes,
colorbars, scale bars, readback, picking, and offscreen capture. The goal is not to maximize the
number of plot types, but to provide a compact engine whose rendering contracts can be reused across
native applications, Python integration, generated examples, and browser demonstrations.

# Statement of need

Many scientific datasets are too large, dynamic, or interactive for static figure-generation
workflows. Neuroscience, microscopy, physics simulation, geospatial analysis, and computational
modeling often require dense point clouds, meshes, image stacks, labels, axes, annotations, linked
views, and camera control while preserving a path to reproducible raster captures. Researchers can
use general-purpose plotting libraries for many publication figures, but backend authors and tool
builders also need a lower layer that exposes predictable rendering semantics, efficient data
updates, embedding hooks, and explicit GPU ownership.

Datoviz addresses this engine-layer need. It is aimed at scientific software developers who build
interactive applications, domain-specific viewers, or reusable plotting backends, rather than at end
users who want a one-line plotting command. Version 0.4 intentionally separates this low-level role
from high-level plotting: Datoviz owns the C engine, native scene/app path, raw/generated Python
binding surface, raster capture, experimental browser path, and a narrow compute-to-render proof.
Higher-level object-oriented plotting APIs are expected to live in projects above Datoviz.

This separation is important for research software maintenance. A backend engine must expose enough
detail for advanced users to reason about memory ownership, framebuffer size, readback freshness,
callback lifetimes, and platform support, while still providing a stable enough scene abstraction
for downstream libraries. Datoviz v0.4 therefore treats API status, backend parity, examples, and
release validation as part of the software surface rather than as auxiliary documentation.

The project builds on earlier Datoviz releases and on the motivation described by Rossant et al.
[-@rossant2021datoviz], but v0.4 changes the architecture enough that it should be treated as a new
software release. The rewrite keeps the core goal of high-performance scientific rendering while
making the supported surface more explicit, testable, and suitable for downstream backend work.

# State of the field

Scientific visualization tools cover a broad range of abstraction levels. Matplotlib is a standard
for publication-oriented static and interactive figures [@hunter2007matplotlib]. VisPy exposes
GPU-accelerated visualization in Python and influenced Datoviz's original direction [@vispy].
napari provides a successful domain-oriented viewer for multidimensional image data [@napari]. VTK
and ParaView provide broad visualization pipelines, especially for meshes, volumes, and simulation
data [@vtk; @paraview]. Vulkan offers a low-overhead native graphics API [@vulkan], while WebGPU is
standardizing modern GPU access for browsers and portable applications [@webgpu].

Datoviz does not try to replace these tools at their own level. Its build-vs-contribute
justification is architectural: the project needs a small, embeddable, C ABI-oriented engine with a
single retained scene contract that can drive native rendering, low-level Python bindings, capture
paths, and experimental browser execution. Contributing this role to a high-level plotting package
would mix user-facing plotting concerns with backend and runtime ownership concerns. Conversely,
using a large visualization framework as the only substrate would make it harder to preserve the
explicit C-first API, command-stream boundary, lightweight Python binding policy, and v0.4 release
discipline needed by Datoviz and its downstream integrations. Datoviz therefore occupies a narrow
middle layer: lower than plotting libraries, more domain-specific than a raw graphics API, and
focused on reusable scientific visualization primitives.

# Software design

The central v0.4 design choice is to keep rendering semantics above the graphics backend but below
high-level plotting. User code creates scenes, figures, panels, controllers, and visuals. Scene
emission produces frame artifacts and frame plans. Those plans lower to DRP2 packet and command
stream snapshots, which are consumed by the native vklite/canvas/stream runtime and, for a declared
subset, by the WebGPU/WASM runner. This separation makes scene behavior testable without forcing
every semantic check through a live window, and it gives the browser backend a shared contract
instead of a JavaScript reimplementation of examples.

The C ABI is the stable center of the engine. Public headers define supported scene, visual, app,
runtime, and FFI helper surfaces, while internal implementation remains in modular subsystems. The
Python story follows that boundary: `datoviz.raw` is generated from the exported C ABI and keeps
exact `ctypes` signatures, while the top-level Python facade keeps C-shaped names but adapts
policy-declared NumPy array arguments. This avoids making a v0.3-compatible plotting API the
compatibility constraint for v0.4.

The design trades some convenience for explicitness. Vulkan ownership rules, borrowed handles,
frame artifacts, readback lifetimes, and backend support status are documented rather than hidden
behind a large object model. Rendering, presentation, frame streams, and browser execution share the
same scene-to-command-stream boundary instead of separate renderer contracts. WebGPU support is
classified as an experimental subset with shared scene semantics and capability diagnostics.
Compute support is similarly limited to a narrow compute-to-render proof instead of a general
compute framework. These constraints reduce accidental parallel runtimes and make the release
surface easier to validate.

Examples are part of the design rather than an afterthought. Public examples are organized by
visual family, feature, runtime behavior, and showcase, and many browser routes reuse the same
canonical C scenario as native validation. This keeps documentation, gallery pages, smoke tests, and
release evidence aligned with the actual engine behavior.

# Research impact statement

Datoviz has already been described in the scientific literature as a high-performance GPU
visualization library [@rossant2021datoviz]. The v0.4 release turns that earlier prototype-era
direction into a more explicit engine for downstream scientific visualization work. It is developed
in the context of the International Brain Laboratory, where interactive inspection of large
neuroscience datasets is a recurring software need, and it is positioned as a low-level backend
target for GSP/VisPy2-style visualization layers.

The release is designed for reproducible evaluation. The repository includes focused tests,
generated C and Python binding checks, native examples, WebGPU fixture and browser smoke tests,
wheel build and installed-consumer checks, release status tables, and gallery examples that double
as validation artifacts. During v0.4 release-candidate validation, the project records
cross-platform wheel and installed-consumer evidence for Linux, macOS, Windows, and Python 3.10
through 3.14, along with native CMake/pkg-config consumer checks. These materials give reviewers
concrete ways to inspect the engine, reproduce examples, and assess the claimed release surface.

The project also has a clear adoption path: Datoviz can be cited directly as the rendering engine
used by a scientific application, or it can be cited as the backend layer beneath a higher-level
plotting or domain-specific tool. The accompanying Zenodo archive for v0.4 provides the
version-specific software record, while this paper describes the research-software contribution and
design rationale.

# AI usage disclosure

[OpenAI Codex](https://openai.com/codex/) assistance was used during the Datoviz v0.4 rewrite and
release process, including implementation, refactoring, test development, debugging,
documentation, release validation, citation metadata, release-checklist text, and drafts of this
paper. Human authors directed and reviewed this work and are responsible for the software design,
implementation decisions, citations, claims, and submitted text. AI-assisted content was reviewed,
edited, tested where applicable, and validated against the repository state and cited sources.

# Acknowledgements

Datoviz development has been supported by the International Brain Laboratory, the Wellcome Trust,
the Simons Foundation, and the Chan Zuckerberg Initiative. The development and release of Datoviz
v0.4 received substantial AI-assisted engineering support from
[OpenAI Codex](https://openai.com/codex/).

# References
