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
datasets with direct control over GPU resources, windows, panels, views, visual objects, and image
capture. The v0.4 release is a substantial rewrite that makes Datoviz a C-first engine rather than a
high-level Python plotting package. Its active path is a retained scene API that produces frame
plans, lowers them to DRP2 command streams, and executes them through the native Vulkan/vklite
runtime. The release also provides generated low-level Python `ctypes` bindings, a top-level
array-aware Python facade for direct engine calls, and an experimental WebGPU/WASM backend for
browser-hosted examples. Datoviz is meant to be embedded in scientific tools, used by backend
authors, and targeted by higher-level systems such as the Graphics Server Protocol and future
VisPy2-style plotting layers.

# Statement of need

Many scientific datasets are too large, dynamic, or interactive for static figure-generation
workflows. Neuroscience, microscopy, physics simulation, geospatial analysis, and computational
modeling often require dense point clouds, meshes, image stacks, labels, axes, annotations, and
interactive camera control while preserving a path to reproducible captures. Researchers can use
general-purpose plotting libraries for many publication figures, but backend authors and tool
builders also need a lower layer that exposes predictable rendering semantics, efficient data
updates, embedding hooks, and explicit GPU ownership.

Datoviz addresses this engine-layer need. It is aimed at scientific software developers who build
interactive applications, domain-specific viewers, or reusable plotting backends, rather than at end
users who want a one-line plotting command. Version 0.4 intentionally separates this low-level role
from high-level plotting: Datoviz owns the C engine, native scene/app path, raw/generated Python
binding surface, raster capture, experimental browser path, and narrow compute-to-render proof;
higher-level object-oriented plotting APIs are expected to live in projects above Datoviz.

The project builds on earlier Datoviz releases and the motivation described by Rossant et al.
[-@rossant2021datoviz], but v0.4 changes the architecture enough that it should be cited as a new
software release. The rewrite keeps the core goal of high-performance scientific rendering while
making the supported surface more explicit, testable, and suitable for downstream backend work.

# State of the field

The Python scientific visualization ecosystem contains mature tools at several levels. Matplotlib
is a standard for publication-oriented static and interactive figures [@hunter2007matplotlib].
VisPy exposes GPU-accelerated visualization in Python and influenced Datoviz's original direction
[@vispy]. napari provides a highly successful domain-oriented viewer for multidimensional image data
[@napari]. VTK and ParaView provide broad visualization pipelines, especially for meshes, volumes,
and simulation data [@vtk]. Browser-focused systems and emerging WebGPU libraries are improving
portable visualization, while Vulkan offers a low-overhead native graphics API [@vulkan].

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
instead of a reimplementation of examples in JavaScript.

The C ABI is the stable center of the engine. Public headers define supported scene, visual, app,
runtime, and FFI helper surfaces, while internal implementation remains in modular subsystems. The
Python story follows that boundary: `datoviz.raw` is generated from the exported C ABI and keeps
exact `ctypes` signatures, while the top-level Python facade keeps C-shaped names but adapts
policy-declared NumPy array arguments. This avoids making a v0.3-compatible plotting API the
compatibility constraint for v0.4.

The design trades convenience for explicitness. Vulkan ownership rules, borrowed handles, frame
artifacts, readback lifetimes, and backend support status are documented rather than hidden behind a
large object model. WebGPU support is deliberately classified as an experimental subset with shared
scene semantics and capability diagnostics. Compute support is similarly limited to a narrow
compute-to-render proof instead of a general compute framework. These constraints reduce accidental
parallel runtimes and make the release surface easier to validate.

# Research impact statement

Datoviz has already been described as a high-performance GPU scientific visualization library in
Computing in Science & Engineering [@rossant2021datoviz]. The v0.4 release turns that earlier
prototype-era direction into a more explicit engine for downstream scientific visualization work.
It is developed in the context of the International Brain Laboratory, where interactive inspection
of large neuroscience datasets is a recurring software need, and it is positioned as a low-level
backend target for GSP/VisPy2-style visualization layers.

The release is also designed for reproducible evaluation. The repository includes focused tests,
generated C and Python binding checks, native examples, WebGPU fixture and browser smoke tests,
wheel build and installed-consumer checks, release status tables, and gallery examples that double
as validation artifacts. TODO: before submission, replace this sentence with concrete release
evidence from the final v0.4 validation record, including the Zenodo archive DOI and any benchmark
or adoption data that should be part of the review.

# AI usage disclosure

TODO: finalize before submission. OpenAI Codex/GPT-5 assistance was used to draft citation
metadata, release-checklist text, and this initial paper draft. Human authors remain responsible for
the software design, implementation decisions, citations, claims, and final submitted text; all
AI-assisted content must be reviewed, edited, and validated before submission.

# Acknowledgements

Datoviz development has been supported by the International Brain Laboratory, the Wellcome Trust,
the Simons Foundation, and the Chan Zuckerberg Initiative. TODO: add exact grant identifiers,
additional contributors, and any JOSS-relevant acknowledgements before submission.

# References
