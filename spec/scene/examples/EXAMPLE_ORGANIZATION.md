# Datoviz and vispy2 Example Suite Organization

> **Status:** Planning
> **Scope:** Datoviz v0.4 C examples, Datoviz raw Python examples, vispy2 GSP examples,
> vispy2 plot examples, fixtures, showcases, regression examples, and stress examples.
> **Goal:** keep examples discoverable by purpose and API layer while sharing scenario IDs across
> repositories.

This document describes how to organize the v0.4 example portfolio across the Datoviz and vispy2
repositories. It complements the compact one-feature inventory in
[FEATURE_FIXTURE_MATRIX.md](FEATURE_FIXTURE_MATRIX.md): that matrix lists fixture ideas, while this
file defines where each class of example should live and which API layer should own it.

The guiding rule is to split examples by **purpose first**, then by **API layer**. A runnable example
should make it clear whether it is teaching the native C API, validating a backend/runtime contract,
showing raw Python access to Datoviz, demonstrating the object-oriented GSP interface, or presenting a
high-level plotting workflow.


## Goals

1. Provide many examples for Datoviz v0.4 without turning the tree into an undifferentiated gallery.
2. Keep C examples as the canonical engine and native API examples.
3. Keep raw Python examples in the Datoviz repository, under `examples/python/`, as thin examples of
   the Datoviz Python surface.
4. Keep GSP and plot examples in the vispy2 repository, where they can use Python-native object
   models, NumPy-oriented workflows, notebooks, and ecosystem integrations.
5. Separate fixtures, documentation examples, showcases, offscreen regression examples, and stress
   tests so that each class has the right data policy and validation policy.
6. Share stable scenario IDs across repositories so the same idea can be implemented in C, raw
   Python, GSP, and plot layers without losing traceability.


## Repository Ownership

### Datoviz repository

The Datoviz repository should own examples that document, validate, or stress the C library and its
runtime layers:

1. C examples for the public native API.
2. Raw Python examples under `examples/python/` for the Datoviz Python surface.
3. Engine fixtures for scene, app, DRP2, vklite, canvas, stream, video, and interop paths.
4. Deterministic offscreen and golden-output examples used for regression feedback.
5. Stress and performance examples that measure engine behavior close to the native layer.
6. A small number of native C showcases that demonstrate engine capability or embedding patterns.
7. Generated DRP2, DVZR, and WebGPU-compatible fixture streams.

The Datoviz examples should not try to be the main Python user-facing gallery once GSP and the plot
interface are active. Raw Python examples in this repository should remain intentionally close to the
Datoviz API and should be useful for binding conformance, quick smoke tests, and advanced users who
need direct access to the native layer.

### vispy2 repository

The vispy2 repository should own examples that present the Python user experience:

1. GSP object-oriented scene examples.
2. vispy2 plot-interface examples.
3. Python-native showcase and gallery examples.
4. Jupyter, napari, Qt, dashboard, and ecosystem integration examples.
5. User-facing scientific workflows that depend on NumPy, pandas, xarray, SciPy, notebooks, or richer
   Python data loading.
6. Plot-level stress examples for large scatter, streaming time series, dense images, dashboards, and
   callback-heavy workflows.

GSP examples should feel like Python scene programming, not translations of C examples. Plot examples
should be task-oriented and hide scene/rendering details whenever possible.


## API Layers

### C

C examples are the canonical Datoviz engine examples. They should document the native API and exercise
the scene -> DRP2 -> runtime path directly.

C should own:

1. one visual-family example per active visual,
2. one feature or technique example per core capability,
3. low-level DRP2, vklite, canvas, stream, video, and interop examples,
4. deterministic offscreen fixtures and golden-output examples,
5. stress tests that measure native runtime behavior,
6. native showcases that demonstrate engine capabilities or embedding patterns.

### Raw Python in Datoviz

Raw Python examples live directly under `examples/python/` in the Datoviz repository. The path should
not include an extra `ctypes/` component: the fact that the implementation may be ctypes-backed is a
binding detail, not the user-facing organization.

Raw Python examples should be thin and explicit. They should prove that the Datoviz Python surface can
load the shared library, create scenes, upload simple data, render offscreen or in a bounded window,
and exercise a small set of interaction/readback paths.

Raw Python should own:

1. minimal fundamentals such as hello scene, offscreen capture, and bounded window,
2. parity smoke examples for points, images, volume slices, and meshes,
3. smoke examples for picking, image probing, partial updates, resize, and capture,
4. small regression scripts that compare behavior against C fixtures when useful.

Raw Python should not become the main high-level Python plotting or object-oriented scene API. If an
example wants rich Python object ownership, context managers, plotting conveniences, notebooks, or
NumPy-first user workflows, it belongs in vispy2.

### GSP

GSP examples live in the vispy2 repository and demonstrate the object-oriented scene interface. They
should mirror key Datoviz C scenarios where useful, but they should read as idiomatic Python.

GSP should own:

1. object-oriented scene construction,
2. retained visual composition,
3. Python callbacks and selection models,
4. linked scene state,
5. Pythonic update and animation patterns,
6. user-defined transforms and richer object configuration,
7. showcase examples that use the scene layer directly.

### Plot

Plot examples live in the vispy2 repository and demonstrate the highest-level user interface. They
should be task-first rather than visual-family-first.

Plot should own:

1. scatter, line, image, volume, surface, and mesh examples,
2. linked plotting views and dashboards,
3. streaming time-series and dense image workflows,
4. colorbars, scales, annotations, labels, and selections exposed as plot features,
5. gallery examples that prioritize concise user code over engine details.


## Example Lanes

### Fundamentals

Fundamentals teach how to create, render, update, and close a scene or application. They should use
small synthetic data and have short source files.

Examples:

1. hello scene,
2. offscreen capture,
3. bounded GLFW or hosted window,
4. resize,
5. frame callback,
6. animation,
7. multi-panel layout,
8. linked panzoom.

### Visuals

Visual examples demonstrate one visual family at a time. These are documentation examples and smoke
fixtures, not showcases. Datoviz C should have one example per active visual. Raw Python and GSP should
mirror the most important user-facing visuals.

Core visual scenarios:

1. point,
2. pixel,
3. marker,
4. primitive,
5. segment,
6. path,
7. mesh,
8. image,
9. volume slice,
10. volume rendering,
11. sphere impostor,
12. text and annotation once rendered text is active.

### Features

Feature examples demonstrate reusable scene/app capabilities such as controllers, fields, picking,
probing, color mapping, partial updates, capture, recording, and export.

Examples:

1. visual data upload,
2. partial range update,
3. mutability hints,
4. visibility,
5. depth test,
6. panzoom,
7. arcball,
8. fly camera,
9. turntable,
10. sampled fields,
11. scalar colormap,
12. categorical colormap,
13. colorbar,
14. point picking,
15. image probing,
16. request coalescing,
17. selection,
18. link channel,
19. pinned readout,
20. video export,
21. DVZR record/replay.

### Techniques

Technique examples document rendering techniques and pass-level behavior. They should remain close to
engine capabilities and usually belong in Datoviz C first, then GSP when exposed through the Python
scene interface.

Examples:

1. alpha blending,
2. WBOIT,
3. depth peeling,
4. depth cue,
5. MSAA,
6. Eye-Dome Lighting,
7. SSAO,
8. volume occlusion,
9. scene occlusion,
10. alpha mask once active,
11. object-id G-buffer once active,
12. selected outline once active.

### Advanced and low-level

Advanced examples target power users and backend developers. They should be separate from scene-level
examples so new users are not exposed to low-level details too early.

Examples:

1. DRP2 hello stream,
2. DRP2 triangle,
3. DRP2 validation,
4. DRP2 runtime execution,
5. borrowed canvas frame,
6. DRP2 readback,
7. external buffer,
8. labels and debug traces,
9. fixture JSON generation,
10. raw DVZR recording,
11. scene DVZR recording and replay,
12. vklite buffer, image, sampler, shader, command, barrier, sync, descriptor, and pipeline examples,
13. canvas draw callback, offscreen capture, present, immediate FPS, video sink, and live-image sink,
14. stream custom sink, registry, multi-sink routing, and borrowed frame examples,
15. window and input examples,
16. platform interop examples.

### Showcases

Showcases combine several features into inspiring examples. They may use larger public datasets,
preprocessing scripts, download caches, richer interactions, and manual validation. They should not be
used as the only regression coverage for core features.

Datoviz C showcases should emphasize native performance, engine capabilities, and embedding patterns.
vispy2 GSP and plot showcases should emphasize Python workflows and concise user-facing APIs.

Example themes:

1. LIDAR point-cloud EDL viewer,
2. protein arcball viewer,
3. brain atlas explorer,
4. spatial omics point cloud,
5. dense labels,
6. Earth or Mars globe,
7. global wind field,
8. DICOM viewer,
9. market microstructure dashboard,
10. streaming DAQ viewer,
11. PD12M image embedding LOD explorer,
12. Wikipedia semantic embedding atlas,
13. diffusion tractography,
14. particle or CFD simulation.

### Regression and golden-output examples

Regression examples should be deterministic and runnable without a user. They may share source with
documentation examples, but the validation harness and expected artifacts should be separate.

Preferred outputs:

1. PNG with region-based comparison,
2. JSON stream or scene description,
3. DVZR recording,
4. stdout assertions,
5. MP4 or raw video only when testing video paths.

Regression examples should use tiny synthetic data when possible and declare tolerances explicitly.

### Stress and benchmark examples

Stress examples measure scale, churn, and performance limits. They should be easy to run locally and
in nightly jobs, but they should not be part of normal smoke CI unless they are bounded and cheap.

Examples:

1. many points,
2. many panels,
3. repeated partial updates,
4. resize churn,
5. many transparent layers,
6. volume streaming,
7. DVZR record/replay throughput,
8. immediate-present FPS,
9. dashboard callback throughput.

Stress examples should emit machine-readable metrics such as frame count, median frame time, p95 frame
time, upload count, pipeline recreation count, descriptor recreation count, and approximate memory use.


## Proposed Datoviz Layout

A target Datoviz examples layout could be:

```text
examples/
  README.md
  manifests/
    examples.yaml
    golden.yaml
    data.yaml

  c/
    CMakeLists.txt
    fundamentals/
    visuals/
    features/
    techniques/
    advanced/
    showcase/
    stress/

  python/
    fundamentals/
    visuals/
    features/
    techniques/
    regression/
    showcase/
    stress/

  webgpu/
  qt/
  benchmarks/
```

The current `examples/c/tools/` group can be treated as the predecessor of `examples/c/advanced/`.
It does not need to be renamed immediately, but new planning documents should prefer the `advanced`
term for direct DRP2, vklite, canvas, stream, WebGPU, DVZR, and interop examples.


## Proposed vispy2 Layout

A target vispy2 examples layout could be:

```text
examples/
  README.md
  manifests/
    examples.yaml

  gsp/
    fundamentals/
    visuals/
    features/
    techniques/
    showcase/
    stress/

  plot/
    basics/
    scientific/
    dashboards/
    gallery/
    stress/

  interop/
    napari/
    jupyter/
    qt/
```

The vispy2 repository may copy the relevant parts of this plan into its own examples README, but the
Datoviz repository should keep this cross-repo planning note as the engine-side source of truth until
the two repositories have matching manifests.


## Scenario IDs

Each conceptual example should have a stable scenario ID independent of language or repository.
Scenario IDs make it possible to connect C, raw Python, GSP, and plot examples without forcing every
API layer to implement every scenario.

Example IDs:

```text
fundamental.hello_scene
fundamental.offscreen_capture
visual.point.basic
visual.image.checkerboard
visual.volume.slice
visual.mesh.arcball
feature.pick.point
feature.probe.image
feature.partial_update
feature.linked_panels
technique.alpha_blend.layers
technique.wboit.layers
technique.depth_peel.layers
technique.edl.points
technique.ssao.spheres
showcase.lidar.edl
showcase.protein.arcball
showcase.embedding.image_lod
showcase.embedding.semantic_atlas
stress.points.many
stress.partial_update_churn
```

A scenario can have several implementations:

```yaml
id: visual.point.basic
title: Basic point visual
category: visual
implementations:
  datoviz.c: examples/c/visuals/point.c
  datoviz.python: examples/python/visuals/point.py
  vispy2.gsp: examples/gsp/visuals/points.py
  vispy2.plot: examples/plot/basics/scatter.py
outputs:
  datoviz.c: png
  datoviz.python: png
  vispy2.gsp: png-or-live
  vispy2.plot: live-or-notebook
```

Not every scenario needs every implementation. Missing entries should be explicit when the omission is
intentional.


## Metadata Manifests

The runnable examples should eventually be indexed by metadata rather than by hand-maintained lists.
The Datoviz repository can start with:

```text
examples/manifests/examples.yaml
examples/manifests/golden.yaml
examples/manifests/data.yaml
```

A manifest entry should record:

1. scenario ID,
2. title,
3. repository,
4. API layer,
5. source path,
6. category and lane,
7. maturity status,
8. output classes,
9. data policy,
10. requirements and backend gates,
11. smoke command,
12. validation command or golden-output comparator,
13. related implementations in other API layers.

Example:

```yaml
id: technique.wboit.layers
title: WBOIT transparent layers
repo: datoviz
api: c
path: examples/c/techniques/wboit.c
lane: techniques
status: smoke
outputs: [png, live]
requirements: [scene, app, vklite]
data: synthetic
related:
  - vispy2.gsp:technique.wboit.layers
```


## Maturity Levels

Use explicit maturity levels so users and CI know what to expect:

| Level | Meaning | Expected validation |
| ----- | ------- | ------------------- |
| `spec` | Markdown idea only | no runtime validation |
| `draft` | source exists but API may change | optional smoke |
| `smoke` | builds and runs with minimal validation | smoke command |
| `golden` | deterministic output compared to baseline | golden/image/JSON comparison |
| `interactive` | requires a user or live window | bounded/manual checklist |
| `showcase` | polished demo, may need external data | manual or nightly smoke |
| `stress` | performance or scale target | metrics, manual, or nightly |


## Data Policy

Use four data classes:

| Data class | Use | Examples |
| ---------- | --- | -------- |
| `inline` | fixtures, docs, CI | three points, one triangle, 4x4 image |
| `synthetic` | fixtures, docs, stress | seeded point cloud, procedural volume |
| `small-bundled` | docs and small showcases | tiny mesh, tiny volume, tiny label map |
| `download-cache` | showcases and large stress tests | LIDAR, atlas, DICOM, geospatial data |

Examples that require external data should define:

1. public source URL and license,
2. expected cache path,
3. preprocessing script,
4. generated file names,
5. checksum or integrity checks,
6. small fallback mode,
7. offline behavior after the cache has been prepared.


## Validation Policy

Fixture and regression examples should prefer deterministic offscreen output. The validation policy
should be chosen by output class:

1. PNG: compare region averages, pixel masks, histograms, or tolerant RMS/SSIM metrics.
2. JSON: compare normalized scene, DRP2, or WebGPU-compatible streams.
3. DVZR: replay and compare frame metadata, stream contents, or captured output.
4. stdout: assert exact success/failure lines or parse structured key-value metrics.
5. MP4/raw video: verify file creation and basic metadata; reserve visual checks for focused tests.
6. bounded GLFW: run for a fixed frame count and exit automatically.

Interactive examples should include a manual checklist, but critical behavior should also have a
separate offscreen or semantic regression test where practical.


## Duplication Policy

A small number of scenarios should exist in all API layers:

| Scenario | C | Datoviz Python | GSP | Plot |
| -------- | - | -------------- | --- | ---- |
| basic points/scatter | yes | yes | yes | yes |
| image | yes | yes | yes | yes |
| line/path | yes | optional | yes | yes |
| mesh/surface | yes | optional | yes | yes if supported |
| volume slice | yes | yes | yes | yes if supported |
| linked panels | yes | optional | yes | yes |
| picking/probing | yes | yes | yes | yes if exposed |
| offscreen capture | yes | yes | yes | optional |

Low-level DRP2, vklite, canvas, stream, and interop examples should remain C-only or mostly C. GSP
and plot examples should not expose those layers unless the point of the example is embedding or
interop.


## Migration Plan

1. Add this organization document and link it from `spec/scene/examples/README.md`.
2. Add `examples/manifests/examples.yaml` with metadata for the current C examples, without moving
   source files.
3. Classify existing C examples into the target lanes: fundamentals, visuals, features, techniques,
   advanced, showcase, and stress.
4. Add a small `examples/python/` lane with raw Python smoke examples for the Datoviz API.
5. Add scenario IDs for examples that should later have vispy2 GSP or plot counterparts.
6. Add `examples/manifests/golden.yaml` for the first deterministic offscreen regression examples.
7. Coordinate with vispy2 so GSP and plot examples use the same scenario IDs.
8. Rename or reorganize existing C groups only after manifests and documentation links make the move
   low-risk.


## Initial Raw Python Scope

The first Datoviz `examples/python/` additions should be deliberately small:

```text
examples/python/fundamentals/offscreen_capture.py
examples/python/visuals/point.py
examples/python/visuals/image.py
examples/python/visuals/volume_slice.py
examples/python/features/image_probe.py
examples/python/features/point_pick.py
examples/python/features/partial_update.py
examples/python/regression/resize_offscreen.py
```

These examples should focus on library loading, object lifecycle, data upload, offscreen rendering,
readback, and request handling. Rich object-oriented examples should be implemented in vispy2 GSP
instead.
